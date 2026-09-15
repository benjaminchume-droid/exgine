#include "exgine/asset_pipeline.hpp"

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {
namespace {

std::string extension(std::string_view uri) {
    const auto dot = uri.find_last_of('.');
    if (dot == std::string_view::npos) return {};
    std::string out(uri.substr(dot + 1));
    std::transform(out.begin(), out.end(), out.begin(), [](char c) {
        return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
    });
    return out;
}

std::vector<std::uint8_t> decode_base64(std::string_view value) {
    std::vector<std::uint8_t> out;
    int accumulator = 0;
    int bits = -8;
    const auto digit = [](char c) -> int {
        if (c >= 'A' && c <= 'Z') return c - 'A';
        if (c >= 'a' && c <= 'z') return c - 'a' + 26;
        if (c >= '0' && c <= '9') return c - '0' + 52;
        if (c == '+') return 62;
        if (c == '/') return 63;
        return -1;
    };
    for (char c : value) {
        if (c == '=') break;
        const int d = digit(c);
        if (d < 0) continue;
        accumulator = (accumulator << 6) | d;
        bits += 6;
        if (bits >= 0) {
            out.push_back(static_cast<std::uint8_t>((accumulator >> bits) & 0xff));
            bits -= 8;
        }
    }
    return out;
}

bool parse_data_uri(std::string_view uri, std::vector<std::uint8_t>& bytes, std::string& extension_name) {
    if (uri.rfind("data:", 0) != 0) return false;
    const auto comma = uri.find(',');
    if (comma == std::string_view::npos) return false;
    const auto metadata = uri.substr(5, comma - 5);
    const bool base64 = metadata.find(";base64") != std::string_view::npos;
    if (!base64) return false;
    if (metadata.find("image/png") != std::string_view::npos) extension_name = "png";
    else if (metadata.find("image/jpeg") != std::string_view::npos) extension_name = "jpg";
    else if (metadata.find("image/webp") != std::string_view::npos) extension_name = "webp";
    else if (metadata.find("image/bmp") != std::string_view::npos) extension_name = "bmp";
    else if (metadata.find("image/x-tga") != std::string_view::npos || metadata.find("image/tga") != std::string_view::npos) extension_name = "tga";
    else if (metadata.find("image/x-portable-pixmap") != std::string_view::npos) extension_name = "ppm";
    else return false;
    bytes = decode_base64(uri.substr(comma + 1));
    return !bytes.empty();
}

std::shared_ptr<Texture2D> make_texture(const ImageData& image) {
    if (!image.valid()) return {};
    auto texture = std::make_shared<Texture2D>();
    texture->width = image.width;
    texture->height = image.height;
    texture->channels = image.channels();
    texture->format = image.channels() == 1 ? TextureFormat::R8 :
                      image.channels() == 3 ? TextureFormat::RGB8 : TextureFormat::RGBA8;
    texture->data.resize(image.pixels.size());
    for (std::size_t i = 0; i < image.pixels.size(); ++i) {
        texture->data[i] = static_cast<float>(image.pixels[i]) / 255.0f;
    }
    return texture;
}

} // namespace

bool DecodedAssetTexture::valid() const noexcept {
    return !slot.empty() && !uri.empty() && asset_id != invalid_asset && image.valid() && texture && texture->valid();
}

bool DecodedAssetMaterial::valid() const noexcept {
    return source.valid() && source.material.valid() && textures.valid();
}

std::shared_ptr<Texture2D> AssetPipeline::to_texture(const ImageData& image) {
    return make_texture(image);
}

AssetPipelineResult AssetPipeline::process(std::string_view uri,
                                           std::string_view payload,
                                           const Loader& loader,
                                           const ImageDecoderRegistry& decoders) const {
    AssetPipelineResult result;
    result.imported = import_gltf(uri, payload);
    if (!result.imported.success) {
        result.error = result.imported.error.empty() ? "asset import failed" : result.imported.error;
        return result;
    }

    result.materials.reserve(result.imported.materials.size());
    for (const auto& source : result.imported.materials) {
        DecodedAssetMaterial material;
        material.source = source;
        TextureSet generated = generate_material_textures(source.material, {});
        material.textures = std::move(generated);
        result.materials.push_back(std::move(material));
    }

    // Resolve image URIs through the established material pipeline. This deliberately
    // runs after geometry/material import so the material factors remain valid even
    // when an optional image cannot be decoded on the current platform.
    GltfMaterialPipeline material_pipeline;
    const auto decoded = material_pipeline.build(uri, payload, loader, decoders);
    if (!decoded.success && !decoded.textures.empty()) {
        result.error = decoded.error;
        return result;
    }

    for (const auto& decoded_texture : decoded.textures) {
        const auto texture = make_texture(*decoded_texture.image);
        if (!texture) continue;
        DecodedAssetTexture item;
        item.slot = decoded_texture.slot;
        item.uri = decoded_texture.uri;
        item.asset_id = decoded_texture.asset_id;
        item.image = *decoded_texture.image;
        item.texture = texture;
        const std::size_t material_index = result.materials.empty() ? 0 :
            std::min<std::size_t>(result.materials.size() - 1, result.materials.front().maps.size());
        if (!result.materials.empty()) {
            auto& target = result.materials[material_index];
            if (item.slot == "base_color") target.textures.base_color = texture;
            else if (item.slot == "normal") target.textures.normal = texture;
            else if (item.slot == "occlusion") target.textures.ambient_occlusion = texture;
            else if (item.slot == "emission") target.textures.emission = texture;
            else if (item.slot == "metallic_roughness") {
                target.textures.metallic = texture;
                target.textures.roughness = texture;
            }
            target.maps.push_back(std::move(item));
        }
    }

    result.success = true;
    return result;
}

} // namespace exgine

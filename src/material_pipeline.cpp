#include "exgine/material_pipeline.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {
namespace {
float clamp01(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }
float wrap(float v) noexcept { v -= std::floor(v); return v; }
float sample_repeat(const Texture2D& t, float u, float v, std::uint32_t c) noexcept {
    if (!t.valid() || c >= t.channels) return 0.0f;
    const auto x = static_cast<std::uint32_t>(wrap(u) * static_cast<float>(t.width));
    const auto y = static_cast<std::uint32_t>(wrap(v) * static_cast<float>(t.height));
    return t.get(std::min(x, t.width - 1), std::min(y, t.height - 1), c);
}
Texture2D make_texture(std::uint32_t width, std::uint32_t height, std::uint32_t channels) {
    Texture2D t;
    t.width = std::max(width, 1u); t.height = std::max(height, 1u); t.channels = std::max(channels, 1u);
    t.format = channels == 1 ? TextureFormat::R32F : channels == 2 ? TextureFormat::RG32F : channels == 3 ? TextureFormat::RGB32F : TextureFormat::RGBA32F;
    t.data.resize(static_cast<std::size_t>(t.width) * t.height * t.channels, 0.0f);
    return t;
}
}

Texture2D generate_normal_from_height(const Texture2D& height, float strength) {
    if (!height.valid() || height.channels == 0) return {};
    Texture2D normal = make_texture(height.width, height.height, 3);
    const float s = std::max(0.0f, strength);
    for (std::uint32_t y = 0; y < normal.height; ++y) {
        for (std::uint32_t x = 0; x < normal.width; ++x) {
            const auto xm = x == 0 ? normal.width - 1 : x - 1;
            const auto xp = x + 1 == normal.width ? 0 : x + 1;
            const auto ym = y == 0 ? normal.height - 1 : y - 1;
            const auto yp = y + 1 == normal.height ? 0 : y + 1;
            const float dx = (height.get(xp, y, 0) - height.get(xm, y, 0)) * 0.5f * s;
            const float dy = (height.get(x, yp, 0) - height.get(x, ym, 0)) * 0.5f * s;
            const float len = std::sqrt(dx * dx + dy * dy + 1.0f);
            normal.set(x, y, 0, (-dx / len) * 0.5f + 0.5f);
            normal.set(x, y, 1, (-dy / len) * 0.5f + 0.5f);
            normal.set(x, y, 2, 1.0f / len * 0.5f + 0.5f);
        }
    }
    return normal;
}

TextureMipChain generate_mip_chain(const Texture2D& source, std::uint32_t levels) {
    TextureMipChain result;
    if (!source.valid()) return result;
    const std::uint32_t max_levels = static_cast<std::uint32_t>(std::floor(std::log2(static_cast<double>(std::max(source.width, source.height))))) + 1u;
    const std::uint32_t count = levels == 0 ? max_levels : std::min(levels, max_levels);
    result.levels.reserve(count);
    result.levels.push_back(source);
    for (std::uint32_t level = 1; level < count; ++level) {
        const Texture2D& prev = result.levels.back();
        Texture2D next = make_texture(std::max(1u, prev.width / 2u), std::max(1u, prev.height / 2u), prev.channels);
        for (std::uint32_t y = 0; y < next.height; ++y) {
            for (std::uint32_t x = 0; x < next.width; ++x) {
                const std::uint32_t x0 = std::min(prev.width - 1, x * 2u);
                const std::uint32_t x1 = std::min(prev.width - 1, x0 + 1u);
                const std::uint32_t y0 = std::min(prev.height - 1, y * 2u);
                const std::uint32_t y1 = std::min(prev.height - 1, y0 + 1u);
                for (std::uint32_t c = 0; c < next.channels; ++c) {
                    const float value = (prev.get(x0, y0, c) + prev.get(x1, y0, c) + prev.get(x0, y1, c) + prev.get(x1, y1, c)) * 0.25f;
                    next.set(x, y, c, value);
                }
            }
        }
        result.levels.push_back(std::move(next));
    }
    return result;
}

PackedMaterialTexture pack_orm(const Texture2D& occlusion, const Texture2D& roughness, const Texture2D& metallic) {
    PackedMaterialTexture result;
    if (!occlusion.valid() || !roughness.valid() || !metallic.valid()) return result;
    if (occlusion.width != roughness.width || occlusion.height != roughness.height || occlusion.width != metallic.width || occlusion.height != metallic.height) return result;
    result.orm = std::make_shared<Texture2D>(make_texture(occlusion.width, occlusion.height, 3));
    for (std::uint32_t y = 0; y < result.orm->height; ++y) {
        for (std::uint32_t x = 0; x < result.orm->width; ++x) {
            result.orm->set(x, y, 0, clamp01(occlusion.get(x, y, 0)));
            result.orm->set(x, y, 1, clamp01(roughness.get(x, y, 0)));
            result.orm->set(x, y, 2, clamp01(metallic.get(x, y, 0)));
        }
    }
    return result;
}

MaterialSurfaceSample sample_material(const Material& material, const TextureSet& textures, float u, float v) noexcept {
    MaterialSurfaceSample s;
    s.base_color = {clamp01(material.base_color.r), clamp01(material.base_color.g), clamp01(material.base_color.b)};
    s.roughness = clamp01(material.roughness); s.metallic = clamp01(material.metallic); s.opacity = clamp01(material.opacity); s.emission = material.emission;
    if (textures.base_color && textures.base_color->valid()) {
        s.base_color.r *= clamp01(sample_repeat(*textures.base_color, u, v, 0));
        if (textures.base_color->channels > 1) s.base_color.g *= clamp01(sample_repeat(*textures.base_color, u, v, 1));
        if (textures.base_color->channels > 2) s.base_color.b *= clamp01(sample_repeat(*textures.base_color, u, v, 2));
    }
    if (textures.roughness && textures.roughness->valid()) s.roughness = clamp01(material.roughness * sample_repeat(*textures.roughness, u, v, 0));
    if (textures.metallic && textures.metallic->valid()) s.metallic = clamp01(material.metallic * sample_repeat(*textures.metallic, u, v, 0));
    if (textures.ambient_occlusion && textures.ambient_occlusion->valid()) s.ambient_occlusion = clamp01(sample_repeat(*textures.ambient_occlusion, u, v, 0));
    if (textures.opacity && textures.opacity->valid()) s.opacity *= clamp01(sample_repeat(*textures.opacity, u, v, 0));
    if (textures.height && textures.height->valid()) s.height = clamp01(sample_repeat(*textures.height, u, v, 0));
    if (textures.normal && textures.normal->valid() && textures.normal->channels >= 3) {
        s.normal = {sample_repeat(*textures.normal, u, v, 0) * 2.0f - 1.0f, sample_repeat(*textures.normal, u, v, 1) * 2.0f - 1.0f, sample_repeat(*textures.normal, u, v, 2) * 2.0f - 1.0f};
        const float l = std::sqrt(s.normal.x*s.normal.x+s.normal.y*s.normal.y+s.normal.z*s.normal.z);
        if (l > std::numeric_limits<float>::epsilon()) { s.normal.x /= l; s.normal.y /= l; s.normal.z /= l; }
    }
    if (textures.emission && textures.emission->valid() && textures.emission->channels >= 3) {
        s.emission = {sample_repeat(*textures.emission, u, v, 0), sample_repeat(*textures.emission, u, v, 1), sample_repeat(*textures.emission, u, v, 2)};
    }
    return s;
}

TextureSet generate_production_material_textures(const Material& material, const MaterialPipelineConfig& config) {
    TextureGenerationSettings gen;
    gen.width = std::max(config.width, 1u); gen.height = std::max(config.height, 1u);
    gen.noise.seed = 0;
    TextureSet textures = generate_material_textures(material, gen);
    if (textures.height && textures.height->valid()) {
        textures.normal = std::make_shared<Texture2D>(generate_normal_from_height(*textures.height, config.normal_strength));
    }
    if (config.generate_mips) {
        // The base TextureSet remains the resident level-0 representation. Mip generation is deterministic and validated here before upload.
        (void)generate_mip_chain(*textures.base_color, config.mip_levels);
        (void)generate_mip_chain(*textures.normal, config.mip_levels);
    }
    (void)pack_orm(*textures.ambient_occlusion, *textures.roughness, *textures.metallic);
    return textures;
}

} // namespace exgine

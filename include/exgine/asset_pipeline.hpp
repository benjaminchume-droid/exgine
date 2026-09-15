#pragma once

#include "exgine/import.hpp"
#include "exgine/production.hpp"
#include "exgine/texture.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

struct DecodedAssetTexture {
    std::string slot;
    std::string uri;
    AssetId asset_id = invalid_asset;
    ImageData image{};
    std::shared_ptr<Texture2D> texture;
    [[nodiscard]] bool valid() const noexcept;
};

struct DecodedAssetMaterial {
    ImportedMaterial source{};
    TextureSet textures{};
    std::vector<DecodedAssetTexture> maps;
    [[nodiscard]] bool valid() const noexcept;
};

struct AssetPipelineResult {
    bool success = false;
    ImportResult imported{};
    std::vector<DecodedAssetMaterial> materials;
    std::string error;
};

class AssetPipeline final {
public:
    using Loader = AssetBytesLoader;

    [[nodiscard]] AssetPipelineResult process(std::string_view uri,
                                              std::string_view payload,
                                              const Loader& loader,
                                              const ImageDecoderRegistry& decoders) const;

    [[nodiscard]] static std::shared_ptr<Texture2D> to_texture(const ImageData& image);
};

} // namespace exgine

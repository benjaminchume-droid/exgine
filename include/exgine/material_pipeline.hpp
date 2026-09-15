#pragma once

#include "exgine/texture.hpp"

#include <cstdint>
#include <memory>
#include <vector>

namespace exgine {

struct MaterialPipelineConfig {
    std::uint32_t width = 512;
    std::uint32_t height = 512;
    std::uint32_t mip_levels = 0;
    float normal_strength = 2.0f;
    bool generate_mips = true;
};

struct TextureMipChain {
    std::vector<Texture2D> levels;
    [[nodiscard]] bool valid() const noexcept { return !levels.empty() && levels.front().valid(); }
};

struct PackedMaterialTexture {
    // R=occlusion, G=roughness, B=metallic. Values are linear.
    std::shared_ptr<Texture2D> orm;
    [[nodiscard]] bool valid() const noexcept { return orm && orm->valid() && orm->channels == 3; }
};

struct MaterialSurfaceSample {
    Color3 base_color{};
    float roughness = 0.5f;
    float metallic = 0.0f;
    Vec3 normal{0.0f, 0.0f, 1.0f};
    float ambient_occlusion = 1.0f;
    Color3 emission{};
    float opacity = 1.0f;
    float height = 0.0f;
};

[[nodiscard]] Texture2D generate_normal_from_height(const Texture2D& height, float strength = 2.0f);
[[nodiscard]] TextureMipChain generate_mip_chain(const Texture2D& source, std::uint32_t levels = 0);
[[nodiscard]] PackedMaterialTexture pack_orm(const Texture2D& occlusion,
                                             const Texture2D& roughness,
                                             const Texture2D& metallic);
[[nodiscard]] MaterialSurfaceSample sample_material(const Material& material,
                                                    const TextureSet& textures,
                                                    float u,
                                                    float v) noexcept;
[[nodiscard]] TextureSet generate_production_material_textures(const Material& material,
                                                               const MaterialPipelineConfig& config);

} // namespace exgine

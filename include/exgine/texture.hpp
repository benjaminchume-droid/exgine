#pragma once

#include "exgine/material.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class TextureFormat : std::uint8_t { R8, RG8, RGB8, RGBA8, R32F, RG32F, RGB32F, RGBA32F };

enum class NoiseType : std::uint8_t { Value, Perlin, Simplex, Fbm, Voronoi, Ridged, Turbulence };

struct Texture2D {
    std::uint32_t width = 0;
    std::uint32_t height = 0;
    std::uint32_t channels = 0;
    TextureFormat format = TextureFormat::RGBA32F;
    std::vector<float> data;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::size_t index(std::uint32_t x, std::uint32_t y,
                                    std::uint32_t channel = 0) const noexcept;
    [[nodiscard]] float get(std::uint32_t x, std::uint32_t y, std::uint32_t channel = 0) const noexcept;
    void set(std::uint32_t x, std::uint32_t y, std::uint32_t channel, float value) noexcept;
};

struct TextureSet {
    std::shared_ptr<Texture2D> base_color;
    std::shared_ptr<Texture2D> roughness;
    std::shared_ptr<Texture2D> metallic;
    std::shared_ptr<Texture2D> normal;
    std::shared_ptr<Texture2D> ambient_occlusion;
    std::shared_ptr<Texture2D> emission;
    std::shared_ptr<Texture2D> opacity;
    std::shared_ptr<Texture2D> height;

    [[nodiscard]] bool valid() const noexcept;
};

struct NoiseSettings {
    NoiseType type = NoiseType::Fbm;
    float scale = 4.0f;
    std::uint32_t octaves = 5;
    float persistence = 0.5f;
    float lacunarity = 2.0f;
    std::uint64_t seed = 0;
};

struct TextureGenerationSettings {
    std::uint32_t width = 256;
    std::uint32_t height = 256;
    NoiseSettings noise{};
};

[[nodiscard]] float sample_noise(float x, float y, const NoiseSettings& settings) noexcept;
[[nodiscard]] Texture2D generate_noise_texture(const TextureGenerationSettings& settings,
                                               std::uint32_t channels = 1);
[[nodiscard]] TextureSet generate_material_textures(const Material& material,
                                                     const TextureGenerationSettings& settings);

} // namespace exgine

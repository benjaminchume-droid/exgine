#include "exgine/texture.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {
namespace {

std::uint32_t hash2(std::int32_t x, std::int32_t y, std::uint64_t seed) noexcept {
    std::uint64_t h = seed + static_cast<std::uint64_t>(static_cast<std::uint32_t>(x)) * 0x9e3779b9U;
    h ^= static_cast<std::uint64_t>(static_cast<std::uint32_t>(y)) * 0x85ebca6bU;
    h ^= h >> 16; h *= 0x7feb352dU; h ^= h >> 15; h *= 0x846ca68bU; h ^= h >> 16;
    return static_cast<std::uint32_t>(h);
}

float random2(std::int32_t x, std::int32_t y, std::uint64_t seed) noexcept {
    return static_cast<float>(hash2(x, y, seed)) / static_cast<float>(std::numeric_limits<std::uint32_t>::max());
}

float smooth(float t) noexcept { return t * t * (3.0f - 2.0f * t); }
float lerp(float a, float b, float t) noexcept { return a + (b - a) * t; }

float value_noise(float x, float y, std::uint64_t seed) noexcept {
    const auto ix = static_cast<std::int32_t>(std::floor(x));
    const auto iy = static_cast<std::int32_t>(std::floor(y));
    const float fx = smooth(x - static_cast<float>(ix));
    const float fy = smooth(y - static_cast<float>(iy));
    const float a = random2(ix, iy, seed), b = random2(ix + 1, iy, seed);
    const float c = random2(ix, iy + 1, seed), d = random2(ix + 1, iy + 1, seed);
    return lerp(lerp(a, b, fx), lerp(c, d, fx), fy) * 2.0f - 1.0f;
}

float perlin_noise(float x, float y, std::uint64_t seed) noexcept {
    const auto ix = static_cast<std::int32_t>(std::floor(x));
    const auto iy = static_cast<std::int32_t>(std::floor(y));
    const float fx = x - static_cast<float>(ix), fy = y - static_cast<float>(iy);
    auto grad = [seed](std::int32_t gx, std::int32_t gy, float dx, float dy) {
        const float angle = random2(gx, gy, seed) * 6.28318530718f;
        return std::cos(angle) * dx + std::sin(angle) * dy;
    };
    const float sx = smooth(fx), sy = smooth(fy);
    return lerp(lerp(grad(ix, iy, fx, fy), grad(ix + 1, iy, fx - 1.0f, fy), sx),
                lerp(grad(ix, iy + 1, fx, fy - 1.0f), grad(ix + 1, iy + 1, fx - 1.0f, fy - 1.0f), sx), sy) * 1.41421356f;
}

float voronoi_noise(float x, float y, std::uint64_t seed) noexcept {
    const auto ix = static_cast<std::int32_t>(std::floor(x));
    const auto iy = static_cast<std::int32_t>(std::floor(y));
    float nearest = 2.0f;
    for (std::int32_t oy = -1; oy <= 1; ++oy) for (std::int32_t ox = -1; ox <= 1; ++ox) {
        const float px = static_cast<float>(ox) + random2(ix + ox, iy + oy, seed);
        const float py = static_cast<float>(oy) + random2(ix + ox, iy + oy, seed + 17);
        const float dx = px - (x - static_cast<float>(ix));
        const float dy = py - (y - static_cast<float>(iy));
        nearest = std::min(nearest, std::sqrt(dx * dx + dy * dy));
    }
    return 1.0f - std::clamp(nearest, 0.0f, 1.0f);
}

float base_noise(float x, float y, const NoiseSettings& s) noexcept {
    switch (s.type) {
    case NoiseType::Value: return value_noise(x, y, s.seed);
    case NoiseType::Perlin: return perlin_noise(x, y, s.seed);
    case NoiseType::Simplex: return perlin_noise(x * 0.97f + y * 0.23f, y * 0.97f - x * 0.23f, s.seed + 31);
    case NoiseType::Voronoi: return voronoi_noise(x, y, s.seed) * 2.0f - 1.0f;
    default: return value_noise(x, y, s.seed);
    }
}

float clamp01(float v) noexcept { return std::clamp(v, 0.0f, 1.0f); }

} // namespace

bool Texture2D::valid() const noexcept {
    if (width == 0 || height == 0 || channels == 0) return false;
    const auto count = static_cast<std::size_t>(width) * height * channels;
    return data.size() == count;
}

std::size_t Texture2D::index(std::uint32_t x, std::uint32_t y, std::uint32_t channel) const noexcept {
    return (static_cast<std::size_t>(y) * width + x) * channels + channel;
}

float Texture2D::get(std::uint32_t x, std::uint32_t y, std::uint32_t channel) const noexcept {
    if (!valid() || x >= width || y >= height || channel >= channels) return 0.0f;
    return data[index(x, y, channel)];
}

void Texture2D::set(std::uint32_t x, std::uint32_t y, std::uint32_t channel, float value) noexcept {
    if (!valid() || x >= width || y >= height || channel >= channels) return;
    data[index(x, y, channel)] = value;
}

bool TextureSet::valid() const noexcept {
    return base_color && roughness && metallic && normal && ambient_occlusion && emission && opacity && height &&
           base_color->valid() && roughness->valid() && metallic->valid() && normal->valid() &&
           ambient_occlusion->valid() && emission->valid() && opacity->valid() && height->valid();
}

float sample_noise(float x, float y, const NoiseSettings& settings) noexcept {
    const float scale = std::max(settings.scale, 1e-5f);
    x *= scale; y *= scale;
    if (settings.type == NoiseType::Fbm || settings.type == NoiseType::Ridged || settings.type == NoiseType::Turbulence) {
        float amplitude = 1.0f, frequency = 1.0f, sum = 0.0f, weight = 0.0f;
        const auto octaves = std::max(settings.octaves, 1u);
        for (std::uint32_t i = 0; i < octaves; ++i) {
            float n = base_noise(x * frequency, y * frequency, {NoiseType::Perlin, 1, 1, .5f, 2, settings.seed + i * 101});
            if (settings.type == NoiseType::Ridged) n = 1.0f - std::abs(n);
            if (settings.type == NoiseType::Turbulence) n = std::abs(n);
            sum += n * amplitude; weight += amplitude;
            amplitude *= std::clamp(settings.persistence, 0.0f, 1.0f);
            frequency *= std::max(settings.lacunarity, 1.01f);
        }
        return weight > 0 ? sum / weight : 0.0f;
    }
    return base_noise(x, y, settings);
}

Texture2D generate_noise_texture(const TextureGenerationSettings& settings, std::uint32_t channels) {
    Texture2D texture;
    texture.width = std::max(settings.width, 1u);
    texture.height = std::max(settings.height, 1u);
    texture.channels = std::max(channels, 1u);
    texture.format = texture.channels == 1 ? TextureFormat::R32F :
                     texture.channels == 2 ? TextureFormat::RG32F :
                     texture.channels == 3 ? TextureFormat::RGB32F : TextureFormat::RGBA32F;
    texture.data.resize(static_cast<std::size_t>(texture.width) * texture.height * texture.channels);
    for (std::uint32_t y = 0; y < texture.height; ++y) for (std::uint32_t x = 0; x < texture.width; ++x) {
        const float u = static_cast<float>(x) / static_cast<float>(texture.width - 1 + (texture.width == 1));
        const float v = static_cast<float>(y) / static_cast<float>(texture.height - 1 + (texture.height == 1));
        const float n = clamp01(sample_noise(u, v, settings.noise) * 0.5f + 0.5f);
        for (std::uint32_t c = 0; c < texture.channels; ++c) texture.data[texture.index(x, y, c)] = n;
    }
    return texture;
}

TextureSet generate_material_textures(const Material& material, const TextureGenerationSettings& settings) {
    auto make = [&](std::uint32_t channels) {
        auto t = std::make_shared<Texture2D>();
        t->width = std::max(settings.width, 1u); t->height = std::max(settings.height, 1u); t->channels = channels;
        t->format = channels == 1 ? TextureFormat::R32F : channels == 3 ? TextureFormat::RGB32F : TextureFormat::RGBA32F;
        t->data.resize(static_cast<std::size_t>(t->width) * t->height * channels);
        return t;
    };
    TextureSet out{make(3), make(1), make(1), make(3), make(1), make(3), make(1), make(1)};
    for (std::uint32_t y = 0; y < out.base_color->height; ++y) for (std::uint32_t x = 0; x < out.base_color->width; ++x) {
        const float u = static_cast<float>(x) / static_cast<float>(out.base_color->width - 1 + (out.base_color->width == 1));
        const float v = static_cast<float>(y) / static_cast<float>(out.base_color->height - 1 + (out.base_color->height == 1));
        float n = clamp01(sample_noise(u, v, settings.noise) * 0.5f + 0.5f);
        float rough = material.roughness, metal = material.metallic, ao = 1.0f, height = n;
        for (const auto& layer : material.layers) {
            NoiseSettings ns = settings.noise; ns.scale = std::max(layer.scale, 0.001f); ns.seed ^= layer.seed;
            const float layer_value = sample_noise(u, v, ns) * 0.5f + 0.5f;
            if (layer.generator == "grain" || layer.generator == "noise" || layer.generator == "fbm") {
                n = clamp01(n + (layer_value - 0.5f) * layer.strength);
                rough = clamp01(rough + (layer_value - 0.5f) * layer.strength * 0.5f);
                height = clamp01(height + (layer_value - 0.5f) * layer.strength);
            } else if (layer.generator == "scratches" || layer.generator == "cracks") {
                const float mark = layer_value > 0.72f ? layer.strength : 0.0f;
                rough = clamp01(rough + mark); height = clamp01(height - mark * 0.25f);
            } else if (layer.generator == "oxidation") {
                metal = clamp01(metal - layer_value * layer.strength * 0.2f);
            }
        }
        const float albedo[3] = {clamp01(material.base_color.r * (0.82f + 0.36f*n)), clamp01(material.base_color.g * (0.82f + 0.36f*n)), clamp01(material.base_color.b * (0.82f + 0.36f*n))};
        for (std::uint32_t c = 0; c < 3; ++c) out.base_color->data[out.base_color->index(x,y,c)] = albedo[c];
        out.roughness->data[out.roughness->index(x,y)] = rough;
        out.metallic->data[out.metallic->index(x,y)] = metal;
        out.ambient_occlusion->data[out.ambient_occlusion->index(x,y)] = ao;
        out.height->data[out.height->index(x,y)] = height;
        out.opacity->data[out.opacity->index(x,y)] = clamp01(material.opacity);
        for (std::uint32_t c=0;c<3;++c) out.emission->data[out.emission->index(x,y,c)] = c==0?material.emission.r:(c==1?material.emission.g:material.emission.b);
        out.normal->data[out.normal->index(x,y,0)] = 0.5f; out.normal->data[out.normal->index(x,y,1)] = 0.5f; out.normal->data[out.normal->index(x,y,2)] = 1.0f;
    }
    return out;
}

} // namespace exgine

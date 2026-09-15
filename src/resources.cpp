#include "exgine/resources.hpp"

#include <bit>
#include <functional>
#include <utility>

namespace exgine {
namespace {
void mix(std::uint64_t& h, std::uint64_t v) noexcept {
    h ^= v + 0x9e3779b97f4a7c15ULL + (h << 6) + (h >> 2);
}
}

std::uint64_t material_generation_key(const Material& material,
                                      const TextureGenerationSettings& settings) noexcept {
    std::uint64_t h = std::hash<std::string>{}(material.name);
    mix(h, std::bit_cast<std::uint32_t>(material.base_color.r));
    mix(h, std::bit_cast<std::uint32_t>(material.base_color.g));
    mix(h, std::bit_cast<std::uint32_t>(material.base_color.b));
    mix(h, std::bit_cast<std::uint32_t>(material.roughness));
    mix(h, std::bit_cast<std::uint32_t>(material.metallic));
    mix(h, std::bit_cast<std::uint32_t>(material.specular));
    mix(h, std::bit_cast<std::uint32_t>(material.opacity));
    mix(h, settings.width); mix(h, settings.height); mix(h, settings.noise.seed);
    mix(h, std::bit_cast<std::uint32_t>(settings.noise.scale));
    mix(h, settings.noise.octaves); mix(h, std::bit_cast<std::uint32_t>(settings.noise.persistence));
    mix(h, std::bit_cast<std::uint32_t>(settings.noise.lacunarity));
    for (const auto& layer : material.layers) {
        mix(h, std::hash<std::string>{}(layer.generator));
        mix(h, std::bit_cast<std::uint32_t>(layer.scale));
        mix(h, std::bit_cast<std::uint32_t>(layer.strength)); mix(h, layer.seed);
    }
    return h;
}

std::shared_ptr<const MaterialResource> ResourceCache::find_material(std::string_view name) const noexcept {
    const auto it = materials_.find(std::string{name});
    return it == materials_.end() ? nullptr : it->second;
}

bool ResourceCache::store_material(std::shared_ptr<MaterialResource> resource) {
    if (!resource || !resource->material.valid()) return false;
    materials_[resource->material.name] = std::move(resource);
    return true;
}

bool ResourceCache::erase_material(std::string_view name) noexcept {
    return materials_.erase(std::string{name}) != 0;
}

void ResourceCache::clear() noexcept { materials_.clear(); }

std::shared_ptr<MaterialResource> build_material_resource(const Material& material,
                                                           const TextureGenerationSettings& settings) {
    if (!material.valid()) return nullptr;
    auto resource = std::make_shared<MaterialResource>();
    resource->material = material;
    resource->generation_key = material_generation_key(material, settings);
    resource->textures = std::make_shared<TextureSet>(generate_material_textures(material, settings));
    if (!resource->textures->valid()) return nullptr;
    return resource;
}

} // namespace exgine

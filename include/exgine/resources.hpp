#pragma once

#include "exgine/material.hpp"
#include "exgine/texture.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>

namespace exgine {

struct MaterialResource {
    Material material;
    std::shared_ptr<TextureSet> textures;
    std::uint64_t generation_key = 0;
};

class ResourceCache {
public:
    [[nodiscard]] std::shared_ptr<const MaterialResource> find_material(std::string_view name) const noexcept;
    bool store_material(std::shared_ptr<MaterialResource> resource);
    bool erase_material(std::string_view name) noexcept;
    void clear() noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return materials_.size(); }

private:
    std::unordered_map<std::string, std::shared_ptr<MaterialResource>> materials_;
};

[[nodiscard]] std::uint64_t material_generation_key(const Material& material,
                                                     const TextureGenerationSettings& settings) noexcept;
[[nodiscard]] std::shared_ptr<MaterialResource> build_material_resource(
    const Material& material, const TextureGenerationSettings& settings);

} // namespace exgine

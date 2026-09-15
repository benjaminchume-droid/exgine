#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class MaterialChannel : std::uint8_t {
    BaseColor,
    Roughness,
    Metallic,
    Normal,
    AmbientOcclusion,
    Emission,
    Opacity,
    Height
};

struct Color3 {
    float r = 1.0f;
    float g = 1.0f;
    float b = 1.0f;
};

struct MaterialLayer {
    std::string generator;
    float scale = 1.0f;
    float strength = 1.0f;
    std::uint64_t seed = 0;
};

struct Material {
    std::string name;
    Color3 base_color{};
    float roughness = 0.5f;
    float metallic = 0.0f;
    float specular = 0.5f;
    Color3 emission{};
    float opacity = 1.0f;
    std::vector<MaterialLayer> layers;

    [[nodiscard]] bool valid() const noexcept;
};

class MaterialLibrary {
public:
    bool define(Material material);
    bool erase(std::string_view name) noexcept;
    [[nodiscard]] const Material* find(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return materials_.size(); }
    void clear() noexcept { materials_.clear(); }

private:
    std::unordered_map<std::string, Material> materials_;
};

[[nodiscard]] Material make_real_world_material(std::string_view name,
                                                 std::uint64_t seed = 0);

} // namespace exgine

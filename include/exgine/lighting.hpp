#pragma once

#include "exgine/material.hpp"
#include "exgine/scene.hpp"

#include <cstdint>
#include <vector>

namespace exgine {

enum class LightType : std::uint8_t { Directional, Point, Spot, Area };
enum class ShadowMode : std::uint8_t { None, Hard, Filtered, Cascaded };

struct Light {
    std::uint64_t id=0;
    LightType type=LightType::Point;
    Vec3 position{};
    Vec3 direction{0,-1,0};
    Color3 color{1,1,1};
    float intensity=1;
    float range=10;
    float inner_cone=0.5f;
    float outer_cone=0.8f;
    Vec2 area_size{1,1};
    ShadowMode shadow_mode=ShadowMode::Filtered;
    std::uint8_t shadow_cascades=4;
    bool active=true;
    [[nodiscard]] bool valid() const noexcept;
};

struct Camera {
    Vec3 position{};
    Vec3 rotation{};
    float vertical_fov_degrees=60;
    float aspect_ratio=16.0f/9.0f;
    float near_plane=0.05f;
    float far_plane=10000;
    bool orthographic=false;
    float orthographic_size=10;
    std::uint32_t culling_mask=0xffffffffu;
    [[nodiscard]] bool valid() const noexcept;
};

struct EnvironmentLighting {
    Color3 sky_color{0.55f,0.68f,0.9f};
    Color3 horizon_color{0.75f,0.78f,0.82f};
    Color3 ground_color{0.22f,0.24f,0.27f};
    float sun_intensity=1;
    float ambient_intensity=0.35f;
    float environment_strength=1;
};
struct FogSettings {
    bool enabled=false;
    Color3 color{0.7f,0.75f,0.8f};
    float density=0.002f;
    float start_distance=25;
    float end_distance=500;
    float height_falloff=0;
};
struct RenderLightingSettings {
    EnvironmentLighting environment{};
    FogSettings fog{};
    float exposure=0;
    float gamma=2.2f;
    bool high_dynamic_range=true;
};

class LightingWorld {
public:
    std::uint64_t create_light(Light light);
    bool destroy_light(std::uint64_t id) noexcept;
    [[nodiscard]] Light* get_light(std::uint64_t id) noexcept;
    [[nodiscard]] const Light* get_light(std::uint64_t id) const noexcept;
    [[nodiscard]] std::size_t light_count() const noexcept{return lights_.size();}
    void clear_lights() noexcept;
    [[nodiscard]] std::vector<const Light*> active_lights() const;
    bool set_main_camera(Camera camera) noexcept;
    [[nodiscard]] const Camera& main_camera() const noexcept{return camera_;}
    [[nodiscard]] Camera& main_camera() noexcept{return camera_;}
    [[nodiscard]] const RenderLightingSettings& settings() const noexcept{return settings_;}
    [[nodiscard]] RenderLightingSettings& settings() noexcept{return settings_;}
    void clear() noexcept;
private:
    std::uint64_t next_light_id_=1;
    std::vector<Light> lights_;
    Camera camera_{};
    RenderLightingSettings settings_{};
};

} // namespace exgine

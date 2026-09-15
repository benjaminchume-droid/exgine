#pragma once

#include <cstdint>
#include <cstddef>
#include <algorithm>
#include <cmath>

namespace exgine {

enum class PerformanceTier : std::uint8_t { Ultra = 0, High = 1, Medium = 2, Low = 3, BatterySaver = 4 };

enum class ThermalState : std::uint8_t { Nominal = 0, Fair = 1, Serious = 2, Critical = 3 };

struct PerformanceProfile {
    PerformanceTier tier = PerformanceTier::Medium;
    std::uint32_t render_width = 1280;
    std::uint32_t render_height = 720;
    float render_scale = 1.0f;
    std::uint32_t target_fps = 60;
    std::uint32_t max_dynamic_lights = 64;
    std::uint32_t shadow_cascades = 2;
    std::uint32_t shadow_map_size = 1024;
    std::uint32_t world_active_radius = 4;
    std::uint32_t world_prefetch_radius = 6;
    std::uint32_t max_loaded_chunks = 96;
    std::uint32_t vegetation_density_percent = 100;
    std::uint32_t texture_budget_mb = 256;
    std::uint32_t geometry_budget_mb = 192;
    bool hdr = true;
    bool bloom = false;
    bool motion_blur = false;
    bool ambient_occlusion = true;
    [[nodiscard]] bool valid() const noexcept;
};

struct DevicePerformanceLimits {
    std::uint32_t max_render_width = 3840;
    std::uint32_t max_render_height = 2160;
    std::uint32_t max_target_fps = 120;
    std::uint32_t max_dynamic_lights = 256;
    std::uint32_t max_texture_budget_mb = 2048;
    std::uint32_t max_geometry_budget_mb = 2048;
    std::size_t max_resident_bytes = 1024ull * 1024ull * 1024ull;
    [[nodiscard]] bool valid() const noexcept;
};

struct FrameBudget {
    float target_ms = 16.666667f;
    float cpu_budget_ms = 8.333333f;
    float gpu_budget_ms = 8.333333f;
    float streaming_budget_ms = 1.5f;
    float physics_budget_ms = 3.0f;
    [[nodiscard]] bool valid() const noexcept;
};

struct PerformanceTelemetry {
    float cpu_frame_ms = 0.0f;
    float gpu_frame_ms = 0.0f;
    float streaming_ms = 0.0f;
    float physics_ms = 0.0f;
    std::size_t resident_bytes = 0;
    std::uint32_t draw_calls = 0;
    std::uint32_t visible_objects = 0;
    std::uint32_t loaded_chunks = 0;
    ThermalState thermal = ThermalState::Nominal;
    [[nodiscard]] bool valid() const noexcept;
};

struct PerformanceDecision {
    PerformanceTier tier = PerformanceTier::Medium;
    float render_scale = 1.0f;
    bool quality_changed = false;
    bool throttle_simulation = false;
    [[nodiscard]] bool valid() const noexcept;
};

class PerformanceController {
public:
    explicit PerformanceController(PerformanceProfile profile = {}, DevicePerformanceLimits limits = {});
    [[nodiscard]] const PerformanceProfile& profile() const noexcept { return profile_; }
    [[nodiscard]] const DevicePerformanceLimits& limits() const noexcept { return limits_; }
    [[nodiscard]] FrameBudget frame_budget() const noexcept;
    [[nodiscard]] PerformanceDecision update(const PerformanceTelemetry& telemetry) noexcept;
    void set_profile(PerformanceProfile profile) noexcept;
    void reset() noexcept;

private:
    PerformanceProfile profile_{};
    DevicePerformanceLimits limits_{};
    std::uint32_t overload_frames_ = 0;
    std::uint32_t recovery_frames_ = 0;

    static PerformanceProfile normalize(PerformanceProfile profile, const DevicePerformanceLimits& limits) noexcept;
    static float min_scale(PerformanceTier tier) noexcept;
    static float max_scale(PerformanceTier tier) noexcept;
    static std::uint32_t fps_for(PerformanceTier tier) noexcept;
};

[[nodiscard]] PerformanceProfile make_mobile_profile(PerformanceTier tier = PerformanceTier::Medium,
                                                       std::uint32_t display_width = 1920,
                                                       std::uint32_t display_height = 1080) noexcept;

} // namespace exgine

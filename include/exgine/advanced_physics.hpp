#pragma once

#include "exgine/physics.hpp"

#include <cstdint>
#include <vector>

namespace exgine {

struct AdvancedPhysicsSettings {
    float max_ccd_step_distance = 0.25f;
    std::uint32_t max_ccd_substeps = 8;
    float buoyancy_water_density = 1000.0f;
    float gravity_magnitude = 9.80665f;
    float impact_damage_threshold = 8.0f;
};

struct CcdPlan {
    std::uint32_t substeps = 1;
    float step_dt = 0.0f;
    bool continuous = false;
};

struct BuoyancySample {
    float submerged_fraction = 0.0f;
    Vec3 center_of_buoyancy{};
    Vec3 force{};
};

struct ImpactAssessment {
    float relative_speed = 0.0f;
    float kinetic_energy = 0.0f;
    bool damaging = false;
};

class HighFidelityPhysicsController {
public:
    explicit HighFidelityPhysicsController(AdvancedPhysicsSettings settings = {}) : settings_(settings) {}

    [[nodiscard]] const AdvancedPhysicsSettings& settings() const noexcept { return settings_; }
    [[nodiscard]] CcdPlan plan_ccd(float dt, float speed, float bounding_radius) const noexcept;
    [[nodiscard]] BuoyancySample compute_buoyancy(float volume, float submerged_fraction,
                                                   Vec3 center, Vec3 water_up = {0, 1, 0}) const noexcept;
    [[nodiscard]] ImpactAssessment assess_impact(float relative_speed, float effective_mass) const noexcept;
    bool apply_buoyancy(PhysicsWorld& world, PhysicsBodyId body, const BuoyancySample& sample,
                        Vec3 world_position) const noexcept;

private:
    AdvancedPhysicsSettings settings_{};
};

} // namespace exgine

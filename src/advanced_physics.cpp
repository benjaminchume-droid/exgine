#include "exgine/advanced_physics.hpp"

#include <algorithm>
#include <cmath>

namespace exgine {
namespace {
float length(Vec3 v) noexcept { return std::sqrt(std::max(0.0f, v.x*v.x+v.y*v.y+v.z*v.z)); }
Vec3 normalize(Vec3 v) noexcept {
    const float l = length(v);
    return l > 1.0e-6f ? Vec3{v.x/l, v.y/l, v.z/l} : Vec3{0.0f, 1.0f, 0.0f};
}
}

CcdPlan HighFidelityPhysicsController::plan_ccd(float dt, float speed, float bounding_radius) const noexcept {
    CcdPlan result{};
    if (!(dt > 0.0f) || !(speed > 0.0f)) return result;
    const float radius = std::max(0.001f, bounding_radius);
    const float distance = speed * dt;
    if (distance <= std::max(0.001f, settings_.max_ccd_step_distance)) {
        result.step_dt = dt;
        return result;
    }
    const float travel_limit = std::max(0.001f, std::min(settings_.max_ccd_step_distance, radius));
    const auto steps = static_cast<std::uint32_t>(std::ceil(distance / travel_limit));
    result.substeps = std::clamp<std::uint32_t>(steps, 1U, std::max(1U, settings_.max_ccd_substeps));
    result.step_dt = dt / static_cast<float>(result.substeps);
    result.continuous = result.substeps > 1U;
    return result;
}

BuoyancySample HighFidelityPhysicsController::compute_buoyancy(float volume, float submerged_fraction,
                                                               Vec3 center, Vec3 water_up) const noexcept {
    BuoyancySample result{};
    const float v = std::max(0.0f, volume);
    const float fraction = std::clamp(submerged_fraction, 0.0f, 1.0f);
    const Vec3 up = normalize(water_up);
    result.submerged_fraction = fraction;
    result.center_of_buoyancy = center;
    const float magnitude = settings_.buoyancy_water_density * settings_.gravity_magnitude * v * fraction;
    result.force = {up.x * magnitude, up.y * magnitude, up.z * magnitude};
    return result;
}

ImpactAssessment HighFidelityPhysicsController::assess_impact(float relative_speed, float effective_mass) const noexcept {
    ImpactAssessment result{};
    result.relative_speed = std::max(0.0f, relative_speed);
    const float mass = std::max(0.0f, effective_mass);
    result.kinetic_energy = 0.5f * mass * result.relative_speed * result.relative_speed;
    result.damaging = result.relative_speed >= settings_.impact_damage_threshold;
    return result;
}

bool HighFidelityPhysicsController::apply_buoyancy(PhysicsWorld& world, PhysicsBodyId body,
                                                    const BuoyancySample& sample, Vec3 world_position) const noexcept {
    if (body == invalid_physics_body || sample.submerged_fraction <= 0.0f) return false;
    if (sample.force.x == 0.0f && sample.force.y == 0.0f && sample.force.z == 0.0f) return false;
    return world.apply_force(body, sample.force, world_position);
}

} // namespace exgine

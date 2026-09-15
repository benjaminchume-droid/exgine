#include "exgine/advanced_physics.hpp"
#include <cassert>
#include <cmath>

using namespace exgine;

int main() {
    HighFidelityPhysicsController controller({0.25f, 8, 1000.0f, 9.80665f, 8.0f});

    const auto plan = controller.plan_ccd(0.016f, 100.0f, 0.2f);
    assert(plan.continuous);
    assert(plan.substeps > 1U);
    assert(std::abs(plan.step_dt * static_cast<float>(plan.substeps) - 0.016f) < 1.0e-6f);

    const auto dry = controller.compute_buoyancy(10.0f, 0.0f, {0, 0, 0});
    assert(dry.force.y == 0.0f);
    const auto wet = controller.compute_buoyancy(2.0f, 0.5f, {1, 2, 3});
    assert(std::abs(wet.force.y - (1000.0f * 9.80665f * 2.0f * 0.5f)) < 1.0e-3f);
    assert(wet.center_of_buoyancy.x == 1.0f && wet.center_of_buoyancy.y == 2.0f && wet.center_of_buoyancy.z == 3.0f);

    const auto impact = controller.assess_impact(10.0f, 100.0f);
    assert(std::abs(impact.kinetic_energy - 5000.0f) < 1.0e-4f);
    assert(impact.damaging);

    PhysicsWorld world;
    PhysicsRigidBodyDesc body_desc;
    body_desc.mass_properties.mass = 1000.0f;
    body_desc.motion_quality = PhysicsMotionQuality::Continuous;
    body_desc.flags |= static_cast<PhysicsBodyFlags>(PhysicsBodyFlag::EnableCCD);
    const auto body = world.create_body(body_desc);
    assert(body != invalid_physics_body);
    const auto force = controller.compute_buoyancy(1.0f, 1.0f, {0, 0, 0});
    assert(controller.apply_buoyancy(world, body, force, {0, 0, 0}));
    const auto step = world.step(1.0f / 60.0f);
    assert(step.substeps > 0U);
    assert(world.has_body(body));
    return 0;
}

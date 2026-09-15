#pragma once
#include "exgine/physics.hpp"
#include "exgine/vehicle.hpp"
#include <array>
#include <cstdint>
namespace exgine {
struct VehicleInput { float throttle=0, brake=0, steering=0; bool handbrake=false; };
struct VehicleWheelSetup { Vec3 local_position{}; float radius=.34f; float suspension_rest=.35f; float suspension_stiffness=24000; float suspension_damping=3500; float longitudinal_grip=1.25f; float lateral_grip=1.15f; bool driven=false; bool steerable=false; };
struct VehicleDynamicsConfig { float mass=1500; float engine_force=6500; float brake_force=10000; float max_steer_radians=.6f; float max_speed=70; float rolling_resistance=.015f; float aerodynamic_drag=.32f; std::array<VehicleWheelSetup,4> wheels{}; };
struct VehicleDynamicsState { PhysicsBodyId body=invalid_physics_body; VehicleInput input{}; float wheel_angular_speed[4]{}; float wheel_slip[4]{}; float speed=0; std::uint8_t gear=1; bool reverse=false; bool possessed=false; };
class VehicleDynamicsController {
public:
    VehicleDynamicsController(PhysicsWorld& physics, VehicleDynamicsConfig config={}, VehicleDynamicsState state={});
    [[nodiscard]] VehicleDynamicsState& state() noexcept{return state_;}
    [[nodiscard]] const VehicleDynamicsState& state() const noexcept{return state_;}
    [[nodiscard]] const VehicleDynamicsConfig& config() const noexcept{return config_;}
    bool bind_body(PhysicsBodyId body) noexcept;
    bool set_input(VehicleInput input) noexcept;
    bool possess(bool value) noexcept;
    bool update(float dt) noexcept;
private:
    PhysicsWorld* physics_{}; VehicleDynamicsConfig config_{}; VehicleDynamicsState state_{};
};
} // namespace exgine

#pragma once

#include "exgine/geometry.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace exgine {

enum class VehicleType : std::uint8_t {
    Car, SUV, SportsCar, Pickup, Truck, Bus, Motorcycle,
    Construction, Emergency, Boat, Aircraft
};
enum class VehicleWheelRole : std::uint8_t { Steering, Driven, FreeRolling, LandingGear };
enum class VehicleDoorSide : std::uint8_t { Left, Right, Rear };

struct VehicleConfig {
    VehicleType type = VehicleType::Car;
    float length = 4.60f;
    float width = 1.84f;
    float height = 1.55f;
    float ground_clearance = 0.16f;
    float wheelbase = 2.70f;
    float track_width = 1.56f;
    float wheel_radius = 0.33f;
    float wheel_width = 0.22f;
    std::uint32_t axles = 2;
    std::uint32_t wheels_per_axle = 2;
    std::uint32_t seats = 5;
    std::uint32_t doors = 4;
    float cabin_length = 2.10f;
    float cabin_width = 1.62f;
    float cabin_height = 0.78f;
    float hood_length = 0.90f;
    float front_overhang = 0.90f;
    float rear_overhang = 1.00f;
    float chassis_height = 0.22f;
    float wing_span = 0.0f;
    float waterline = 0.0f;
    std::uint64_t seed = 0;
    std::string body_material = "steel";
    std::string glass_material = "glass";
    std::string rubber_material = "rubber";
    std::string metal_material = "aluminium";
    std::string interior_material = "rubber";
};

struct VehicleWheel {
    std::uint64_t id = 0;
    std::uint32_t axle = 0;
    std::uint32_t index = 0;
    VehicleWheelRole role = VehicleWheelRole::FreeRolling;
    Vec3 position{};
    float radius = 0.0f;
    float width = 0.0f;
    float suspension_rest = 0.0f;
    float suspension_travel = 0.0f;
    bool driven = false;
    bool steering = false;
};

struct VehicleDoor {
    std::uint64_t id = 0;
    VehicleDoorSide side = VehicleDoorSide::Left;
    Vec3 position{};
    Vec3 size{};
    bool open = false;
};

struct VehicleSeat {
    std::uint64_t id = 0;
    Vec3 position{};
    float width = 0.0f;
    bool driver = false;
};

struct VehicleLight {
    std::uint64_t id = 0;
    Vec3 position{};
    Vec3 direction{0,0,1};
    float intensity = 1.0f;
    bool brake = false;
    bool indicator = false;
};

struct VehiclePhysicsAttachment {
    std::uint64_t id = 0;
    std::string name;
    Vec3 position{};
    Vec3 axis{0,1,0};
};

struct VehicleCollisionVolume {
    std::uint64_t id = 0;
    Vec3 min{};
    Vec3 max{};
    bool wheel = false;
};

struct VehicleDefinition {
    std::uint64_t seed = 0;
    VehicleConfig config{};
    MeshAssembly geometry;
    std::vector<VehicleWheel> wheels;
    std::vector<VehicleDoor> doors;
    std::vector<VehicleSeat> seats;
    std::vector<VehicleLight> lights;
    std::vector<VehiclePhysicsAttachment> physics_attachments;
    std::vector<VehicleCollisionVolume> collision;

    [[nodiscard]] bool valid() const noexcept;
};

using VehicleInstance = VehicleDefinition;

[[nodiscard]] VehicleDefinition generate_vehicle(const VehicleConfig& config = {});
[[nodiscard]] bool set_vehicle_door_open(VehicleDefinition& vehicle, std::uint64_t door_id, bool open) noexcept;
[[nodiscard]] const VehicleDoor* find_vehicle_door(const VehicleDefinition& vehicle, std::uint64_t id) noexcept;
[[nodiscard]] const VehicleWheel* find_vehicle_wheel(const VehicleDefinition& vehicle, std::uint64_t id) noexcept;
[[nodiscard]] std::vector<VehicleCollisionVolume> active_vehicle_collision(const VehicleDefinition& vehicle);

} // namespace exgine

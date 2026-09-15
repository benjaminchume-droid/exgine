#pragma once

#include "exgine/geometry.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace exgine {

enum class BuildingOpeningType : std::uint8_t { Door, Window };
enum class BuildingInteractableType : std::uint8_t { Door, Window, LightSwitch };
enum class BuildingFurnitureType : std::uint8_t { Bed, Desk, Chair, Table, Sofa, Counter, Cabinet };

enum class BuildingWallAxis : std::uint8_t { X, Z };

struct BuildingConfig {
    float width = 14.0f;
    float depth = 12.0f;
    std::uint32_t floors = 2;
    float floor_height = 3.0f;
    float wall_thickness = 0.20f;
    float floor_thickness = 0.18f;
    std::uint32_t rooms_per_floor = 4;
    float door_width = 0.95f;
    float door_height = 2.15f;
    float window_width = 1.25f;
    float window_height = 1.15f;
    std::uint32_t windows_per_wall = 2;
    float stair_width = 1.10f;
    std::uint32_t furniture_density = 2;
    std::uint64_t seed = 1;
    std::string wall_material = "concrete";
    std::string floor_material = "wood";
    std::string glass_material = "glass";
    std::string door_material = "wood";
    std::string furniture_material = "wood";
};

struct BuildingRoom {
    std::uint64_t id = 0;
    std::uint32_t floor = 0;
    std::string name;
    Vec3 min{};
    Vec3 max{};
    Vec3 center{};

    [[nodiscard]] bool contains(float x, float y, float z) const noexcept;
};

struct BuildingOpening {
    std::uint64_t id = 0;
    BuildingOpeningType type = BuildingOpeningType::Door;
    std::uint32_t floor = 0;
    BuildingWallAxis axis = BuildingWallAxis::X;
    Vec3 position{};
    Vec3 size{};
    bool open = false;
    bool exterior = false;

    [[nodiscard]] bool valid() const noexcept;
};

struct BuildingInteractable {
    std::uint64_t id = 0;
    BuildingInteractableType type = BuildingInteractableType::Door;
    std::uint64_t target_id = 0;
    Vec3 position{};
    bool enabled = true;

    [[nodiscard]] bool valid() const noexcept;
};

struct BuildingFurniture {
    std::uint64_t id = 0;
    BuildingFurnitureType type = BuildingFurnitureType::Bed;
    std::uint64_t room_id = 0;
    Vec3 position{};
    Vec3 size{1,1,1};
    float rotation = 0.0f;

    [[nodiscard]] bool valid() const noexcept;
};

struct BuildingCollisionVolume {
    std::uint64_t id = 0;
    Vec3 min{};
    Vec3 max{};
    std::uint64_t door_id = 0;

    [[nodiscard]] bool valid() const noexcept;
};

struct BuildingDefinition {
    std::uint64_t seed = 0;
    BuildingConfig config{};
    MeshAssembly geometry;
    std::vector<BuildingRoom> rooms;
    std::vector<BuildingOpening> openings;
    std::vector<BuildingInteractable> interactables;
    std::vector<BuildingFurniture> furniture;
    std::vector<BuildingCollisionVolume> collision;

    [[nodiscard]] bool valid() const noexcept;
};

using BuildingInstance = BuildingDefinition;

[[nodiscard]] BuildingDefinition generate_building(const BuildingConfig& config = {});
bool set_building_door_open(BuildingDefinition& building, std::uint64_t door_id, bool open) noexcept;
[[nodiscard]] const BuildingOpening* find_building_door(const BuildingDefinition& building,
                                                         std::uint64_t door_id) noexcept;
[[nodiscard]] std::vector<BuildingCollisionVolume> active_building_collision(
    const BuildingDefinition& building);
[[nodiscard]] const BuildingRoom* find_building_room(const BuildingDefinition& building,
                                                     Vec3 position) noexcept;

} // namespace exgine

#include "exgine/building.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <utility>

namespace exgine {
namespace {
constexpr float pi = 3.14159265358979323846f;

std::uint64_t mix64(std::uint64_t x) noexcept {
    x += 0x9e3779b97f4a7c15ULL;
    x = (x ^ (x >> 30U)) * 0xbf58476d1ce4e5b9ULL;
    x = (x ^ (x >> 27U)) * 0x94d049bb133111ebULL;
    return x ^ (x >> 31U);
}

float unit(std::uint64_t x) noexcept {
    return static_cast<float>(mix64(x) >> 40U) / static_cast<float>(1U << 24U);
}

bool finite(float x) noexcept { return std::isfinite(x); }
bool finite3(Vec3 v) noexcept { return finite(v.x) && finite(v.y) && finite(v.z); }

void add_part(MeshAssembly& out, std::string name, Mesh mesh, std::string material, Vec3 position) {
    if (mesh.valid() && !material.empty()) out.parts.push_back({std::move(name), std::move(mesh), std::move(material), position, {1,1,1}});
}

void add_box(MeshAssembly& out, std::string name, Vec3 size, std::string material, Vec3 position) {
    if (size.x <= 0 || size.y <= 0 || size.z <= 0) return;
    add_part(out, std::move(name), make_box({size}), std::move(material), position);
}

struct Gap { float center = 0; float width = 0; float bottom = 0; float top = 0; };

void add_x_wall(MeshAssembly& out, std::vector<BuildingCollisionVolume>& collision,
                const std::string& material, const std::string& prefix, float z, float y,
                float length, float thickness, float height, std::vector<Gap> gaps,
                std::uint64_t& collision_id) {
    std::sort(gaps.begin(), gaps.end(), [](const Gap& a, const Gap& b) { return a.center < b.center; });
    float cursor = -length * 0.5f;
    for (const Gap& gap : gaps) {
        const float start = std::clamp(gap.center - gap.width * 0.5f, -length * 0.5f, length * 0.5f);
        const float end = std::clamp(gap.center + gap.width * 0.5f, -length * 0.5f, length * 0.5f);
        if (start > cursor + 1e-4f) {
            const float width = start - cursor;
            const Vec3 pos{(cursor + start) * 0.5f, y, z};
            add_box(out, prefix + "_segment", {width, height, thickness}, material, pos);
            collision.push_back({collision_id++, {pos.x - width*0.5f, y - height*0.5f, z - thickness*0.5f},
                                 {pos.x + width*0.5f, y + height*0.5f, z + thickness*0.5f}, 0});
        }
        cursor = std::max(cursor, end);
    }
    if (cursor < length * 0.5f - 1e-4f) {
        const float width = length * 0.5f - cursor;
        const Vec3 pos{(cursor + length*0.5f) * 0.5f, y, z};
        add_box(out, prefix + "_segment", {width, height, thickness}, material, pos);
        collision.push_back({collision_id++, {pos.x - width*0.5f, y - height*0.5f, z - thickness*0.5f},
                             {pos.x + width*0.5f, y + height*0.5f, z + thickness*0.5f}, 0});
    }
}

void add_z_wall(MeshAssembly& out, std::vector<BuildingCollisionVolume>& collision,
                const std::string& material, const std::string& prefix, float x, float y,
                float length, float thickness, float height, std::vector<Gap> gaps,
                std::uint64_t& collision_id) {
    std::sort(gaps.begin(), gaps.end(), [](const Gap& a, const Gap& b) { return a.center < b.center; });
    float cursor = -length * 0.5f;
    for (const Gap& gap : gaps) {
        const float start = std::clamp(gap.center - gap.width * 0.5f, -length * 0.5f, length * 0.5f);
        const float end = std::clamp(gap.center + gap.width * 0.5f, -length * 0.5f, length * 0.5f);
        if (start > cursor + 1e-4f) {
            const float width = start - cursor;
            const Vec3 pos{x, y, (cursor + start) * 0.5f};
            add_box(out, prefix + "_segment", {thickness, height, width}, material, pos);
            collision.push_back({collision_id++, {x - thickness*0.5f, y - height*0.5f, pos.z - width*0.5f},
                                 {x + thickness*0.5f, y + height*0.5f, pos.z + width*0.5f}, 0});
        }
        cursor = std::max(cursor, end);
    }
    if (cursor < length * 0.5f - 1e-4f) {
        const float width = length * 0.5f - cursor;
        const Vec3 pos{x, y, (cursor + length*0.5f) * 0.5f};
        add_box(out, prefix + "_segment", {thickness, height, width}, material, pos);
        collision.push_back({collision_id++, {x - thickness*0.5f, y - height*0.5f, pos.z - width*0.5f},
                             {x + thickness*0.5f, y + height*0.5f, pos.z + width*0.5f}, 0});
    }
}

std::uint32_t grid_columns(std::uint32_t room_count) noexcept {
    return std::max<std::uint32_t>(1U, static_cast<std::uint32_t>(std::ceil(std::sqrt(static_cast<float>(room_count)))));
}

void add_furniture_part(BuildingDefinition& out, const BuildingFurniture& f) {
    switch (f.type) {
    case BuildingFurnitureType::Bed:
        add_box(out.geometry, "bed_" + std::to_string(f.id), f.size, out.config.furniture_material, f.position);
        add_box(out.geometry, "pillow_" + std::to_string(f.id), {f.size.x*0.35f, f.size.y*0.2f, f.size.z*0.45f}, out.config.furniture_material,
                {f.position.x, f.position.y + f.size.y*0.58f, f.position.z + f.size.z*0.2f});
        break;
    case BuildingFurnitureType::Desk:
    case BuildingFurnitureType::Table:
    case BuildingFurnitureType::Counter:
        add_box(out.geometry, "surface_" + std::to_string(f.id), f.size, out.config.furniture_material, f.position);
        break;
    case BuildingFurnitureType::Chair:
        add_box(out.geometry, "chair_" + std::to_string(f.id), f.size, out.config.furniture_material, f.position);
        break;
    case BuildingFurnitureType::Sofa:
        add_box(out.geometry, "sofa_" + std::to_string(f.id), f.size, out.config.furniture_material, f.position);
        break;
    case BuildingFurnitureType::Cabinet:
        add_box(out.geometry, "cabinet_" + std::to_string(f.id), f.size, out.config.furniture_material, f.position);
        break;
    }
}

} // namespace

bool BuildingRoom::contains(float x, float y, float z) const noexcept {
    return x >= min.x && x <= max.x && y >= min.y && y <= max.y && z >= min.z && z <= max.z;
}

bool BuildingOpening::valid() const noexcept {
    return id != 0 && floor < std::numeric_limits<std::uint32_t>::max() && finite3(position) && finite3(size) &&
           size.x > 0 && size.y > 0 && size.z > 0 && (type == BuildingOpeningType::Door || type == BuildingOpeningType::Window);
}

bool BuildingInteractable::valid() const noexcept {
    return id != 0 && target_id != 0 && finite3(position);
}

bool BuildingFurniture::valid() const noexcept {
    return id != 0 && room_id != 0 && finite3(position) && finite3(size) && size.x > 0 && size.y > 0 && size.z > 0 && finite(rotation);
}

bool BuildingCollisionVolume::valid() const noexcept {
    return id != 0 && finite3(min) && finite3(max) && min.x <= max.x && min.y <= max.y && min.z <= max.z;
}

bool BuildingDefinition::valid() const noexcept {
    if (seed == 0 || !geometry.valid() || rooms.empty()) return false;
    for (const auto& room : rooms) if (room.id == 0 || room.name.empty() || !finite3(room.min) || !finite3(room.max) || room.min.x >= room.max.x || room.min.z >= room.max.z) return false;
    for (const auto& opening : openings) if (!opening.valid()) return false;
    for (const auto& interactable : interactables) if (!interactable.valid()) return false;
    for (const auto& item : furniture) if (!item.valid()) return false;
    for (const auto& box : collision) if (!box.valid()) return false;
    return true;
}

BuildingDefinition generate_building(const BuildingConfig& input) {
    BuildingConfig c = input;
    c.width = std::max(c.width, 4.0f);
    c.depth = std::max(c.depth, 4.0f);
    c.floors = std::clamp(c.floors, 1U, 32U);
    c.floor_height = std::max(c.floor_height, 2.4f);
    c.wall_thickness = std::clamp(c.wall_thickness, 0.05f, 0.6f);
    c.floor_thickness = std::clamp(c.floor_thickness, 0.05f, 0.5f);
    c.rooms_per_floor = std::clamp(c.rooms_per_floor, 1U, 16U);
    c.door_width = std::clamp(c.door_width, 0.6f, std::min(1.6f, c.width * 0.25f));
    c.door_height = std::clamp(c.door_height, 1.8f, c.floor_height - 0.2f);
    c.window_width = std::clamp(c.window_width, 0.5f, c.width * 0.35f);
    c.window_height = std::clamp(c.window_height, 0.6f, c.floor_height * 0.5f);
    c.windows_per_wall = std::clamp(c.windows_per_wall, 0U, 6U);
    c.stair_width = std::clamp(c.stair_width, 0.7f, std::min(c.width, c.depth) * 0.45f);
    c.furniture_density = std::clamp(c.furniture_density, 0U, 5U);

    BuildingDefinition out;
    out.seed = c.seed == 0 ? 1 : c.seed;
    out.config = c;
    std::uint64_t collision_id = 1;
    std::uint64_t opening_id = 1;
    std::uint64_t interactable_id = 1;
    std::uint64_t furniture_id = 1;
    const std::uint32_t cols = grid_columns(c.rooms_per_floor);
    const std::uint32_t rows = static_cast<std::uint32_t>(std::ceil(static_cast<float>(c.rooms_per_floor) / static_cast<float>(cols)));
    const float inner_w = c.width - 2.0f * c.wall_thickness;
    const float inner_d = c.depth - 2.0f * c.wall_thickness;
    const float room_w = inner_w / static_cast<float>(cols);
    const float room_d = inner_d / static_cast<float>(rows);
    const std::uint64_t base_seed = mix64(out.seed);

    for (std::uint32_t floor = 0; floor < c.floors; ++floor) {
        const float floor_base = static_cast<float>(floor) * c.floor_height;
        const float slab_y = floor_base + c.floor_thickness * 0.5f;
        add_box(out.geometry, "floor_" + std::to_string(floor), {c.width, c.floor_thickness, c.depth}, c.floor_material,
                {0, slab_y, 0});
        if (floor == c.floors - 1U)
            add_box(out.geometry, "roof", {c.width, c.floor_thickness, c.depth}, c.floor_material,
                    {0, floor_base + c.floor_height - c.floor_thickness*0.5f, 0});

        for (std::uint32_t room_index = 0; room_index < c.rooms_per_floor; ++room_index) {
            const std::uint32_t row = room_index / cols;
            const std::uint32_t col = room_index % cols;
            const float min_x = -inner_w * 0.5f + static_cast<float>(col) * room_w;
            const float max_x = min_x + room_w;
            const float min_z = -inner_d * 0.5f + static_cast<float>(row) * room_d;
            const float max_z = min_z + room_d;
            const std::uint64_t room_id = mix64(base_seed ^ (static_cast<std::uint64_t>(floor) << 32U) ^ room_index);
            out.rooms.push_back({room_id, floor, "room_" + std::to_string(floor) + "_" + std::to_string(room_index),
                                 {min_x, floor_base, min_z}, {max_x, floor_base + c.floor_height, max_z},
                                 {(min_x+max_x)*0.5f, floor_base + c.floor_height*0.5f, (min_z+max_z)*0.5f}});

            if (c.furniture_density > 0) {
                const float jitter_x = (unit(room_id ^ 11U) - 0.5f) * room_w * 0.12f;
                const float jitter_z = (unit(room_id ^ 17U) - 0.5f) * room_d * 0.12f;
                const std::uint32_t count = std::min<std::uint32_t>(c.furniture_density, 3U);
                if (count >= 1U && room_w > 2.4f && room_d > 2.4f)
                    out.furniture.push_back({furniture_id++, BuildingFurnitureType::Bed, room_id,
                                             {out.rooms.back().center.x + jitter_x, floor_base + 0.28f, out.rooms.back().max.z - 0.75f},
                                             {1.7f,0.5f,0.9f}, 0});
                if (count >= 2U)
                    out.furniture.push_back({furniture_id++, BuildingFurnitureType::Desk, room_id,
                                             {out.rooms.back().center.x - room_w*0.22f, floor_base + 0.55f, out.rooms.back().center.z},
                                             {1.1f,0.12f,0.55f}, 0});
                if (count >= 3U)
                    out.furniture.push_back({furniture_id++, BuildingFurnitureType::Chair, room_id,
                                             {out.rooms.back().center.x - room_w*0.22f, floor_base + 0.45f, out.rooms.back().center.z + 0.55f},
                                             {0.45f,0.9f,0.45f}, 0});
            }
        }

        // Interior partitions. Each divider receives one deterministic doorway.
        for (std::uint32_t col = 1; col < cols; ++col) {
            const float x = -inner_w * 0.5f + static_cast<float>(col) * room_w;
            for (std::uint32_t row = 0; row < rows; ++row) {
                const std::uint32_t room_index = row * cols + (col - 1U);
                if (room_index >= c.rooms_per_floor || room_index + 1U >= c.rooms_per_floor) continue;
                const float z0 = -inner_d * 0.5f + static_cast<float>(row) * room_d;
                const float z1 = z0 + room_d;
                const float door_center = z0 + room_d * (0.35f + unit(base_seed ^ col ^ (static_cast<std::uint64_t>(row)<<8U) ^ floor) * 0.3f);
                add_z_wall(out.geometry, out.collision, c.wall_material, "partition_x", x, floor_base + c.floor_height*0.5f,
                           z1-z0, c.wall_thickness, c.floor_height,
                           {{door_center - (z0+z1)*0.5f, c.door_width, 0, c.door_height}}, collision_id);
                const std::uint64_t door_id = mix64(base_seed ^ 0xD00RULL ^ floor ^ (static_cast<std::uint64_t>(col)<<16U) ^ row);
                if (door_id != 0) {
                    out.openings.push_back({door_id, BuildingOpeningType::Door, floor, BuildingWallAxis::Z,
                                            {x, floor_base + c.door_height*0.5f, door_center}, {c.wall_thickness, c.door_height, c.door_width}, false, false});
                    out.interactables.push_back({interactable_id++, BuildingInteractableType::Door, door_id,
                                                 {x, floor_base + 1.0f, door_center}, true});
                    out.collision.push_back({collision_id++, {x-c.wall_thickness*0.5f, floor_base, door_center-c.door_width*0.5f},
                                              {x+c.wall_thickness*0.5f, floor_base+c.floor_height, door_center+c.door_width*0.5f}, door_id});
                }
            }
        }
        for (std::uint32_t row = 1; row < rows; ++row) {
            const float z = -inner_d * 0.5f + static_cast<float>(row) * room_d;
            for (std::uint32_t col = 0; col < cols; ++col) {
                const std::uint32_t room_index = (row - 1U) * cols + col;
                if (room_index >= c.rooms_per_floor || room_index + cols >= c.rooms_per_floor) continue;
                const float x0 = -inner_w * 0.5f + static_cast<float>(col) * room_w;
                const float x1 = x0 + room_w;
                const float door_center = x0 + room_w * (0.35f + unit(base_seed ^ row ^ (static_cast<std::uint64_t>(col)<<8U) ^ floor) * 0.3f);
                add_x_wall(out.geometry, out.collision, c.wall_material, "partition_z", z, floor_base + c.floor_height*0.5f,
                           x1-x0, c.wall_thickness, c.floor_height,
                           {{door_center-(x0+x1)*0.5f,c.door_width,0,c.door_height}}, collision_id);
                const std::uint64_t door_id = mix64(base_seed ^ 0xD00EULL ^ floor ^ (static_cast<std::uint64_t>(row)<<16U) ^ col);
                if (door_id != 0) {
                    out.openings.push_back({door_id, BuildingOpeningType::Door, floor, BuildingWallAxis::X,
                                            {door_center, floor_base + c.door_height*0.5f, z}, {c.door_width, c.door_height, c.wall_thickness}, false, false});
                    out.interactables.push_back({interactable_id++, BuildingInteractableType::Door, door_id,
                                                 {door_center, floor_base + 1.0f, z}, true});
                    out.collision.push_back({collision_id++, {door_center-c.door_width*0.5f, floor_base, z-c.wall_thickness*0.5f},
                                              {door_center+c.door_width*0.5f, floor_base+c.floor_height, z+c.wall_thickness*0.5f}, door_id});
                }
            }
        }

        // Exterior walls and an entrance on the ground floor.
        const float wall_y = floor_base + c.floor_height*0.5f;
        std::vector<Gap> south_gaps;
        if (floor == 0U) south_gaps.push_back({0.0f, c.door_width, 0, c.door_height});
        add_x_wall(out.geometry, out.collision, c.wall_material, "south", -c.depth*0.5f + c.wall_thickness*0.5f,
                   wall_y, c.width-c.wall_thickness*2, c.wall_thickness, c.floor_height, south_gaps, collision_id);
        add_x_wall(out.geometry, out.collision, c.wall_material, "north", c.depth*0.5f-c.wall_thickness*0.5f,
                   wall_y, c.width-c.wall_thickness*2, c.wall_thickness, c.floor_height, {}, collision_id);
        add_z_wall(out.geometry, out.collision, c.wall_material, "west", -c.width*0.5f+c.wall_thickness*0.5f,
                   wall_y, c.depth-c.wall_thickness*2, c.wall_thickness, c.floor_height, {}, collision_id);
        add_z_wall(out.geometry, out.collision, c.wall_material, "east", c.width*0.5f-c.wall_thickness*0.5f,
                   wall_y, c.depth-c.wall_thickness*2, c.wall_thickness, c.floor_height, {}, collision_id);

        if (floor == 0U) {
            const std::uint64_t door_id = mix64(base_seed ^ 0xE17EULL);
            out.openings.push_back({door_id, BuildingOpeningType::Door, floor, BuildingWallAxis::X,
                                    {0, c.door_height*0.5f, -c.depth*0.5f+c.wall_thickness*0.5f}, {c.door_width,c.door_height,c.wall_thickness}, false, true});
            out.interactables.push_back({interactable_id++, BuildingInteractableType::Door, door_id,
                                         {0, 1.0f, -c.depth*0.5f}, true});
            out.collision.push_back({collision_id++, {-c.door_width*0.5f,0,-c.depth*0.5f},
                                     {c.door_width*0.5f,c.floor_height,-c.depth*0.5f+c.wall_thickness},door_id});
        }

        // Window visuals and opening metadata. Window collision remains part of the wall intentionally;
        // full window break-through is a later physics/portal concern.
        if (c.windows_per_wall > 0U) {
            for (std::uint32_t w = 0; w < c.windows_per_wall; ++w) {
                const float t = (static_cast<float>(w)+1.0f)/static_cast<float>(c.windows_per_wall+1U);
                const float x = -inner_w*0.5f + t*inner_w;
                const float wy = floor_base + c.floor_height*0.55f;
                const float z_n = c.depth*0.5f-c.wall_thickness*1.1f;
                const float z_s = -c.depth*0.5f+c.wall_thickness*1.1f;
                add_box(out.geometry,"window_n_"+std::to_string(floor)+"_"+std::to_string(w),{c.window_width, c.window_height, 0.04f},c.glass_material,{x,wy,z_n});
                add_box(out.geometry,"window_s_"+std::to_string(floor)+"_"+std::to_string(w),{c.window_width, c.window_height, 0.04f},c.glass_material,{x,wy,z_s});
                const auto id_n=mix64(base_seed^0x1000ULL^floor^(static_cast<std::uint64_t>(w)<<16U));
                const auto id_s=mix64(base_seed^0x2000ULL^floor^(static_cast<std::uint64_t>(w)<<16U));
                out.openings.push_back({id_n,BuildingOpeningType::Window,floor,BuildingWallAxis::X,{x,wy,z_n},{c.window_width,c.window_height,0.04f},false,true});
                out.openings.push_back({id_s,BuildingOpeningType::Window,floor,BuildingWallAxis::X,{x,wy,z_s},{c.window_width,c.window_height,0.04f},false,true});
                out.interactables.push_back({interactable_id++,BuildingInteractableType::Window,id_n,{x,wy,z_n},true});
                out.interactables.push_back({interactable_id++,BuildingInteractableType::Window,id_s,{x,wy,z_s},true});
            }
        }
    }

    // Stair flight connecting floors, placed away from the main entrance.
    if (c.floors > 1U) {
        const float stair_x = c.width*0.27f;
        const float stair_z = c.depth*0.18f;
        const std::uint32_t steps = std::max<std::uint32_t>(10U, static_cast<std::uint32_t>(std::ceil(c.floor_height/0.20f)));
        for (std::uint32_t floor = 0; floor + 1U < c.floors; ++floor) {
            const float base_y = static_cast<float>(floor)*c.floor_height;
            for (std::uint32_t step=0; step<steps; ++step) {
                const float t = static_cast<float>(step+1U)/static_cast<float>(steps);
                add_box(out.geometry,"stair_"+std::to_string(floor)+"_"+std::to_string(step),
                        {c.stair_width,0.20f,c.stair_width*0.48f},c.floor_material,
                        {stair_x,base_y+0.10f+ t*(c.floor_height-0.20f)*0.5f, stair_z + (t-0.5f)*c.stair_width*2.2f});
                const float y=base_y+0.20f+t*c.floor_height;
                out.collision.push_back({collision_id++,
                    {stair_x-c.stair_width*0.5f,base_y,stair_z+(t-0.5f)*c.stair_width*2.2f-c.stair_width*0.24f},
                    {stair_x+c.stair_width*0.5f,y,stair_z+(t-0.5f)*c.stair_width*2.2f+c.stair_width*0.24f},0});
            }
        }
    }

    for (const auto& f : out.furniture) add_furniture_part(out, f);

    // Foundation keeps the generated shell grounded and supplies a stable collision footprint.
    add_box(out.geometry,"foundation",{c.width,0.30f,c.depth},c.wall_material,{0,-0.15f,0});
    out.collision.push_back({collision_id++,{-c.width*0.5f,-0.3f,-c.depth*0.5f},{c.width*0.5f,0,c.depth*0.5f},0});
    return out;
}

bool set_building_door_open(BuildingDefinition& building, std::uint64_t door_id, bool open) noexcept {
    auto* door = const_cast<BuildingOpening*>(find_building_door(building, door_id));
    if (!door) return false;
    door->open = open;
    return true;
}

const BuildingOpening* find_building_door(const BuildingDefinition& building, std::uint64_t door_id) noexcept {
    for (const auto& opening : building.openings)
        if (opening.id == door_id && opening.type == BuildingOpeningType::Door) return &opening;
    return nullptr;
}

std::vector<BuildingCollisionVolume> active_building_collision(const BuildingDefinition& building) {
    std::vector<BuildingCollisionVolume> active;
    active.reserve(building.collision.size());
    for (const auto& volume : building.collision) {
        if (volume.door_id != 0) {
            const auto* door = find_building_door(building, volume.door_id);
            if (door && door->open) continue;
        }
        active.push_back(volume);
    }
    return active;
}

const BuildingRoom* find_building_room(const BuildingDefinition& building, Vec3 position) noexcept {
    for (const auto& room : building.rooms)
        if (room.contains(position.x, position.y, position.z)) return &room;
    return nullptr;
}

} // namespace exgine

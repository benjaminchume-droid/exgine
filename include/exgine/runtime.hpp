#pragma once

#include "exgine/geometry.hpp"
#include "exgine/ir.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

namespace exgine {

using EntityId = std::uint64_t;
inline constexpr EntityId invalid_entity = 0;

struct Transform {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Entity {
    EntityId id = invalid_entity;
    NodeKind kind = NodeKind::Property;
    std::string name;
    Transform transform{};
    bool active = true;
    std::vector<Property> properties;
    std::shared_ptr<MeshAssembly> geometry;
};

class EntityRegistry {
public:
    EntityId create(NodeKind kind, std::string name);
    bool destroy(EntityId id) noexcept;
    [[nodiscard]] Entity* get(EntityId id) noexcept;
    [[nodiscard]] const Entity* get(EntityId id) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return entities_.size(); }
    void clear() noexcept;

private:
    EntityId next_id_ = 1;
    std::unordered_map<EntityId, Entity> entities_;
};

struct WorldState {
    EntityId world_entity = invalid_entity;
    EntityRegistry entities;
    std::uint64_t tick = 0;
    double elapsed_seconds = 0.0;
};

class Runtime {
public:
    Runtime() = default;

    bool load(const IR& ir);
    void update(double delta_seconds) noexcept;
    void reset() noexcept;

    bool attach_geometry(EntityId id, MeshAssembly geometry);
    [[nodiscard]] const WorldState& state() const noexcept { return state_; }
    [[nodiscard]] WorldState& state() noexcept { return state_; }

private:
    WorldState state_;
    bool loaded_ = false;
};

} // namespace exgine

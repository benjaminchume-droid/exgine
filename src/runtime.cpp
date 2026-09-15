#include "exgine/runtime.hpp"

#include <limits>
#include <utility>

namespace exgine {

EntityId EntityRegistry::create(NodeKind kind, std::string name) {
    if (next_id_ == invalid_entity || next_id_ == std::numeric_limits<EntityId>::max()) return invalid_entity;
    const EntityId id = next_id_++;
    entities_.emplace(id, Entity{id, kind, std::move(name), {}, true, {}});
    return id;
}

bool EntityRegistry::destroy(EntityId id) noexcept { return entities_.erase(id) != 0; }

Entity* EntityRegistry::get(EntityId id) noexcept {
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : &it->second;
}

const Entity* EntityRegistry::get(EntityId id) const noexcept {
    const auto it = entities_.find(id);
    return it == entities_.end() ? nullptr : &it->second;
}

void EntityRegistry::clear() noexcept {
    entities_.clear();
    next_id_ = 1;
}

namespace {

bool instantiate(const Node& node, EntityRegistry& registry, EntityId& world_entity) {
    const EntityId id = registry.create(node.kind, node.name);
    if (id == invalid_entity) return false;
    if (node.kind == NodeKind::World) world_entity = id;
    auto* entity = registry.get(id);
    entity->properties = node.properties;
    for (const auto& child : node.children) {
        if (!instantiate(child, registry, world_entity)) return false;
    }
    return true;
}

} // namespace

bool Runtime::load(const IR& ir) {
    reset();
    if (ir.root.kind != NodeKind::World) return false;
    if (!instantiate(ir.root, state_.entities, state_.world_entity)) {
        reset();
        return false;
    }
    loaded_ = state_.world_entity != invalid_entity;
    return loaded_;
}

void Runtime::update(double delta_seconds) noexcept {
    if (!loaded_ || delta_seconds < 0.0) return;
    state_.elapsed_seconds += delta_seconds;
    ++state_.tick;
}

void Runtime::reset() noexcept {
    state_ = {};
    loaded_ = false;
}

} // namespace exgine

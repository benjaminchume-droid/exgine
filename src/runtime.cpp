#include "exgine/runtime.hpp"

#include <utility>

namespace exgine {

EntityId EntityRegistry::create(NodeKind kind, std::string name) {
    const EntityId id = next_id_++;
    entities_.emplace(id, Entity{id, kind, std::move(name), {}, true, {}});
    return id;
}

bool EntityRegistry::destroy(EntityId id) noexcept {
    return entities_.erase(id) != 0;
}

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

void instantiate(const Node& node, EntityRegistry& registry, EntityId& world_entity) {
    const EntityId id = registry.create(node.kind, node.name);
    if (node.kind == NodeKind::World) world_entity = id;
    if (auto* entity = registry.get(id)) entity->properties = node.properties;
    for (const auto& child : node.children) instantiate(child, registry, world_entity);
}

} // namespace

bool Runtime::load(const IR& ir) {
    reset();
    if (ir.root.kind != NodeKind::World) return false;
    instantiate(ir.root, state_.entities, state_.world_entity);
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

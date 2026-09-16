#include "exgine/world_render.hpp"

#include "exgine/material.hpp"

#include <algorithm>
#include <string>

namespace exgine {

EntityId WorldRenderBridge::create_render_entity(Runtime& runtime, NodeKind kind, const char* name) {
    const auto id = runtime.state().entities.create(kind, name ? std::string{name} : std::string{});
    if (!id) return invalid_entity;
    auto* entity = runtime.state().entities.get(id);
    if (!entity) return invalid_entity;
    entity->scene_node = runtime.scene().create(runtime.scene().root());
    if (!entity->scene_node) {
        runtime.state().entities.destroy(id);
        return invalid_entity;
    }
    return id;
}

bool WorldRenderBridge::attach_mesh(Runtime& runtime, EntityId entity, const Mesh& mesh, const char* material) {
    if (!entity || !mesh.valid() || !material || !*material) return false;
    MeshAssembly assembly;
    MeshPart part;
    part.name = material;
    part.mesh = mesh;
    part.material_slot = material;
    assembly.parts.push_back(std::move(part));
    return runtime.attach_geometry(entity, std::move(assembly));
}

bool WorldRenderBridge::sync(Runtime& runtime, Vec3 focus_position) {
    if (!runtime.stream_open_world(focus_position)) return false;
    auto* streamer = runtime.open_world_streamer();
    if (!streamer) return false;

    // World rendering is a normal material-resource consumer. Nothing in this
    // bridge is tied to a particular game's art direction.
    (void)runtime.define_material(make_real_world_material("terrain", 0x5445525241494Eull));
    (void)runtime.define_material(make_real_world_material("water", 0x5741544552ull));

    const auto& chunks = streamer->chunks();
    for (const auto& [coord, streamed] : chunks) {
        const auto& chunk = streamed.chunk;

        if (chunk.terrain.valid()) {
            auto it = terrain_entities_.find(coord);
            EntityId id = it == terrain_entities_.end() ? invalid_entity : it->second;
            if (!id || !runtime.state().entities.get(id)) {
                id = create_render_entity(runtime, NodeKind::Terrain, "WorldChunkTerrain");
                if (!id) return false;
                terrain_entities_[coord] = id;
            }
            if (!attach_mesh(runtime, id, chunk.terrain, "terrain")) return false;
            if (auto* e = runtime.state().entities.get(id)) {
                e->transform = {0, 0, 0};
                (void)runtime.scene().set_local_transform(e->scene_node,
                    SceneTransform{{0, 0, 0}, {0, 0, 0}, {1, 1, 1}});
            }
        }

        if (chunk.water.valid() && !chunk.water.indices.empty()) {
            auto it = water_entities_.find(coord);
            EntityId id = it == water_entities_.end() ? invalid_entity : it->second;
            if (!id || !runtime.state().entities.get(id)) {
                id = create_render_entity(runtime, NodeKind::Prop, "WorldChunkWater");
                if (!id) return false;
                water_entities_[coord] = id;
            }
            if (!attach_mesh(runtime, id, chunk.water, "water")) return false;
            if (auto* e = runtime.state().entities.get(id)) {
                e->transform = {0, 0, 0};
                (void)runtime.scene().set_local_transform(e->scene_node,
                    SceneTransform{{0, 0, 0}, {0, 0, 0}, {1, 1, 1}});
            }
        }
    }

    for (auto it = terrain_entities_.begin(); it != terrain_entities_.end();) {
        if (chunks.find(it->first) == chunks.end()) {
            runtime.state().entities.destroy(it->second);
            it = terrain_entities_.erase(it);
        } else ++it;
    }
    for (auto it = water_entities_.begin(); it != water_entities_.end();) {
        if (chunks.find(it->first) == chunks.end()) {
            runtime.state().entities.destroy(it->second);
            it = water_entities_.erase(it);
        } else ++it;
    }

    runtime.scene().update_world_transforms();
    return true;
}

void WorldRenderBridge::clear(Runtime& runtime) noexcept {
    for (const auto& [coord, id] : terrain_entities_) (void)coord, runtime.state().entities.destroy(id);
    for (const auto& [coord, id] : water_entities_) (void)coord, runtime.state().entities.destroy(id);
    terrain_entities_.clear();
    water_entities_.clear();
}

} // namespace exgine

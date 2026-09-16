#include "exgine/world_render.hpp"

#include "exgine/material.hpp"

#include <cmath>
#include <string>
#include <utility>

namespace exgine {

namespace {

void destroy_render_entity(Runtime& runtime, EntityId id) noexcept {
    if (!id) return;
    if (auto* entity = runtime.state().entities.get(id)) {
        if (entity->scene_node != invalid_scene_node)
            (void)runtime.scene().destroy(entity->scene_node);
    }
    (void)runtime.state().entities.destroy(id);
}

}

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

    if (!runtime.material_resource("terrain"))
        (void)runtime.define_material(make_real_world_material("terrain", 0x5445525241494Eull));
    if (!runtime.material_resource("water"))
        (void)runtime.define_material(make_real_world_material("water", 0x5741544552ull));

    for (const auto id : runtime.state().entities.ids()) {
        auto* e = runtime.state().entities.get(id);
        if (!e) continue;
        if (e->name == "GroundPlate" || e->name == "GroundGrass" || e->name == "GroundWater")
            e->active = false;
    }

    const auto& chunks = streamer->chunks();
    const float time = static_cast<float>(runtime.state().elapsed_seconds);
    for (const auto& [coord, streamed] : chunks) {
        const auto& chunk = streamed.chunk;
        const auto generation = streamed.generation;

        if (chunk.terrain.valid()) {
            auto it = terrain_entities_.find(coord);
            EntityId id = it == terrain_entities_.end() ? invalid_entity : it->second;
            if (!id || !runtime.state().entities.get(id)) {
                id = create_render_entity(runtime, NodeKind::Terrain, "WorldChunkTerrain");
                if (!id) return false;
                terrain_entities_[coord] = id;
            }
            if (terrain_generations_[coord] != generation) {
                if (!attach_mesh(runtime, id, chunk.terrain, "terrain")) return false;
                terrain_generations_[coord] = generation;
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
            if (water_generations_[coord] != generation) {
                if (!attach_mesh(runtime, id, chunk.water, "water")) return false;
                water_generations_[coord] = generation;
            }

            // Interim generic water animation: move each streamed patch by a
            // small phase-shifted vertical wave. It deliberately lives in the
            // generic world bridge; a future water render pass can replace it
            // with vertex displacement/reflection without changing gameplay.
            if (auto* e = runtime.state().entities.get(id); e && e->scene_node) {
                const float phase = static_cast<float>(coord.x) * 0.71f +
                                    static_cast<float>(coord.z) * 1.13f;
                const float wave = std::sin(time * 1.7f + phase) * 0.025f;
                (void)runtime.scene().set_local_transform(
                    e->scene_node,
                    SceneTransform{{0.f, wave, 0.f}, {0.f, 0.f, 0.f}, {1.f, 1.f, 1.f}});
            }
        }
    }

    for (auto it = terrain_entities_.begin(); it != terrain_entities_.end();) {
        if (chunks.find(it->first) == chunks.end()) {
            destroy_render_entity(runtime, it->second);
            terrain_generations_.erase(it->first);
            it = terrain_entities_.erase(it);
        } else ++it;
    }
    for (auto it = water_entities_.begin(); it != water_entities_.end();) {
        if (chunks.find(it->first) == chunks.end()) {
            destroy_render_entity(runtime, it->second);
            water_generations_.erase(it->first);
            it = water_entities_.erase(it);
        } else ++it;
    }

    runtime.scene().update_world_transforms();
    return true;
}

void WorldRenderBridge::clear(Runtime& runtime) noexcept {
    for (const auto& [coord, id] : terrain_entities_) {
        (void)coord;
        destroy_render_entity(runtime, id);
    }
    for (const auto& [coord, id] : water_entities_) {
        (void)coord;
        destroy_render_entity(runtime, id);
    }
    terrain_entities_.clear();
    water_entities_.clear();
    terrain_generations_.clear();
    water_generations_.clear();
}

} // namespace exgine

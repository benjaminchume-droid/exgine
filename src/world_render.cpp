#include "exgine/world_render.hpp"

#include "exgine/material.hpp"

#include <cmath>
#include <memory>
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

PhysicsBodyId WorldRenderBridge::attach_heightfield(Runtime& runtime, WorldChunkCoord coord,
                                                     const Mesh& terrain, float chunk_size) {
    auto* physics = runtime.physics();
    if (!physics || !terrain.valid() || terrain.vertices.size() < 4 || chunk_size <= 0.f)
        return invalid_physics_body;

    const auto count = terrain.vertices.size();
    const auto side = static_cast<std::size_t>(std::llround(std::sqrt(static_cast<double>(count))));
    if (side < 2 || side * side != count) return invalid_physics_body;

    auto heights = std::make_shared<std::vector<float>>();
    heights->reserve(count);
    for (const auto& vertex : terrain.vertices) {
        if (!std::isfinite(vertex.position.y)) return invalid_physics_body;
        heights->push_back(vertex.position.y);
    }

    PhysicsRigidBodyDesc body;
    body.type = PhysicsBodyType::Static;
    body.transform.position = {
        static_cast<float>(coord.x) * chunk_size,
        0.f,
        static_cast<float>(coord.z) * chunk_size
    };
    body.flags = static_cast<PhysicsBodyFlags>(PhysicsBodyFlag::AllowSleep);
    const auto body_id = physics->create_body(body);
    if (!body_id) return invalid_physics_body;

    PhysicsHeightFieldShape heightfield;
    heightfield.width = static_cast<std::uint32_t>(side);
    heightfield.depth = static_cast<std::uint32_t>(side);
    heightfield.cell_size = chunk_size / static_cast<float>(side - 1);
    heightfield.heights = std::move(heights);

    PhysicsColliderDesc collider;
    collider.shape.type = PhysicsShapeType::HeightField;
    collider.shape.data = std::move(heightfield);
    collider.material.static_friction = 0.9f;
    collider.material.dynamic_friction = 0.75f;
    collider.material.restitution = 0.02f;

    if (!physics->add_collider(body_id, collider)) {
        (void)physics->destroy_body(body_id);
        return invalid_physics_body;
    }
    return body_id;
}

void WorldRenderBridge::destroy_physics(Runtime& runtime, WorldChunkCoord coord) noexcept {
    const auto it = terrain_physics_.find(coord);
    if (it == terrain_physics_.end()) return;
    if (auto* physics = runtime.physics()) (void)physics->destroy_body(it->second);
    terrain_physics_.erase(it);
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

                destroy_physics(runtime, coord);
                const auto physics_body = attach_heightfield(
                    runtime, coord, chunk.terrain,
                    runtime.world() ? runtime.world()->config().chunk_size : 0.f);
                if (physics_body) terrain_physics_[coord] = physics_body;
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
            destroy_physics(runtime, it->first);
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
        destroy_render_entity(runtime, id);
        destroy_physics(runtime, coord);
    }
    for (const auto& [coord, id] : water_entities_) {
        destroy_render_entity(runtime, id);
    }
    terrain_entities_.clear();
    water_entities_.clear();
    terrain_generations_.clear();
    water_generations_.clear();
    terrain_physics_.clear();
}

} // namespace exgine

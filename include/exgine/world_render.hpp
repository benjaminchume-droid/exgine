#pragma once

#include "exgine/runtime.hpp"

#include <cstdint>
#include <unordered_map>

namespace exgine {

// Converts streamed procedural WorldChunks into ordinary renderable runtime
// entities. The bridge is deliberately generic: it does not know about a game,
// mission, city, or asset catalog. It only binds terrain/water meshes to the
// engine's normal Entity -> SceneGraph -> RenderFrame pipeline.
class WorldRenderBridge final {
public:
    bool sync(Runtime& runtime, Vec3 focus_position);
    void clear(Runtime& runtime) noexcept;
    [[nodiscard]] std::size_t terrain_entities() const noexcept { return terrain_entities_.size(); }
    [[nodiscard]] std::size_t water_entities() const noexcept { return water_entities_.size(); }

private:
    std::unordered_map<WorldChunkCoord, EntityId, WorldChunkCoordHash> terrain_entities_;
    std::unordered_map<WorldChunkCoord, EntityId, WorldChunkCoordHash> water_entities_;

    static EntityId create_render_entity(Runtime& runtime, NodeKind kind, const char* name);
    static bool attach_mesh(Runtime& runtime, EntityId entity, const Mesh& mesh, const char* material);
};

} // namespace exgine

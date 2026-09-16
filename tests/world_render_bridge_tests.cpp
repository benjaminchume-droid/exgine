#include "exgine/world_render.hpp"

#include <cassert>
#include <iostream>

int main() {
    exgine::IR ir;
    ir.root.kind = exgine::NodeKind::World;
    ir.root.name = "world";
    auto& terrain = ir.add_child(exgine::NodeKind::Terrain, "Terrain");
    terrain.properties.push_back({"amplitude", 18.0});
    terrain.properties.push_back({"chunk_size", 32.0});
    terrain.properties.push_back({"resolution", static_cast<std::int64_t>(8)});
    terrain.properties.push_back({"sea_level", 0.0});
    terrain.properties.push_back({"seed", static_cast<std::int64_t>(42)});
    auto& player = ir.add_child(exgine::NodeKind::Player, "Player");
    player.properties.push_back({"x", 0.0});
    player.properties.push_back({"y", 2.0});
    player.properties.push_back({"z", 0.0});

    exgine::Runtime runtime;
    assert(runtime.load(ir));

    exgine::WorldRenderBridge bridge;
    assert(bridge.sync(runtime, {0, 0, 0}));
    assert(bridge.terrain_entities() > 0);
    assert(runtime.state().entities.size() > 2);

    bool found_terrain = false;
    for (const auto id : runtime.state().entities.ids()) {
        const auto* entity = runtime.state().entities.get(id);
        if (entity && entity->kind == exgine::NodeKind::Terrain && entity->geometry && entity->geometry->valid()) {
            found_terrain = true;
            break;
        }
    }
    assert(found_terrain);

    bridge.clear(runtime);
    assert(bridge.terrain_entities() == 0);
    assert(bridge.water_entities() == 0);
    std::cout << "World render bridge: PASS\n";
    return 0;
}

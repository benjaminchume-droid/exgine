#include "exgine/showcase.hpp"
#include <cassert>
#include <string>

int main() {
    using namespace exgine;
    const std::string manifest =
        "name = Integrated Showcase\nversion = 1.0\nstartup_scene = Main\ntick_rate = 60\nscene = Main\n";
    const std::string scene =
        "world World {\n"
        "terrain Terrain { amplitude = 40 seed = 42 }\n"
        "player Player { x = 7.5 y = 2 z = 12 }\n"
        "npc Guide { x = 11 y = 1 z = 7 }\n"
        "building MainHouse { floors = 2 rooms = 4 doors = 2 windows = 6 stairs = 1 furniture = 4 seed = 42 }\n"
        "vehicle DemoCar { x = 14 y = 1 z = 3 type = SportsCar }\n"
        "prop HouseAsset { x = -7 y = 0 z = 2 }\n"
        "prop RainFX { x = 7 y = 0 z = 5 }\n"
        "prop PhysicsCrate { x = 2 y = 5 z = 2 }\n"
        "prop HUDCrosshair { x = 7.5 y = 5.2 z = 11.5 }\n"
        "prop HUDHealth { x = 7.5 y = 4.75 z = 11.5 }\n"
        "}\n";
    const std::string house_obj =
        "v -1 0 -1\n v 1 0 -1\n v 1 2 -1\n v -1 2 -1\n"
        "v -1 0 1\n v 1 0 1\n v 1 2 1\n v -1 2 1\n"
        "vt 0 0\n vt 1 0\n vt 1 1\n vt 0 1\n"
        "vn 0 0 -1\n vn 1 0 0\n vn 0 0 1\n vn -1 0 0\n vn 0 1 0\n"
        "f 1/1/1 2/2/1 3/3/1\n f 1/1/1 3/3/1 4/4/1\n"
        "f 2/1/2 6/2/2 7/3/2\n f 2/1/2 7/3/2 3/4/2\n"
        "f 6/1/3 5/2/3 8/3/3\n f 6/1/3 8/3/3 7/4/3\n"
        "f 5/1/4 1/2/4 4/3/4\n f 5/1/4 4/3/4 8/4/4\n"
        "f 4/1/5 3/2/5 7/3/5\n f 4/1/5 7/3/5 8/4/5\n";
    ShowcaseGame game([&](std::string_view uri, std::string& out) {
        if (uri == "scenes/main.scene") { out = scene; return true; }
        if (uri == "showcase/assets/house.obj") { out = house_obj; return true; }
        return false;
    });
    assert(game.open(manifest));
    assert(game.start());

    RenderFrame frame;
    RenderResult render;
    for (int i = 0; i < 180; ++i) {
        assert(game.update(1.0 / 60.0));
        assert(game.build_frame(frame, render));
        assert(render.success && !frame.draws.empty());
    }
    const auto before = game.metrics();
    const auto save = game.save();
    assert(!save.empty());
    assert(game.restore(save));
    assert(before.frames == 180 && before.rendered_frames == 180);
    assert(before.weather_particles > 0);
    assert(before.physics_steps > 0);
    assert(before.ui_widgets >= 2);
    assert(before.audio_sources >= 1);
    assert(before.draw_calls > 0);
    assert(before.valid());
    return 0;
}

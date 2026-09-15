#include "exgine/game.hpp"

#include <cassert>
#include <string>

using namespace exgine;

namespace {
constexpr const char* kProject =
    "name = Seasons Demo\n"
    "version = 1.0\n"
    "startup_scene = Main\n"
    "tick_rate = 30\n"
    "seconds_per_day = 120\n"
    "days_per_year = 40\n"
    "days_per_week = 7\n"
    "dawn_start = 5\n"
    "dawn_end = 7\n"
    "dusk_start = 18\n"
    "dusk_end = 20\n"
    "season = Spring,0,10,0.1,1.1,1\n"
    "season = Summer,10,10,0.3,1,1.05\n"
    "season = Winter,30,10,-0.3,0.3,0.9\n"
    "scene = Main\n"
    "asset = scene,MainScene,scenes/main.scene\n"
    "asset = world,World,worlds/world.world\n"
    "setting.language = en\n";

void scene_loader(std::string_view name, IR& ir) {
    if (name != "Main") return;
    ir.root.children.push_back(Node{NodeKind::Prop,"Tree",{},{}});
}
}

int main() {
    const auto parsed = parse_project(kProject);
    assert(parsed.success);
    assert(parsed.project.valid());
    assert(parsed.project.name == "Seasons Demo");
    assert(parsed.project.tick_rate == 30);
    assert(parsed.project.assets.size() == 2);
    const auto round_trip = parse_project(serialize_project(parsed.project));
    assert(round_trip.success && round_trip.project.assets.size() == parsed.project.assets.size());

    GameRuntime game;
    assert(game.load_project(parsed.project, [](std::string_view name, IR& ir) { scene_loader(name, ir); return name == "Main"; }));
    assert(game.loaded() && game.active_scene() == "Main");
    assert(game.runtime().state().entities.size() >= 1);
    assert(game.set_variable("difficulty", "hard"));
    assert(game.variable("difficulty") == "hard");
    const double before = game.environment().state().elapsed_seconds;
    assert(game.update(1.0));
    assert(game.environment().state().elapsed_seconds > before);

    const auto save = game.save();
    assert(save.valid());
    const auto encoded = serialize_save(save);
    const auto decoded = parse_save(encoded);
    assert(decoded.valid() && decoded.project_name == save.project_name);
    assert(game.restore(decoded));

    game.reset();
    assert(!game.loaded());
    return 0;
}

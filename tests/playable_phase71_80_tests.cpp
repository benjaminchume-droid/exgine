#include "exgine/playable.hpp"
#include <cassert>

using namespace exgine;

int main() {
    const std::string manifest =
        "name = Playable\nversion = 1.0\nstartup_scene = Main\ntick_rate = 60\n"
        "seconds_per_day = 1200\ndays_per_year = 360\nscene = Main\n";
    PlayableGame game({}, [](std::string_view, std::string& out) {
        out = "game Main { world World { player Player { x = 0 y = 2 z = 0 } npc Guide { x = 3 y = 1 z = 3 } building House { floors = 2 } vehicle Car { type = Car } } }";
        return true;
    });
    assert(game.open_project(manifest));
    assert(game.status() == PlayableStatus::Menu);
    assert(game.render_plan().valid);
    assert(game.render_plan().order.size() == 12);
    assert(game.start());
    assert(game.status() == PlayableStatus::Playing);
    for (int i = 0; i < 3; ++i) assert(game.update(1.0 / 60.0));
    PlayableFrame frame{};
    assert(game.build_frame(frame));
    assert(frame.updated && frame.render_frame_valid && frame.frame_id > 0);
    assert(game.pause());
    assert(game.status() == PlayableStatus::Paused);
    const auto save = game.save();
    assert(!save.empty());
    assert(game.status() == PlayableStatus::Playing);
    assert(game.restore(save));
    assert(game.status() == PlayableStatus::Playing);
    assert(game.acceptance().ready);
    assert(game.stats().frames >= 3);
    return 0;
}

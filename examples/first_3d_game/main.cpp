#include "exgine/showcase.hpp"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <string_view>

namespace {

bool read_text(const std::string& path, std::string& out) {
    std::ifstream file(path, std::ios::binary);
    if (!file) return false;
    out.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
    return true;
}

bool load_project_asset(std::string_view uri, std::string& out) {
    const std::string direct(uri);
    if (read_text(direct, out)) return true;

    const std::string first_light = "examples/first_3d_game/" + direct;
    if (read_text(first_light, out)) return true;

    const std::string showcase = "examples/showcase/" + direct;
    return read_text(showcase, out);
}

} // namespace

int main() {
    std::string manifest;
    if (!load_project_asset("project.exg", manifest)) {
        std::cerr << "First Light: project.exg not found\n";
        return 1;
    }

    exgine::ShowcaseGame game(load_project_asset);
    if (!game.open(manifest)) {
        std::cerr << "First Light: project failed to open\n";
        return 2;
    }
    if (!game.start()) {
        std::cerr << "First Light: runtime failed to start\n";
        return 3;
    }

    for (int frame = 0; frame < 120; ++frame) {
        if (!game.update(1.0 / 60.0)) {
            std::cerr << "First Light: update failed at frame " << frame << "\n";
            return 4;
        }
    }

    const auto& metrics = game.metrics();
    if (!metrics.valid()) {
        std::cerr << "First Light: invalid runtime metrics\n";
        return 5;
    }

    std::cout << "EXGINE First Light OK\n"
              << "frames=" << metrics.frames << '\n'
              << "rendered_frames=" << metrics.rendered_frames << '\n'
              << "physics_steps=" << metrics.physics_steps << '\n'
              << "weather_particles=" << metrics.weather_particles << '\n'
              << "audio_events=" << metrics.procedural_audio_events << '\n';
    return 0;
}

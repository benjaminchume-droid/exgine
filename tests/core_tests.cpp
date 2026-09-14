#include "exgine/diagnostic.hpp"
#include "exgine/ir.hpp"
#include "exgine/version.hpp"
#include "exgine/world.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

namespace {

void test_version() {
    assert(exgine::version_major == 0);
    assert(exgine::version_minor == 1);
    assert(exgine::version_patch == 0);
    assert(exgine::version_string == "0.1.0");
}

void test_diagnostics() {
    exgine::DiagnosticBag diagnostics;
    assert(diagnostics.empty());
    assert(!diagnostics.has_errors());

    diagnostics.note("parser note", {4, 2, 5});
    diagnostics.warning("unused property", {10, 3, 2});
    assert(!diagnostics.has_errors());
    assert(diagnostics.all().size() == 2);
    assert(diagnostics.all()[0].location.line == 2);

    diagnostics.error("invalid value", {20, 4, 1});
    assert(diagnostics.has_errors());
    assert(diagnostics.all().size() == 3);
}

void test_ir() {
    exgine::IR game;
    game.add_property("terrain", std::string{"procedural"});
    game.add_property("terrain_height", std::int64_t{20});

    auto& building = game.add_child(exgine::NodeKind::Building, "house");
    building.properties.push_back({"floors", std::int64_t{2}});

    assert(game.root.kind == exgine::NodeKind::World);
    assert(game.root.name == "world");
    assert(game.root.properties.size() == 2);
    assert(game.root.children.size() == 1);
    assert(game.root.children[0].name == "house");

    const auto* floors = game.root.children[0].find_property("floors");
    assert(floors != nullptr);
    assert(std::get<std::int64_t>(floors->value) == 2);
    assert(game.root.find_property("missing") == nullptr);
}

void test_world_determinism() {
    const exgine::TerrainConfig config{20, 1337, 32.0f};
    const exgine::ProceduralWorld first(config);
    const exgine::ProceduralWorld second(config);

    const float a = first.sample_height(32, 64);
    const float b = second.sample_height(32, 64);

    assert(std::isfinite(a));
    assert(a == b);
    assert(first.config().height == 20);
    assert(first.config().seed == 1337);
    assert(first.config().chunk_size == 32.0f);
}

} // namespace

int main() {
    test_version();
    test_diagnostics();
    test_ir();
    test_world_determinism();
    return 0;
}

#include "exgine/ast.hpp"
#include "exgine/character.hpp"
#include "exgine/compiler.hpp"
#include "exgine/diagnostic.hpp"
#include "exgine/engine.hpp"
#include "exgine/geometry.hpp"
#include "exgine/ir.hpp"
#include "exgine/lexer.hpp"
#include "exgine/runtime.hpp"
#include "exgine/source.hpp"
#include "exgine/token.hpp"
#include "exgine/version.hpp"
#include "exgine/world.hpp"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <string>

namespace {
const exgine::SourceText valid_source(R"(
    game "Green World" {
        world {
            terrain { type = procedural height = 20 }
            building "house" { floors = 2 enabled = true }
            vehicle "car" { type = "sports_car" }
        }
    }
)");

void test_version() { assert(exgine::version_string == "0.1.0"); }
void test_diagnostics() {
    exgine::DiagnosticBag diagnostics;
    assert(diagnostics.empty());
    diagnostics.note("parser note", {4, 2, 5}); diagnostics.warning("unused property", {10, 3, 2});
    diagnostics.error("invalid value", {20, 4, 1});
    assert(diagnostics.has_errors() && diagnostics.all().size() == 3);
}
void test_ir() {
    exgine::IR game;
    game.add_property("terrain", std::string{"procedural"});
    auto& building = game.add_child(exgine::NodeKind::Building, "house");
    building.properties.push_back({"floors", std::int64_t{2}});
    assert(std::get<std::int64_t>(game.root.children[0].find_property("floors")->value) == 2);
    assert(exgine::NodeKind::NPC != exgine::NodeKind::Player);
}
void test_world_determinism() {
    const exgine::TerrainConfig config{20, 1337, 32.0f};
    const exgine::ProceduralWorld first(config), second(config);
    assert(std::isfinite(first.sample_height(32, 64)));
    assert(first.sample_height(32, 64) == second.sample_height(32, 64));
}
void test_language_pipeline() {
    exgine::DiagnosticBag diagnostics;
    const auto tokens = exgine::Lexer(valid_source).tokenize(diagnostics);
    assert(!diagnostics.has_errors() && tokens.front().kind == exgine::TokenKind::Game);
    const auto result = exgine::Compiler{}.compile(valid_source);
    assert(result.succeeded() && result.ir.has_value());
    assert(result.ir->root.kind == exgine::NodeKind::World && result.ir->root.children.size() == 3);
    assert(result.ir->root.children[1].name == "house");
    assert(std::get<std::int64_t>(result.ir->root.children[1].find_property("floors")->value) == 2);
}
void test_runtime_instantiation() {
    const auto compiled = exgine::Compiler{}.compile(valid_source);
    assert(compiled.succeeded());
    exgine::Runtime runtime;
    assert(runtime.load(*compiled.ir));
    assert(runtime.state().world_entity != exgine::invalid_entity && runtime.state().entities.size() == 4);
    const auto building_id = runtime.state().world_entity + 1;
    assert(runtime.attach_geometry(building_id, exgine::MeshAssembly{{{"house_mesh", exgine::make_box({{8,3,8}}), "concrete", {}, {1,1,1}}}}));
    assert(runtime.state().entities.get(building_id)->geometry != nullptr);
    runtime.update(0.5); runtime.update(0.25);
    assert(runtime.state().tick == 2 && runtime.state().elapsed_seconds == 0.75);
    runtime.reset();
    assert(runtime.state().entities.size() == 0 && runtime.state().tick == 0);
}
void test_geometry_primitives() {
    const auto box = exgine::make_box({{2,3,4}});
    const auto sphere = exgine::make_sphere({1.0f,16,8});
    const auto cylinder = exgine::make_cylinder({0.5f,2.0f,16});
    const auto capsule = exgine::make_capsule();
    assert(box.valid() && sphere.valid() && cylinder.valid() && capsule.valid());
    assert(box.vertices.size() == 24 && sphere.vertices.size() == 16 * 9);
}
void test_procedural_characters() {
    exgine::CharacterDefinition npc;
    npc.type = exgine::CharacterType::NPC; npc.appearance.seed = 42;
    npc.appearance.watch = true; npc.appearance.backpack = true;
    const auto first = exgine::generate_character(npc);
    const auto second = exgine::generate_character(npc);
    assert(first.valid() && second.valid() && first.parts.size() == second.parts.size());
    assert(first.parts.size() == 10 && first.parts[0].mesh.vertices.size() == second.parts[0].mesh.vertices.size());
    npc.appearance.seed = 43;
    const auto different = exgine::generate_character(npc);
    assert(different.valid() && different.parts[0].position.x != first.parts[0].position.x);
}
void test_engine_end_to_end() {
    exgine::Engine engine;
    assert(engine.load(valid_source));
    assert(engine.runtime().state().entities.size() == 4);
    engine.update(1.0 / 60.0); assert(engine.runtime().state().tick == 1);
    assert(!engine.load(exgine::SourceText("game \"Broken\" { world { terrain_height = 0 } }")));
    assert(engine.runtime().state().entities.size() == 0 && engine.diagnostics().has_errors());
}
} // namespace

int main() {
    test_version(); test_diagnostics(); test_ir(); test_world_determinism(); test_language_pipeline();
    test_runtime_instantiation(); test_geometry_primitives(); test_procedural_characters(); test_engine_end_to_end();
    return 0;
}

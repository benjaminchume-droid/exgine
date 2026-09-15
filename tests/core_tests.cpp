#include "exgine/ast.hpp"
#include "exgine/compiler.hpp"
#include "exgine/diagnostic.hpp"
#include "exgine/engine.hpp"
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
    diagnostics.note("parser note", {4, 2, 5});
    diagnostics.warning("unused property", {10, 3, 2});
    diagnostics.error("invalid value", {20, 4, 1});
    assert(diagnostics.has_errors() && diagnostics.all().size() == 3);
}

void test_ir() {
    exgine::IR game;
    game.add_property("terrain", std::string{"procedural"});
    auto& building = game.add_child(exgine::NodeKind::Building, "house");
    building.properties.push_back({"floors", std::int64_t{2}});
    assert(std::get<std::int64_t>(game.root.children[0].find_property("floors")->value) == 2);
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
    assert(result.ir->root.kind == exgine::NodeKind::World);
    assert(result.ir->root.children.size() == 3);
    assert(result.ir->root.children[1].name == "house");
    assert(std::get<std::int64_t>(result.ir->root.children[1].find_property("floors")->value) == 2);
}

void test_runtime_instantiation() {
    const auto compiled = exgine::Compiler{}.compile(valid_source);
    assert(compiled.succeeded());
    exgine::Runtime runtime;
    assert(runtime.load(*compiled.ir));
    assert(runtime.state().world_entity != exgine::invalid_entity);
    assert(runtime.state().entities.size() == 4);
    runtime.update(0.5);
    runtime.update(0.25);
    assert(runtime.state().tick == 2);
    assert(runtime.state().elapsed_seconds == 0.75);
    runtime.reset();
    assert(runtime.state().entities.size() == 0 && runtime.state().tick == 0);
}

void test_engine_end_to_end() {
    exgine::Engine engine;
    assert(engine.load(valid_source));
    assert(engine.runtime().state().entities.size() == 4);
    engine.update(1.0 / 60.0);
    assert(engine.runtime().state().tick == 1);
    assert(!engine.load(exgine::SourceText("game \"Broken\" { world { terrain_height = 0 } }")));
    assert(engine.runtime().state().entities.size() == 0);
    assert(engine.diagnostics().has_errors());
}

} // namespace

int main() {
    test_version(); test_diagnostics(); test_ir(); test_world_determinism();
    test_language_pipeline(); test_runtime_instantiation(); test_engine_end_to_end();
    return 0;
}

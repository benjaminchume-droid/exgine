#include "exgine/ast.hpp"
#include "exgine/compiler.hpp"
#include "exgine/diagnostic.hpp"
#include "exgine/ir.hpp"
#include "exgine/lexer.hpp"
#include "exgine/source.hpp"
#include "exgine/token.hpp"
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
    diagnostics.note("parser note", {4, 2, 5});
    diagnostics.warning("unused property", {10, 3, 2});
    assert(!diagnostics.has_errors());
    assert(diagnostics.all().size() == 2);
    assert(diagnostics.all()[0].location.line == 2);
    diagnostics.error("invalid value", {20, 4, 1});
    assert(diagnostics.has_errors());
}

void test_ir() {
    exgine::IR game;
    game.add_property("terrain", std::string{"procedural"});
    game.add_property("terrain_height", std::int64_t{20});
    auto& building = game.add_child(exgine::NodeKind::Building, "house");
    building.properties.push_back({"floors", std::int64_t{2}});
    assert(game.root.children.size() == 1);
    assert(std::get<std::int64_t>(game.root.children[0].find_property("floors")->value) == 2);
}

void test_world_determinism() {
    const exgine::TerrainConfig config{20, 1337, 32.0f};
    const exgine::ProceduralWorld first(config), second(config);
    assert(std::isfinite(first.sample_height(32, 64)));
    assert(first.sample_height(32, 64) == second.sample_height(32, 64));
}

void test_lexer() {
    const exgine::SourceText source("game \"Demo\" { world { terrain = procedural height = 20 } }");
    exgine::DiagnosticBag diagnostics;
    const auto tokens = exgine::Lexer(source).tokenize(diagnostics);
    assert(!diagnostics.has_errors());
    assert(tokens.size() > 8);
    assert(tokens[0].kind == exgine::TokenKind::Game);
    assert(tokens[1].kind == exgine::TokenKind::String);
    assert(tokens.back().kind == exgine::TokenKind::EndOfFile);
}

void test_compiler_pipeline() {
    const exgine::SourceText source(R"(
        // A complete Phase 1 source program.
        game "Green World" {
            world {
                terrain { type = procedural height = 20 }
                building "house" { floors = 2 enabled = true }
                vehicle "car" { type = "sports_car" }
            }
        }
    )");
    const auto result = exgine::Compiler{}.compile(source);
    assert(result.succeeded());
    assert(result.ir.has_value());
    assert(result.ir->root.kind == exgine::NodeKind::World);
    assert(result.ir->root.name == "world");
    assert(result.ir->root.children.size() == 3);
    assert(result.ir->root.children[0].kind == exgine::NodeKind::Terrain);
    assert(result.ir->root.children[1].name == "house");
    assert(std::get<std::int64_t>(result.ir->root.children[1].find_property("floors")->value) == 2);
}

void test_semantic_rejection() {
    const auto result = exgine::Compiler{}.compile(exgine::SourceText(
        "game \"Broken\" { world { height = 0 height = 3 } }"));
    assert(!result.succeeded());
    assert(result.diagnostics.has_errors());
}

} // namespace

int main() {
    test_version();
    test_diagnostics();
    test_ir();
    test_world_determinism();
    test_lexer();
    test_compiler_pipeline();
    test_semantic_rejection();
    return 0;
}

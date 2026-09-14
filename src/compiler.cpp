#include "exgine/compiler.hpp"

#include "exgine/ast.hpp"
#include "exgine/lexer.hpp"
#include "exgine/semantic.hpp"

#include <utility>

namespace exgine {
namespace {

NodeKind ir_kind(AstNodeKind kind) {
    switch (kind) {
    case AstNodeKind::World: return NodeKind::World;
    case AstNodeKind::Terrain: return NodeKind::Terrain;
    case AstNodeKind::Vegetation: return NodeKind::Vegetation;
    case AstNodeKind::Building: return NodeKind::Building;
    case AstNodeKind::Vehicle: return NodeKind::Vehicle;
    }
    return NodeKind::Property;
}

PropertyValue ir_value(const AstValue& value) {
    return std::visit([](const auto& item) -> PropertyValue { return item; }, value);
}

Node lower_node(const AstNode& ast) {
    Node node{ir_kind(ast.kind), ast.name, {}, {}};
    node.properties.reserve(ast.properties.size());
    node.children.reserve(ast.children.size());
    for (const auto& property : ast.properties)
        node.properties.push_back({property.name, ir_value(property.value)});
    for (const auto& child : ast.children)
        node.children.push_back(lower_node(child));
    return node;
}

} // namespace

CompileResult Compiler::compile(SourceText source) const {
    CompileResult result;
    Lexer lexer(source);
    const auto tokens = lexer.tokenize(result.diagnostics);
    Parser parser(tokens);
    const auto ast = parser.parse(result.diagnostics);

    if (!result.diagnostics.has_errors()) {
        SemanticValidator validator;
        validator.validate(ast, result.diagnostics);
    }
    if (result.diagnostics.has_errors() || ast.games.empty()) return result;

    IR ir;
    const auto& game = ast.games.front();
    for (const auto& ast_node : game.nodes) {
        if (ast_node.kind == AstNodeKind::World) {
            ir.root = lower_node(ast_node);
            break;
        }
    }
    result.ir = std::move(ir);
    return result;
}

} // namespace exgine

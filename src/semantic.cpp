#include "exgine/semantic.hpp"

#include <cstdint>
#include <string>
#include <unordered_set>

namespace exgine {

void SemanticValidator::validate(const AstDocument& document, DiagnosticBag& diagnostics) const {
    if (document.games.empty()) {
        diagnostics.error("source must contain a game declaration");
        return;
    }
    if (document.games.size() > 1) {
        diagnostics.error("a source unit may contain only one game declaration", document.games[1].location);
    }

    const auto& game = document.games.front();
    if (game.name.empty()) diagnostics.error("game name cannot be empty", game.location);

    std::size_t world_count = 0;
    for (const auto& node : game.nodes) {
        if (node.kind == AstNodeKind::World) ++world_count;
        else diagnostics.error("only a world block may appear directly inside a game", node.location);
        validate_node(node, node.kind == AstNodeKind::World, diagnostics);
    }
    if (world_count == 0) diagnostics.error("game must contain exactly one world block", game.location);
    if (world_count > 1) diagnostics.error("game must contain exactly one world block", game.location);
}

void SemanticValidator::validate_node(const AstNode& node, bool inside_world, DiagnosticBag& diagnostics) const {
    std::unordered_set<std::string> names;
    for (const auto& property : node.properties) {
        if (!names.insert(property.name).second) {
            diagnostics.error("duplicate property '" + property.name + "'", property.location);
        }
        if (property.name == "terrain_height") {
            if (const auto* value = std::get_if<std::int64_t>(&property.value); value != nullptr && *value <= 0)
                diagnostics.error("terrain_height must be greater than zero", property.location);
        }
    }

    if (node.kind == AstNodeKind::Terrain && !inside_world)
        diagnostics.error("terrain must be declared inside world", node.location);

    for (const auto& child : node.children) {
        if (node.kind != AstNodeKind::World && child.kind == AstNodeKind::World)
            diagnostics.error("world blocks cannot be nested", child.location);
        validate_node(child, inside_world || node.kind == AstNodeKind::World, diagnostics);
    }
}

} // namespace exgine

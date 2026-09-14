#include "exgine/ast.hpp"

#include <cstdlib>
#include <string>

namespace exgine {
namespace {

class ParserImpl {
public:
    ParserImpl(const std::vector<Token>& tokens, DiagnosticBag& diagnostics)
        : tokens_(tokens), diagnostics_(diagnostics) {}

    AstDocument parse() {
        AstDocument document;
        while (!check(TokenKind::EndOfFile)) {
            if (!match(TokenKind::Game)) {
                error_here("expected 'game' declaration");
                recover_to(TokenKind::Game);
                continue;
            }
            document.games.push_back(parse_game(previous().location));
        }
        return document;
    }

private:
    const std::vector<Token>& tokens_;
    DiagnosticBag& diagnostics_;
    std::size_t current_ = 0;

    const Token& peek() const { return tokens_[current_]; }
    const Token& previous() const { return tokens_[current_ - 1]; }
    bool check(TokenKind kind) const { return peek().kind == kind; }
    const Token& advance() { if (!check(TokenKind::EndOfFile)) ++current_; return previous(); }
    bool match(TokenKind kind) { if (!check(kind)) return false; advance(); return true; }

    void error_here(const std::string& message) { diagnostics_.error(message, peek().location); }
    void recover_to(TokenKind kind) {
        while (!check(kind) && !check(TokenKind::EndOfFile)) advance();
    }

    const Token* consume(TokenKind kind, const char* message) {
        if (check(kind)) return &advance();
        error_here(message);
        return nullptr;
    }

    AstGame parse_game(SourceLocation location) {
        AstGame game;
        game.location = location;
        if (check(TokenKind::String) || check(TokenKind::Identifier)) game.name = advance().lexeme;
        else error_here("expected game name after 'game'");
        if (!consume(TokenKind::LeftBrace, "expected '{' after game name")) return game;
        while (!check(TokenKind::RightBrace) && !check(TokenKind::EndOfFile)) {
            auto node = parse_node();
            if (node.has_value()) game.nodes.push_back(std::move(*node));
        }
        consume(TokenKind::RightBrace, "expected '}' at end of game");
        return game;
    }

    static AstNodeKind node_kind(TokenKind kind) {
        switch (kind) {
        case TokenKind::World: return AstNodeKind::World;
        case TokenKind::Terrain: return AstNodeKind::Terrain;
        case TokenKind::Vegetation: return AstNodeKind::Vegetation;
        case TokenKind::Building: return AstNodeKind::Building;
        case TokenKind::Vehicle: return AstNodeKind::Vehicle;
        default: return AstNodeKind::World;
        }
    }

    std::optional<AstNode> parse_node() {
        const Token start = peek();
        const bool known = start.kind == TokenKind::World || start.kind == TokenKind::Terrain ||
                           start.kind == TokenKind::Vegetation || start.kind == TokenKind::Building ||
                           start.kind == TokenKind::Vehicle;
        if (!known) { error_here("expected a world, terrain, vegetation, building, or vehicle block"); advance(); return std::nullopt; }
        advance();
        AstNode node{node_kind(start.kind), {}, {}, {}, start.location};
        if (check(TokenKind::String) || check(TokenKind::Identifier)) node.name = advance().lexeme;
        if (!consume(TokenKind::LeftBrace, "expected '{' after node declaration")) return node;
        while (!check(TokenKind::RightBrace) && !check(TokenKind::EndOfFile)) {
            if (check(TokenKind::Identifier) || check(TokenKind::Terrain) || check(TokenKind::Vegetation) ||
                check(TokenKind::Building) || check(TokenKind::Vehicle) || check(TokenKind::World)) {
                const std::size_t before = current_;
                auto child = parse_node();
                if (child.has_value()) node.children.push_back(std::move(*child));
                if (current_ == before) advance();
                continue;
            }
            if (check(TokenKind::Identifier)) parse_property(node);
            else { error_here("expected property assignment or child block"); advance(); }
        }
        consume(TokenKind::RightBrace, "expected '}' at end of node");
        return node;
    }

    void parse_property(AstNode& node) {
        const Token name = advance();
        if (!consume(TokenKind::Equals, "expected '=' after property name")) return;
        AstValue value;
        if (match(TokenKind::Integer)) value = static_cast<std::int64_t>(std::strtoll(previous().lexeme.c_str(), nullptr, 10));
        else if (match(TokenKind::Number)) value = std::strtod(previous().lexeme.c_str(), nullptr);
        else if (match(TokenKind::True)) value = true;
        else if (match(TokenKind::False)) value = false;
        else if (match(TokenKind::String)) value = previous().lexeme;
        else if (match(TokenKind::Identifier) || match(TokenKind::Procedural)) value = previous().lexeme;
        else { error_here("expected a literal value after '='"); return; }
        node.properties.push_back({name.lexeme, std::move(value), name.location});
    }
};

} // namespace

AstDocument Parser::parse(DiagnosticBag& diagnostics) const {
    ParserImpl parser(tokens_, diagnostics);
    return parser.parse();
}

} // namespace exgine

#include "exgine/token.hpp"

namespace exgine {

std::string_view token_kind_name(TokenKind kind) noexcept {
    switch (kind) {
    case TokenKind::EndOfFile: return "end of file";
    case TokenKind::Identifier: return "identifier";
    case TokenKind::String: return "string";
    case TokenKind::Integer: return "integer";
    case TokenKind::Number: return "number";
    case TokenKind::True: return "true";
    case TokenKind::False: return "false";
    case TokenKind::Game: return "game";
    case TokenKind::World: return "world";
    case TokenKind::Terrain: return "terrain";
    case TokenKind::Vegetation: return "vegetation";
    case TokenKind::Building: return "building";
    case TokenKind::Vehicle: return "vehicle";
    case TokenKind::Procedural: return "procedural";
    case TokenKind::Equals: return "=";
    case TokenKind::LeftBrace: return "{";
    case TokenKind::RightBrace: return "}";
    case TokenKind::LeftBracket: return "[";
    case TokenKind::RightBracket: return "]";
    case TokenKind::Comma: return ",";
    }
    return "unknown token";
}

} // namespace exgine

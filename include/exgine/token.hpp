#pragma once
#include "exgine/diagnostic.hpp"
#include <string>
#include <string_view>
namespace exgine { enum class TokenKind { EndOfFile, Identifier, String, Integer, Number, True, False, Game, World, Terrain, Vegetation, Building, Vehicle, Player, NPC, Bridge, Prop, Procedural, Equals, LeftBrace, RightBrace, LeftBracket, RightBracket, Comma }; struct Token { TokenKind kind=TokenKind::EndOfFile; std::string lexeme; SourceLocation location{}; }; [[nodiscard]] std::string_view token_kind_name(TokenKind) noexcept; }

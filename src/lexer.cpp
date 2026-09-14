#include "exgine/lexer.hpp"

#include <cctype>
#include <string>
#include <unordered_map>

namespace exgine {
namespace {

TokenKind keyword_kind(std::string_view word) {
    static const std::unordered_map<std::string_view, TokenKind> keywords{
        {"game", TokenKind::Game}, {"world", TokenKind::World},
        {"terrain", TokenKind::Terrain}, {"vegetation", TokenKind::Vegetation},
        {"building", TokenKind::Building}, {"vehicle", TokenKind::Vehicle},
        {"procedural", TokenKind::Procedural}, {"true", TokenKind::True},
        {"false", TokenKind::False}
    };
    const auto it = keywords.find(word);
    return it == keywords.end() ? TokenKind::Identifier : it->second;
}

} // namespace

std::vector<Token> Lexer::tokenize(DiagnosticBag& diagnostics) const {
    std::vector<Token> tokens;
    std::size_t i = 0;
    std::size_t line = 1;
    std::size_t column = 1;

    const auto advance = [&](std::size_t count = 1) mutable {
        for (std::size_t n = 0; n < count && i < source_.size(); ++n) {
            if (source_.at(i) == '\n') { ++line; column = 1; }
            else { ++column; }
            ++i;
        }
    };
    const auto location = [&] { return SourceLocation{i, line, column}; };

    while (i < source_.size()) {
        const char c = source_.at(i);
        if (std::isspace(static_cast<unsigned char>(c))) { advance(); continue; }
        if (c == '/' && source_.at(i + 1) == '/') {
            while (i < source_.size() && source_.at(i) != '\n') advance();
            continue;
        }

        const SourceLocation start = location();
        if (std::isalpha(static_cast<unsigned char>(c)) || c == '_') {
            const std::size_t begin = i;
            while (std::isalnum(static_cast<unsigned char>(source_.at(i))) || source_.at(i) == '_') advance();
            const std::string word(source_.text().substr(begin, i - begin));
            tokens.push_back({keyword_kind(word), word, start});
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c)) || (c == '-' && std::isdigit(static_cast<unsigned char>(source_.at(i + 1))))) {
            const std::size_t begin = i;
            bool decimal = false;
            if (c == '-') advance();
            while (std::isdigit(static_cast<unsigned char>(source_.at(i)))) advance();
            if (source_.at(i) == '.') {
                decimal = true; advance();
                while (std::isdigit(static_cast<unsigned char>(source_.at(i)))) advance();
            }
            if (source_.at(i) == '.') {
                diagnostics.error("invalid number: multiple decimal points", start);
                while (std::isdigit(static_cast<unsigned char>(source_.at(i))) || source_.at(i) == '.') advance();
            }
            tokens.push_back({decimal ? TokenKind::Number : TokenKind::Integer,
                              std::string(source_.text().substr(begin, i - begin)), start});
            continue;
        }

        if (c == '"') {
            advance();
            std::string value;
            bool closed = false;
            while (i < source_.size()) {
                const char ch = source_.at(i);
                if (ch == '"') { advance(); closed = true; break; }
                if (ch == '\n') {
                    diagnostics.error("unterminated string literal", start);
                    break;
                }
                if (ch == '\\') {
                    advance();
                    if (i >= source_.size()) break;
                    const char escaped = source_.at(i);
                    switch (escaped) {
                    case 'n': value.push_back('\n'); break;
                    case 'r': value.push_back('\r'); break;
                    case 't': value.push_back('\t'); break;
                    case '"': value.push_back('"'); break;
                    case '\\': value.push_back('\\'); break;
                    default: diagnostics.error("unknown string escape", {i, line, column}); value.push_back(escaped); break;
                    }
                    advance();
                } else { value.push_back(ch); advance(); }
            }
            if (closed) tokens.push_back({TokenKind::String, std::move(value), start});
            continue;
        }

        TokenKind kind = TokenKind::EndOfFile;
        switch (c) {
        case '=': kind = TokenKind::Equals; break;
        case '{': kind = TokenKind::LeftBrace; break;
        case '}': kind = TokenKind::RightBrace; break;
        case '[': kind = TokenKind::LeftBracket; break;
        case ']': kind = TokenKind::RightBracket; break;
        case ',': kind = TokenKind::Comma; break;
        default:
            diagnostics.error("unexpected character '" + std::string(1, c) + "'", start);
            advance();
            continue;
        }
        tokens.push_back({kind, std::string(1, c), start});
        advance();
    }

    tokens.push_back({TokenKind::EndOfFile, {}, {i, line, column}});
    return tokens;
}

} // namespace exgine

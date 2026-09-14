#pragma once

#include "exgine/diagnostic.hpp"
#include "exgine/source.hpp"
#include "exgine/token.hpp"

#include <vector>

namespace exgine {

class Lexer {
public:
    explicit Lexer(SourceText source) noexcept : source_(source) {}

    [[nodiscard]] std::vector<Token> tokenize(DiagnosticBag& diagnostics) const;

private:
    SourceText source_;
};

} // namespace exgine

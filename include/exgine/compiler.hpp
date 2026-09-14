#pragma once

#include "exgine/diagnostic.hpp"
#include "exgine/ir.hpp"
#include "exgine/source.hpp"

#include <optional>

namespace exgine {

struct CompileResult {
    std::optional<IR> ir;
    DiagnosticBag diagnostics;

    [[nodiscard]] bool succeeded() const noexcept {
        return ir.has_value() && !diagnostics.has_errors();
    }
};

class Compiler {
public:
    [[nodiscard]] CompileResult compile(SourceText source) const;
};

} // namespace exgine

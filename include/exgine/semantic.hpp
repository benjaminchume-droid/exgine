#pragma once

#include "exgine/ast.hpp"
#include "exgine/diagnostic.hpp"

namespace exgine {

class SemanticValidator {
public:
    void validate(const AstDocument& document, DiagnosticBag& diagnostics) const;

private:
    void validate_node(const AstNode& node, bool inside_world, DiagnosticBag& diagnostics) const;
};

} // namespace exgine

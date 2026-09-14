#include "exgine/diagnostic.hpp"

#include <utility>

namespace exgine {

void DiagnosticBag::add(DiagnosticSeverity severity, std::string message,
                        SourceLocation location) {
    diagnostics_.push_back(Diagnostic{severity, std::move(message), location});
}

void DiagnosticBag::note(std::string message, SourceLocation location) {
    add(DiagnosticSeverity::Note, std::move(message), location);
}

void DiagnosticBag::warning(std::string message, SourceLocation location) {
    add(DiagnosticSeverity::Warning, std::move(message), location);
}

void DiagnosticBag::error(std::string message, SourceLocation location) {
    add(DiagnosticSeverity::Error, std::move(message), location);
}

bool DiagnosticBag::has_errors() const noexcept {
    for (const auto& diagnostic : diagnostics_) {
        if (diagnostic.severity == DiagnosticSeverity::Error) {
            return true;
        }
    }
    return false;
}

bool DiagnosticBag::empty() const noexcept {
    return diagnostics_.empty();
}

const std::vector<Diagnostic>& DiagnosticBag::all() const noexcept {
    return diagnostics_;
}

} // namespace exgine

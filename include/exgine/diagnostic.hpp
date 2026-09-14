#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class DiagnosticSeverity {
    Note,
    Warning,
    Error
};

struct SourceLocation {
    std::size_t offset = 0;
    std::size_t line = 1;
    std::size_t column = 1;
};

struct Diagnostic {
    DiagnosticSeverity severity;
    std::string message;
    SourceLocation location{};
};

class DiagnosticBag {
public:
    void add(DiagnosticSeverity severity, std::string message,
             SourceLocation location = {});

    void note(std::string message, SourceLocation location = {});
    void warning(std::string message, SourceLocation location = {});
    void error(std::string message, SourceLocation location = {});

    [[nodiscard]] bool has_errors() const noexcept;
    [[nodiscard]] bool empty() const noexcept;
    [[nodiscard]] const std::vector<Diagnostic>& all() const noexcept;

private:
    std::vector<Diagnostic> diagnostics_;
};

} // namespace exgine

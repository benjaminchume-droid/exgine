#pragma once

#include "exgine/compiler.hpp"
#include "exgine/runtime.hpp"

namespace exgine {

class Engine {
public:
    [[nodiscard]] bool load(SourceText source);
    void update(double delta_seconds) noexcept;
    void reset() noexcept;

    [[nodiscard]] const Runtime& runtime() const noexcept { return runtime_; }
    [[nodiscard]] Runtime& runtime() noexcept { return runtime_; }
    [[nodiscard]] const DiagnosticBag& diagnostics() const noexcept { return diagnostics_; }

private:
    Runtime runtime_;
    DiagnosticBag diagnostics_;
};

} // namespace exgine

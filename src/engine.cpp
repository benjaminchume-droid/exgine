#include "exgine/engine.hpp"

namespace exgine {

bool Engine::load(SourceText source) {
    diagnostics_ = {};
    const auto result = Compiler{}.compile(std::move(source));
    diagnostics_ = result.diagnostics;
    if (!result.succeeded()) {
        runtime_.reset();
        return false;
    }
    return runtime_.load(*result.ir);
}

void Engine::update(double delta_seconds) noexcept {
    runtime_.update(delta_seconds);
}

void Engine::reset() noexcept {
    runtime_.reset();
    diagnostics_ = {};
}

} // namespace exgine

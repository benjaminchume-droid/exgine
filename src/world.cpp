#include "exgine/world.hpp"

#include <cmath>

namespace exgine {

ProceduralWorld::ProceduralWorld(TerrainConfig config) : config_(config) {}

float ProceduralWorld::sample_height(std::int64_t world_x,
                                     std::int64_t world_z) const noexcept {
    // Phase 0 deliberately keeps generation dependency-free and deterministic.
    // The production terrain generator will be introduced in Phase 3 without
    // changing the public world sampling contract.
    const double x = static_cast<double>(world_x) * 0.035;
    const double z = static_cast<double>(world_z) * 0.035;
    const double seed = static_cast<double>(config_.seed) * 0.001;
    const double base = std::sin(x + seed) * 0.55 + std::cos(z - seed) * 0.45;
    const double detail = std::sin((x + z) * 2.7) * 0.10;
    return static_cast<float>((base + detail) * static_cast<double>(config_.height));
}

} // namespace exgine

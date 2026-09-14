#include "exgine/world.hpp"

#include <cmath>

namespace exgine {

ProceduralWorld::ProceduralWorld(TerrainConfig config) : config_(config) {}

float ProceduralWorld::sample_height(int64_t world_x, int64_t world_z) const {
    // Deterministic, dependency-free prototype noise. This is intentionally
    // simple; a real generator will replace it with layered noise/octaves.
    const double x = static_cast<double>(world_x) * 0.035;
    const double z = static_cast<double>(world_z) * 0.035;
    const double seed = static_cast<double>(config_.seed) * 0.001;
    const double base = std::sin(x + seed) * 0.55 + std::cos(z - seed) * 0.45;
    const double detail = std::sin((x + z) * 2.7) * 0.10;
    return static_cast<float>((base + detail) * static_cast<double>(config_.height));
}

} // namespace exgine

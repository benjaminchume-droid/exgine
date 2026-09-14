#pragma once

#include <cstdint>

namespace exgine {

struct TerrainConfig {
    int height = 20;
    int seed = 1337;
    float chunk_size = 32.0f;
};

class ProceduralWorld {
public:
    explicit ProceduralWorld(TerrainConfig config = {});

    [[nodiscard]] float sample_height(std::int64_t world_x, std::int64_t world_z) const noexcept;
    [[nodiscard]] const TerrainConfig& config() const noexcept { return config_; }

private:
    TerrainConfig config_;
};

} // namespace exgine

#pragma once

#include <cstdint>
#include <string>

namespace exgine {

struct TerrainConfig {
    int height = 20;
    int seed = 1337;
    float chunk_size = 32.0f;
};

class ProceduralWorld {
public:
    explicit ProceduralWorld(TerrainConfig config = {});

    float sample_height(int64_t world_x, int64_t world_z) const;
    const TerrainConfig& config() const noexcept { return config_; }

private:
    TerrainConfig config_;
};

} // namespace exgine

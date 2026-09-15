#pragma once

#include "exgine/high_fidelity.hpp"
#include "exgine/world.hpp"
#include <cstdint>
#include <vector>

namespace exgine {

enum class WorldLayerKind : std::uint8_t { Terrain, Water, Vegetation, Road, Settlement, Landmark };
enum class WorldDetailLevel : std::uint8_t { Far=0, Mid=1, Near=2, Hero=3 };

struct WorldInterestPoint { Vec3 position{}; float radius=256.0f; float priority=1.0f; };
struct WorldChunkDetail {
    WorldChunkCoord coord{};
    WorldDetailLevel detail=WorldDetailLevel::Far;
    bool terrain=false,water=false,vegetation=false,structures=false,roads=false;
};
struct WorldRegion {
    WorldChunkCoord coord{};
    Vec3 origin{};
    float size=256.0f;
    std::uint64_t seed=0;
    float terrain_min=0.0f,terrain_max=0.0f;
    std::uint32_t water_count=0,vegetation_count=0,structure_count=0,road_count=0;
    std::vector<WorldInterestPoint> interests;
    [[nodiscard]] bool valid() const noexcept { return size>0.0f && seed!=0; }
};
struct WorldBuildConfig {
    float chunk_size=256.0f;
    std::uint32_t samples_per_axis=33;
    std::uint32_t vegetation_instances=512;
    std::uint32_t structure_budget=16;
    std::uint32_t road_budget=8;
    float water_level=0.0f;
    std::uint64_t seed=1;
    TerrainHighDetailConfig terrain{};
};
struct WorldQueryResult { bool valid=false; float height=0.0f; BiomeSample biome{}; bool water=false; float water_depth=0.0f; };

class HighFidelityWorld {
public:
    explicit HighFidelityWorld(WorldBuildConfig config={});
    [[nodiscard]] const WorldBuildConfig& config() const noexcept { return config_; }
    [[nodiscard]] std::uint64_t seed() const noexcept { return config_.seed; }
    [[nodiscard]] WorldChunkCoord chunk_for(Vec3 position) const noexcept;
    [[nodiscard]] WorldRegion build_region(WorldChunkCoord coord) const;
    [[nodiscard]] WorldQueryResult query(Vec3 position) const noexcept;
    [[nodiscard]] std::vector<WorldChunkDetail> select_details(const std::vector<WorldInterestPoint>& interests,const std::vector<WorldChunkCoord>& candidates) const;
    [[nodiscard]] std::vector<WorldRegion> build_regions(const std::vector<WorldChunkCoord>& coords) const;
private:
    WorldBuildConfig config_{};
    [[nodiscard]] float terrain_height(float x,float z) const noexcept;
    [[nodiscard]] bool has_water(float x,float z,float& depth) const noexcept;
    [[nodiscard]] BiomeSample sample_biome(float x,float z,float height) const noexcept;
    [[nodiscard]] std::uint64_t region_seed(WorldChunkCoord coord) const noexcept;
};

} // namespace exgine

#pragma once

#include "exgine/geometry.hpp"
#include "exgine/texture.hpp"

#include <cstdint>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class Biome : std::uint8_t { Ocean, Beach, Plains, Forest, Desert, Rocky, Snow, Wetland };
enum class WaterBodyType : std::uint8_t { Ocean, Lake, River, Stream, Pond, Waterfall };

struct TerrainConfig {
    float amplitude = 80.0f;
    std::uint64_t seed = 1337;
    float chunk_size = 32.0f;
    std::uint32_t resolution = 32;
    float base_frequency = 0.0035f;
    float detail_frequency = 0.018f;
    float sea_level = 0.0f;
    std::uint32_t vegetation_density = 18;
};

struct BiomeSample { Biome biome = Biome::Plains; float moisture = 0.5f; float temperature = 0.5f; float slope = 0.0f; };
struct WaterBody { WaterBodyType type=WaterBodyType::Lake; std::uint64_t id=0; float level=0.0f; float depth=0.0f; Vec3 flow{}; };
struct VegetationInstance { std::uint64_t id=0; Biome biome=Biome::Plains; Vec3 position{}; float scale=1.0f; float rotation=0.0f; std::uint8_t species=0; };
struct WorldChunkCoord { std::int32_t x=0; std::int32_t z=0; friend bool operator==(const WorldChunkCoord&,const WorldChunkCoord&) noexcept=default; };
struct WorldChunk { WorldChunkCoord coord{}; Mesh terrain; Mesh water; std::vector<VegetationInstance> vegetation; std::vector<WaterBody> water_bodies; float min_height=0.0f; float max_height=0.0f; [[nodiscard]] bool valid() const noexcept; };
struct WorldChunkCoordHash { [[nodiscard]] std::size_t operator()(WorldChunkCoord value) const noexcept; };

class ProceduralWorld {
public:
    explicit ProceduralWorld(TerrainConfig config = {});
    [[nodiscard]] float sample_height(float world_x,float world_z) const noexcept;
    [[nodiscard]] float sample_height(std::int64_t world_x,std::int64_t world_z) const noexcept;
    [[nodiscard]] BiomeSample sample_biome(float world_x,float world_z) const noexcept;
    [[nodiscard]] float sample_water_depth(float world_x,float world_z) const noexcept;
    [[nodiscard]] const TerrainConfig& config() const noexcept { return config_; }
    [[nodiscard]] WorldChunk generate_chunk(WorldChunkCoord coord) const;
private: TerrainConfig config_;
};

class WorldStreamer {
public:
    explicit WorldStreamer(const ProceduralWorld& world);
    void update(Vec3 focus_position,std::uint32_t view_radius_chunks);
    void clear() noexcept;
    [[nodiscard]] const WorldChunk* find(WorldChunkCoord coord) const noexcept;
    [[nodiscard]] std::size_t loaded_count() const noexcept { return chunks_.size(); }
    [[nodiscard]] const std::unordered_map<WorldChunkCoord,WorldChunk,WorldChunkCoordHash>& chunks() const noexcept { return chunks_; }
private:
    const ProceduralWorld* world_=nullptr;
    std::unordered_map<WorldChunkCoord,WorldChunk,WorldChunkCoordHash> chunks_;
};

} // namespace exgine

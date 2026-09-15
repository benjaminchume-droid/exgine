#pragma once

#include "exgine/world.hpp"
#include <cstdint>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class WorldChunkLod : std::uint8_t { Near = 0, Medium = 1, Far = 2 };

struct OpenWorldStreamingConfig {
    std::uint32_t active_radius_chunks = 6;
    std::uint32_t prefetch_radius_chunks = 9;
    std::uint32_t unload_radius_chunks = 11;
    std::uint32_t max_loaded_chunks = 256;
    std::uint32_t max_new_chunks_per_update = 8;
    std::uint32_t max_unloads_per_update = 32;
    std::uint32_t medium_lod_start = 3;
    std::uint32_t far_lod_start = 6;
};

struct StreamedChunk {
    WorldChunk chunk;
    WorldChunkLod lod = WorldChunkLod::Near;
    float distance_chunks = 0.0f;
    std::uint64_t generation = 0;
    [[nodiscard]] bool valid() const noexcept;
};

struct StreamingStats {
    std::uint64_t updates = 0;
    std::uint64_t generated = 0;
    std::uint64_t unloaded = 0;
    std::uint64_t lod_changes = 0;
    std::uint64_t rejected_by_budget = 0;
    std::size_t loaded = 0;
};

class OpenWorldStreamer {
public:
    explicit OpenWorldStreamer(const ProceduralWorld& world, OpenWorldStreamingConfig config = {});

    bool update(Vec3 focus_position);
    void clear() noexcept;
    void set_config(OpenWorldStreamingConfig config) noexcept;
    [[nodiscard]] const OpenWorldStreamingConfig& config() const noexcept { return config_; }
    [[nodiscard]] const StreamedChunk* find(WorldChunkCoord coord) const noexcept;
    [[nodiscard]] std::size_t loaded_count() const noexcept { return chunks_.size(); }
    [[nodiscard]] const std::unordered_map<WorldChunkCoord, StreamedChunk, WorldChunkCoordHash>& chunks() const noexcept { return chunks_; }
    [[nodiscard]] const StreamingStats& stats() const noexcept { return stats_; }

private:
    const ProceduralWorld* world_ = nullptr;
    OpenWorldStreamingConfig config_{};
    std::unordered_map<WorldChunkCoord, StreamedChunk, WorldChunkCoordHash> chunks_;
    StreamingStats stats_{};
    std::uint64_t generation_ = 0;

    static OpenWorldStreamingConfig normalize(OpenWorldStreamingConfig config) noexcept;
    static WorldChunkLod choose_lod(std::uint32_t distance, const OpenWorldStreamingConfig& config) noexcept;
    [[nodiscard]] WorldChunk generate_lod_chunk(WorldChunkCoord coord, WorldChunkLod lod) const;
};

struct WorldOrigin {
    Vec3 offset{};
    std::uint64_t revision = 0;
};

struct OriginShift {
    Vec3 old_offset{};
    Vec3 new_offset{};
    Vec3 delta{};
    std::uint64_t revision = 0;
    [[nodiscard]] bool changed() const noexcept { return revision != 0 && (delta.x != 0.0f || delta.y != 0.0f || delta.z != 0.0f); }
};

struct FloatingOriginConfig {
    float rebase_distance = 512.0f;
    float grid_size = 256.0f;
};

class FloatingOrigin {
public:
    explicit FloatingOrigin(FloatingOriginConfig config = {});
    [[nodiscard]] const FloatingOriginConfig& config() const noexcept { return config_; }
    [[nodiscard]] const WorldOrigin& origin() const noexcept { return origin_; }
    OriginShift update(Vec3 absolute_focus) noexcept;
    void reset() noexcept;

private:
    FloatingOriginConfig config_{};
    WorldOrigin origin_{};
};

} // namespace exgine

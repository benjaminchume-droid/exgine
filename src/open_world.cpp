#include "exgine/open_world.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <utility>

namespace exgine { namespace {

std::uint32_t lod_resolution(std::uint32_t base, WorldChunkLod lod) noexcept {
    const std::uint32_t divisor = lod == WorldChunkLod::Near ? 1u : (lod == WorldChunkLod::Medium ? 2u : 4u);
    return std::max(2u, base / divisor);
}

float distance_chunks(WorldChunkCoord a, WorldChunkCoord b) noexcept {
    const float dx = static_cast<float>(a.x - b.x);
    const float dz = static_cast<float>(a.z - b.z);
    return std::sqrt(dx * dx + dz * dz);
}

} // namespace

bool StreamedChunk::valid() const noexcept {
    return chunk.valid() && std::isfinite(distance_chunks) && std::isfinite(static_cast<double>(generation));
}

OpenWorldStreamingConfig OpenWorldStreamer::normalize(OpenWorldStreamingConfig c) noexcept {
    c.active_radius_chunks = std::min(c.active_radius_chunks, 64u);
    c.prefetch_radius_chunks = std::clamp(c.prefetch_radius_chunks, c.active_radius_chunks, 64u);
    c.unload_radius_chunks = std::clamp(c.unload_radius_chunks, c.prefetch_radius_chunks, 64u);
    c.max_loaded_chunks = std::clamp(c.max_loaded_chunks, 1u, 4096u);
    c.max_new_chunks_per_update = std::clamp(c.max_new_chunks_per_update, 1u, 4096u);
    c.max_unloads_per_update = std::clamp(c.max_unloads_per_update, 1u, 4096u);
    c.medium_lod_start = std::clamp(c.medium_lod_start, 1u, c.prefetch_radius_chunks);
    c.far_lod_start = std::clamp(c.far_lod_start, c.medium_lod_start + 1u, c.prefetch_radius_chunks + 1u);
    return c;
}

WorldChunkLod OpenWorldStreamer::choose_lod(std::uint32_t distance, const OpenWorldStreamingConfig& c) noexcept {
    if (distance >= c.far_lod_start) return WorldChunkLod::Far;
    if (distance >= c.medium_lod_start) return WorldChunkLod::Medium;
    return WorldChunkLod::Near;
}

OpenWorldStreamer::OpenWorldStreamer(const ProceduralWorld& world, OpenWorldStreamingConfig config)
    : world_(&world), config_(normalize(config)) {}

WorldChunk OpenWorldStreamer::generate_lod_chunk(WorldChunkCoord coord, WorldChunkLod lod) const {
    if (!world_) return {};
    TerrainConfig c = world_->config();
    c.resolution = lod_resolution(c.resolution, lod);
    ProceduralWorld generator(c);
    return generator.generate_chunk(coord);
}

bool OpenWorldStreamer::update(Vec3 focus) {
    if (!world_ || !std::isfinite(focus.x) || !std::isfinite(focus.y) || !std::isfinite(focus.z)) return false;
    ++stats_.updates;
    stats_.generated = stats_.unloaded = stats_.lod_changes = stats_.rejected_by_budget = 0;
    const float size = world_->config().chunk_size;
    const auto cx = static_cast<std::int32_t>(std::floor(focus.x / size));
    const auto cz = static_cast<std::int32_t>(std::floor(focus.z / size));
    const WorldChunkCoord center{cx, cz};

    struct Candidate { WorldChunkCoord coord; std::uint32_t distance; };
    std::vector<Candidate> candidates;
    const auto radius = static_cast<std::int32_t>(config_.prefetch_radius_chunks);
    candidates.reserve(static_cast<std::size_t>((radius * 2 + 1) * (radius * 2 + 1)));
    for (std::int32_t z = cz - radius; z <= cz + radius; ++z) {
        for (std::int32_t x = cx - radius; x <= cx + radius; ++x) {
            const auto dx = x - cx, dz = z - cz;
            const auto d2 = static_cast<std::uint64_t>(dx) * static_cast<std::uint64_t>(dx) + static_cast<std::uint64_t>(dz) * static_cast<std::uint64_t>(dz);
            if (d2 > static_cast<std::uint64_t>(radius) * static_cast<std::uint64_t>(radius)) continue;
            candidates.push_back({{x, z}, static_cast<std::uint32_t>(std::sqrt(static_cast<double>(d2)))});
        }
    }
    std::sort(candidates.begin(), candidates.end(), [](const Candidate& a, const Candidate& b) {
        if (a.distance != b.distance) return a.distance < b.distance;
        if (a.coord.x != b.coord.x) return a.coord.x < b.coord.x;
        return a.coord.z < b.coord.z;
    });

    std::uint32_t generated = 0;
    for (const auto& candidate : candidates) {
        const auto desired = choose_lod(candidate.distance, config_);
        auto it = chunks_.find(candidate.coord);
        if (it == chunks_.end()) {
            if (chunks_.size() >= config_.max_loaded_chunks || generated >= config_.max_new_chunks_per_update) {
                ++stats_.rejected_by_budget;
                continue;
            }
            auto chunk = generate_lod_chunk(candidate.coord, desired);
            if (!chunk.valid()) return false;
            chunks_.emplace(candidate.coord, StreamedChunk{std::move(chunk), desired, static_cast<float>(candidate.distance), ++generation_});
            ++generated;
            ++stats_.generated;
        } else {
            it->second.distance_chunks = static_cast<float>(candidate.distance);
            if (it->second.lod != desired && candidate.distance <= config_.prefetch_radius_chunks) {
                auto chunk = generate_lod_chunk(candidate.coord, desired);
                if (!chunk.valid()) return false;
                it->second.chunk = std::move(chunk);
                it->second.lod = desired;
                it->second.generation = ++generation_;
                ++stats_.lod_changes;
            }
        }
    }

    struct Eviction { WorldChunkCoord coord; std::uint32_t distance; };
    std::vector<Eviction> evictions;
    for (const auto& [coord, chunk] : chunks_) {
        const auto d = distance_chunks(coord, center);
        if (d > static_cast<float>(config_.unload_radius_chunks))
            evictions.push_back({coord, static_cast<std::uint32_t>(std::ceil(d))});
    }
    std::sort(evictions.begin(), evictions.end(), [](const Eviction& a, const Eviction& b) {
        if (a.distance != b.distance) return a.distance > b.distance;
        if (a.coord.x != b.coord.x) return a.coord.x < b.coord.x;
        return a.coord.z < b.coord.z;
    });
    const auto unload_count = std::min<std::size_t>(evictions.size(), config_.max_unloads_per_update);
    for (std::size_t i = 0; i < unload_count; ++i) {
        chunks_.erase(evictions[i].coord);
        ++stats_.unloaded;
    }
    stats_.loaded = chunks_.size();
    return true;
}

void OpenWorldStreamer::clear() noexcept {
    chunks_.clear();
    stats_ = {};
    generation_ = 0;
}

void OpenWorldStreamer::set_config(OpenWorldStreamingConfig config) noexcept {
    config_ = normalize(config);
    if (chunks_.size() > config_.max_loaded_chunks) {
        std::vector<std::pair<WorldChunkCoord, std::uint64_t>> order;
        order.reserve(chunks_.size());
        for (const auto& [coord, chunk] : chunks_) order.emplace_back(coord, chunk.generation);
        std::sort(order.begin(), order.end(), [](const auto& a, const auto& b) { return a.second < b.second; });
        while (chunks_.size() > config_.max_loaded_chunks && !order.empty()) {
            chunks_.erase(order.back().first);
            order.pop_back();
        }
    }
}

const StreamedChunk* OpenWorldStreamer::find(WorldChunkCoord coord) const noexcept {
    const auto it = chunks_.find(coord);
    return it == chunks_.end() ? nullptr : &it->second;
}

FloatingOrigin::FloatingOrigin(FloatingOriginConfig config) : config_(config) {
    config_.rebase_distance = std::max(config_.rebase_distance, 1.0f);
    config_.grid_size = std::max(config_.grid_size, 1.0f);
}

OriginShift FloatingOrigin::update(Vec3 focus) noexcept {
    OriginShift result{origin_.offset, origin_.offset, {}, 0};
    if (!std::isfinite(focus.x) || !std::isfinite(focus.y) || !std::isfinite(focus.z)) return result;
    const float dx = focus.x - origin_.offset.x;
    const float dy = focus.y - origin_.offset.y;
    const float dz = focus.z - origin_.offset.z;
    const float distance = std::sqrt(dx * dx + dy * dy + dz * dz);
    if (distance < config_.rebase_distance) return result;
    auto snap = [g = config_.grid_size](float value) noexcept { return std::floor(value / g) * g; };
    const Vec3 next{snap(focus.x), snap(focus.y), snap(focus.z)};
    if (next.x == origin_.offset.x && next.y == origin_.offset.y && next.z == origin_.offset.z) return result;
    result.old_offset = origin_.offset;
    result.new_offset = next;
    result.delta = {next.x - origin_.offset.x, next.y - origin_.offset.y, next.z - origin_.offset.z};
    origin_.offset = next;
    ++origin_.revision;
    result.revision = origin_.revision;
    return result;
}

void FloatingOrigin::reset() noexcept { origin_ = {}; }

} // namespace exgine

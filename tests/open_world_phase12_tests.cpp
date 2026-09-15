#include "exgine/open_world.hpp"
#include <cassert>
#include <cmath>
#include <cstdint>
#include <iostream>

using namespace exgine;

static void test_streaming_budget_lod_and_determinism() {
    TerrainConfig terrain;
    terrain.seed = 9001;
    terrain.chunk_size = 32.0f;
    terrain.resolution = 16;
    terrain.vegetation_density = 4;
    ProceduralWorld world(terrain);

    OpenWorldStreamingConfig config;
    config.active_radius_chunks = 2;
    config.prefetch_radius_chunks = 4;
    config.unload_radius_chunks = 5;
    config.max_loaded_chunks = 24;
    config.max_new_chunks_per_update = 24;
    config.max_unloads_per_update = 24;
    config.medium_lod_start = 2;
    config.far_lod_start = 3;

    OpenWorldStreamer a(world, config);
    OpenWorldStreamer b(world, config);
    assert(a.update({0.0f, 0.0f, 0.0f}));
    assert(b.update({0.0f, 0.0f, 0.0f}));
    assert(a.loaded_count() == b.loaded_count());
    assert(a.loaded_count() <= config.max_loaded_chunks);
    assert(a.stats().generated > 0);

    const auto* center = a.find({0, 0});
    assert(center && center->valid());
    assert(center->lod == WorldChunkLod::Near);
    const auto* far = a.find({3, 0});
    assert(far && far->lod == WorldChunkLod::Far);
    assert(center->chunk.terrain.vertices.size() > far->chunk.terrain.vertices.size());

    const auto* same = b.find({3, 0});
    assert(same && same->chunk.terrain.indices == far->chunk.terrain.indices);
    assert(same->chunk.terrain.vertices.size() == far->chunk.terrain.vertices.size());

    assert(a.update({320.0f, 0.0f, 0.0f}));
    assert(a.loaded_count() <= config.max_loaded_chunks);
    assert(a.stats().unloaded > 0 || a.stats().rejected_by_budget > 0);
}

static void test_streaming_hysteresis_and_reconfiguration() {
    TerrainConfig terrain;
    terrain.resolution = 8;
    ProceduralWorld world(terrain);
    OpenWorldStreamingConfig config;
    config.active_radius_chunks = 1;
    config.prefetch_radius_chunks = 2;
    config.unload_radius_chunks = 4;
    config.max_loaded_chunks = 16;
    config.max_new_chunks_per_update = 16;
    config.max_unloads_per_update = 16;
    OpenWorldStreamer streamer(world, config);

    assert(streamer.update({0, 0, 0}));
    const auto before = streamer.loaded_count();
    assert(before > 0);
    assert(streamer.find({0, 0}) != nullptr);
    assert(streamer.update({32, 0, 0}));
    assert(streamer.find({0, 0}) != nullptr); // hysteresis keeps recently useful chunks alive

    config.max_loaded_chunks = 4;
    streamer.set_config(config);
    assert(streamer.loaded_count() <= 4);
}

static void test_floating_origin() {
    FloatingOrigin origin({100.0f, 64.0f});
    assert(origin.update({10, 0, 10}).revision == 0);
    const auto shift = origin.update({130, 0, 0});
    assert(shift.changed());
    assert(shift.new_offset.x == 128.0f);
    assert(origin.origin().revision == 1);
    assert(origin.update({130, 0, 0}).revision == 0);
    origin.reset();
    assert(origin.origin().offset.x == 0.0f);
    assert(origin.origin().revision == 0);
}

int main() {
    test_streaming_budget_lod_and_determinism();
    test_streaming_hysteresis_and_reconfiguration();
    test_floating_origin();
    std::cout << "Phase 12 open-world streaming tests passed\n";
}

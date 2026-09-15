#include "exgine/world_high_fidelity.hpp"
#include <cassert>
#include <cmath>
#include <vector>
using namespace exgine;

int main() {
    WorldBuildConfig config;
    config.chunk_size=256.0f;
    config.samples_per_axis=17;
    config.vegetation_instances=96;
    config.structure_budget=12;
    config.road_budget=6;
    config.seed=42;
    HighFidelityWorld world(config);

    const auto origin=world.chunk_for({-0.1f,0.0f,-256.1f});
    assert(origin.x==-1 && origin.z==-2);

    const auto a=world.build_region({0,0});
    const auto b=world.build_region({0,0});
    assert(a.valid() && b.valid());
    assert(a.seed==b.seed && a.terrain_min==b.terrain_min && a.terrain_max==b.terrain_max);
    assert(a.vegetation_count==b.vegetation_count && a.structure_count==b.structure_count);
    assert(std::isfinite(a.terrain_min) && std::isfinite(a.terrain_max));

    const auto low=world.query({120.0f,0.0f,80.0f});
    assert(low.valid && std::isfinite(low.height));
    assert(low.biome.slope>=0.0f && low.biome.slope<=1.0f);
    assert(low.biome.moisture>=0.0f && low.biome.moisture<=1.0f);

    std::vector<WorldInterestPoint> interests{{{128,0,128},200,1},{ {700,0,700},250,2 }};
    const std::vector<WorldChunkCoord> candidates{{0,0},{3,3},{10,10}};
    const auto details=world.select_details(interests,candidates);
    assert(details.size()==3);
    assert(details.front().detail>=details.back().detail);
    assert(details.front().terrain);

    const auto regions=world.build_regions({{0,0},{1,0},{0,1}});
    assert(regions.size()==3 && regions[0].seed!=regions[1].seed && regions[0].seed!=regions[2].seed);
    return 0;
}

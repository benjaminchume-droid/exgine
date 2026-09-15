#include "exgine/world_high_fidelity.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {
namespace {
constexpr float pi=3.14159265358979323846f;
std::uint64_t hash64(std::uint64_t x) noexcept { x^=x>>30; x*=0xbf58476d1ce4e5b9ULL; x^=x>>27; x*=0x94d049bb133111ebULL; return x^(x>>31); }
float unit(std::uint64_t x) noexcept { return static_cast<float>((hash64(x)>>40)&0xFFFFFFu)/16777215.0f; }
float smooth(float x) noexcept { return x*x*(3.0f-2.0f*x); }
float noise2(float x,float z,std::uint64_t seed) noexcept {
    const int xi=static_cast<int>(std::floor(x)); const int zi=static_cast<int>(std::floor(z));
    const float fx=x-static_cast<float>(xi), fz=z-static_cast<float>(zi);
    auto n=[&](int a,int b){return unit(seed+static_cast<std::uint64_t>(a)*0x9e3779b97f4a7c15ULL+static_cast<std::uint64_t>(b)*0xc2b2ae3d27d4eb4fULL)*2.0f-1.0f;};
    const float u=smooth(fx),v=smooth(fz),n00=n(xi,zi),n10=n(xi+1,zi),n01=n(xi,zi+1),n11=n(xi+1,zi+1);
    const float nx0=n00+(n10-n00)*u,nx1=n01+(n11-n01)*u; return nx0+(nx1-nx0)*v;
}
float distance_sq(Vec3 a,Vec3 b) noexcept { const float x=a.x-b.x,z=a.z-b.z; return x*x+z*z; }
}

HighFidelityWorld::HighFidelityWorld(WorldBuildConfig config):config_(config){
    config_.chunk_size=std::max(1.0f,config_.chunk_size);
    config_.samples_per_axis=std::max<std::uint32_t>(3,config_.samples_per_axis);
    config_.vegetation_instances=std::max<std::uint32_t>(1,config_.vegetation_instances);
    config_.terrain.chunk_resolution=std::max<std::uint32_t>(3,config_.terrain.chunk_resolution);
}

WorldChunkCoord HighFidelityWorld::chunk_for(Vec3 position) const noexcept {
    return {static_cast<std::int32_t>(std::floor(position.x/config_.chunk_size)),static_cast<std::int32_t>(std::floor(position.z/config_.chunk_size))};
}

std::uint64_t HighFidelityWorld::region_seed(WorldChunkCoord coord) const noexcept {
    std::uint64_t x=config_.seed^0x6a09e667f3bcc909ULL;
    x^=static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.x))*0x9e3779b97f4a7c15ULL;
    x^=static_cast<std::uint64_t>(static_cast<std::int64_t>(coord.z))*0xc2b2ae3d27d4eb4fULL;
    return hash64(x)|1ULL;
}

float HighFidelityWorld::terrain_height(float x,float z) const noexcept {
    const float s=std::max(.001f,config_.terrain.world_scale);
    float h=0.0f,amp=1.0f,freq=.0035f;
    for(int octave=0;octave<5;++octave){ h+=noise2(x*freq/s,z*freq/s,config_.seed+static_cast<std::uint64_t>(octave)*1013ULL)*amp; amp*=.5f;freq*=1.95f; }
    const float ridged=1.0f-std::abs(noise2(x*.008f/s,z*.008f/s,config_.seed+9001));
    h+=ridged*0.55f;
    return eroded_height(h*80.0f,noise2(x*.01f,z*.01f,config_.seed+77)*.5f,.5f,hash64(static_cast<std::uint64_t>(std::llround(x*8))+static_cast<std::uint64_t>(std::llround(z*17))));
}

BiomeSample HighFidelityWorld::sample_biome(float x,float z,float height) const noexcept {
    const float moisture=std::clamp(.5f+.5f*noise2(x*.0025f,z*.0025f,config_.seed+303),0.0f,1.0f);
    const float temperature=std::clamp(.6f-.004f*height+.25f*noise2(x*.0015f,z*.0015f,config_.seed+701),0.0f,1.0f);
    const float sx=terrain_height(x+2,z)-terrain_height(x-2,z); const float sz=terrain_height(x,z+2)-terrain_height(x,z-2);
    const float slope=std::clamp(std::sqrt(sx*sx+sz*sz)/8.0f,0.0f,1.0f);
    BiomeSample s; s.moisture=moisture;s.temperature=temperature;s.slope=slope;
    if(height>config_.terrain.snow_line)s.biome=Biome::Snow;
    else if(height>config_.terrain.rock_line||slope>.7f)s.biome=Biome::Rocky;
    else if(height<config_.water_level+2.0f&&moisture>.68f)s.biome=Biome::Wetland;
    else if(temperature<.28f)s.biome=Biome::Snow;
    else if(moisture>.65f)s.biome=Biome::Forest;
    else if(moisture<.25f&&temperature>.6f)s.biome=Biome::Desert;
    else if(height<config_.water_level+8.0f)s.biome=Biome::Beach;
    else s.biome=Biome::Plains;
    return s;
}

bool HighFidelityWorld::has_water(float x,float z,float& depth) const noexcept {
    const float h=terrain_height(x,z); const float basin=noise2(x*.0018f,z*.0018f,config_.seed+5150)*10.0f;
    depth=std::max(0.0f,config_.water_level-h+basin-6.0f);
    return depth>0.0f;
}

WorldQueryResult HighFidelityWorld::query(Vec3 position) const noexcept {
    WorldQueryResult r; r.valid=true; r.height=terrain_height(position.x,position.z); r.biome=sample_biome(position.x,position.z,r.height); r.water=has_water(position.x,position.z,r.water_depth); return r;
}

WorldRegion HighFidelityWorld::build_region(WorldChunkCoord coord) const {
    WorldRegion r; r.coord=coord;r.size=config_.chunk_size;r.origin={coord.x*r.size,0.0f,coord.z*r.size};r.seed=region_seed(coord);
    r.terrain_min=std::numeric_limits<float>::max();r.terrain_max=std::numeric_limits<float>::lowest();
    const auto n=config_.samples_per_axis;
    for(std::uint32_t z=0;z<n;++z) for(std::uint32_t x=0;x<n;++x){
        const float px=r.origin.x+r.size*(static_cast<float>(x)/(n-1)); const float pz=r.origin.z+r.size*(static_cast<float>(z)/(n-1));
        const auto q=query({px,0,pz}); r.terrain_min=std::min(r.terrain_min,q.height);r.terrain_max=std::max(r.terrain_max,q.height);if(q.water)++r.water_count;
    }
    const auto density=std::clamp(static_cast<std::uint32_t>(std::round(static_cast<float>(config_.vegetation_instances)*std::max(.1f,config_.terrain.slope_blend))),1u,8192u);
    for(std::uint32_t i=0;i<density;++i){const float ux=unit(r.seed+i*17ULL),uz=unit(r.seed+i*29ULL);const Vec3 p{r.origin.x+ux*r.size,0,r.origin.z+uz*r.size};const auto q=query(p);if(!q.water&&q.biome.biome!=Biome::Rocky&&q.biome.biome!=Biome::Snow&&q.biome.biome!=Biome::Desert)++r.vegetation_count;}
    r.structure_count=(r.terrain_max-r.terrain_min>250.0f)?std::min(config_.structure_budget,4u):config_.structure_budget;
    r.road_count=(r.water_count>n?std::max(1u,config_.road_budget/2):config_.road_budget);
    r.interests.push_back({{r.origin.x+r.size*.5f,0,r.origin.z+r.size*.5f},r.size*.75f,1.0f});
    if(r.structure_count)r.interests.push_back({{r.origin.x+r.size*.62f,0,r.origin.z+r.size*.44f},r.size*.25f,1.5f});
    return r;
}

std::vector<WorldChunkDetail> HighFidelityWorld::select_details(const std::vector<WorldInterestPoint>& interests,const std::vector<WorldChunkCoord>& candidates) const {
    std::vector<WorldChunkDetail> out;out.reserve(candidates.size());
    for(const auto& c:candidates){const Vec3 center{(c.x+.5f)*config_.chunk_size,0,(c.z+.5f)*config_.chunk_size};float best=std::numeric_limits<float>::max(),priority=1.0f;for(const auto& i:interests){const float d=std::sqrt(distance_sq(center,i.position));const float score=d/std::max(1.0f,i.priority);if(score<best){best=score;priority=i.priority;}}
        const float hero=128.0f*priority,near=320.0f*priority,mid=768.0f*priority;WorldChunkDetail d;d.coord=c;if(best<=hero)d.detail=WorldDetailLevel::Hero;else if(best<=near)d.detail=WorldDetailLevel::Near;else if(best<=mid)d.detail=WorldDetailLevel::Mid;else d.detail=WorldDetailLevel::Far;d.terrain=true;d.water=d.detail>=WorldDetailLevel::Mid;d.vegetation=d.detail>=WorldDetailLevel::Mid;d.structures=d.detail>=WorldDetailLevel::Near;d.roads=d.detail>=WorldDetailLevel::Near;out.push_back(d);
    }
    std::stable_sort(out.begin(),out.end(),[](const WorldChunkDetail&a,const WorldChunkDetail&b){if(a.detail!=b.detail)return static_cast<int>(a.detail)>static_cast<int>(b.detail);if(a.coord.x!=b.coord.x)return a.coord.x<b.coord.x;return a.coord.z<b.coord.z;});
    return out;
}

std::vector<WorldRegion> HighFidelityWorld::build_regions(const std::vector<WorldChunkCoord>& coords) const {std::vector<WorldRegion> out;out.reserve(coords.size());for(const auto& c:coords)out.push_back(build_region(c));return out;}

} // namespace exgine

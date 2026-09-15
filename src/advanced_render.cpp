#include "exgine/advanced_render.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <numeric>

namespace exgine {
namespace {
constexpr float pi=3.14159265358979323846f;
float clamp01(float v){return std::clamp(v,0.0f,1.0f);}
float len(Vec3 v){return std::sqrt(v.x*v.x+v.y*v.y+v.z*v.z);}
Vec3 sub(Vec3 a,Vec3 b){return {a.x-b.x,a.y-b.y,a.z-b.z};}
Vec3 add(Vec3 a,Vec3 b){return {a.x+b.x,a.y+b.y,a.z+b.z};}
Vec3 mul(Vec3 a,float s){return {a.x*s,a.y*s,a.z*s};}
float dot(Vec3 a,Vec3 b){return a.x*b.x+a.y*b.y+a.z*b.z;}
Vec3 normalize(Vec3 v){float l=len(v);return l>1e-6f?mul(v,1.0f/l):Vec3{0,1,0};}
float dist(Vec3 a,Vec3 b){return len(sub(a,b));}
float smoothstep(float a,float b,float x){float t=clamp01((x-a)/(b-a));return t*t*(3-2*t);}
}

bool RenderFeatureSet::enabled(RenderFeature f) const noexcept {
    switch(f){
        case RenderFeature::Shadows:return shadows; case RenderFeature::Hdr:return hdr;
        case RenderFeature::Bloom:return bloom; case RenderFeature::Exposure:return exposure;
        case RenderFeature::Ibl:return ibl; case RenderFeature::ReflectionProbes:return reflection_probes;
        case RenderFeature::Atmosphere:return atmosphere; case RenderFeature::Fog:return fog;
        case RenderFeature::Terrain:return terrain; case RenderFeature::Vegetation:return vegetation;
        case RenderFeature::Water:return water; case RenderFeature::GpuCulling:return gpu_culling;
        case RenderFeature::Streaming:return streaming; case RenderFeature::WorldStreaming:return world_streaming;
        case RenderFeature::AdvancedMaterials:return advanced_materials;
    } return false;
}

ShadowSettings ShadowSystem::build_directional(const Camera& camera,const Light& light,std::uint32_t atlas_size) const noexcept {
    ShadowSettings out; out.atlas_size=std::max(256u,atlas_size);
    out.enabled=light.active && light.shadow_mode!=ShadowMode::None && light.type==LightType::Directional;
    out.cascades=std::clamp<std::uint32_t>(light.shadow_cascades,1,4);
    float n=std::max(0.001f,camera.near_plane), f=std::max(n+0.001f,camera.far_plane);
    for(std::uint32_t i=0;i<out.cascades;i++){
        float p=float(i+1)/float(out.cascades);
        float logarithmic=n*std::pow(f/n,p), uniform=n+(f-n)*p;
        float split=out.split_lambda*logarithmic+(1-out.split_lambda)*uniform;
        out.cascade[i].near_distance=(i==0?n:out.cascade[i-1].far_distance);
        out.cascade[i].far_distance=split;
        out.cascade[i].view_projection=Mat4::identity(); // Backend fills the light-space matrix.
    }
    return out;
}
bool ShadowSystem::should_cast(const Light& l) const noexcept { return l.active && l.shadow_mode!=ShadowMode::None; }

std::uint64_t ReflectionProbeWorld::add(ReflectionProbe p){if(p.id==0)p.id=next_id_++;else next_id_=std::max(next_id_,p.id+1);probes_.push_back(p);return p.id;}
bool ReflectionProbeWorld::remove(std::uint64_t id) noexcept {auto i=std::remove_if(probes_.begin(),probes_.end(),[&](const auto&p){return p.id==id;});bool r=i!=probes_.end();probes_.erase(i,probes_.end());return r;}
const ReflectionProbe* ReflectionProbeWorld::closest(Vec3 p) const noexcept {const ReflectionProbe* best=nullptr;float bd=std::numeric_limits<float>::max();for(const auto&x:probes_){float d=dist(p,x.position);if(d<=x.blend_distance+len(x.extents)&&d<bd){bd=d;best=&x;}}return best;}

void PostProcessPipeline::configure(std::uint32_t width,std::uint32_t height,const HdrSettings& hdr,const BloomSettings& bloom){
    passes_.clear();hdr_required_=hdr.enabled;
    auto add=[&](const char*n,std::uint32_t w=width,std::uint32_t h=height){passes_.push_back({n,true,w,h,w,h});};
    if(hdr.enabled) add("HDR Render Target");
    if(bloom.enabled&&hdr.enabled){std::uint32_t w=width,h=height;for(std::uint32_t i=0;i<std::max(1u,bloom.mip_count);++i){w=std::max(1u,w/2);h=std::max(1u,h/2);passes_.push_back({"Bloom Downsample "+std::to_string(i),true,w*2,h*2,w,h});}for(std::uint32_t i=0;i<std::max(1u,bloom.mip_count);++i)passes_.push_back({"Bloom Upsample "+std::to_string(i),true,width,height,width,height);}
    }
    if(hdr.enabled) add("Exposure + Tone Map");
    add("Color Grade + Output Transform");
}

float ExposureController::update(float average_luminance,float target_middle_gray,float dt,const HdrSettings& s) noexcept {
    average_luminance=std::max(average_luminance,1e-4f);target_middle_gray=std::max(target_middle_gray,1e-4f);
    float target=std::log2(target_middle_gray/average_luminance);
    target=std::clamp(target,s.min_exposure,s.max_exposure);float k=1.0f-std::exp(-std::max(0.0f,dt)*std::max(0.0f,s.adaptation_speed));current_+= (target-current_)*k;current_=std::clamp(current_,s.min_exposure,s.max_exposure);return current_;
}

FogResult evaluate_fog(const FogSettings& s,float distance,float height) noexcept {FogResult r{1.0f,s.color};if(!s.enabled)return r;float d=std::max(0.0f,distance);float density=s.density;if(s.height_falloff!=0) density*=std::exp(-std::max(0.0f,height)*s.height_falloff);float optical=std::max(0.0f,d-s.start_distance)*density;float expf=std::exp(-optical);float range=s.end_distance>s.start_distance?1.0f-smoothstep(s.start_distance,s.end_distance,d):1.0f;r.factor=std::clamp(expf*range,0.0f,1.0f);return r;}
Color3 evaluate_sky(const AtmosphereSettings& s,Vec3 direction,Vec3 sun_direction) noexcept {Vec3 d=normalize(direction),sun=normalize(sun_direction);float h=clamp01(d.y*0.5f+0.5f);Color3 c{s.sky_horizon.x+(s.sky_top.x-s.sky_horizon.x)*h,s.sky_horizon.y+(s.sky_top.y-s.sky_horizon.y)*h,s.sky_horizon.z+(s.sky_top.z-s.sky_horizon.z)*h};float sunf=std::pow(std::max(0.0f,dot(d,sun)),80.0f)*s.sun_disc;c.x+=sunf;c.y+=sunf*0.9f;c.z+=sunf*0.65f;return c;}

std::uint64_t TerrainChunk::key() const noexcept {return (std::uint64_t(std::uint32_t(x))*0x9e3779b1ULL)^(std::uint64_t(std::uint32_t(z))*0x85ebca6bULL)^std::uint64_t(lod);}
std::uint8_t TerrainSystem::choose_lod(float d) const noexcept {if(d<=config_.lod0_distance)return 0;float x=d/config_.lod0_distance;std::uint8_t lod=0;while(lod+1<config_.lod_count&&x>1){x/=config_.lod_multiplier;++lod;}return lod;}
std::vector<TerrainChunk> TerrainSystem::visible_chunks(Vec3 camera,float view_distance) const {std::vector<TerrainChunk> out;float cs=float(config_.chunk_size)*config_.world_scale;int r=int(std::ceil(view_distance/cs));int cx=int(std::floor(camera.x/cs)),cz=int(std::floor(camera.z/cs));for(int z=-r;z<=r;++z)for(int x=-r;x<=r;++x){float wx=(x+cx+0.5f)*cs,wz=(z+cz+0.5f)*cs;float d=std::sqrt((wx-camera.x)*(wx-camera.x)+(wz-camera.z)*(wz-camera.z));if(d<=view_distance){TerrainChunk c;c.x=cx+x;c.z=cz+z;c.lod=choose_lod(d);c.loaded=true;out.push_back(c);}}return out;}

std::uint64_t VegetationSystem::add_prototype(VegetationPrototype p){auto id=next_id_++;prototypes_.emplace(id,p);return id;}
void VegetationSystem::clear() noexcept{prototypes_.clear();}
std::uint8_t VegetationSystem::choose_lod(const VegetationPrototype&p,float d)const noexcept{for(std::uint8_t i=0;i<4;i++)if(d<=p.lod_distances[i])return i;return 3;}
std::vector<VegetationInstance> VegetationSystem::cull(const std::vector<VegetationInstance>& in,Vec3 camera,float maxd)const{std::vector<VegetationInstance>out;out.reserve(in.size());for(auto v:in){auto it=prototypes_.find(v.prototype);if(it==prototypes_.end())continue;float d=dist(v.position,camera);float limit=std::min(maxd,it->second.cull_distance);if(d<=limit){v.lod=choose_lod(it->second,d);out.push_back(v);}}return out;}

WaterSample sample_water(const WaterSettings&s,Vec3 p,float t)noexcept{float k=2*pi/std::max(0.01f,s.wave_length);float phase=k*(p.x+p.z*0.7f)+t*s.wave_speed;float h=s.wave_amplitude*std::sin(phase);float slope=s.wave_amplitude*k*std::cos(phase);Vec3 n=normalize({-slope,-slope*0.7f,1.0f});float foam=clamp01((std::fabs(h)/std::max(0.001f,s.wave_amplitude)-s.foam_threshold)/(1.0f-s.foam_threshold));return {h,n,foam};}

Frustum make_frustum(const Mat4&m)noexcept{Frustum f;auto plane=[&](int r,bool add){FrustumPlane p;for(int c=0;c<3;c++)p.n.x=(c==0?m.at(3,0)+(add?m.at(0,0):-m.at(0,0)):p.n.x);return p;};(void)plane; // Explicit extraction below keeps column-major Mat4 semantics obvious.
    const float* a=m.m.data();
    f.planes[0]={{a[3]+a[0],a[7]+a[4],a[11]+a[8]},a[15]+a[12]};
    f.planes[1]={{a[3]-a[0],a[7]-a[4],a[11]-a[8]},a[15]-a[12]};
    f.planes[2]={{a[3]+a[1],a[7]+a[5],a[11]+a[9]},a[15]+a[13]};
    f.planes[3]={{a[3]-a[1],a[7]-a[5],a[11]-a[9]},a[15]-a[13]};
    f.planes[4]={{a[3]+a[2],a[7]+a[6],a[11]+a[10]},a[15]+a[14]};
    f.planes[5]={{a[3]-a[2],a[7]-a[6],a[11]-a[10]},a[15]-a[14]};
    for(auto&p:f.planes){float l=len(p.n);if(l>1e-6f){p.n=mul(p.n,1/l);p.d/=l;}}
    return f;}
bool intersects(const Frustum&f,const Bounds3&b)noexcept{for(const auto&p:f.planes){Vec3 q{p.n.x>=0?b.max.x:b.min.x,p.n.y>=0?b.max.y:b.min.y,p.n.z>=0?b.max.z:b.min.z};if(dot(p.n,q)+p.d<0)return false;}return true;}
CullResult GpuSceneCuller::cull(const std::vector<RenderInstance>&in,const Frustum&f)const{CullResult r;r.visible.reserve(in.size());for(const auto&i:in){if(intersects(f,i.bounds))r.visible.push_back(i);else ++r.frustum_culled;}return r;}

void AssetStreamingQueue::request(StreamRequest r){auto it=std::find_if(requests_.begin(),requests_.end(),[&](const auto&x){return x.asset_id==r.asset_id;});if(it!=requests_.end()){if(r.priority>it->priority)*it=r;return;}requests_.push_back(r);}
std::optional<StreamRequest> AssetStreamingQueue::pop(){if(requests_.empty())return std::nullopt;auto it=std::max_element(requests_.begin(),requests_.end(),[](const auto&a,const auto&b){return a.priority<b.priority;});auto r=*it;requests_.erase(it);return r;}
void AssetStreamingQueue::cancel(std::uint64_t id)noexcept{requests_.erase(std::remove_if(requests_.begin(),requests_.end(),[&](auto&r){return r.asset_id==id;}),requests_.end());}
void AssetStreamingQueue::clear()noexcept{requests_.clear();}

WorldCell WorldStreamingManager::cell_for(Vec3 p)const noexcept{return {int(std::floor(p.x/cell_size_)),int(std::floor(p.y/cell_size_)),int(std::floor(p.z/cell_size_)),false,0};}
std::vector<WorldCell> WorldStreamingManager::desired_cells(Vec3 p,int radius)const{WorldCell c=cell_for(p);std::vector<WorldCell>o;for(int z=-radius;z<=radius;z++)for(int y=-std::min(radius,1);y<=std::min(radius,1);y++)for(int x=-radius;x<=radius;x++){WorldCell n{c.x+x,c.y+y,c.z+z};n.generation=0;auto it=std::find_if(loaded_.begin(),loaded_.end(),[&](const auto&l){return l.x==n.x&&l.y==n.y&&l.z==n.z;});n.loaded=it!=loaded_.end();if(n.loaded)n.generation=it->generation;o.push_back(n);}return o;}
void WorldStreamingManager::mark_loaded(WorldCell c){auto it=std::find_if(loaded_.begin(),loaded_.end(),[&](const auto&x){return x.x==c.x&&x.y==c.y&&x.z==c.z;});if(it==loaded_.end()){c.loaded=true;c.generation++;loaded_.push_back(c);}else{it->loaded=true;it->generation=std::max(it->generation+1,c.generation);}}
void WorldStreamingManager::unload_outside(Vec3 p,int radius){auto c=cell_for(p);loaded_.erase(std::remove_if(loaded_.begin(),loaded_.end(),[&](const auto&x){return std::abs(x.x-c.x)>radius||std::abs(x.z-c.z)>radius||std::abs(x.y-c.y)>std::min(radius,1);}),loaded_.end());}

Vec3 LargeWorld::to_local(Vec3 w)const noexcept{return sub(w,origin_.origin);}Vec3 LargeWorld::to_world(Vec3 l)const noexcept{return add(l,origin_.origin);}Vec3 LargeWorld::recenter(Vec3 camera)noexcept{auto snap=[](float v,float g){return std::floor(v/g+0.5f)*g;};origin_.origin={snap(camera.x,origin_.grid),snap(camera.y,origin_.grid),snap(camera.z,origin_.grid)};return origin_.origin;}

void AnimationMixer::add_layer(AnimationLayer l){layers_.push_back(l);}void AnimationMixer::update(float dt)noexcept{base_.time+=std::max(0.0f,dt);for(auto&l:layers_)l.time+=std::max(0.0f,dt);}float AnimationMixer::normalized_base_time()const noexcept{return base_.time-std::floor(base_.time);}
std::uint64_t MaterialPipelineKey::hash()const noexcept{std::uint64_t h=1469598103934665603ULL;auto mix=[&](bool v){h^=std::uint64_t(v);h*=1099511628211ULL;};mix(features.normal_map);mix(features.ambient_occlusion);mix(features.emission);mix(features.opacity);mix(features.clear_coat);mix(features.sheen);mix(features.transmission);mix(features.subsurface);mix(skinned);mix(transparent);return h;}

void RenderGraph::import_resource(std::string n){if(std::find(resources_.begin(),resources_.end(),n)==resources_.end())resources_.push_back(std::move(n));}
void RenderGraph::add_pass(RenderPassNode p){passes_.push_back(std::move(p));}
bool RenderGraph::compile(std::string& error)const{for(std::size_t i=0;i<passes_.size();++i){for(const auto&r:passes_[i].reads)if(std::find(resources_.begin(),resources_.end(),r)==resources_.end()){bool produced=false;for(std::size_t j=0;j<i;j++)if(std::find(passes_[j].writes.begin(),passes_[j].writes.end(),r)!=passes_[j].writes.end())produced=true;if(!produced){error="RenderGraph: missing resource '"+r+"'";return false;}}}error.clear();return true;}

RenderPipelinePlan AdvancedRenderPipeline::build(const RenderFrame& frame,const RenderFeatureSet& features)const{
    RenderPipelinePlan p;p.features=features;p.hdr.exposure_ev=frame.lighting.exposure;p.hdr.gamma=frame.lighting.gamma;p.hdr.enabled=features.hdr&&frame.config.high_dynamic_range;p.bloom.enabled=features.bloom&&p.hdr.enabled;p.shadows.enabled=features.shadows;
    p.post.configure(frame.config.width,frame.config.height,p.hdr,p.bloom);
    p.graph.import_resource("scene_depth");p.graph.import_resource("hdr_color");p.graph.import_resource("shadow_atlas");p.graph.import_resource("environment_ibl");
    if(p.shadows.enabled){p.graph.add_pass({"Shadow Cascades",{}, {"shadow_atlas"}});}p.graph.add_pass({"Depth + Visibility",{}, {"scene_depth"}});p.graph.add_pass({"Opaque PBR", {"scene_depth","shadow_atlas","environment_ibl"},{"hdr_color"}});if(features.atmosphere)p.graph.add_pass({"Atmosphere + Sky", {"scene_depth"},{"hdr_color"}});if(features.water)p.graph.add_pass({"Water", {"scene_depth","environment_ibl"},{"hdr_color"}});p.graph.add_pass({"Transparent", {"scene_depth","environment_ibl"},{"hdr_color"}});for(const auto&pass:p.post.passes())p.graph.add_pass({pass.name,{"hdr_color"},{"hdr_color"}});return p;
}

} // namespace exgine

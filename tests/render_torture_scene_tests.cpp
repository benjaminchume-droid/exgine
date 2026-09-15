#include "exgine/advanced_render.hpp"
#include <cassert>
#include <cmath>
#include <iostream>
#include <vector>
using namespace exgine;
int main(){
    RenderFeatureSet f;
    assert(f.shadows&&f.hdr&&f.bloom&&f.ibl&&f.reflection_probes&&f.atmosphere&&f.fog&&f.terrain&&f.vegetation&&f.water&&f.gpu_culling&&f.streaming&&f.world_streaming);
    Camera cam; cam.near_plane=.1f; cam.far_plane=10000.f;
    Light sun; sun.type=LightType::Directional; sun.active=true; sun.shadow_mode=ShadowMode::Cascaded; sun.shadow_cascades=4;
    auto sh=ShadowSystem{}.build_directional(cam,sun,4096); assert(sh.enabled&&sh.cascades==4); for(int i=1;i<4;i++)assert(sh.cascade[i].far_distance>sh.cascade[i-1].far_distance);
    TerrainSystem terrain; auto chunks=terrain.visible_chunks({0,10,0},4096); assert(chunks.size()>100); for(auto&c:chunks)assert(c.lod<6);
    VegetationSystem veg; auto tree=veg.add_prototype({101,.7f,1.4f,1000,{25,100,300,1000},true}); std::vector<VegetationInstance> trees; trees.reserve(10000); for(int i=0;i<10000;i++)trees.push_back({tree,{float(i%100)*10.f,0,float(i/100)*10.f},0,1,0}); auto vt=veg.cull(trees,{0,0,0},1000); assert(!vt.empty()&&vt.size()<trees.size());
    for(float t=0;t<10;t+=.1f){auto w=sample_water(WaterSettings{}, {t,0,t*.5f},t); assert(std::isfinite(w.height)); assert(std::isfinite(w.normal.x)&&std::isfinite(w.normal.y)&&std::isfinite(w.normal.z));}
    std::vector<RenderInstance> scene; scene.reserve(5000); Bounds3 b{{-1,-1,-1},{1,1,1}}; for(int i=0;i<5000;i++){float x=float(i%100)*20.f,z=float(i/100)*20.f; scene.push_back({std::uint64_t(i),{{x-1,-1,z-1},{x+1,3,z+1}},std::uint64_t(i%32),std::uint64_t(i%16),0});} auto fr=make_frustum(Mat4::identity()); auto c=GpuSceneCuller{}.cull(scene,fr); assert(c.visible.size()+c.frustum_culled==scene.size());
    WorldStreamingManager world(256); auto wanted=world.desired_cells({0,0,0},4); assert(wanted.size()>=81); for(auto cell:wanted)world.mark_loaded(cell); assert(world.loaded_cells().size()==wanted.size()); world.unload_outside({0,0,0},2); assert(world.loaded_cells().size()<wanted.size());
    LargeWorld large; large.recenter({10000,500,20000}); auto local=large.to_local({10010,510,20010}); auto worldpos=large.to_world(local); assert(std::fabs(worldpos.x-10010)<1e-4f&&std::fabs(worldpos.z-20010)<1e-4f);
    HdrSettings hdr; BloomSettings bloom; PostProcessPipeline post; post.configure(2560,1440,hdr,bloom); assert(post.passes().size()>=12);
    ReflectionProbeWorld probes; for(int i=0;i<64;i++)probes.add({0,{float(i*20),3,0},{8,8,8},4,128,false}); assert(probes.closest({20,3,0})!=nullptr);
    AdvancedRenderPipeline pipeline; RenderFrame frame; frame.config.width=2560; frame.config.height=1440; frame.camera=cam; frame.lights.push_back(sun); auto plan=pipeline.build(frame); std::string err; assert(plan.graph.compile(err)); assert(!plan.graph.passes().empty());
    std::cout<<"render torture scene passed: terrain="<<chunks.size()<<" vegetation="<<vt.size()<<" scene="<<scene.size()<<"\n";
}
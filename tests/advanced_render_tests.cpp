#include "exgine/advanced_render.hpp"
#include <cassert>
#include <cmath>
#include <iostream>

using namespace exgine;

int main(){
    RenderFeatureSet features;
    assert(features.enabled(RenderFeature::Shadows));
    Camera camera; camera.near_plane=0.1f; camera.far_plane=1000.0f;
    Light sun; sun.type=LightType::Directional; sun.shadow_mode=ShadowMode::Cascaded; sun.shadow_cascades=4;
    ShadowSystem shadows; auto ss=shadows.build_directional(camera,sun); assert(ss.enabled&&ss.cascades==4); assert(ss.cascade[0].far_distance<ss.cascade[3].far_distance);

    HdrSettings hdr; ExposureController exposure; exposure.reset(); float e=exposure.update(0.02f,0.18f,1.0f,hdr); assert(std::isfinite(e));
    BloomSettings bloom; PostProcessPipeline post; post.configure(1920,1080,hdr,bloom); assert(!post.passes().empty());

    ReflectionProbeWorld probes; auto pid=probes.add({0,{0,0,0},{10,10,10},2,128,false}); assert(probes.closest({1,0,0})); assert(probes.remove(pid));
    AtmosphereSettings atmosphere; auto sky=evaluate_sky(atmosphere,{0,1,0},{0,1,0}); assert(sky.x>=0&&sky.y>=0&&sky.z>=0);
    FogSettings fog; fog.enabled=true; auto fr=evaluate_fog(fog,100); assert(fr.factor<1&&fr.factor>=0);

    TerrainSystem terrain; auto chunks=terrain.visible_chunks({0,0,0},256); assert(!chunks.empty());
    VegetationSystem vegetation; auto vid=vegetation.add_prototype({1,0.8f,1.2f,100,{10,25,50,100},true}); std::vector<VegetationInstance> trees{{vid,{5,0,0},0,1,0},{vid,{500,0,0},0,1,0}}; auto visible=vegetation.cull(trees,{0,0,0},200); assert(visible.size()==1);
    auto water=sample_water(WaterSettings{}, {1,0,2}, 3); assert(std::isfinite(water.height)); assert(std::fabs(len(water.normal)-1.0f)<0.01f);

    Frustum f=make_frustum(Mat4::identity()); Bounds3 inside{{-0.1f,-0.1f,-0.1f},{0.1f,0.1f,0.1f}}; Bounds3 outside{{100,100,100},{101,101,101}}; assert(intersects(f,inside)); assert(!intersects(f,outside));
    GpuSceneCuller culler; std::vector<RenderInstance> instances{{1,inside,1,1,0},{2,outside,1,1,0}}; auto cr=culler.cull(instances,f); assert(cr.visible.size()==1&&cr.frustum_culled==1);

    AssetStreamingQueue queue; queue.request({1,1,0}); queue.request({2,5,0}); assert(queue.pop()->asset_id==2); assert(queue.size()==1);
    WorldStreamingManager world; auto cell=world.cell_for({300,0,-300}); assert(cell.x==1&&cell.z==-2); world.mark_loaded(cell); assert(world.loaded_cells().size()==1);
    LargeWorld large; auto origin=large.recenter({600,10,700}); assert(origin.x==512&&origin.z==768); auto local=large.to_local({512,10,768}); assert(std::fabs(local.x)<1e-5f);

    AnimationMixer mixer; mixer.set_base({1,1,0,false}); mixer.add_layer({2,0.5f,0,true}); mixer.update(0.25f); assert(std::fabs(mixer.normalized_base_time()-0.25f)<1e-5f);
    MaterialPipelineKey key; key.skinned=true; key.features.clear_coat=true; assert(key.hash()!=0);

    RenderGraph graph; graph.import_resource("depth"); graph.import_resource("color"); graph.add_pass({"Depth",{}, {"depth"}}); graph.add_pass({"Opaque",{"depth"},{"color"}}); std::string error; assert(graph.compile(error));
    RenderFrame frame; frame.config.width=1280; frame.config.height=720; frame.config.high_dynamic_range=true; AdvancedRenderPipeline pipeline; auto plan=pipeline.build(frame); assert(!plan.graph.passes().empty()); assert(plan.graph.compile(error));

    std::cout<<"advanced_render tests passed\n"; return 0;
}

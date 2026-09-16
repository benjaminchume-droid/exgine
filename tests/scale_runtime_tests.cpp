#include "exgine/scale_runtime.hpp"
#include <cassert>
#include <cstddef>

using namespace exgine::scale;

int main(){
    WorldStreamingConfig cfg; cfg.cell_size=100; cfg.full_radius=100; cfg.reduced_radius=200; cfg.proxy_radius=300; cfg.unload_radius=400; cfg.max_loaded_cells=128; cfg.byte_budget=64*1024*1024;
    WorldPartition world(cfg); world.update({0,0,0}); assert(world.loaded_count()>0); assert(world.resident_bytes()>0); auto origin=world.cell_for({0,0,0}); const WorldCellId expected{0,0,0}; assert(origin==expected);

    HlodSystem hlod; hlod.add({1,{0,0,0},10,0,1024,true}); hlod.update({0,0,0}); assert(hlod.nodes().at(1).visible);

    AsyncStreamingScheduler stream(1024); bool loaded=false; stream.request({1,512,10,[&]{loaded=true;},[]{}}); stream.tick(); assert(loaded&&stream.resident_bytes()==512);

    SimulationLodSystem sim; sim.add({1,{0,0,0},1}); sim.add({2,{2000,0,0},0}); sim.update({0,0,0}); assert(sim.objects().at(1).detail==DetailLevel::Full); assert(sim.objects().at(2).detail==DetailLevel::Persistent);

    JobSystem jobs; int count=0; jobs.submit({1,[&]{++count;},1}); jobs.submit({2,[&]{++count;},2}); assert(jobs.run()==2&&count==2);

    NavigationGraph nav; nav.add({1,{0,0,0},{2},true}); nav.add({2,{1,0,0},{3},true}); nav.add({3,{2,0,0},{},true}); assert(nav.find_path(1,3).size()==3);
    CrowdNavigation crowd(nav); crowd.add({1,1,3,2,{},{0}}); crowd.update(); assert(crowd.agents().at(1).path.size()==3);

    VehicleSimulation vehicles; vehicles.add({1,{0,0,0},0,0,1,0,2.6f}); vehicles.tick(0.1f); assert(vehicles.vehicles().at(1).speed>0);

    PersistentWorld save; save.put({1,"door",{std::byte{1}}}); auto snap=save.snapshot(); PersistentWorld restored; restored.restore(snap); assert(restored.get(1)!=nullptr);

    auto vk=RenderCapabilityProfile::desktop_vulkan(); assert(vk.gpu_culling&&vk.bindless&&vk.virtual_geometry&&vk.virtual_textures);
    VirtualGeometry vg; vg.add({1,{0,1,2},0.1f}); vg.add({2,{0,1,2},10.0f}); vg.select(1.0f); assert(vg.visible().size()==1);
    VirtualTextureCache vt(2); vt.request({1,0,0,0,0,false}); vt.request({1,1,0,0,0,false}); vt.request({1,2,0,0,0,false}); assert(vt.resident_pages()==2);
    GlobalIllumination gi; gi.add_probe({{0,0,0},2}); assert(gi.sample({0,0,0})>1.9f);
    ReflectionSystem reflection; assert(reflection.trace({{}, {0,0,1},50}).hit);
    VolumetricAtmosphere atmosphere; assert(atmosphere.transmittance(1,0)==1.0f);

    ScaleRuntime runtime(cfg); runtime.simulation().add({1,{0,0,0},1}); runtime.vehicles().add({1,{0,0,0},0,0,0,0,2.6f}); runtime.update({0,0,0},0.016f); assert(runtime.stats().cells>0&&runtime.stats().sim_objects==1);
    return 0;
}

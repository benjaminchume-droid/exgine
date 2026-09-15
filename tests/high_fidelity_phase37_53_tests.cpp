#include "exgine/high_fidelity.hpp"
#include <cassert>
#include <cmath>
using namespace exgine;

int main(){
    MaterialRuntimeState materials; assert(materials.set("carpaint",{.5f,1,.9f,.4f,.2f,.1f,.7f,.2f,.8f,.1f,.05f,.15f,.02f})); assert(materials.set_weathering("carpaint",.4f,.1f,.2f,.05f,.3f,.1f)); assert(materials.get("carpaint")->wetness==.4f);
    MeshAssembly assembly; assembly.parts.push_back({"body",make_box({2,1,4}),"carpaint",{},{1,1,1},{}}); const auto ms=inspect_mesh(assembly); assert(ms.triangles>0&&ms.draw_parts==1);
    HighQualityLighting lighting; assert(lighting.cascaded_shadows&&lighting.screen_space_reflections);
    AtmosphereModel atmosphere; assert(atmosphere.planet_radius>6.0e6f);
    WaterSurface water; water.wave_amplitude=.4f; assert(water.wave_amplitude>.0f);
    VegetationPatch patch; patch.species.push_back({"Tree",2,15,.7f,1}); const auto veg=generate_vegetation(patch,64); assert(veg.size()==64);
    assert(std::isfinite(eroded_height(10,.5f,.4f,12)));
    ParticleWorld particles; const auto rain=particles.create({ParticleKind::Rain,{0,10,0},{0,-1,0},100,2,4,.05f,256,9}); assert(rain!=0); assert(particles.update(rain,.5f)); assert(particles.particles(rain));
    CharacterMotionState motion; LocomotionController locomotion; assert(locomotion.update(motion,0,0,true,false,false,.016f)&&motion.state==LocomotionState::Idle); assert(locomotion.update(motion,3,0,true,false,false,.016f)&&motion.state==LocomotionState::Run); assert(locomotion.update(motion,0,-2,false,false,false,.016f)&&motion.state==LocomotionState::Fall);
    AdvancedPhysicsPolicy physics_policy; physics_policy.set_surface("asphalt",{1,.05f,.01f,false,false}); physics_policy.set_surface("rubber",{1.2f,.02f,.01f,false,false}); assert(physics_policy.effective_friction("asphalt","rubber")>.9f&&physics_policy.config().enable_ccd);
    HighFidelityVehicle car(1500); car.set_input(.8f,.1f,.25f,false); float loads[4]={3678,3678,3678,3678}; car.update(.016f,18,loads); assert(car.longitudinal_force(0)>0&&car.state().drivetrain.rpm>800);
    VehiclePossession possession; assert(possession.enter(1,2)&&possession.active()&&possession.player()==1&&possession.vehicle()==2); assert(possession.exit());
    CrowdSolver crowd; crowd.add({1,{},{.35f},2.5f,8}); crowd.add({2,{},{.35f},2.5f,8}); const auto solved=crowd.solve({{0,0,0},{.2f,0,0}},.016f); assert(solved.size()==2);
    LivingWorld world; world.advance(2400); assert(world.state().day==2); world.set_density(2,3,4); assert(world.state().traffic_density==2);
    AudioListener listener; AudioCue cue{invalid_asset,{5,0,0},1,1,10,false}; const auto mix=SpatialAudioProcessor::process(listener,cue,.5f); assert(mix.gain>0&&mix.lowpass<1);
    GameplayEventBus bus; bool received=false; const auto hid=bus.subscribe([&](const GameplayEvent&e){received=e.type==GameplayEventType::VehicleEnter;}); bus.emit({GameplayEventType::VehicleEnter,1,2,"car"}); assert(received); assert(bus.unsubscribe(hid));
    PersistentWorldState save; save.project="Demo"; save.scene="Main"; save.world_time=123; save.variables.push_back({"money","100"}); save.entities.push_back({42,{1,2,3},{4,5,6},.8f,7,{{"door","open"}}}); const auto bytes=PersistentWorldStore::encode(save); PersistentWorldState restored; std::string error; assert(PersistentWorldStore::decode(bytes,restored,error)); assert(restored.entities.size()==1&&restored.entities[0].position.z==3&&restored.variables[0].second=="100");
    AdaptiveQuality quality; for(int i=0;i<8;++i) quality.sample(30,25,45); assert(quality.settings().render_scale<1.0f); assert(WorldStreamScheduler{}.budget().max_uploads>0);
    WorldStreamScheduler streams({2,1,100}); assert(streams.enqueue("far",50,false)); assert(streams.enqueue("near",5,true)); assert(streams.enqueue("mid",20,false)); const auto dispatched=streams.dispatch(); assert(dispatched.size()==2&&dispatched[0]=="near");
    WorldTemplateLibrary templates; assert(templates.define({"crate","prop",{}, {}})); assert(templates.find("crate"));
    ScriptEnvironment script; assert(script.set("difficulty",{"hard"})); assert(script.get("difficulty")->value=="hard");
    SnapshotInterpolator snapshots; snapshots.push({1,10,{0,0,0},{1,0,0}}); snapshots.push({1,20,{10,0,0},{1,0,0}}); const auto sample=snapshots.sample(15); assert(sample&&sample->position.x==5);
    DestructibleObject wall(.5f,12); assert(wall.apply({{}, {0,1,0}, .6f})); assert(wall.state().destroyed);
    const auto android=make_android_packaging_plan({"com.example.exgine","EXGINE Demo","26","35","1.0.0",1,true}); assert(android.valid&&android.gradle_project.find("applicationId")!=std::string::npos&&android.app_manifest.find("NativeActivity")!=std::string::npos);
    return 0;
}

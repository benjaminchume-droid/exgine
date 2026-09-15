#include "exgine/nextgen.hpp"
#include <cassert>
#include <chrono>
#include <thread>

using namespace exgine;

int main() {
    NextGenRuntime runtime;
    assert(runtime.renderer().order().size() == 11);
    assert(runtime.execute_frame(1,0.25f,0.8f,true));
    assert(runtime.stats().frames == 1 && runtime.stats().render_passes == 11);

    GltfRuntimeMaterialResolver resolver;
    RuntimeMaterialState material;
    assert(resolver.bind(material,"car",0.8f,0.2f,{{"base_color",make_asset_id("albedo",{1,2,3}),true}}));
    assert(resolver.ready(material));

    EndToEndAssetPipeline assets([](std::string_view uri,std::vector<std::uint8_t>& bytes,std::string&) {
        bytes.assign(uri.begin(),uri.end()); return true;
    },1);
    assert(assets.request("car.glb",StreamPriority::Critical) != 0);
    for(int i=0;i<100 && assets.stats().decoded==0;++i){assets.pump();std::this_thread::sleep_for(std::chrono::milliseconds(1));}
    assert(assets.stats().submitted==1 && assets.stats().decoded==1 && assets.stats().uploaded==1);

    WorldNavigationController nav({12,12,1.0f,.8f,.35f});
    assert(nav.build([](float,float){return 0.0f;},[](float x,float z){return !(x>4&&x<7&&z>4&&z<7);}));
    NavAgentState agent{42,{0.5f,0,0.5f},{},2.0f,false};
    assert(nav.set_destination(agent,{10.5f,0,10.5f}));
    assert(nav.update(agent,0.1f));

    IntegratedVehicleSimulation car(1500.0f);
    car.input(0.9f,0.0f,0.2f,false);
    float loads[4]={3678,3678,3678,3678};
    car.update(1.0f/120.0f,20.0f,loads);
    assert(car.state().longitudinal>0 && car.state().vehicle.state().drivetrain.engine_torque>0);

    AnimationStateMachine machine;
    assert(machine.add_state({1,"Idle",1}));
    assert(machine.add_state({2,"Run",2}));
    assert(machine.add_transition({1,2,"speed",1.0f,true,0.1f}));
    assert(machine.set_initial(1));
    CharacterAnimationDriver driver;
    assert(driver.set_machine(std::move(machine)));
    bool played=false;
    assert(driver.drive(2.0f,0,true,false,false,0.016f,[&](AnimationClipId,float){played=true;}));
    assert(played && driver.animation_state()==2 && driver.locomotion()==LocomotionState::Run);

    EnvironmentSystem environment;
    environment.set_weather(WeatherType::Rain,0.9f);
    WeatherVisualController visuals;
    visuals.update(environment.state(),100.0f);
    assert(visuals.frame().weather.precipitation>0.8f && visuals.frame().fog_transmittance>0);

    AudioFrameRuntime audio;
    const auto mix=audio.evaluate({},AudioCue{invalid_asset,{5,0,0},1,1,10,false},0.5f);
    assert(mix.gain>0 && mix.lowpass<1);

    UiInteractionRouter ui;
    assert(ui.dispatch({1,0,UiWidgetType::Button,{10,10,100,50},"Play",0,true,true},{50,30,true}));

    SaveRuntimeBridge saves;
    SaveRuntimeSnapshot snapshot;
    assert(saves.capture(snapshot,"Game","Main",123,{{7,{1,2,3},{4,5,6},0.8f,2,{{"door","open"}}}},{{"money","100"}}));
    PersistentWorldState restored; std::string error;
    assert(saves.restore(snapshot.bytes,restored,error));
    assert(restored.entities.size()==1 && restored.variables[0].second=="100");

    AndroidShippingValidator shipping;
    assert(shipping.validate(AndroidPackagingConfig{}).valid);
    return 0;
}

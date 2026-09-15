#include "exgine/production.hpp"
#include <cassert>
#include <chrono>
#include <thread>

using namespace exgine;

int main() {
    ImageDecoderRegistry images;
    std::vector<std::uint8_t> ppm{'P','3',' ','2',' ','1',' ','2','5','5','\n','2','5','5',' ','0',' ','0',' ','0',' ','2','5','5','\n'};
    ImageData decoded{}; std::string error;
    assert(images.decode("albedo.ppm", ppm, decoded, error)); assert(decoded.valid() && decoded.width == 2);

    AsyncAssetStreamer streamer([](std::string_view uri,std::vector<std::uint8_t>& bytes,std::string&)->bool { bytes.assign(uri.begin(),uri.end()); return true; },1);
    const auto id=streamer.submit("assets/player.glb",StreamPriority::Critical); assert(id!=0); std::vector<AssetStreamResult> results;
    for(int i=0;i<100 && results.empty();++i){ results=streamer.poll(); std::this_thread::sleep_for(std::chrono::milliseconds(1)); }
    assert(results.size()==1 && results[0].success); streamer.shutdown();

    NavigationMesh nav({8,8,1,.8f,.35f}); assert(nav.build([](float,float){return 0.0f;},[](float x,float z){return !(x>=3&&x<5&&z>=3&&z<5);}));
    auto path=nav.find_path({0.5f,0,0.5f},{7.5f,0,7.5f}); assert(path.success && path.points.size()>1);

    AnimationStateMachine sm; assert(sm.add_state({1,"Idle",1})); assert(sm.add_state({2,"Run",2})); assert(sm.add_transition({1,2,"speed",1,true,.1f})); assert(sm.set_initial(1)); assert(sm.set_float("speed",2)); bool played=false; assert(sm.update(.016f,[&](AnimationClipId,float){played=true;})); assert(sm.state()==2 && played);

    EnvironmentSystem env; env.set_weather(WeatherType::Rain,0.8f); EnvironmentRendererState visuals; visuals.update(env.state()); assert(visuals.weather().precipitation>=0.79f);

    AudioWorld audio; const auto sid=audio.create_source({0,invalid_asset,{5,0,0},{},1,1,1,10,false,true,true}); assert(sid!=0); assert(!audio.mix(0).empty());
    UiWorld ui; auto label=ui.create(UiWidgetType::Label); assert(ui.set_text(label,"Health")); assert(ui.widget(label)->text=="Health");

    ProductionSave save; save.game.project_name="Production"; save.game.scene_name="Main"; save.game.environment_seconds=10; save.game.runtime_tick=12; save.game.variables["health"]="100"; save.blobs.push_back({"world",{1,2,3}});
    const auto encoded=SaveStore::encode(save); assert(!encoded.empty()); ProductionSave restored; assert(SaveStore::decode(encoded,restored,error)); assert(restored.game.variables["health"]=="100" && restored.blobs.size()==1);

    const std::string project="name = Production\nversion = 1.0\nstartup_scene = Main\ntick_rate = 60\nseconds_per_day = 1200\ndays_per_year = 360\nscene = Main\n";
    GameSession session({},[](std::string_view,std::string& out){out="world {}";return true;}); assert(session.open_project(project)); assert(session.open()); assert(session.update(1.0/60.0)); const auto game_save=session.save(); assert(!game_save.empty()); assert(session.restore(game_save)); assert(session.close());
    return 0;
}

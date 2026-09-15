#include "exgine/playable.hpp"

#include <algorithm>

namespace exgine {

namespace {

bool add_default_features(RenderFeatureGraph& graph) {
    return graph.add({RenderFeature::Shadow,"shadow",{}, {"shadow_map"},{}}) &&
           graph.add({RenderFeature::Depth,"depth",{}, {"depth"},{}}) &&
           graph.add({RenderFeature::GBuffer,"gbuffer",{"depth","shadow_map"},{"scene_color"},{"depth","shadow"}}) &&
           graph.add({RenderFeature::Lighting,"lighting",{"scene_color"},{"lit"},{"gbuffer"}}) &&
           graph.add({RenderFeature::Reflections,"reflections",{"lit","depth"},{"reflected"},{"lighting"}}) &&
           graph.add({RenderFeature::Atmosphere,"atmosphere",{"reflected","depth"},{"atmosphere"},{"reflections"}}) &&
           graph.add({RenderFeature::Water,"water",{"atmosphere","depth"},{"water"},{"atmosphere"}}) &&
           graph.add({RenderFeature::Vegetation,"vegetation",{"water","depth"},{"vegetation"},{"water"}}) &&
           graph.add({RenderFeature::Vfx,"vfx",{"vegetation"},{"vfx"},{"vegetation"}}) &&
           graph.add({RenderFeature::Transparent,"transparent",{"vfx"},{"transparent"},{"vfx"}}) &&
           graph.add({RenderFeature::PostProcess,"post",{"transparent","atmosphere"},{"post"},{"transparent"}}) &&
           graph.add({RenderFeature::UI,"ui",{"post"},{"present"},{"post"}});
}

} // namespace

PlayableGame::PlayableGame(PlayableConfig config, ProjectSourceLoader loader)
    : config_(config), session_({}, std::move(loader)), quality_(config.quality) {
    (void)add_default_features(features_);
    temporal_.resize(config_.history_width, config_.history_height);
    render_plan_=features_.compile();
}

bool PlayableGame::open_project(std::string_view manifest) {
    status_=PlayableStatus::Loading;
    if(!flow_.transition(GameFlowState::Loading)) { status_=PlayableStatus::Error; return false; }
    if(!session_.open_project(manifest) || !compile_render_plan()) {
        flow_.transition(GameFlowState::Error,"project or render pipeline failed to initialize");
        status_=PlayableStatus::Error;
        return false;
    }
    project_open_=true;
    temporal_.reset();
    flow_.transition(GameFlowState::MainMenu);
    status_=PlayableStatus::Menu;
    refresh_acceptance();
    return true;
}

bool PlayableGame::show_menu() noexcept {
    if(!project_open_ || !flow_.can_transition(GameFlowState::MainMenu)) return false;
    if(!flow_.transition(GameFlowState::MainMenu)) return false;
    status_=PlayableStatus::Menu;
    refresh_acceptance();
    return true;
}

bool PlayableGame::start() noexcept {
    if(!project_open_ || !session_.open() || !flow_.transition(GameFlowState::Playing)) return false;
    status_=PlayableStatus::Playing;
    refresh_acceptance();
    return true;
}

bool PlayableGame::pause() noexcept {
    if(status_!=PlayableStatus::Playing || !flow_.transition(GameFlowState::Paused)) return false;
    status_=PlayableStatus::Paused;
    refresh_acceptance();
    return true;
}

bool PlayableGame::resume() noexcept {
    if(status_!=PlayableStatus::Paused || !flow_.transition(GameFlowState::Playing)) return false;
    status_=PlayableStatus::Playing;
    return true;
}

bool PlayableGame::update(double dt) noexcept {
    if(status_!=PlayableStatus::Playing || dt<0) return false;
    profiler_.begin("game_update");
    const bool ok=session_.update(dt);
    profiler_.end("game_update",0.0);
    if(!ok) { status_=PlayableStatus::Error; flow_.transition(GameFlowState::Error,"game session update failed"); return false; }
    presentation_.set_auto_exposure(1.0f,dt);
    quality_.sample(16.666f,8.333f,40.0f);
    ++frame_id_;
    temporal_.begin_frame(frame_id_,0.0f,0.0f);
    refresh_acceptance();
    return true;
}

bool PlayableGame::build_frame(PlayableFrame& result) noexcept {
    result={};
    if(!project_open_ || !session_.open() || status_==PlayableStatus::Closed || status_==PlayableStatus::Error) return false;
    Renderer renderer(config_.render);
    RenderFrame frame;
    const bool built=renderer.build_frame(session_.game().runtime(),frame);
    result.updated=built;
    result.render_frame_valid=built && renderer.validate(frame);
    result.temporal_history_valid=temporal_.history().valid;
    result.frame_id=frame.frame_id;
    refresh_acceptance();
    acceptance_.ready=acceptance_.ready && result.render_frame_valid;
    if(!result.render_frame_valid) {
        acceptance_.ready=false;
        acceptance_.missing.push_back("render_frame_valid");
    }
    return result.render_frame_valid;
}

std::vector<std::uint8_t> PlayableGame::save() {
    if(status_!=PlayableStatus::Playing && status_!=PlayableStatus::Paused) return {};
    if(!flow_.transition(GameFlowState::Saving)) return {};
    status_=PlayableStatus::Saving;
    const auto bytes=session_.save();
    if(bytes.empty()) { flow_.transition(GameFlowState::Error,"save failed"); status_=PlayableStatus::Error; return {}; }
    flow_.transition(GameFlowState::Playing);
    status_=PlayableStatus::Playing;
    refresh_acceptance();
    return bytes;
}

bool PlayableGame::restore(const std::vector<std::uint8_t>& bytes) noexcept {
    if(bytes.empty() || !project_open_ || !session_.restore(bytes)) return false;
    if(status_==PlayableStatus::Paused || status_==PlayableStatus::Menu) {
        flow_.transition(GameFlowState::Playing);
        status_=PlayableStatus::Playing;
    }
    temporal_.reset();
    refresh_acceptance();
    return true;
}

bool PlayableGame::compile_render_plan() noexcept {
    render_plan_=features_.compile();
    return render_plan_.valid && !render_plan_.order.empty();
}

void PlayableGame::refresh_acceptance() noexcept {
    ProductionAcceptanceInput input;
    input.project_open=project_open_ && session_.open();
    input.scene_loaded=session_.game().loaded() && !session_.game().active_scene().empty();
    input.player_spawned=session_.game().runtime().state().entities.size()>1;
    input.render_frame_valid=!render_plan_.order.empty();
    input.physics_running=session_.game().runtime().physics()!=nullptr;
    input.streaming_running=session_.game().runtime().open_world_streamer()!=nullptr;
    const auto android=make_android_packaging_plan(AndroidPackagingConfig{});
    input.android_build_configured=android.valid;
    acceptance_=evaluate_production_acceptance(input);
}

} // namespace exgine

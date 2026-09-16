#include "exgine/playable.hpp"

#include <algorithm>

namespace exgine {
namespace {

bool add_default_features(RenderFeatureGraph& graph) {
    const RenderFeatureNode shadow{
        RenderFeature::Shadow, "shadow", {}, {"shadow_map"}, {}, true};
    const RenderFeatureNode depth{
        RenderFeature::Depth, "depth", {}, {"depth"}, {}, true};
    const RenderFeatureNode gbuffer{
        RenderFeature::GBuffer, "gbuffer", {"depth", "shadow_map"},
        {"scene_color"}, {"depth", "shadow"}, true};
    const RenderFeatureNode lighting{
        RenderFeature::Lighting, "lighting", {"scene_color"},
        {"lit"}, {"gbuffer"}, true};
    const RenderFeatureNode reflections{
        RenderFeature::Reflections, "reflections", {"lit", "depth"},
        {"reflected"}, {"lighting"}, true};
    const RenderFeatureNode atmosphere{
        RenderFeature::Atmosphere, "atmosphere", {"reflected", "depth"},
        {"atmosphere"}, {"reflections"}, true};
    const RenderFeatureNode water{
        RenderFeature::Water, "water", {"atmosphere", "depth"},
        {"water"}, {"atmosphere"}, true};
    const RenderFeatureNode vegetation{
        RenderFeature::Vegetation, "vegetation", {"water", "depth"},
        {"vegetation"}, {"water"}, true};
    const RenderFeatureNode vfx{
        RenderFeature::Vfx, "vfx", {"vegetation"},
        {"vfx"}, {"vegetation"}, true};
    const RenderFeatureNode transparent{
        RenderFeature::Transparent, "transparent", {"vfx"},
        {"transparent"}, {"vfx"}, true};
    const RenderFeatureNode post{
        RenderFeature::PostProcess, "post", {"transparent", "atmosphere"},
        {"post"}, {"transparent"}, true};
    const RenderFeatureNode ui{
        RenderFeature::UI, "ui", {"post"},
        {"present"}, {"post"}, true};

    return graph.add(shadow) && graph.add(depth) && graph.add(gbuffer) &&
           graph.add(lighting) && graph.add(reflections) && graph.add(atmosphere) &&
           graph.add(water) && graph.add(vegetation) && graph.add(vfx) &&
           graph.add(transparent) && graph.add(post) && graph.add(ui);
}

} // namespace

PlayableGame::PlayableGame(PlayableConfig config, ProjectSourceLoader loader)
    : config_(config), session_({}, std::move(loader)), quality_(config.quality) {
    (void)add_default_features(features_);
    temporal_.resize(config_.history_width, config_.history_height);
    render_plan_ = features_.compile();
}

bool PlayableGame::open_project(std::string_view manifest) {
    status_ = PlayableStatus::Loading;
    last_render_frame_valid_ = false;
    if (!flow_.transition(GameFlowState::Loading)) {
        status_ = PlayableStatus::Error;
        return false;
    }
    if (!session_.open_project(manifest) || !compile_render_plan()) {
        flow_.transition(GameFlowState::Error, "project or render pipeline failed to initialize");
        status_ = PlayableStatus::Error;
        return false;
    }
    project_open_ = true;
    save_roundtrip_ = false;
    temporal_.reset();
    flow_.transition(GameFlowState::MainMenu);
    status_ = PlayableStatus::Menu;
    refresh_acceptance();
    return true;
}

bool PlayableGame::show_menu() noexcept {
    if (!project_open_ || !flow_.can_transition(GameFlowState::MainMenu)) return false;
    if (!flow_.transition(GameFlowState::MainMenu)) return false;
    status_ = PlayableStatus::Menu;
    refresh_acceptance();
    return true;
}

bool PlayableGame::start() noexcept {
    if (!project_open_ || !session_.open() || !flow_.transition(GameFlowState::Playing)) return false;
    status_ = PlayableStatus::Playing;
    refresh_acceptance();
    return true;
}

bool PlayableGame::pause() noexcept {
    if (status_ != PlayableStatus::Playing || !flow_.transition(GameFlowState::Paused)) return false;
    status_ = PlayableStatus::Paused;
    refresh_acceptance();
    return true;
}

bool PlayableGame::resume() noexcept {
    if (status_ != PlayableStatus::Paused || !flow_.transition(GameFlowState::Playing)) return false;
    status_ = PlayableStatus::Playing;
    return true;
}

bool PlayableGame::update(double dt) noexcept {
    if (status_ != PlayableStatus::Playing || dt < 0) return false;
    profiler_.begin("game_update");
    const bool ok = session_.update(dt);
    profiler_.end("game_update", 0.0);
    if (!ok) {
        status_ = PlayableStatus::Error;
        flow_.transition(GameFlowState::Error, "game session update failed");
        return false;
    }
    presentation_.set_auto_exposure(1.0f, dt);
    quality_.sample(16.666f, 8.333f, 40.0f);
    ++frame_id_;
    temporal_.begin_frame(frame_id_, 0.0f, 0.0f);
    const float fog = std::clamp(1.0f - presentation_.state().contrast * 0.05f, 0.0f, 1.0f);
    if (!nextgen_.execute_frame(frame_id_, presentation_.state().exposure, fog,
                                temporal_.history().valid)) {
        status_ = PlayableStatus::Error;
        flow_.transition(GameFlowState::Error, "next-generation frame execution failed");
        return false;
    }
    return true;
}

bool PlayableGame::build_frame(PlayableFrame& result) noexcept {
    result = {};
    last_render_frame_valid_ = false;
    if (!project_open_ || !session_.open() || status_ == PlayableStatus::Closed ||
        status_ == PlayableStatus::Error) return false;

    Renderer renderer(config_.render);
    RenderFrame frame;
    const bool built = renderer.build_frame(session_.game().runtime(), frame);
    result.updated = built;
    result.render_frame_valid = built && renderer.validate(frame);
    result.temporal_history_valid = temporal_.history().valid;
    result.nextgen_render_valid = frame_id_ > 0 && nextgen_.stats().frames == frame_id_;
    result.frame_id = frame.frame_id;
    last_render_frame_valid_ = result.render_frame_valid && result.nextgen_render_valid;
    refresh_acceptance();
    if (!result.render_frame_valid) {
        acceptance_.ready = false;
        acceptance_.missing.push_back("render_frame_valid");
    }
    if (!result.nextgen_render_valid) {
        acceptance_.ready = false;
        acceptance_.missing.push_back("nextgen_render_valid");
    }
    return last_render_frame_valid_;
}

std::vector<std::uint8_t> PlayableGame::save() {
    if (status_ != PlayableStatus::Playing && status_ != PlayableStatus::Paused) return {};
    if (!flow_.transition(GameFlowState::Saving)) return {};
    status_ = PlayableStatus::Saving;
    const auto bytes = session_.save();
    if (bytes.empty()) {
        flow_.transition(GameFlowState::Error, "save failed");
        status_ = PlayableStatus::Error;
        return {};
    }
    save_roundtrip_ = true;
    flow_.transition(GameFlowState::Playing);
    status_ = PlayableStatus::Playing;
    refresh_acceptance();
    return bytes;
}

bool PlayableGame::restore(const std::vector<std::uint8_t>& bytes) noexcept {
    if (bytes.empty() || !project_open_ || !session_.restore(bytes)) return false;
    if (status_ == PlayableStatus::Paused || status_ == PlayableStatus::Menu) {
        flow_.transition(GameFlowState::Playing);
        status_ = PlayableStatus::Playing;
    }
    temporal_.reset();
    last_render_frame_valid_ = false;
    refresh_acceptance();
    return true;
}

bool PlayableGame::compile_render_plan() noexcept {
    render_plan_ = features_.compile();
    return render_plan_.valid && !render_plan_.order.empty();
}

void PlayableGame::refresh_acceptance() noexcept {
    ProductionAcceptanceInput input;
    input.project_open = project_open_ && session_.open();
    input.scene_loaded = session_.game().loaded() && !session_.game().active_scene().empty();
    input.player_spawned = session_.game().runtime().state().entities.size() > 1;
    input.render_frame_valid = last_render_frame_valid_;
    input.physics_running = session_.game().runtime().physics() != nullptr;
    input.streaming_running = session_.game().runtime().open_world_streamer() != nullptr;
    input.save_roundtrip = save_roundtrip_;
    const auto android = make_android_packaging_plan(AndroidPackagingConfig{});
    input.android_build_configured = android.valid;
    acceptance_ = evaluate_production_acceptance(input);
}

} // namespace exgine

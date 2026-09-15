#pragma once

#include "exgine/aaa.hpp"
#include "exgine/frontier.hpp"
#include "exgine/production.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class PlayableStatus : std::uint8_t { Closed, Loading, Menu, Playing, Paused, Saving, Error };

struct PlayableConfig {
    RenderConfig render{};
    QualitySettings quality{};
    std::uint32_t history_width=1280;
    std::uint32_t history_height=720;
};

struct PlayableFrame {
    bool updated=false;
    bool render_frame_valid=false;
    bool temporal_history_valid=false;
    std::uint64_t frame_id=0;
};

class PlayableGame {
public:
    explicit PlayableGame(PlayableConfig config={}, ProjectSourceLoader loader={});
    [[nodiscard]] bool open_project(std::string_view manifest);
    [[nodiscard]] bool show_menu() noexcept;
    [[nodiscard]] bool start() noexcept;
    [[nodiscard]] bool pause() noexcept;
    [[nodiscard]] bool resume() noexcept;
    [[nodiscard]] bool update(double dt) noexcept;
    [[nodiscard]] bool build_frame(PlayableFrame& result) noexcept;
    [[nodiscard]] std::vector<std::uint8_t> save();
    [[nodiscard]] bool restore(const std::vector<std::uint8_t>& bytes) noexcept;
    [[nodiscard]] PlayableStatus status() const noexcept { return status_; }
    [[nodiscard]] const RenderExecutionPlan& render_plan() const noexcept { return render_plan_; }
    [[nodiscard]] const TemporalHistory& temporal_history() const noexcept { return temporal_.history(); }
    [[nodiscard]] const PresentationState& presentation() const noexcept { return presentation_.state(); }
    [[nodiscard]] const GameSessionStats& stats() const noexcept { return session_.stats(); }
    [[nodiscard]] const ProductionAcceptanceResult& acceptance() const noexcept { return acceptance_; }
    [[nodiscard]] GameSession& session() noexcept { return session_; }
    [[nodiscard]] const GameSession& session() const noexcept { return session_; }
private:
    PlayableConfig config_{};
    GameSession session_{};
    RenderFeatureGraph features_{};
    RenderExecutionPlan render_plan_{};
    TemporalReconstruction temporal_{};
    GameFlowController flow_{};
    WorldPresentation presentation_{};
    EngineProfiler profiler_{};
    AdaptiveQuality quality_{};
    ProductionAcceptanceResult acceptance_{};
    PlayableStatus status_=PlayableStatus::Closed;
    std::uint64_t frame_id_=0;
    bool project_open_=false;
    bool save_roundtrip_=false;

    bool compile_render_plan() noexcept;
    void refresh_acceptance() noexcept;
};

} // namespace exgine

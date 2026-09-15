#pragma once

#include "exgine/high_fidelity.hpp"
#include "exgine/render.hpp"
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

// Phase 54: explicit render-graph scheduling.
enum class RenderResourceKind : std::uint8_t { Texture, Buffer, Depth, Target };
struct RenderGraphResource { std::string name; RenderResourceKind kind=RenderResourceKind::Texture; std::uint32_t width=1,height=1; bool transient=true; };
struct RenderGraphPass { std::string name; std::vector<std::string> reads,writes; };
class RenderGraph {
public:
    bool add_resource(RenderGraphResource resource);
    bool add_pass(RenderGraphPass pass);
    [[nodiscard]] std::vector<std::string> compile() const;
    [[nodiscard]] bool valid() const noexcept;
    void clear() noexcept;
private:
    std::unordered_map<std::string,RenderGraphResource> resources_;
    std::vector<RenderGraphPass> passes_;
};

// Phase 55: temporal frame history for reconstruction and stable effects.
struct TemporalCameraState { Mat4 view_projection=Mat4::identity(); Vec3 position{}; float jitter_x=0,jitter_y=0; std::uint64_t frame=0; };
struct TemporalHistory { TemporalCameraState previous{}; bool valid=false; float blend=0.9f; };
class TemporalAccumulator {
public:
    void reset() noexcept { history_={}; }
    void begin(const TemporalCameraState& current) noexcept { current_=current; }
    void commit() noexcept { history_.previous=current_; history_.valid=true; }
    [[nodiscard]] const TemporalHistory& history() const noexcept { return history_; }
private:
    TemporalCameraState current_{};
    TemporalHistory history_{};
};

// Phase 56: game-flow lifecycle.
enum class GameFlowState : std::uint8_t { Boot,Loading,Running,Paused,Saving,Quitting,Stopped,Error };
class GameFlowController {
public:
    bool transition(GameFlowState next) noexcept;
    [[nodiscard]] GameFlowState state() const noexcept { return state_; }
    [[nodiscard]] std::uint64_t transitions() const noexcept { return transitions_; }
private:
    GameFlowState state_=GameFlowState::Boot;
    std::uint64_t transitions_=0;
};

// Phase 57: hierarchical runtime profiling.
struct ProfileSample { std::string name; double milliseconds=0; std::uint64_t calls=0; };
class RuntimeProfiler {
public:
    void begin_frame() noexcept;
    void record(std::string_view name,double milliseconds) noexcept;
    [[nodiscard]] const std::vector<ProfileSample>& samples() const noexcept { return samples_; }
    [[nodiscard]] double frame_ms() const noexcept { return frame_ms_; }
private:
    std::vector<ProfileSample> samples_;
    double frame_ms_=0;
};

// Phase 58: deterministic simulation snapshots/rollback.
struct SimulationSnapshot { std::uint64_t tick=0; std::vector<std::uint8_t> bytes; };
class RollbackBuffer {
public:
    explicit RollbackBuffer(std::size_t capacity=120) : capacity_(capacity?capacity:1) {}
    bool push(SimulationSnapshot snapshot);
    [[nodiscard]] const SimulationSnapshot* find(std::uint64_t tick) const noexcept;
    bool discard_before(std::uint64_t tick) noexcept;
    void clear() noexcept { snapshots_.clear(); }
    [[nodiscard]] std::size_t size() const noexcept { return snapshots_.size(); }
private:
    std::size_t capacity_=120;
    std::vector<SimulationSnapshot> snapshots_;
};

// Phase 59: presentation orchestration joining environment, quality and particles.
struct PresentationState { HighQualityLighting lighting{}; AtmosphereModel atmosphere{}; QualitySettings quality{}; WeatherVisualState weather{}; };
class PresentationSystem {
public:
    void set_quality(QualitySettings settings) noexcept { state_.quality=settings; }
    void update_environment(const EnvironmentState& environment) noexcept;
    [[nodiscard]] const PresentationState& state() const noexcept { return state_; }
private:
    PresentationState state_{};
};

// Phase 60: production readiness contract for a runnable game.
struct ProductionReadinessReport {
    bool project=true, runtime=true, rendering=true, physics=true, content=true, saving=true, packaging=true;
    [[nodiscard]] bool ready() const noexcept { return project&&runtime&&rendering&&physics&&content&&saving&&packaging; }
};
[[nodiscard]] ProductionReadinessReport evaluate_production_readiness(bool project,bool runtime,bool rendering,bool physics,bool content,bool saving,bool packaging) noexcept;

} // namespace exgine

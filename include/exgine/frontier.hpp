#pragma once

#include "exgine/render.hpp"
#include "exgine/game.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class RenderFeature : std::uint8_t { Shadow, Depth, GBuffer, Lighting, Reflections, Atmosphere, Water, Vegetation, Vfx, Transparent, PostProcess, UI };
struct RenderFeatureNode { RenderFeature feature{}; std::string name; std::vector<std::string> reads; std::vector<std::string> writes; std::vector<std::string> depends_on; bool enabled=true; };
struct RenderExecutionPlan { bool valid=false; std::vector<std::string> order; std::string error; };
class RenderFeatureGraph { public: bool add(RenderFeatureNode node); bool set_enabled(std::string_view name,bool enabled) noexcept; [[nodiscard]] RenderExecutionPlan compile() const; void clear() noexcept { nodes_.clear(); } [[nodiscard]] std::size_t size() const noexcept { return nodes_.size(); } private: std::unordered_map<std::string,RenderFeatureNode> nodes_; };

struct TemporalJitter { float x=0,y=0; std::uint32_t index=0; };
struct TemporalHistory { std::uint32_t width=0,height=0; bool valid=false; std::uint64_t frame_id=0; float blend=0.9f; TemporalJitter jitter{}; };
class TemporalReconstruction { public: void resize(std::uint32_t width,std::uint32_t height) noexcept; void reset() noexcept { history_.valid=false; history_.frame_id=0; history_.jitter={}; } [[nodiscard]] TemporalJitter next_jitter() noexcept; void begin_frame(std::uint64_t frame_id,float motion_magnitude,float disocclusion) noexcept; [[nodiscard]] const TemporalHistory& history() const noexcept { return history_; } private: TemporalHistory history_{}; };

enum class GameFlowState : std::uint8_t { Boot, Loading, MainMenu, Playing, Paused, Saving, Error, Shutdown };
struct GameFlowSnapshot { GameFlowState state=GameFlowState::Boot; GameFlowState previous=GameFlowState::Boot; std::uint64_t transitions=0; std::string error; };
class GameFlowController { public: bool transition(GameFlowState next,std::string error={}); [[nodiscard]] bool can_transition(GameFlowState next) const noexcept; [[nodiscard]] GameFlowState state() const noexcept { return snapshot_.state; } [[nodiscard]] const GameFlowSnapshot& snapshot() const noexcept { return snapshot_; } private: GameFlowSnapshot snapshot_{}; };

struct ProfileSample { std::string name; double milliseconds=0; std::uint64_t calls=0; };
class EngineProfiler { public: void begin(std::string_view name) noexcept; void end(std::string_view name,double milliseconds) noexcept; void add_counter(std::string_view name,std::uint64_t amount=1) noexcept; [[nodiscard]] const ProfileSample* sample(std::string_view name) const noexcept; [[nodiscard]] std::uint64_t counter(std::string_view name) const noexcept; [[nodiscard]] std::vector<ProfileSample> samples() const; void reset() noexcept; private: std::unordered_map<std::string,ProfileSample> samples_; std::unordered_map<std::string,std::uint64_t> counters_; std::unordered_map<std::string,std::uint64_t> active_; };

struct RollbackFrame { std::uint32_t tick=0; std::vector<std::uint8_t> state; };
class RollbackBuffer { public: explicit RollbackBuffer(std::size_t capacity=120) : capacity_(capacity?capacity:1) {} bool push(std::uint32_t tick,std::vector<std::uint8_t> state); [[nodiscard]] const RollbackFrame* exact(std::uint32_t tick) const noexcept; [[nodiscard]] const RollbackFrame* latest_at_or_before(std::uint32_t tick) const noexcept; bool discard_after(std::uint32_t tick) noexcept; void clear() noexcept { frames_.clear(); } [[nodiscard]] std::size_t size() const noexcept { return frames_.size(); } private: std::size_t capacity_=120; std::vector<RollbackFrame> frames_; };

struct PresentationState { float exposure=0,contrast=1,white_point=1,sharpening=0; bool hdr=true; };
class WorldPresentation { public: void set_sun_elevation(float radians) noexcept; void set_weather_fog(float density) noexcept; void set_auto_exposure(float luminance,double dt) noexcept; [[nodiscard]] const PresentationState& state() const noexcept { return state_; } private: PresentationState state_{}; };

struct ProductionAcceptanceInput { bool project_open=false,scene_loaded=false,player_spawned=false,render_frame_valid=false,physics_running=false,streaming_running=false,save_roundtrip=false,android_build_configured=false; };
struct ProductionAcceptanceResult { bool ready=false; std::vector<std::string> missing; };
[[nodiscard]] ProductionAcceptanceResult evaluate_production_acceptance(const ProductionAcceptanceInput& input);

} // namespace exgine

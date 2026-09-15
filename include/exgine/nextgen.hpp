#pragma once

#include "exgine/aaa.hpp"
#include "exgine/frontier.hpp"
#include "exgine/high_fidelity.hpp"
#include "exgine/playable.hpp"
#include "exgine/production.hpp"

#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

enum class NextGenPass : std::uint8_t { Shadow, Geometry, Lighting, Reflections, Atmosphere, Water, Vegetation, Vfx, Transparency, PostProcess, UI };
struct NextGenPassContext { std::uint64_t frame_id=0; float exposure=0; float fog_transmittance=1; bool history_valid=false; };
using NextGenPassFn=std::function<bool(NextGenPassContext&)>;
class NextGenRenderExecutor {
public:
    bool add(NextGenPass pass,std::string name,NextGenPassFn fn);
    bool execute(std::uint64_t frame_id,float exposure,float fog_transmittance,bool history_valid) const;
    [[nodiscard]] const std::vector<std::string>& order() const noexcept { return order_; }
private:
    struct Pass { NextGenPass pass{}; std::string name; NextGenPassFn fn; };
    std::vector<Pass> passes_;
    std::vector<std::string> order_;
};

struct TextureBindingRecord { std::string slot; AssetId asset=invalid_asset; bool resident=false; };
struct RuntimeMaterialState { std::string name; float metallic=0.0f,roughness=0.5f; float transmission=0,clear_coat=0,subsurface=0; std::vector<TextureBindingRecord> textures; };
class GltfRuntimeMaterialResolver {
public:
    bool bind(RuntimeMaterialState& out,std::string name,float metallic,float roughness,std::vector<TextureBindingRecord> textures) const;
    [[nodiscard]] bool ready(const RuntimeMaterialState& material) const noexcept;
};

struct AssetPipelineStats { std::uint64_t submitted=0,decoded=0,uploaded=0,failed=0; std::uint64_t resident_bytes=0; };
class EndToEndAssetPipeline {
public:
    explicit EndToEndAssetPipeline(AsyncAssetStreamer::Loader loader={},std::uint32_t workers=2);
    std::uint64_t request(std::string uri,StreamPriority priority=StreamPriority::Normal);
    std::size_t pump(std::size_t max_results=16);
    [[nodiscard]] const AssetPipelineStats& stats() const noexcept { return stats_; }
private:
    AsyncAssetStreamer streamer_;
    AssetPipelineStats stats_{};
};

struct NavAgentState { EntityId entity=invalid_entity; Vec3 position{}; Vec3 destination{}; float speed=2.5f; bool active=false; };
class WorldNavigationController {
public:
    explicit WorldNavigationController(NavMeshConfig config={}) : navigation_(config) {}
    bool build(const std::function<float(float,float)>& height,const std::function<bool(float,float)>& walkable);
    bool set_destination(NavAgentState& agent,Vec3 destination);
    bool update(NavAgentState& agent,float dt) const noexcept;
    [[nodiscard]] const NavigationMesh& mesh() const noexcept { return navigation_.mesh(); }
private:
    NavigationSystem navigation_;
};

struct VehicleFrameState { HighFidelityVehicle vehicle; float longitudinal=0,lateral=0; };
class IntegratedVehicleSimulation {
public:
    explicit IntegratedVehicleSimulation(float mass=1500.0f):state_{HighFidelityVehicle(mass),0,0}{}
    void input(float throttle,float brake,float steer,bool reverse) noexcept;
    void update(float dt,float speed,float normal_load[4]) noexcept;
    [[nodiscard]] const VehicleFrameState& state() const noexcept { return state_; }
private:
    VehicleFrameState state_;
};

class CharacterAnimationDriver {
public:
    bool set_machine(AnimationStateMachine machine);
    bool drive(float horizontal_speed,float vertical_speed,bool grounded,bool swimming,bool climbing,float dt,const std::function<void(AnimationClipId,float)>& play) noexcept;
    [[nodiscard]] LocomotionState locomotion() const noexcept { return locomotion_.state; }
    [[nodiscard]] AnimationStateId animation_state() const noexcept { return machine_.state(); }
private:
    AnimationStateMachine machine_{};
    LocomotionController locomotion_controller_{};
    CharacterMotionState locomotion_{};
};

struct WeatherVisualFrame { SkyState sky{}; WeatherVisualState weather{}; float fog_transmittance=1.0f; };
class WeatherVisualController {
public:
    void update(const EnvironmentState& state,float altitude=0) noexcept;
    [[nodiscard]] const WeatherVisualFrame& frame() const noexcept { return frame_; }
private:
    EnvironmentRendererState renderer_{};
    WeatherVisualFrame frame_{};
};

struct AudioFrameSample { float gain=1,pan=0,lowpass=1; };
class AudioFrameRuntime {
public:
    AudioFrameSample evaluate(const AudioListener& listener,const AudioCue& cue,float occlusion) const noexcept;
};

struct UiInputPoint { float x=0,y=0; bool pressed=false; };
class UiInteractionRouter {
public:
    bool hit(UiRect rect,UiInputPoint point) const noexcept;
    bool dispatch(const UiWidget& widget,UiInputPoint point) const noexcept;
};

struct SaveRuntimeSnapshot { PersistentWorldState world{}; std::vector<std::uint8_t> bytes; };
class SaveRuntimeBridge {
public:
    bool capture(SaveRuntimeSnapshot& snapshot,std::string project,std::string scene,double time,const std::vector<PersistedEntityState>& entities,const std::vector<std::pair<std::string,std::string>>& variables) const;
    bool restore(const std::vector<std::uint8_t>& bytes,PersistentWorldState& out,std::string& error) const;
};

struct AndroidPackageCheck { bool valid=false; std::vector<std::string> missing; };
class AndroidShippingValidator {
public:
    AndroidPackageCheck validate(const AndroidPackagingConfig& config) const;
};

struct NextGenRuntimeStats { std::uint64_t frames=0,render_passes=0,asset_uploads=0,navigation_updates=0,vehicle_updates=0; };
class NextGenRuntime {
public:
    NextGenRuntime();
    bool execute_frame(std::uint64_t frame_id,float exposure,float fog_transmittance,bool history_valid);
    [[nodiscard]] const NextGenRuntimeStats& stats() const noexcept { return stats_; }
    [[nodiscard]] const NextGenRenderExecutor& renderer() const noexcept { return renderer_; }
private:
    NextGenRenderExecutor renderer_{};
    NextGenRuntimeStats stats_{};
};

} // namespace exgine

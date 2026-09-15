#pragma once
#include "exgine/geometry.hpp"
#include "exgine/material.hpp"
#include "exgine/production.hpp"
#include "exgine/physics.hpp"
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace exgine {

// Phase 27-29: production asset/material/geometry readiness.
enum class TextureColorSpace : std::uint8_t { Linear, SRGB, HDR };
enum class TextureCompression : std::uint8_t { Raw, BC, ETC2, ASTC };
struct TextureRuntimeDesc {
    std::uint32_t width=0,height=0,mip_levels=1;
    TextureColorSpace color_space=TextureColorSpace::SRGB;
    TextureCompression compression=TextureCompression::Raw;
    bool streamed=false;
};
struct MaterialRuntimeDesc {
    float roughness_scale=1.0f, metallic_scale=1.0f, normal_scale=1.0f;
    float clear_coat=0.0f, transmission=0.0f, subsurface=0.0f, sheen=0.0f;
    float wetness=0.0f,dust=0.0f,mud=0.0f,snow=0.0f,rust=0.0f,moss=0.0f;
};
class MaterialRuntimeState {
public:
    bool set(std::string name, MaterialRuntimeDesc desc);
    bool set_weathering(std::string_view name,float wetness,float dust,float mud,float snow,float rust,float moss) noexcept;
    [[nodiscard]] const MaterialRuntimeDesc* get(std::string_view name) const noexcept;
private:
    std::unordered_map<std::string,MaterialRuntimeDesc> materials_;
};
struct MeshRuntimeStats { std::uint64_t vertices=0,indices=0,triangles=0,draw_parts=0; };
[[nodiscard]] MeshRuntimeStats inspect_mesh(const MeshAssembly& assembly) noexcept;

// Phase 30-35: high-fidelity lighting, atmosphere, water, vegetation and terrain.
struct HighQualityLighting {
    Vec3 sun_direction{0.35f,-0.75f,0.2f};
    Color3 sun_color{1.0f,0.96f,0.88f};
    float sun_intensity=5.0f;
    float exposure=0.0f;
    float ambient_intensity=0.25f;
    float ao_strength=1.0f;
    float contact_shadow_distance=2.0f;
    float reflection_strength=1.0f;
    bool cascaded_shadows=true;
    bool screen_space_reflections=true;
};
struct AtmosphereModel {
    float planet_radius=6360000.0f;
    float atmosphere_height=80000.0f;
    float rayleigh_scale_height=8000.0f;
    float mie_scale_height=1200.0f;
    float ozone_strength=1.0f;
    float fog_density=0.0f;
    float aerial_perspective=1.0f;
    float cloud_coverage=0.0f;
    float cloud_density=0.0f;
    Vec3 sun_direction{0.35f,-0.75f,0.2f};
};
struct WaterSurface {
    Vec3 center{};
    float width=1.0f,depth=1.0f,level=0.0f;
    float wave_amplitude=0.05f,wave_length=8.0f,wave_speed=1.0f;
    float roughness=0.08f,foam=0.1f,flow_speed=0.0f;
    bool ocean=false;
};
struct VegetationSpecies {
    std::string name;
    float min_height=0.5f,max_height=12.0f,density=0.2f;
    float wind_response=1.0f;
    std::uint64_t seed=0;
};
struct VegetationPatch { Vec3 center{}; float radius=10.0f; std::vector<VegetationSpecies> species; };
struct TerrainHighDetailConfig {
    std::uint32_t chunk_resolution=129;
    float world_scale=1.0f;
    float erosion_strength=0.35f;
    float thermal_strength=0.15f;
    float slope_blend=1.0f;
    float snow_line=1600.0f;
    float rock_line=900.0f;
};
float eroded_height(float base,float slope,float moisture,std::uint64_t seed) noexcept;
std::vector<Vec3> generate_vegetation(const VegetationPatch& patch,std::uint32_t count) noexcept;

// Phase 35: GPU/CPU visual effects as deterministic simulation descriptions.
enum class ParticleKind : std::uint8_t { Rain,Snow,Dust,Smoke,Fire,Sparks,Leaves,WaterSpray,Debris };
struct ParticleEmitter { ParticleKind kind=ParticleKind::Rain; Vec3 position{}; Vec3 direction{0,-1,0}; float rate=100.0f,lifetime=1.0f,speed=1.0f,size=0.05f; std::uint32_t max_particles=4096; std::uint64_t seed=0; };
struct ParticleState { Vec3 position{},velocity{}; float life=0; };
class ParticleWorld {
public:
    std::uint64_t create(ParticleEmitter emitter={});
    bool destroy(std::uint64_t id) noexcept;
    bool update(std::uint64_t id,float dt) noexcept;
    [[nodiscard]] const std::vector<ParticleState>* particles(std::uint64_t id) const noexcept;
    void clear() noexcept;
private:
    struct EmitterState { ParticleEmitter emitter; float accumulator=0; std::vector<ParticleState> particles; };
    std::unordered_map<std::uint64_t,EmitterState> emitters_;
    std::uint64_t next_id_=1;
};

// Phase 36-37: animation/character motion quality.
enum class LocomotionState : std::uint8_t { Idle,Walk,Run,Crouch,Jump,Fall,Swim,Climb,Interact };
struct CharacterMotionState { LocomotionState state=LocomotionState::Idle; float speed=0,vertical_speed=0,blend=0; bool grounded=true; };
class LocomotionController {
public:
    bool update(CharacterMotionState& state,float horizontal_speed,float vertical_speed,bool grounded,bool swimming,bool climbing,float dt) noexcept;
private:
    LocomotionState select(float horizontal_speed,float vertical_speed,bool grounded,bool swimming,bool climbing) const noexcept;
};

// Phase 38: advanced physical material/contact policy.
struct PhysicsQualityConfig { std::uint32_t solver_iterations=12,velocity_iterations=8; float ccd_speed_threshold=12.0f,penetration_slop=0.005f; bool enable_ccd=true,deterministic=true; };
struct PhysicalSurface { float friction=0.8f,restitution=0.05f,rolling_resistance=0.015f; bool buoyant=false,destructible=false; };
struct ImpactEvent { EntityId a=invalid_entity,b=invalid_entity; Vec3 point{},normal{}; float impulse=0; };
class AdvancedPhysicsPolicy {
public:
    explicit AdvancedPhysicsPolicy(PhysicsQualityConfig config={}) : config_(config) {}
    void set_surface(std::string material,PhysicalSurface surface);
    [[nodiscard]] const PhysicsQualityConfig& config() const noexcept { return config_; }
    [[nodiscard]] const PhysicalSurface* surface(std::string_view material) const noexcept;
    [[nodiscard]] float effective_friction(std::string_view a,std::string_view b) const noexcept;
private:
    PhysicsQualityConfig config_{};
    std::unordered_map<std::string,PhysicalSurface> surfaces_;
};

// Phase 39: vehicle dynamics.
struct TireState { float radius=0.34f,width=0.22f,load=0,slip_ratio=0,slip_angle=0,longitudinal_force=0,lateral_force=0; };
struct DrivetrainState { float throttle=0,brake=0,steer=0,rpm=800,gear=1,engine_torque=0,drive_torque=0; bool reverse=false; };
struct SuspensionState { float rest_length=0.35f,length=0.35f,compression=0,velocity=0,spring_force=0,damper_force=0; };
struct VehicleDynamicsState { DrivetrainState drivetrain{}; TireState tires[4]{}; SuspensionState suspension[4]{}; float speed=0,yaw_rate=0,roll=0,pitch=0; };
class HighFidelityVehicle {
public:
    explicit HighFidelityVehicle(float mass=1500.0f);
    void set_input(float throttle,float brake,float steer,bool reverse) noexcept;
    void update(float dt,float forward_speed,float normal_load[4]) noexcept;
    [[nodiscard]] const VehicleDynamicsState& state() const noexcept { return state_; }
    [[nodiscard]] float longitudinal_force(std::size_t wheel) const noexcept;
    [[nodiscard]] float lateral_force(std::size_t wheel) const noexcept;
private:
    float mass_=1500.0f;
    VehicleDynamicsState state_{};
};
class VehiclePossession {
public:
    bool enter(EntityId player,EntityId vehicle) noexcept;
    bool exit() noexcept;
    [[nodiscard]] bool active() const noexcept { return player_!=invalid_entity&&vehicle_!=invalid_entity; }
    [[nodiscard]] EntityId player() const noexcept { return player_; }
    [[nodiscard]] EntityId vehicle() const noexcept { return vehicle_; }
private:
    EntityId player_=invalid_entity,vehicle_=invalid_entity;
};

// Phase 40: navigation/crowds.
struct CrowdAgent { EntityId entity=invalid_entity; Vec3 velocity{}; float radius=0.35f,max_speed=2.5f,acceleration=8.0f; };
class CrowdSolver {
public:
    void add(CrowdAgent agent);
    void clear() noexcept;
    [[nodiscard]] std::vector<CrowdAgent> solve(const std::vector<Vec3>& positions,float dt) const noexcept;
private:
    std::vector<CrowdAgent> agents_;
};

// Phase 41: living-world simulation.
struct WorldSimulationState { double time_seconds=0; std::uint32_t day=0; std::uint32_t season=0; float traffic_density=1.0f,pedestrian_density=1.0f,wildlife_activity=1.0f; };
class LivingWorld {
public:
    void set_time(double seconds) noexcept;
    void advance(double dt,float day_length_seconds=1200.0f) noexcept;
    void set_density(float traffic,float pedestrians,float wildlife) noexcept;
    [[nodiscard]] const WorldSimulationState& state() const noexcept { return state_; }
private:
    WorldSimulationState state_{};
};

// Phase 42: audio scheduling/occlusion policy.
struct AudioCue { AssetId clip=invalid_asset; Vec3 position{}; float gain=1,pitch=1,radius=25; bool looping=false; };
struct AudioMixResult { float gain=1,pan=0,lowpass=1; };
class SpatialAudioProcessor {
public:
    [[nodiscard]] static AudioMixResult process(const AudioListener& listener,const AudioCue& cue,float occlusion) noexcept;
};

// Phase 43-44: game UI and gameplay events.
enum class GameplayEventType : std::uint8_t { Interact,Damage,Death,Pickup,QuestStart,QuestComplete,VehicleEnter,VehicleExit,Dialogue };
struct GameplayEvent { GameplayEventType type=GameplayEventType::Interact; EntityId source=invalid_entity,target=invalid_entity; std::string payload; };
class GameplayEventBus {
public:
    using Handler=std::function<void(const GameplayEvent&)>;
    std::uint64_t subscribe(Handler handler);
    bool unsubscribe(std::uint64_t id) noexcept;
    void emit(const GameplayEvent& event);
    void clear() noexcept;
private:
    std::unordered_map<std::uint64_t,Handler> handlers_;
    std::uint64_t next_id_=1;
};

// Phase 45: persistent world records suitable for full game saves.
struct PersistedEntityState {
    EntityId id=invalid_entity;
    Vec3 position{};
    Vec3 velocity{};
    float health=1.0f;
    std::uint32_t flags=0;
    std::vector<std::pair<std::string,std::string>> variables;
};
struct PersistentWorldState {
    std::uint32_t version=2;
    std::string project,scene;
    double world_time=0;
    std::vector<PersistedEntityState> entities;
    std::vector<std::pair<std::string,std::string>> variables;
};
class PersistentWorldStore {
public:
    [[nodiscard]] static std::vector<std::uint8_t> encode(const PersistentWorldState& state);
    [[nodiscard]] static bool decode(const std::vector<std::uint8_t>& bytes,PersistentWorldState& state,std::string& error);
};

// Phase 46-47: streaming and quality control.
enum class QualityTier : std::uint8_t { Low,Medium,High,Ultra,Auto };
struct QualitySettings { QualityTier tier=QualityTier::Auto; float render_scale=1.0f; float shadow_distance=100.0f; float vegetation_scale=1.0f; float particle_scale=1.0f; bool reflections=true,volumetrics=true; };
class AdaptiveQuality {
public:
    explicit AdaptiveQuality(QualitySettings settings={}) : settings_(settings) {}
    void sample(float frame_ms,float gpu_ms,float temperature_c) noexcept;
    [[nodiscard]] const QualitySettings& settings() const noexcept { return settings_; }
private:
    QualitySettings settings_{}; std::uint32_t pressure_frames_=0,headroom_frames_=0;
};
struct StreamBudget { std::uint32_t max_uploads=8,max_generation=4; std::uint64_t gpu_bytes=256ull*1024ull*1024ull; };
class WorldStreamScheduler {
public:
    explicit WorldStreamScheduler(StreamBudget budget={}) : budget_(budget) {}
    bool enqueue(std::string uri,float distance,bool visible);
    [[nodiscard]] std::vector<std::string> dispatch();
    [[nodiscard]] const StreamBudget& budget() const noexcept { return budget_; }
    void clear() noexcept { queue_.clear(); }
private:
    struct Job { std::string uri; float distance; bool visible; };
    StreamBudget budget_{}; std::vector<Job> queue_;
};

// Phase 48: renderer/backend capability profile.
struct RendererCapabilities { bool compute=false,mesh_shading=false,ray_tracing=false,hdr=true; std::uint32_t max_texture_size=4096; };
struct RenderProfile { RendererCapabilities capabilities{}; QualitySettings quality{}; };

// Phase 49-50: editor/game scripting surface.
struct WorldObjectTemplate { std::string name,type; MeshAssembly geometry{}; MaterialRuntimeDesc material{}; };
class WorldTemplateLibrary {
public:
    bool define(WorldObjectTemplate object);
    [[nodiscard]] const WorldObjectTemplate* find(std::string_view name) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return objects_.size(); }
private:
    std::unordered_map<std::string,WorldObjectTemplate> objects_;
};
struct ScriptValue { std::string value; };
class ScriptEnvironment {
public:
    bool set(std::string name,ScriptValue value);
    [[nodiscard]] const ScriptValue* get(std::string_view name) const noexcept;
    void clear() noexcept { values_.clear(); }
private:
    std::unordered_map<std::string,ScriptValue> values_;
};

// Phase 51-52: networking and destruction foundations.
struct NetworkEntitySnapshot { EntityId id=invalid_entity; std::uint32_t tick=0; Vec3 position{},velocity{}; };
class SnapshotInterpolator {
public:
    void push(NetworkEntitySnapshot snapshot);
    [[nodiscard]] std::optional<NetworkEntitySnapshot> sample(std::uint32_t tick) const noexcept;
    void clear() noexcept { snapshots_.clear(); }
private:
    std::vector<NetworkEntitySnapshot> snapshots_;
};
struct DestructionImpulse { Vec3 point{},direction{0,1,0}; float energy=0; };
struct DestructionState { float integrity=1.0f; bool destroyed=false; std::uint32_t fragments=0; };
class DestructibleObject {
public:
    explicit DestructibleObject(float integrity=1.0f,std::uint32_t fragments=8):state_{integrity,false,fragments}{}
    bool apply(const DestructionImpulse& impulse) noexcept;
    [[nodiscard]] const DestructionState& state() const noexcept { return state_; }
private:
    DestructionState state_{};
};

// Phase 53: shipping target and build contract.
struct AndroidPackagingConfig { std::string application_id="com.exgine.game"; std::string display_name="EXGINE Game"; std::string min_sdk="26"; std::string target_sdk="35"; std::string version_name="1.0.0"; std::uint32_t version_code=1; bool bundle=true; };
struct AndroidPackagingPlan { std::string gradle_project; std::string app_manifest; std::string cmake_arguments; bool valid=false; };
[[nodiscard]] AndroidPackagingPlan make_android_packaging_plan(const AndroidPackagingConfig& config);

} // namespace exgine

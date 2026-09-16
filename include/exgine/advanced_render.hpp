#pragma once

#include "exgine/geometry.hpp"
#include "exgine/lighting.hpp"
#include "exgine/render.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>

namespace exgine {

// Advanced rendering/runtime primitives. These are backend-neutral so the same
// scene decisions can be exercised in CI and consumed by OpenGL ES/Vulkan/etc.

enum class RenderFeature : std::uint8_t {
    Shadows, Hdr, Bloom, Exposure, Ibl, ReflectionProbes, Atmosphere,
    Fog, Terrain, Vegetation, Water, GpuCulling, Streaming, WorldStreaming,
    AdvancedMaterials
};

struct RenderFeatureSet {
    bool shadows=true, hdr=true, bloom=true, exposure=true, ibl=true;
    bool reflection_probes=true, atmosphere=true, fog=true, terrain=true;
    bool vegetation=true, water=true, gpu_culling=true, streaming=true;
    bool world_streaming=true, advanced_materials=true;
    [[nodiscard]] bool enabled(RenderFeature f) const noexcept;
};

struct HdrSettings {
    bool enabled=true;
    float exposure_ev=0.0f;
    float min_exposure=-8.0f;
    float max_exposure=8.0f;
    float adaptation_speed=2.0f;
    float white_point=4.0f;
    float gamma=2.2f;
};

struct BloomSettings {
    bool enabled=true;
    float threshold=1.0f;
    float knee=0.5f;
    float intensity=0.08f;
    std::uint32_t mip_count=5;
};

struct ShadowCascade {
    float near_distance=0.05f;
    float far_distance=100.0f;
    Mat4 view_projection=Mat4::identity();
};

struct ShadowSettings {
    bool enabled=true;
    std::uint32_t atlas_size=2048;
    std::uint32_t cascades=4;
    float split_lambda=0.7f;
    float depth_bias=0.0015f;
    float normal_bias=0.0025f;
    std::array<ShadowCascade,4> cascade{};
};

class ShadowSystem {
public:
    [[nodiscard]] ShadowSettings build_directional(const Camera&, const Light&, std::uint32_t atlas_size=2048) const noexcept;
    [[nodiscard]] bool should_cast(const Light&) const noexcept;
};

struct IblSettings {
    bool enabled=true;
    std::uint32_t irradiance_size=32;
    std::uint32_t specular_size=128;
    std::uint32_t specular_mips=7;
    float intensity=1.0f;
    std::uint64_t environment_asset=0;
};

struct ReflectionProbe {
    std::uint64_t id=0;
    Vec3 position{};
    Vec3 extents{5,5,5};
    float blend_distance=1.0f;
    std::uint32_t resolution=128;
    bool realtime=false;
};

class ReflectionProbeWorld {
public:
    std::uint64_t add(ReflectionProbe probe);
    bool remove(std::uint64_t id) noexcept;
    [[nodiscard]] const ReflectionProbe* closest(Vec3 position) const noexcept;
    [[nodiscard]] const std::vector<ReflectionProbe>& probes() const noexcept { return probes_; }
private:
    std::uint64_t next_id_=1;
    std::vector<ReflectionProbe> probes_;
};

struct PostProcessPass {
    std::string name;
    bool enabled=true;
    std::uint32_t input_width=0, input_height=0;
    std::uint32_t output_width=0, output_height=0;
};

class PostProcessPipeline {
public:
    void configure(std::uint32_t width, std::uint32_t height, const HdrSettings&, const BloomSettings&);
    [[nodiscard]] const std::vector<PostProcessPass>& passes() const noexcept { return passes_; }
    [[nodiscard]] bool hdr_required() const noexcept { return hdr_required_; }
private:
    bool hdr_required_=false;
    std::vector<PostProcessPass> passes_;
};

struct ExposureMeter {
    float luminance=0.18f;
    float exposure_ev=0.0f;
};

class ExposureController {
public:
    void reset(float exposure_ev=0.0f) noexcept { current_=exposure_ev; }
    [[nodiscard]] float update(float average_luminance, float target_middle_gray,
                               float delta_seconds, const HdrSettings&) noexcept;
    [[nodiscard]] float current() const noexcept { return current_; }
private:
    float current_=0.0f;
};

struct AtmosphereSettings {
    bool enabled=true;
    Color3 sky_top{0.28f,0.48f,0.9f};
    Color3 sky_horizon{0.75f,0.82f,0.95f};
    Color3 ground{0.20f,0.22f,0.25f};
    float rayleigh=1.0f;
    float mie=0.08f;
    float sun_disc=1.0f;
    float sun_angular_radius=0.00465f;
    float cloud_coverage=0.0f;
};

struct FogResult { float factor=1.0f; Color3 color{}; };
[[nodiscard]] FogResult evaluate_fog(const FogSettings&, float distance, float height=0.0f) noexcept;
[[nodiscard]] Color3 evaluate_sky(const AtmosphereSettings&, Vec3 direction, Vec3 sun_direction) noexcept;

struct TerrainConfig {
    std::uint32_t chunk_size=64;
    std::uint32_t resolution=65;
    float world_scale=2.0f;
    float height_scale=100.0f;
    std::uint32_t lod_count=6;
    float lod0_distance=80.0f;
    float lod_multiplier=2.0f;
};

struct TerrainChunk {
    std::int32_t x=0,z=0;
    std::uint8_t lod=0;
    float min_height=0,max_height=0;
    bool loaded=false;
    [[nodiscard]] std::uint64_t key() const noexcept;
};

class TerrainSystem {
public:
    explicit TerrainSystem(TerrainConfig config={}) : config_(config) {}
    [[nodiscard]] std::vector<TerrainChunk> visible_chunks(Vec3 camera, float view_distance) const;
    [[nodiscard]] std::uint8_t choose_lod(float distance) const noexcept;
    [[nodiscard]] const TerrainConfig& config() const noexcept { return config_; }
private:
    TerrainConfig config_{};
};

struct VegetationPrototype {
    std::uint64_t mesh_asset=0;
    float min_scale=0.8f, max_scale=1.2f;
    float cull_distance=250.0f;
    std::array<float,4> lod_distances{25,75,150,250};
    bool wind=true;
};

struct VegetationInstance {
    std::uint64_t prototype=0;
    Vec3 position{};
    float rotation=0.0f;
    float scale=1.0f;
    std::uint8_t lod=0;
};

class VegetationSystem {
public:
    std::uint64_t add_prototype(VegetationPrototype);
    void clear() noexcept;
    [[nodiscard]] std::vector<VegetationInstance> cull(const std::vector<VegetationInstance>&,
                                                        Vec3 camera, float max_distance) const;
    [[nodiscard]] std::uint8_t choose_lod(const VegetationPrototype&, float distance) const noexcept;
    [[nodiscard]] std::size_t instance_count() const noexcept { return prototypes_.size(); }
private:
    std::uint64_t next_id_=1;
    std::unordered_map<std::uint64_t,VegetationPrototype> prototypes_;
};

struct WaterSettings {
    float wave_amplitude=0.15f;
    float wave_length=4.0f;
    float wave_speed=0.8f;
    float roughness=0.08f;
    float ior=1.333f;
    float absorption=0.04f;
    float foam_threshold=0.7f;
    Color3 shallow_color{0.08f,0.32f,0.36f};
    Color3 deep_color{0.01f,0.05f,0.12f};
};

struct WaterSample { float height=0.0f; Vec3 normal{0,1,0}; float foam=0.0f; };
[[nodiscard]] WaterSample sample_water(const WaterSettings&, Vec3 position, float time) noexcept;

struct FrustumPlane { Vec3 n{}; float d=0.0f; };
struct Frustum { std::array<FrustumPlane,6> planes{}; };
[[nodiscard]] Frustum make_frustum(const Mat4& view_projection) noexcept;
[[nodiscard]] bool intersects(const Frustum&, const Bounds3&) noexcept;

struct RenderInstance {
    std::uint64_t entity_id=0;
    Bounds3 bounds{};
    std::uint64_t mesh_key=0;
    std::uint64_t material_key=0;
    std::uint8_t lod=0;
};

struct CullResult {
    std::vector<RenderInstance> visible;
    std::size_t frustum_culled=0;
};

class GpuSceneCuller {
public:
    [[nodiscard]] CullResult cull(const std::vector<RenderInstance>&, const Frustum&) const;
};

struct StreamRequest { std::uint64_t asset_id=0; float priority=0.0f; std::uint32_t generation=0; };
class AssetStreamingQueue {
public:
    void request(StreamRequest);
    [[nodiscard]] std::optional<StreamRequest> pop();
    void cancel(std::uint64_t asset_id) noexcept;
    void clear() noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return requests_.size(); }
private:
    std::vector<StreamRequest> requests_;
};

struct WorldCell { std::int32_t x=0,y=0,z=0; bool loaded=false; std::uint32_t generation=0; };
class WorldStreamingManager {
public:
    explicit WorldStreamingManager(std::int32_t cell_size=256) : cell_size_(cell_size) {}
    [[nodiscard]] WorldCell cell_for(Vec3 position) const noexcept;
    [[nodiscard]] std::vector<WorldCell> desired_cells(Vec3 position, int radius) const;
    void mark_loaded(WorldCell);
    [[nodiscard]] const std::vector<WorldCell>& loaded_cells() const noexcept { return loaded_; }
    void unload_outside(Vec3 position, int radius);
private:
    std::int32_t cell_size_=256;
    std::vector<WorldCell> loaded_;
};

struct FloatingOrigin { Vec3 origin{}; float grid=256.0f; };
class LargeWorld {
public:
    explicit LargeWorld(float origin_grid=256.0f) { origin_.grid=origin_grid; }
    [[nodiscard]] Vec3 to_local(Vec3 world) const noexcept;
    [[nodiscard]] Vec3 to_world(Vec3 local) const noexcept;
    [[nodiscard]] Vec3 recenter(Vec3 camera_world) noexcept;
    [[nodiscard]] const FloatingOrigin& origin() const noexcept { return origin_; }
private:
    FloatingOrigin origin_{};
};

struct AnimationLayer { std::uint32_t clip=0; float weight=1.0f; float time=0.0f; bool additive=false; };
class AnimationMixer {
public:
    void set_base(AnimationLayer layer) noexcept { base_=layer; }
    void add_layer(AnimationLayer layer);
    void update(float dt) noexcept;
    [[nodiscard]] float normalized_base_time() const noexcept;
    [[nodiscard]] const std::vector<AnimationLayer>& layers() const noexcept { return layers_; }
private:
    AnimationLayer base_{};
    std::vector<AnimationLayer> layers_;
};

enum class MaterialFeature : std::uint8_t { NormalMap, AmbientOcclusion, Emission, Opacity, ClearCoat, Sheen, Transmission, Subsurface };
struct MaterialFeatures {
    bool normal_map=true, ambient_occlusion=true, emission=true, opacity=false;
    bool clear_coat=false, sheen=false, transmission=false, subsurface=false;
};

struct MaterialPipelineKey {
    MaterialFeatures features{};
    bool skinned=false;
    bool transparent=false;
    [[nodiscard]] std::uint64_t hash() const noexcept;
};

struct RenderPassNode {
    std::string name;
    std::vector<std::string> reads;
    std::vector<std::string> writes;
};
class RenderGraph {
public:
    void clear() noexcept { passes_.clear(); resources_.clear(); }
    void import_resource(std::string name);
    void add_pass(RenderPassNode);
    [[nodiscard]] bool compile(std::string& error) const;
    [[nodiscard]] const std::vector<RenderPassNode>& passes() const noexcept { return passes_; }
private:
    std::vector<std::string> resources_;
    std::vector<RenderPassNode> passes_;
};

struct RenderPipelinePlan {
    RenderFeatureSet features{};
    HdrSettings hdr{};
    BloomSettings bloom{};
    ShadowSettings shadows{};
    IblSettings ibl{};
    AtmosphereSettings atmosphere{};
    PostProcessPipeline post;
    RenderGraph graph;
};

class AdvancedRenderPipeline {
public:
    RenderPipelinePlan build(const RenderFrame&, const RenderFeatureSet& features={}) const;
};

} // namespace exgine

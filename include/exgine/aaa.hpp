#pragma once

#include "exgine/advanced_physics.hpp"
#include "exgine/frontier.hpp"
#include "exgine/high_fidelity.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace exgine {

// Phase 61: physically based indirect-lighting cache.
struct IrradianceProbe { Vec3 position{}; Color3 irradiance{}; float radius=8.0f; };
class IrradianceProbeGrid {
public:
    IrradianceProbeGrid(std::uint32_t nx=8,std::uint32_t ny=2,std::uint32_t nz=8,float spacing=8.0f);
    bool generate(Vec3 origin,const HighQualityLighting& lighting);
    [[nodiscard]] Color3 sample(Vec3 position) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return probes_.size(); }
private:
    std::uint32_t nx_,ny_,nz_; float spacing_; std::vector<IrradianceProbe> probes_;
};

// Phase 62: screen-space/planar reflection policy and ray query budgets.
struct ReflectionQuery { Vec3 origin{},direction{0,0,1}; float max_distance=100.0f; };
struct ReflectionHit { bool hit=false; float distance=0; Vec3 normal{}; };
[[nodiscard]] ReflectionHit trace_reflection_plane(const ReflectionQuery& query,Vec3 plane_point,Vec3 plane_normal) noexcept;

// Phase 63: volumetric fog/scattering evaluation.
struct VolumetricFog { float density=0.01f,height_falloff=0.1f,scattering=0.8f,extinction=1.0f; };
[[nodiscard]] float fog_transmittance(const VolumetricFog& fog,float distance,float height) noexcept;

// Phase 64: HDR/tone mapping/exposure pipeline.
struct HdrColor { float r=0,g=0,b=0; };
[[nodiscard]] HdrColor aces_tonemap(HdrColor color,float exposure) noexcept;

// Phase 65: terrain material sampling and biome blending.
struct TerrainMaterialWeights { float grass=0,sand=0,rock=0,snow=0,mud=0; };
[[nodiscard]] TerrainMaterialWeights blend_terrain(float height,float slope,float moisture,float snow_line,float rock_line) noexcept;

// Phase 66: advanced tire response with combined slip.
struct CombinedSlipInput { float slip_ratio=0,slip_angle=0,normal_load=0,mu=1.0f; };
struct TireForce { float longitudinal=0,lateral=0; };
[[nodiscard]] TireForce combined_slip_force(const CombinedSlipInput& input) noexcept;

// Phase 67: character IK utility.
struct IkResult { bool solved=false; Vec3 foot{}; };
[[nodiscard]] IkResult solve_ground_foot(Vec3 hip,Vec3 target,float leg_length,float ground_y) noexcept;

// Phase 68: deterministic utility-AI score selection.
struct AiActionScore { std::string action; float score=0; };
class UtilityAiSelector {
public:
    void add(AiActionScore action);
    [[nodiscard]] std::string choose() const;
    void clear() noexcept { actions_.clear(); }
private:
    std::vector<AiActionScore> actions_;
};

// Phase 69: audio propagation model.
struct AcousticPath { float distance=0,occlusion=0,reverb=0; };
[[nodiscard]] AcousticPath evaluate_acoustics(Vec3 listener,Vec3 source,float room_absorption) noexcept;

// Phase 70: end-to-end AAA gate metrics.
struct AaaAcceptanceMetrics {
    bool indirect_lighting=false;
    bool reflections=false;
    bool volumetrics=false;
    bool hdr=false;
    bool terrain_materials=false;
    bool vehicle_tires=false;
    bool character_ik=false;
    bool ai=false;
    bool audio=false;
    bool runtime=false;
    [[nodiscard]] bool ready() const noexcept { return indirect_lighting&&reflections&&volumetrics&&hdr&&terrain_materials&&vehicle_tires&&character_ik&&ai&&audio&&runtime; }
};

} // namespace exgine

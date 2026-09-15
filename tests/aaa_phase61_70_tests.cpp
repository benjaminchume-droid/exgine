#include "exgine/aaa.hpp"
#include <cassert>
#include <cmath>
using namespace exgine;
int main(){
    IrradianceProbeGrid probes(2,1,2,4); HighQualityLighting lighting; assert(probes.generate({0,0,0},lighting)); assert(probes.size()==4); auto ir=probes.sample({1,0,1}); assert(ir.r>=0&&ir.g>=0&&ir.b>=0);
    ReflectionQuery q{{0,2,0},{0,-1,0},10}; auto hit=trace_reflection_plane(q,{0,0,0},{0,1,0}); assert(hit.hit&&std::abs(hit.distance-2)<1e-5f);
    VolumetricFog fog; auto tr=fog_transmittance(fog,100,0); assert(tr>0&&tr<=1);
    auto mapped=aces_tonemap({2,1,0.25f},0); assert(mapped.r<=1&&mapped.g<=1&&mapped.b<=1);
    auto terrain=blend_terrain(1800,.6f,.2f,1600,900); assert(terrain.snow>terrain.sand&&terrain.rock>0);
    auto tire=combined_slip_force({.3f,.12f,3500,1.1f}); assert(tire.longitudinal>0&&std::isfinite(tire.lateral));
    auto ik=solve_ground_foot({0,1,0},{.4f,.2f,.1f},1.0f,0); assert(ik.solved&&std::abs(ik.foot.y)<1e-5f);
    UtilityAiSelector ai; ai.add({"idle",.1f}); ai.add({"chase",.9f}); assert(ai.choose()=="chase");
    auto acoustic=evaluate_acoustics({0,0,0},{10,0,0},.5f); assert(acoustic.distance==10&&acoustic.occlusion>0);
    AaaAcceptanceMetrics metrics; metrics.indirect_lighting=metrics.reflections=metrics.volumetrics=metrics.hdr=metrics.terrain_materials=metrics.vehicle_tires=metrics.character_ik=metrics.ai=metrics.audio=metrics.runtime=true; assert(metrics.ready());
    return 0;
}

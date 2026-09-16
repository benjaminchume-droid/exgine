#pragma once
#include "exgine/gpu.hpp"
#include "exgine/gles_tracks.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace exgine {
struct ReflectionProbeGpuData { GlesCubeResource environment{}; GlesCubeResource irradiance{}; GlesCubeResource specular{}; GlUInt depth_rbo=0; std::uint64_t last_capture_frame=0; bool valid=false; };
class ReflectionProbeRegistry {
public:
    std::uint64_t create(Vec3 position,float radius,float weight);
    bool destroy(std::uint64_t id,OpenGLESApi&) noexcept;
    ReflectionProbe* find(std::uint64_t id) noexcept;
    const ReflectionProbe* find(std::uint64_t id) const noexcept;
    ReflectionProbeGpuData* gpu_find(std::uint64_t id) noexcept;
    const ReflectionProbeGpuData* gpu_find(std::uint64_t id) const noexcept;
    bool set_gpu(std::uint64_t,const ReflectionProbeGpuData&) noexcept;
    const ReflectionProbe* select(Vec3 position) const noexcept;
    std::vector<const ReflectionProbe*> blend(Vec3 position,std::size_t max_probes=4) const;
    void clear(OpenGLESApi&) noexcept;
private:
    struct Record { ReflectionProbe meta{}; ReflectionProbeGpuData gpu{}; };
    std::uint64_t next_id_=1;
    std::vector<Record> probes_;
};
GpuSubmitResult submit_production_gles_complete(OpenGLESRenderer&,const RenderFrame&); void release_production_gles_complete(OpenGLESRenderer&) noexcept; bool capture_reflection_probe(OpenGLESRenderer&,const RenderFrame&,Vec3,std::uint32_t,GlesCubeResource&,std::string&);
}
#pragma once
#include "exgine/gpu.hpp"
#include "exgine/gles_tracks.hpp"
#include <cstdint>
#include <string>
#include <vector>
namespace exgine {
struct ReflectionProbe { std::uint64_t id=0; Vec3 position{}; float radius=10.0f; float weight=1.0f; GlesCubeResource environment{}; GlesCubeResource irradiance{}; GlesCubeResource specular{}; GlUInt depth_rbo=0; std::uint64_t last_capture_frame=0; bool valid=false; };
class ReflectionProbeRegistry { public: std::uint64_t create(Vec3 position,float radius,float weight) ; bool destroy(std::uint64_t id,OpenGLESApi& api) noexcept; ReflectionProbe* find(std::uint64_t id) noexcept; const ReflectionProbe* find(std::uint64_t id) const noexcept; const ReflectionProbe* select(Vec3 position) const noexcept; std::vector<const ReflectionProbe*> blend(Vec3 position,std::size_t max_probes=4) const; void clear(OpenGLESApi& api) noexcept; private: std::uint64_t next_id_=1; std::vector<ReflectionProbe> probes_; };
GpuSubmitResult submit_production_gles_complete(OpenGLESRenderer&,const RenderFrame&); void release_production_gles_complete(OpenGLESRenderer&) noexcept; bool capture_reflection_probe(OpenGLESRenderer&,const RenderFrame&,Vec3,std::uint32_t,GlesCubeResource&,std::string&);
}
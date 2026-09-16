#pragma once
#include "exgine/gles_ibl.hpp"
#include "exgine/gles_probe_capture.hpp"
#include "exgine/gles_probe_material.hpp"
#include <cstdint>
#include <string>
namespace exgine {
class ReflectionProbeGpuRuntime {
public:
    explicit ReflectionProbeGpuRuntime(OpenGLESRenderer&) noexcept;
    ~ReflectionProbeGpuRuntime();
    ReflectionProbeGpuRuntime(const ReflectionProbeGpuRuntime&)=delete;
    ReflectionProbeGpuRuntime& operator=(const ReflectionProbeGpuRuntime&)=delete;
    ReflectionProbeRegistry& registry() noexcept{return registry_;}
    bool capture_and_build(std::uint64_t,const RenderFrame&,std::uint32_t,std::string&) noexcept;
    ProbeMaterialSet material_set(Vec3,std::uint32_t=4) const noexcept;
    void clear() noexcept;
private:
    OpenGLESRenderer& renderer_;
    ReflectionProbeRegistry registry_;
    std::unordered_map<std::uint64_t,ReflectionProbeCaptureTarget> captures_;
};
}
#pragma once
#include "exgine/gles_tracks.hpp"
#include "exgine/gpu.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace exgine {
struct GpuIblResources { GlesCubeResource environment{}; GlesCubeResource irradiance{}; GlesCubeResource specular{}; bool valid=false; };
struct EnvironmentFacePixels { const void* data=nullptr; GlInt internal_format=0; GlEnum format=0; GlEnum type=0; std::uint32_t size=0; };
bool ingest_environment_cubemap(OpenGLESApi&,const std::array<EnvironmentFacePixels,6>&,std::uint32_t,GlesCubeResource&,std::string&) noexcept;
bool gpu_convolve_irradiance(OpenGLESRenderer&,const GlesCubeResource&,std::uint32_t,GlesCubeResource&,std::string&) noexcept;
bool gpu_prefilter_ggx(OpenGLESRenderer&,const GlesCubeResource&,std::uint32_t,std::uint32_t,GlesCubeResource&,std::string&) noexcept;
bool build_gpu_ibl(OpenGLESRenderer&,const GlesCubeResource&,std::uint32_t,std::uint32_t,GpuIblResources&,std::string&) noexcept;
const char* gpu_ibl_vertex_shader() noexcept;
const char* gpu_irradiance_fragment_shader() noexcept;
const char* gpu_ggx_prefilter_fragment_shader() noexcept;
}
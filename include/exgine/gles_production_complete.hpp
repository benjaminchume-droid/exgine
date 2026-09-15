#pragma once
#include "exgine/gpu.hpp"
#include "exgine/gles_tracks.hpp"
namespace exgine { GpuSubmitResult submit_production_gles_complete(OpenGLESRenderer&,const RenderFrame&); void release_production_gles_complete(OpenGLESRenderer&) noexcept; bool capture_reflection_probe(OpenGLESRenderer&,const RenderFrame&,Vec3,std::uint32_t,GlesCubeResource&,std::string&); }

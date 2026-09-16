#pragma once
#include "exgine/gles_tracks.hpp"
#include "exgine/gpu.hpp"
#include <cstdint>
#include <functional>
#include <string>
namespace exgine {
struct ReflectionProbeCaptureTarget { GlUInt fbo=0,color_cube=0,depth_rbo=0; std::uint32_t size=0; };
using ProbeFaceRenderer=std::function<bool(OpenGLESRenderer&,const RenderFrame&,const Mat4&,const Mat4&,std::uint32_t,std::string&)>;
bool create_probe_capture_target(OpenGLESApi&,std::uint32_t,ReflectionProbeCaptureTarget&,std::string&) noexcept;
void destroy_probe_capture_target(OpenGLESApi&,ReflectionProbeCaptureTarget&) noexcept;
bool capture_probe_six_faces(OpenGLESRenderer&,const RenderFrame&,Vec3,std::uint32_t,ReflectionProbeCaptureTarget&,const ProbeFaceRenderer&,std::string&) noexcept;
}
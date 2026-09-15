#pragma once
#include "exgine/gpu.hpp"

namespace exgine {

// Renderer-owned advanced GLES state. The implementation keeps transient render
// targets, post-processing resources, shadow data and environment resources alive
// across frames and destroys them with the renderer/context lifecycle.
GpuSubmitResult submit_production_gles(OpenGLESRenderer&, const RenderFrame&);
void release_production_gles(OpenGLESRenderer&) noexcept;

} // namespace exgine

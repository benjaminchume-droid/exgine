#include "exgine/gpu.hpp"
#include "exgine/advanced_render.hpp"

namespace exgine {

GpuSubmitResult render_world_systems(OpenGLESApi&, const RenderFrame&, const RenderPipelinePlan&) {
    // The advanced backend no longer creates placeholder terrain/vegetation.
    // World geometry is authored/streamed into RenderFrame::draws and is
    // consumed by the normal GPU submission path. This seam remains for
    // world-specialized passes such as water and future probe capture.
    return GpuSubmitResult{true, 0, 0, 0, {}};
}

} // namespace exgine

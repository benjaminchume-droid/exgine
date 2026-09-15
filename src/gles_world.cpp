#include "exgine/gpu.hpp"
#include "exgine/advanced_render.hpp"

namespace exgine {

GpuSubmitResult render_world_systems(OpenGLESApi&, const RenderFrame&, const AdvancedRenderPipeline&) {
    // World rendering is now asset-driven. Terrain, vegetation and water are
    // submitted as ordinary RenderDrawCall records produced by the scene/asset
    // streaming layer, so this hook must never synthesize meshes or instances.
    // Returning success preserves the feature-plan seam while the normal GPU
    // renderer consumes arbitrary streamed MeshAssembly/SkinnedMesh resources.
    return GpuSubmitResult{true, 0, 0, 0, {}};
}

} // namespace exgine

# EXGINE Phase 17 — GPU Skeletal Skinning & Animation Rendering

Phase 17 connects the Phase 16 animation runtime to the Phase 15 OpenGL ES renderer.

## Delivered

- Runtime skinned-mesh assets attached to entities with explicit material slots.
- RenderDrawCall support for animated geometry with self-contained bone matrices.
- RenderConfig limit for per-draw bone palettes, capped at the shader capacity of 128 bones.
- OpenGL ES skeletal PBR shader with four bone IDs and four weights per vertex.
- GPU upload of evaluated bone matrices through `glUniformMatrix4fv`.
- Separate static and skinned mobile PBR programs so static rendering remains unchanged.
- Animated draw ordering uses the same opaque/transparent sorting as static geometry.
- Runtime animation controllers continue to advance from `Runtime::update()`.
- Phase 17 regression coverage verifies shader contracts, runtime attachment, render-frame palette generation and GPU submission.

## Data flow

```text
Skeleton + AnimationClip
          |
          v
 AnimationController
          |
          v
   SkeletonPose
          |
          v
   RenderFrame
   + bone_palette
          |
          v
 OpenGL ES skinned vertex shader
          |
          v
 Animated character pixels
```

## Correctness boundaries

The renderer uploads evaluated model-space bone transforms; it does not own animation state. A render frame therefore remains a deterministic snapshot that can be validated before GPU submission.

Animated geometry is conservatively kept visible rather than relying on a potentially stale bind-pose frustum bound. This avoids incorrectly culling a posed character outside its undeformed mesh bounds.

The shader supports four influences per vertex and up to 128 bones per draw. Higher influence counts or larger skeleton palettes require a future asset/runtime contract expansion rather than silent truncation.

## Deferred work

Android EGL/window/swapchain integration remains platform work. Future rendering phases can move palette upload from uniform arrays to persistent GPU buffers/SSBOs, add dual-quaternion or matrix skinning options, GPU instancing for crowds, animation LOD, and compute-based skinning where appropriate.

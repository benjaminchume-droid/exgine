# Phase 7 — Renderer & Shaders

## Goal

Phase 7 establishes the renderer contract between EXGINE runtime state and platform GPU implementations. It is backend-neutral at the core and has a deterministic headless submission path so the render pipeline can be verified without requiring a graphics driver in CI.

## Implemented

- Column-major 4x4 matrix utilities with model, view, perspective, and orthographic projection generation.
- Runtime mesh bounds generation and transformed world-space AABBs.
- Deterministic six-plane frustum culling.
- `RenderFrame` snapshots containing camera matrices, environment/fog/exposure settings, active lights, and validated draw calls.
- Render material bindings carrying material factors and generated texture resources with shared ownership.
- Deterministic entity ordering for render-frame construction.
- Opaque/transparent pass classification from material opacity.
- Configurable viewport, HDR policy, light budget, and culling policy.
- Headless renderer submission and strict frame validation.
- Built-in GLSL ES 3.10 PBR and unlit shader programs.
- PBR shader inputs for camera transforms, material factors, direct lights, ambient contribution, emission, tone mapping, and gamma correction.

## Connected data flow

```text
EXGINE source
    -> Compiler
    -> IR
    -> Runtime
       -> Entities + Scene
       -> Materials + Texture resources
       -> Lighting + Camera
                 |
              Renderer
                 |
            RenderFrame
                 |
       Headless / GPU backend
                 |
               Shaders
```

## Backend boundary

The core renderer deliberately does not create OpenGL/Vulkan/Metal/Direct3D objects. Those implementations belong in platform/backend modules and consume the stable `RenderFrame` contract. The current `Headless` backend validates and accepts a frame; selecting a GPU backend without a platform implementation is an explicit error rather than a false success.

## Validation contract

A render frame is rejected when its viewport, matrices, camera, lights, draw geometry, transformed bounds, or material/texture resources are invalid. Missing material resources are treated as content errors rather than silently drawing with an undocumented fallback.

## Shader contract

The bundled shaders use GLSL ES 3.10 and are stored as normal source strings so higher layers can compile them through the eventual platform shader compiler. Phase 7 provides the source and binding layout; GPU compilation, shader binaries, descriptor allocation, render targets, shadow maps, post-processing, and platform presentation remain backend responsibilities.

## Checkpoint

Phase 7 is complete when the full repository builds with warnings enabled, the complete test suite passes, renderer frame construction is covered by tests, shader contracts are present and valid, and the architecture contains no parallel renderer path.

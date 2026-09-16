# EXGINE Integration Audit — 2026-09-16

## Executive result

The Android renderer is viable. The failure observed in First Light was not evidence that EXGINE cannot render a large 3D game. The codebase had an integration gap between world/asset systems and the final GPU frame, plus an EXWORLD fallback ground that could occlude the procedural terrain.

The repository contains real GLES/EGL presentation, mesh/texture upload, PBR/skinned shader paths, procedural world generation, animation, audio, physics, and streaming. Several of these systems were previously only connected by tests or demo code rather than one authoritative runtime path.

## Batch findings

### 1. Android/EGL/GLES
- EGL window surface and ES 3.1 context are real.
- `AndroidEglPresenter::present()` reaches `OpenGLESRenderer::submit()` and swaps the surface.
- CI can validate the software/native build path but cannot certify a physical device's GPU output/performance.

### 2. Runtime / scene activation
- `.exg` project manifests parse correctly and startup scenes are activated through the `GameRuntime` scene loader.
- Runtime hydration is enabled during scene activation.
- Scene nodes are created for instantiated entities, which is required by the renderer.

### 3. Procedural world
- `ProceduralWorld` generates terrain and water meshes.
- `OpenWorldStreamer` generates and unloads chunks.
- Prior to this audit, streamed chunks were data owned by the world system but were not converted into normal renderable runtime entities.
- **Fixed:** `WorldRenderBridge` now binds streamed terrain/water to the same Entity -> SceneGraph -> RenderFrame path used by ordinary assets.

### 4. EXGINE GPU residency
- Persistent GPU mesh residency exists as a separate subsystem.
- It is intentionally optional and does not own runtime scene nodes.
- The bridge now avoids recreating mesh assemblies every frame; chunk generation IDs control reattachment.

### 5. Materials/textures
- Procedural PBR material texture generation exists.
- GPU texture upload and mip generation exist.
- `Runtime::define_material()` rebuilds generated material resources, therefore callers must not invoke it every frame.
- **Fixed:** the world render bridge only defines terrain/water resources when missing.

### 6. EXWORLD baked assets
- The EXWORLD package registry previously accepted missing package files and silently retained empty bodies.
- The package index also referenced `character.exg` and `vehicle_sedan.exg` that were not shipped.
- **Fixed:** package loading now fails fast on missing/malformed packages, and the index lists only packages actually present.
- **Fixed:** EXWORLD visual bootstrap now consumes OBJ multipart geometry embedded inside baked `.exg` packages before using procedural fallbacks.

### 7. First Light visual fallback
- EXWORLD created an 800 m flat asphalt plate at approximately y=0.
- Procedural terrain can legitimately be above or below that height, so the fallback plate could visually occlude the generated terrain and produce the observed flat gray world.
- **Fixed:** once streamed terrain is active, the legacy flat ground plates are disabled and streamed terrain becomes the authoritative world surface.

### 8. Characters/animation
- Runtime character generation and EXAnimation exist.
- Renderer supports skinned meshes with up to four influences and a 128-bone palette.
- AnimationDriver deliberately avoids attaching a skeleton by itself because the renderer rejects incomplete skinned payloads. A future character asset pipeline must attach skeleton + skinned mesh + pose atomically.

### 9. UI/input
- Android input reaches EXWORLD and virtual-control geometry exists.
- The current UI path is still a 3D marker approach rather than a dedicated GPU overlay/render-target UI pass. This is an integration limitation, not an Android input failure.

### 10. Water
- Water geometry is generated procedurally and is now rendered through the world bridge.
- The existing material model describes water, but the renderer does not yet provide a complete dedicated water pass with time-dependent displacement/reflection/refraction/foam. That is a real remaining rendering-system task.

### 11. Physics
- CPU rigid-body physics is integrated with runtime/gameplay.
- Terrain/world collision is not yet automatically derived from streamed terrain meshes. This must be added for a fully physical open world; otherwise a visual terrain surface and physical ground can diverge.

### 12. Open-world scalability
- Streaming and LOD metadata exist.
- Render submission is still fundamentally entity/draw-call based; vegetation instancing, GPU-driven visibility, terrain clipmaps, material virtualization and full render-graph passes are still required for very large high-detail worlds.

### 13. Build/codebase hygiene
- `src/runtime.cpp` is a stale alternate implementation and is not the authoritative source; `src/runtime_clean.cpp` is the compiled runtime implementation. It should be removed after the current branch is certified to prevent source drift.
- Phase documentation often uses the phrase `IMPLEMENTED FOUNDATION`; that must not be interpreted as device-certified AAA rendering.

## Target content architecture

The intended model is:

```text
Authoring / asset import
        |
        v
EXG content package
  meshes / textures / materials / skeletons / animation / collision
        |
        v
Runtime asset registry
        |
        +---- authored scene base
        |
        +---- procedural continuation
        |
        +---- streaming / LOD / simulation
        |
        v
Entity + SceneGraph
        |
        v
RenderFrame
        |
        v
RenderGraph / GPU residency
        |
        v
OpenGL ES / other backend
```

This permits a game to ship high-fidelity authored 3D assets in EXG packages without scraping or downloading external assets at runtime, while still allowing procedural systems to continue, transform, stream and recombine those assets.

## Acceptance rule

A phase is not considered visually complete merely because its C++ API and unit tests exist. The integrated First Light Android build must produce real visible geometry, materials, characters, controls, water and lighting on a physical device, with runtime telemetry proving the asset -> scene -> render -> GPU path.

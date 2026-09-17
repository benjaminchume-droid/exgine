# EXGINE codebase audit — 2026-09-17

This is an engineering audit, not a demo checklist. A subsystem is only considered complete when data can traverse the real runtime path and reach the target backend.

## Batch 1 — Build and module topology

### Findings
- EXGINE has a broad subsystem surface, but several phases were implemented as adjacent APIs rather than one verified end-to-end path.
- Runtime ownership is split across `runtime_clean.cpp` and focused extension units such as `runtime_exaudio.cpp`; this is currently linkable, but the boundary needs to remain explicit.
- EXWORLD must own game content. EXGINE owns engine primitives/runtime.

### Repair
- First Light game content was moved to EXWORLD.
- Android rendering diagnostics were added.
- The build remains the gate for every later batch.

## Batch 2 — Asset/package model

EXGINE already contains a generic binary asset package primitive (`AssetRecord`, `AssetDatabase`, `pack_assets`, `unpack_assets`). The package uses an `XGPK` magic and is suitable as the foundation for an engine-native `.exg` compiled container. The problem is not absence of a package primitive; the problem is that EXWORLD's current baked registry is a separate text abstraction and does not yet feed the engine asset database directly.

### Required architecture

`source assets -> importer -> compiler/baker -> XGPK/.exg container -> runtime asset database -> resource residency -> scene instantiation -> renderer`

The runtime must not depend on regenerating authored meshes at startup.

## Batch 3 — Texture/material contract

### Finding
The old GLES material upload wrote `u_base_roughness` and `u_metal_specular` with scalar uniform calls even though the mobile shaders declared them as `vec4`. That is an OpenGL ES uniform type mismatch and can generate `GL_INVALID_OPERATION`; base-color data also was not being explicitly sent in the untextured path.

### Repair
- Mobile shaders now use a consistent `vec4` material contract.
- GLES now calls `Uniform4f` for those uniforms.
- Optional PBR maps are accepted by `TextureSet`.
- Neutral GPU fallback maps are supplied for missing roughness/metallic/normal/AO/emission/opacity maps.

## Batch 4 — GLES GPU resource path

### Findings
- Mesh and texture GPU residency existed, but error reporting was too weak.
- Android did not expose `glGetError` through the engine API.
- A single GPU failure could leave the user looking at a clear/fallback frame with no useful diagnosis.

### Repair
- Added `glGetError` loading.
- GLES submission reports the last GPU error.
- Mesh and texture cache paths remain persistent across frames.
- Android presentation now checks render submission before swapping buffers.

## Batch 5 — World/terrain/water

### Finding
World streaming and `WorldRenderBridge` already create render entities for streamed terrain/water, but water had no GPU-side animation path.

### Repair
- Added time/water uniforms to the mobile vertex path.
- Water materials now receive animated procedural displacement.
- The existing world-streamed water transform remains available for low-frequency surface motion.

## Batch 6 — Character/animation

### Finding
EXWORLD intentionally avoids attaching a skeleton when a skinned mesh is absent because the renderer treats an incomplete skinned entity as invalid. That defensive behavior prevents a bad entity from corrupting the frame, but it also proves the character asset path is not yet a complete authored skinned-asset pipeline.

### Required completion
`skinned mesh + skeleton + inverse bind data + animation clip + pose palette -> GPU skinning`

The fallback procedural character is useful for engine tests but is not the final content pipeline.

## Batch 7 — Scene/runtime bridge

### Finding
The `.exg` project parser can describe a scene and assets, but a manifest declaration is not itself a runtime GPU resource. EXWORLD currently hydrates some baked text packages separately.

### Required completion
One authoritative loader must resolve package asset IDs, dependencies, scene nodes, materials and geometry before the first render frame.

## Batch 8 — Render graph

Current GLES submission is a forward/mobile PBR path. It is sufficient as a functional vertical slice but not yet an AAA renderer.

### Required expansion
- depth prepass
- shadow maps/cascades
- opaque/transparent separation
- material batching
- GPU culling
- LOD selection
- terrain material blending
- vegetation instancing
- water/refraction/reflection
- particles/VFX
- post processing/tonemapping
- temporal reconstruction where supported

## Batch 9 — Mobile gameplay/input

Android input reaches EXGINE, but a complete virtual-control HUD and screen-space overlay path still needs to be connected to gameplay and camera controls.

## Batch 10 — Open-world scale

The engine can use a finite authored base plus procedural continuation. GTA-scale world size is therefore a content/streaming/memory problem rather than a requirement to load the entire world into RAM. The correct model is chunked authored content plus deterministic procedural expansion.

## Acceptance rule

The first release candidate is not "APK builds". It is:

1. APK installs.
2. `.exg` package opens.
3. Authored mesh is loaded from the package.
4. Authored texture reaches GPU residency.
5. Scene instantiates.
6. Camera sees geometry.
7. Materials render.
8. Water animates.
9. Character renders and animates.
10. Touch controls move the player/camera.
11. World chunks stream without invalidating the frame.
12. Diagnostics expose asset/draw/GPU counts.

Only after this vertical slice is green should the engine scale outward toward the full AAA feature set.

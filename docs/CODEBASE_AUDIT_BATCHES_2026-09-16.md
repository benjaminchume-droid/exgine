# EXGINE / EXWORLD Codebase Audit — Batch Plan and Findings

Date: 2026-09-16

This audit treats a subsystem as complete only when its data can travel through the production runtime into the Android GPU frame. API existence and unit tests are not sufficient.

## Architecture decision

EXGINE is the general-purpose engine. EXWORLD is the game/content layer. Authored 3D content is shipped in engine-native `.exg` packages; runtime procedural systems may continue, transform, stream, combine and instantiate those assets without downloading or scraping external world data.

The target path is:

`authoring -> EXG bake/package -> package registry -> scene/entity graph -> streaming/LOD -> RenderFrame -> GPU residency -> GLES/Vulkan -> device`

## Batch status

| Batch | Area | Current finding | State |
|---|---|---|---|
| 1 | Core/runtime lifecycle | Runtime, PlayableGame and GameSession are connected | audited |
| 2 | EXG parser/compiler | Project and scene parsing are connected | audited |
| 3 | EXG asset/package format | Package concept exists; needs engine-level binary/container standardization | remaining |
| 4 | Asset import | OBJ and glTF paths exist | audited |
| 5 | Runtime asset registry | Imported mesh attachment exists; broader package-to-resource binding needs completion | remaining |
| 6 | Scene graph | Entity -> SceneGraph transforms are connected | audited |
| 7 | RenderFrame construction | Works but is too fail-fast for malformed optional entities | remaining |
| 8 | GLES GPU residency | Mesh and texture upload are real | audited |
| 9 | Materials/textures | Procedural PBR texture generation and GPU upload exist | audited |
| 10 | Camera/projection | EXWORLD gameplay camera convention was opposite the renderer convention | **fixed in EXWORLD** |
| 11 | Procedural terrain | Terrain mesh generation exists | audited |
| 12 | World streaming | OpenWorldStreamer exists but streamed chunks were not automatically visible in the game frame | **fixed by WorldRenderBridge integration** |
| 13 | Water | Water geometry exists; dedicated animated water shading is missing | remaining |
| 14 | Vegetation | Vegetation instances are generated but need render instancing/batching | remaining |
| 15 | Buildings | Procedural building geometry/collision exists | audited |
| 16 | Vehicles | Procedural vehicle geometry and dynamics exist | audited |
| 17 | Characters | Procedural character geometry exists | audited |
| 18 | Skinning/animation | GPU skinning exists; attachment is not yet atomic across mesh/skeleton/pose | remaining |
| 19 | Lighting | Light data reaches mobile PBR path | audited |
| 20 | Shadows | Metadata/API exists; full mobile shadow-map render pass is still required | remaining |
| 21 | PBR/environment | Mobile PBR path exists; image-based lighting/environment probes are incomplete | remaining |
| 22 | VFX | Particle/environment systems exist but need production GPU integration | remaining |
| 23 | EXSound | Runtime procedural audio path exists | audited |
| 24 | Physics/gameplay | CPU physics/gameplay integration exists | audited |
| 25 | Terrain collision | Visual terrain is not yet automatically the authoritative physical surface | remaining |
| 26 | Mobile input/UI | Android events reach EXWORLD; UI is currently world-space geometry rather than a dedicated overlay pass | remaining |
| 27 | Save/restore | Production save path exists | audited |
| 28 | APK/package pipeline | Installable signed debug artifact path exists; physical-device GPU certification remains required | in verification |

## Confirmed foundational defects

1. **Camera convention mismatch.** EXWORLD's gameplay camera uses +X/+Z forward semantics while EXGINE's renderer uses conventional -Z camera space. This can place the intended scene behind the camera. The EXWORLD camera now converts its convention at the engine boundary.
2. **Procedural chunks were not automatically renderable.** `WorldRenderBridge` existed in EXGINE but EXWORLD did not invoke it. EXWORLD's camera now synchronizes the bridge with the runtime focus, putting streamed terrain/water through the ordinary entity/scene/render path.
3. **Legacy flat showcase surfaces could occlude the procedural world.** The bridge disables those fallback plates after streamed world activation.
4. **Streamed render entity lifetime leaked SceneGraph nodes.** `WorldRenderBridge` now destroys the associated SceneGraph node before destroying its EntityRegistry entry.
5. **Engine documentation overstated completion.** Phase APIs/tests are not equivalent to device-certified AAA rendering. This audit uses end-to-end acceptance instead.

## Remaining high-impact rendering work

- Render graph with explicit depth, shadow, opaque, water, transparent, vegetation, VFX and UI passes.
- Dedicated water shader with time-dependent displacement, depth fade, reflection/refraction approximation and foam.
- GPU shadow maps and cascaded/partitioned shadows suitable for open-world scale.
- Vegetation instancing and impostor/LOD path.
- Terrain clipmap/LOD rendering instead of one ordinary draw per streamed chunk.
- GPU-driven frustum/occlusion/LOD selection for very large scenes.
- Atomic skinned asset binding: skeleton + skinned mesh + animation pose + materials.
- Authored texture/material maps inside `.exg` packages rather than relying primarily on runtime procedural material generation.
- Automatic collision generation/streaming from the same terrain chunks used for visual rendering.
- Dedicated screen-space UI/input rendering instead of world-space HUD geometry.
- Runtime telemetry reporting asset counts, draw counts, GPU residency, shader failures and stream state on-device.

## Definition of GTA/Asphalt-level feasibility

The engine does not need GTA/Asphalt-specific code. It needs general primitives capable of supporting their requirements: high-fidelity authored assets, streaming, LOD, batching/instancing, physically based materials, shadows, animation, vehicles, physics, audio, VFX, UI and scalable world partitioning.

The `.exg` package is the shipped base. Procedural generation is the continuation mechanism. The runtime must never depend on scraping external content to complete the world.

A physical Android acceptance test is required before claiming the visual target is achieved.

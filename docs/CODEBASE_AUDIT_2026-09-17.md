# EXGINE codebase audit — 2026-09-17

This is an engineering audit, not a demo checklist. A subsystem is only considered complete when data can traverse the real runtime path and reach the target backend.

## Batch 1 — Build and module topology

EXGINE has a broad subsystem surface, but several phases were implemented as adjacent APIs rather than one verified end-to-end path. Runtime ownership is split across focused extension units. EXWORLD owns game content; EXGINE owns engine primitives/runtime.

**Repair:** First Light game content was moved to EXWORLD; Android presentation now checks renderer submission; the build remains the gate for later batches.

## Batch 2 — Asset/package model

EXGINE already contains a dependency-aware binary asset container (`XGPK`, version 1). The `.exg` extension is now explicitly documented as the compiled game-content form, and `tools/exgpack` was added as a real package compiler. This is the correct foundation for authored meshes/textures/materials/scenes plus procedural continuation.

**Remaining integration:** EXWORLD still has a separate text baked-package registry. It must be migrated to the EXGINE `AssetDatabase` so one package system is authoritative at runtime.

## Batch 3 — Texture/material contract

**Finding:** the old GLES path wrote `u_base_roughness` and `u_metal_specular` with scalar uniform calls even though mobile shaders declared them as `vec4`. That is an OpenGL ES uniform type mismatch. The old untextured path also did not explicitly upload the base material color.

**Repair:** mobile shaders and GLES now use a consistent `vec4` material contract. Optional PBR maps are accepted and neutral fallback maps are generated for missing maps.

## Batch 4 — GLES GPU resource path

**Finding:** mesh/texture GPU residency existed, but the renderer had weak error reporting and there are currently two mesh residency implementations (`OpenGLESRenderer` cache and `OpenGLESAssetResidency`). Keeping both risks divergent lifetime/budget behavior.

**Repair:** `glGetError` is loaded and render submission reports GPU failures. The next resource pass will unify the two residency paths behind one budgeted cache.

## Batch 5 — World/terrain/water

World streaming and `WorldRenderBridge` already create generic render entities for streamed terrain/water. Water had no GPU-side animation path.

**Repair:** mobile vertex shaders now receive time/water uniforms and water materials get procedural displacement. This is the first real animated-water path; reflections/refraction/foam are still later render-graph work.

## Batch 6 — Character/animation

EXWORLD intentionally avoids attaching a skeleton when a skinned mesh is absent because the renderer rejects incomplete skinned entities. This prevents frame corruption but also proves the authored character pipeline is incomplete.

**Required completion:** `skinned mesh + skeleton + inverse bind data + animation clip + pose palette -> GPU skinning`.

## Batch 7 — Scene/runtime bridge

The `.exg` project language can describe scenes/assets, but a manifest is metadata, not GPU data. EXWORLD's current text hydration path is separate from EXGINE's binary AssetDatabase.

**Required completion:** one authoritative package loader must resolve asset IDs, dependencies, scene nodes, materials and geometry before first frame.

## Batch 8 — Render graph

The Android path is currently a forward/mobile PBR renderer. It is a functional vertical slice, not yet an AAA renderer.

**Required expansion:** depth prepass, shadow maps/cascades, opaque/transparent separation, batching, GPU culling, LOD, terrain material blending, vegetation instancing, water reflection/refraction, particles/VFX and post-processing/tonemapping.

## Batch 9 — Mobile gameplay/input

Android input reaches EXGINE, but the virtual joystick/camera/action UI still needs a true screen-space overlay path connected to gameplay.

## Batch 10 — Camera contract

A concrete EXWORLD bug was found outside the renderer: the game camera converted gameplay yaw using `PI - yaw`. With EXGINE's `-Z` camera convention, this mirrored horizontal camera orientation as yaw changed and could leave the player/scene outside the view. EXWORLD now uses `yaw + PI`, which maps gameplay yaw 0 (+Z) to EXGINE's equivalent camera rotation and preserves direction across the full yaw range.

## Batch 11 — glTF/import pipeline

The current glTF importer supports embedded buffer data and material scalar factors, but rejects external buffer URIs and does not yet import image payloads into `Texture2D`. Therefore a normal production glTF with external `.bin`/image files cannot yet become a fully textured EXGINE asset without a preprocessing step.

**Required completion:** source resolver for external buffers/images plus a build-time image transcode stage into an EXG-native GPU-friendly texture payload. This is exactly where the `.exg` package should absorb complexity so phones do not scrape or regenerate source assets at runtime.

## Batch 12 — Open-world scale

A finite authored base plus procedural continuation is valid. The engine must stream chunks, keep an explicit CPU/GPU residency budget, use LOD/HLOD-like representations, and preserve stable asset IDs so content can be unloaded/reloaded without changing gameplay identity.

## Acceptance rule

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

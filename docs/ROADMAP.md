# EXGINE Roadmap

EXGINE is developed as connected phases. A phase is complete only after its repository-wide checkpoint passes.

## Phase 0 — Foundation — IMPLEMENTED

Establish project contracts: C++20 build, public API boundaries, diagnostics, version identity, deterministic core behavior, tests, documentation, and repository structure.

## Phase 1 — Language — IMPLEMENTED
Build the source pipeline: `source -> lexer -> parser -> AST -> semantic validation -> IR`.

## Phase 2 — Runtime — IMPLEMENTED
Turn validated IR into live runtime state with entity identity, world state, registry ownership, loading, updating, reset, and one engine lifecycle facade.

## Phase 3 — Geometry Foundation — IMPLEMENTED FOUNDATION
Build continuous 3D geometry APIs and mesh assemblies that every visual object can reuse.

## Phase 4 — Materials and Procedural Textures — IMPLEMENTED
Build extensible physically meaningful material definitions, procedural texture generation, noise families, resource caching, and deterministic material generation keys.

## Phase 5 — World, Terrain, Water and Scenery — IMPLEMENTED
Build deterministic continuous terrain, biomes, water bodies, vegetation, world-space sampling, chunk generation, streaming, unloading and reproducible regeneration foundations.

## Phase 6 — Scene and Lighting — IMPLEMENTED
Build the runtime scene hierarchy, stable scene IDs, parent/child transforms, cameras, lighting, shadows, environment lighting and fog metadata.

## Phase 7 — Renderer and Shaders — IMPLEMENTED
Build the renderer boundary, render-frame validation, matrices, bounds, frustum culling, material bindings, render passes and shader contracts.

## Phase 8 — Interactive Buildings — IMPLEMENTED
Generate deterministic multi-floor buildings with rooms, walls, doors, windows, stairs, furniture, interactions and collision metadata.

## Phase 9 — Vehicles and Complex Objects — IMPLEMENTED
Generate configuration-driven continuous 3D vehicles across multiple families using shared geometry/material/resource/render contracts.

## Phase 10 — Physics — CORE IMPLEMENTED / ADVANCED EXTENSIONS PLANNED
Deliver the stable physics contract and connected CPU rigid-body core; advanced CCD, vehicle dynamics, buoyancy and articulated systems remain on the extension roadmap.

## Phase 11 — Characters, NPCs, Items and Gameplay — IMPLEMENTED FOUNDATION
Character controllers, gameplay state, needs, damage, inventory and NPC scheduling foundations are connected to runtime physics.

## Phase 12 — Open-World Streaming and Scale — IMPLEMENTED FOUNDATION
Scalable chunk streaming, budgets, LOD and floating-origin foundations are connected to runtime world state.

## Phase 13 — Mobile Runtime and Performance — IMPLEMENTED FOUNDATION
Device quality tiers, adaptive scale, thermal response and runtime mobile budgets exist in the engine contract.

## Phase 14 — Editor and Tooling — IMPLEMENTED FOUNDATION
Project model, scene hierarchy editing, assets, undo/redo, serialization and preview are connected to the same runtime contracts.

## Phase 15 — GPU Backend — IMPLEMENTED
Real OpenGL ES 3.1 GPU submission through the renderer boundary.

## Phase 16 — Animation and Skeletal Runtime — IMPLEMENTED
Skeletons, poses, animation tracks, blending, skinning and runtime animation progression.

## Phase 17 — GPU Skeletal Skinning and Animation Rendering — IMPLEMENTED
Skinned meshes and bone palettes feed the OpenGL ES renderer through the existing render-frame contract.

## Phase 18 — Android EGL + Surface + Swapchain + Real Mobile Presentation — IMPLEMENTED
Android `ANativeWindow` + EGL display/context/surface lifecycle connected to the OpenGL ES renderer and buffer presentation.

## Phase 19 — Android Mobile Runtime Integration — IMPLEMENTED
Lifecycle, surface-aware renderability, bounded touch/key input and pause/resume-safe mobile frame pacing.

## Phase 20 — Asset Pipeline and Runtime Packaging — IMPLEMENTED
Stable content-derived asset IDs, dependency graphs and deterministic versioned XGPK package serialization.

## Phase 21 — Asset Import Framework — IMPLEMENTED
Concrete authored-content ingestion begins with an importer abstraction and a real OBJ decoder supporting positions, UVs, normals, negative indices, polygon triangulation and generated normals.

**Checkpoint:** authored mesh data becomes the existing `Mesh` contract with deterministic asset identity and without renderer-specific ownership.

## Phase 22 — Runtime Asset Instancing — IMPLEMENTED
Decoded assets can be cached by stable `AssetId`, looked up by URI and attached to live Runtime entities through the existing `MeshAssembly`/scene/render path. Material-slot overrides are applied at instance time.

**Checkpoint:** imported geometry is registered once, instantiated on a live entity and remains inside the existing Runtime/SceneGraph/Renderer ownership model.

## Next extensions

Phase 23 can expand the importer set to glTF/GLB, including textures, materials, skeletons and animations. Phase 24 can add GPU resource residency and asynchronous streaming on top of the stable asset identities and runtime instancing layer.

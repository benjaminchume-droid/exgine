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

## Phase 10 — Physics — CORE IMPLEMENTED / ADVANCED EXTENSIONS IN PROGRESS
Deliver the stable physics contract and connected CPU rigid-body core; advanced CCD/TOI, high-fidelity contacts, buoyancy, articulated simulation and specialized mobile backends continue through later milestones.

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

## Phase 22 — Runtime Asset Instancing — IMPLEMENTED
Decoded assets can be cached by stable `AssetId`, looked up by URI and attached to live Runtime entities through the existing `MeshAssembly`/scene/render path.

## Phase 23 — glTF / GLB Ingestion — IMPLEMENTED FOUNDATION
Memory-based glTF 2.0 and GLB ingestion decodes mesh primitives, basic PBR factors and node transforms into existing EXGINE contracts.

## Phase 24 — GPU Asset Residency and Streaming — IMPLEMENTED FOUNDATION
Persistent OpenGL ES mesh residency is managed by asset identity with configurable budgets, usage tracking, eviction and deterministic teardown.

## Phase 25 — Universal Game Project and Runtime — IMPLEMENTED
Data-driven `project.exg` manifests define game metadata, startup scenes, assets, runtime settings and calendar configuration. `GameRuntime` binds one existing Runtime to project scenes, variables, update ticking and save/restore state. Scene storage is supplied by an application callback, keeping the engine independent of filesystem/package/network choices.

## Phase 26 — Universal Time, Day/Night, Seasons and Weather State — IMPLEMENTED
`EnvironmentSystem` provides continuous simulation time, day/year/week counters, configurable Dawn/Morning/Afternoon/Dusk/Evening/Night phases, named seasons with reusable coefficients and a generic weather/intensity channel.

## Phase 27–53 — High-Fidelity Production Track — IMPLEMENTED FOUNDATION
Deep production contracts for asset/image processing, asynchronous streaming, navigation, animation state control, weather visuals, audio/UI, persistence, adaptive quality, world streaming, scripting, networking/destruction foundations, and Android shipping configuration are connected in the engine.

## Phase 54 — Render Feature Graph — IMPLEMENTED
Explicit dependency ordering for shadows, depth, geometry, lighting, reflections, atmosphere, water, vegetation, VFX, transparency, post-processing and UI.

## Phase 55 — Temporal Reconstruction — IMPLEMENTED
Deterministic frame jitter and history state for temporal AA/upscaling/reflection consumers.

## Phase 56 — Game Flow Lifecycle — IMPLEMENTED
Boot/loading/menu/playing/paused/saving/error/shutdown lifecycle with validated transitions.

## Phase 57 — Engine Profiling — IMPLEMENTED
Aggregated subsystem timings, call counts and counters for benchmarks and runtime telemetry.

## Phase 58 — Rollback State Buffer — IMPLEMENTED
Bounded ordered simulation history for rollback and future network reconciliation.

## Phase 59 — World Presentation — IMPLEMENTED
Shared HDR exposure, sun-elevation and weather-fog presentation state consumed by rendering without coupling to environment simulation.

## Phase 60 — Production Acceptance Gate — IMPLEMENTED
Machine-readable acceptance contract covering project loading, scene activation, player spawn, render frame validity, physics, streaming, save round-trip and Android configuration.

## Phase 61 — High-Fidelity World Orchestration — IN PROGRESS
Unifies deterministic terrain sampling, biome classification, water presence, vegetation/structure/road budgets, regional seeds, interest-point-driven world detail and multi-chunk region construction. The service is exposed through `GameSession` so game runtime code can consume one coherent world layer rather than composing independent terrain/streaming systems.

**Checkpoint:** identical inputs produce identical region outputs; negative world coordinates map correctly; world queries return terrain/biome/water state; detail selection prioritizes active interest points; multiple regions remain deterministic and independently seeded.

## Phase 62 — Indirect Lighting and Irradiance — IN PROGRESS
Add probe-grid irradiance generation and world-space sampling so static/dynamic objects can consume a coherent indirect-lighting signal.

## Phase 63 — Reflection and Refraction Queries — IN PROGRESS
Add physically bounded reflection-plane tracing and the policy surface for screen-space, planar and future hardware ray-based reflection paths.

## Phase 64 — Volumetric Atmosphere — IN PROGRESS
Add Beer-Lambert style fog transmittance and the data path required for volumetric atmosphere/cloud consumers.

## Phase 65 — HDR Tone Mapping — IN PROGRESS
Add an HDR-to-display tone-mapping step with exposure control for the final presentation path.

## Phase 66 — Terrain Material Fusion — IN PROGRESS
Add slope/height/moisture/snow-aware material blending so terrain visual identity follows world simulation rather than a single texture.

## Phase 67 — Combined-Slip Tire Model — IN PROGRESS
Add normalized longitudinal/lateral combined tire force evaluation as the bridge from vehicle state into physics.

## Phase 68 — Character Grounding / IK — IN PROGRESS
Add deterministic ground-foot solving for animation/locomotion integration over uneven terrain.

## Phase 69 — Utility AI and Acoustic Propagation — IN PROGRESS
Add reusable utility-based action selection and distance/absorption acoustic evaluation for living-world behavior and spatial audio.

## Phase 70 — AAA Acceptance Metrics — IN PROGRESS
Define the integrated graphics/simulation/audio/runtime acceptance surface. The engine is AAA-ready only when the integrated implementation passes these metrics in an actual running game, not merely because interfaces exist.

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

## Phase 61–70 — AAA-Oriented Rendering/Simulation Foundations — IMPLEMENTED FOUNDATION
World orchestration, irradiance probes, reflection queries, volumetric transmittance, HDR tone mapping, terrain material fusion, combined-slip tires, character IK, utility AI/acoustics and explicit AAA acceptance metrics.

## Phase 71–80 — Integrated Playable Game Runtime — IMPLEMENTED FOUNDATION
`PlayableGame` connects project boot, loading, menu/playing/paused/saving lifecycle, GameSession, render-feature planning, temporal history, adaptive quality and production acceptance into one executable game-facing runtime path.

## Phase 81 — Next-Generation Render Pass Execution — IMPLEMENTED FOUNDATION
A pass executor consumes the high-fidelity rendering sequence each playable frame so shadow, geometry, lighting, reflection, atmosphere, water, vegetation, VFX, transparency, post-process and UI phases are traversed as one ordered runtime.

## Phase 82 — Runtime Material Resolution — IMPLEMENTED FOUNDATION
Runtime material records bind stable asset IDs to material texture slots and validate residency before a material becomes render-ready.

## Phase 83 — End-to-End Async Asset Pump — IMPLEMENTED FOUNDATION
Asset requests are submitted asynchronously and pumped through completion accounting for decode/upload/resident-byte telemetry.

## Phase 84 — World Navigation Controller — IMPLEMENTED FOUNDATION
Navmesh build, target acquisition and waypoint-driven agent movement are exposed as one world-navigation controller.

## Phase 85 — Integrated Vehicle Simulation — IMPLEMENTED FOUNDATION
High-fidelity tire/drivetrain vehicle state is advanced as one simulation unit with per-wheel longitudinal/lateral forces.

## Phase 86 — Character Animation Driver — IMPLEMENTED FOUNDATION
Locomotion state drives animation-machine parameters and actual clip playback callbacks.

## Phase 87 — Weather Visual Controller — IMPLEMENTED FOUNDATION
Environment weather state is converted into sky/precipitation/fog presentation with altitude-aware transmittance.

## Phase 88 — Spatial Audio Frame Runtime — IMPLEMENTED FOUNDATION
Listener/source distance, occlusion and directional panning are exposed through one audio-frame evaluation path.

## Phase 89 — UI Interaction Router — IMPLEMENTED FOUNDATION
Screen-space hit testing and enabled/visible widget dispatch are available to the game UI layer.

## Phase 90 — Save Runtime Bridge — IMPLEMENTED FOUNDATION
Persistent-world records can be captured to and restored from the binary save representation through a single runtime bridge.

## Phase 91 — Android Shipping Validation — IMPLEMENTED FOUNDATION
Android application configuration is checked for a valid packaging plan, application ID, Android plugin and NativeActivity manifest.

## Phase 92 — NextGen Runtime Integration — IMPLEMENTED FOUNDATION
Playable runtime frames now execute the next-generation ordered rendering phases in addition to validating the ordinary RenderFrame path.

## Phase 93 — Integrated Runtime Checkpoint — IMPLEMENTED
ShowcaseGame now connects the playable lifecycle, runtime physics/gameplay, procedural ExAnimation skeleton/graph generation and per-frame animation updates, procedural ExSound generation and Runtime audio submission, weather/particles, spatial audio, UI, persistence, and both headless frame validation and Android presentation paths. The dedicated integration test exercises a complete in-memory project for 180 update/render frames plus save/restore and verifies procedural animation/audio activity.

## Phase 94 — AAA Runtime Benchmark Gate — IN PROGRESS
Measure visual/simulation/runtime behavior in a real running game and establish hard acceptance thresholds instead of treating API existence as a quality result.

## End target

The engine's acceptance target is a real continuous 3D game running from an `.exg` project: world loading, player control, buildings, vehicles, NPCs, weather, interaction, persistence, high-fidelity rendering, physics, streaming and Android packaging all operate together. AAA-level quality remains an empirical benchmark that the integrated implementation must earn through rendered and simulated results.

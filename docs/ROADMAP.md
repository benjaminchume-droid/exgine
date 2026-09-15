# EXGINE Roadmap

EXGINE is developed as connected phases. A phase is complete only after its repository-wide checkpoint passes.

## Phase 0 — Foundation — IMPLEMENTED

Establish project contracts: C++20 build, public API boundaries, diagnostics, version identity, deterministic core behavior, tests, documentation, and repository structure.

**Checkpoint:** full-tree structural audit + clean build + automated tests.

## Phase 1 — Language — IMPLEMENTED

Build the source pipeline: `source -> lexer -> parser -> AST -> semantic validation -> IR`.

**Checkpoint:** source programs compile into validated IR and invalid programs fail through diagnostics.

## Phase 2 — Runtime — IMPLEMENTED

Turn validated IR into live runtime state with entity identity, world state, registry ownership, loading, updating, reset, and one engine lifecycle facade.

**Checkpoint:** complete programs create live entities, advance runtime state, reset cleanly, and failed loads cannot leave stale runtime state.

## Phase 3 — Geometry Foundation — IMPLEMENTED FOUNDATION

Build continuous 3D geometry APIs and mesh assemblies that every visual object can reuse. The foundation includes primitive meshes, capsules, deterministic procedural character parts, extended node vocabulary, and an initial physics/material architecture that later phases expand.

**Checkpoint:** geometry is generated through reusable public APIs, attached to runtime entities, procedural character generation is deterministic, and the full repository passes CI.

## Phase 4 — Materials and Procedural Textures — IMPLEMENTED

Build extensible physically meaningful material definitions, procedural texture generation, noise families, resource caching, and deterministic material generation keys.

**Checkpoint:** one managed material/resource pipeline can describe and supply surface appearance to geometry without a fixed tiny material enum.

## Phase 5 — World, Terrain, Water and Scenery — IMPLEMENTED

Build deterministic continuous terrain, biomes, water bodies, vegetation, world-space sampling, chunk generation, streaming, unloading and reproducible regeneration foundations.

**Checkpoint:** a seeded scenery world can be generated, streamed, unloaded, regenerated and reproduced through shared world-space sampling.

## Phase 6 — Scene and Lighting — IMPLEMENTED

Build the runtime scene hierarchy, stable scene IDs, parent/child transforms, cameras, directional/point/spot/area lights, shadow policy, environment lighting, fog, exposure and HDR metadata.

**Checkpoint:** runtime entities connect to one scene graph and renderer-facing lighting/camera state without a graphics API dependency.

## Phase 7 — Renderer and Shaders — IN PROGRESS

Build the renderer boundary that converts runtime scene/material/light state into validated render frames. Establish matrices, bounds, frustum culling, material bindings, render passes, shader contracts, and platform GPU backend interfaces. Keep GPU API details behind platform modules.

**Checkpoint:** the actual runtime world and its generated geometry/materials can produce one validated render frame and be accepted by the headless renderer; GPU backend implementations must consume the same contract rather than a parallel path.

## Phase 8 — Interactive Buildings

Generate buildings from reusable geometry, curves and constraints, including varied floorplans, rooms, doors, windows, stairs, furniture, lighting and collision. Support entering/exiting and interior exploration as normal runtime state.

**Checkpoint:** generated buildings have unique reproducible layouts and are enterable/interactable in the same world as their exteriors.

## Phase 9 — Vehicles and Complex Objects

Generate curved vehicle bodies and mechanical assemblies for cars, trucks, buses, motorcycles, boats, aircraft and other complex objects. Add wheels, interiors, lights, collision and physics attachment points.

**Checkpoint:** complex generated vehicles are ordinary runtime entities using the shared geometry/material/resource pipeline.

## Phase 10 — Physics

Expand the physics foundation into robust broadphase/narrowphase collision, rigid bodies, continuous collision detection, joints, constraints, character controllers, vehicle dynamics, friction, triggers, sleeping, buoyancy and deterministic simulation options.

**Checkpoint:** physical interaction operates on the same runtime entities, geometry and world state used by rendering.

## Phase 11 — Characters, NPCs, Items and Gameplay

Expand procedural players and NPCs into full characters with body variation, faces, hair, clothing, accessories, animation attachment points, equipment and reusable items. Add gameplay components, interaction, combat and state systems.

**Checkpoint:** players/NPCs can be generated from seeds, equipped from reusable item definitions and remain compatible with animation, physics and rendering.

## Phase 12 — Open-World Streaming and Scale

Extend the Phase 5 streaming foundation into scalable region management, asynchronous generation, persistence, prioritized loading/unloading, visibility-driven budgets, large-world coordinates and runtime world-state continuity.

**Checkpoint:** large worlds stream without changing game logic and without duplicating world/entity representations.

## Phase 13 — Mobile Runtime and Performance

Make Android first-class with lifecycle handling, mobile input, GPU integration, capability detection, frame-time and memory budgets, dynamic resolution, adaptive LOD, batching, profiling and device-tier quality settings.

**Checkpoint:** the same game architecture runs on Android and quality adapts to device capability without changing game logic.

## Phase 14 — Editor and Tooling

Build project tooling, scene/world inspection, asset management, debugging, profiling and authoring workflows on the same engine contracts.

**Checkpoint:** tooling never maintains a parallel representation of the runtime world.

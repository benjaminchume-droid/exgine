# EXGINE Roadmap

EXGINE is developed as connected phases. A phase is complete only after its repository-wide checkpoint passes.

## Phase 0 — Foundation

Establish the project contracts: C++20 build, public API boundaries, diagnostics, version identity, deterministic core behavior, tests, documentation, and repository structure.

**Checkpoint:** full-tree structural audit + clean build + automated tests.

## Phase 1 — Language — IMPLEMENTED

Build the real EXGINE source pipeline:

`source -> lexer -> parser -> AST -> semantic validation -> IR`

The language now supports nested blocks, properties, typed scalar values, comments, string escapes, source locations, parser recovery, semantic validation, and a single public compiler entry point.

**Checkpoint:** implemented end-to-end tests compile complete sample programs into validated IR and reject malformed/invalid programs through diagnostics. External CI status must still be green-verified before the checkpoint is formally signed off.

## Phase 2 — Runtime

Turn validated IR into live runtime state. Establish world, entity, component, resource, scene, update, and lifecycle contracts.

**Checkpoint:** a complete EXGINE program creates and updates runtime state through one connected path.

## Phase 3 — Procedural World

Implement deterministic terrain, biomes, vegetation, structures, chunks, streaming, and world coordinates.

**Checkpoint:** a generated world can be streamed, unloaded, regenerated, and reproduced from the same seed.

## Phase 4 — Procedural Objects

Build reusable geometry-generation systems for buildings, vehicles, vegetation, props, and materials.

**Checkpoint:** generated objects become normal runtime entities and can participate in rendering and physics.

## Phase 5 — Renderer

Build the 3D rendering architecture, GPU resource lifecycle, materials, lighting, camera, culling, instancing, LOD, and backend abstraction.

**Checkpoint:** the actual runtime world is rendered through the renderer; no disconnected rendering demo is accepted.

## Phase 6 — Physics and Gameplay

Add collision, bodies, character movement, vehicle physics, interaction, triggers, animation hooks, and gameplay systems.

**Checkpoint:** gameplay operates on the same entities/world state rendered by the engine.

## Phase 7 — Resources and Assets

Implement asset discovery, import, caching, lifetime management, streaming, materials, textures, meshes, animation, and audio resource contracts.

**Checkpoint:** resources can move from source asset to runtime use through one managed pipeline.

## Phase 8 — Mobile Runtime

Make Android a first-class target with input, application lifecycle, GPU backend integration, packaging, memory limits, and mobile controls.

**Checkpoint:** a real EXGINE game runs on Android through the same engine architecture.

## Phase 9 — Performance

Add capability detection, quality profiles, frame-time budgeting, dynamic resolution, adaptive LOD, memory budgeting, and profiling.

**Checkpoint:** performance behavior is measurable and adapts without changing game logic.

## Phase 10 — Editor and Tooling

Build project tooling, scene/world inspection, asset management, debugging, profiling, and eventually a visual editor.

**Checkpoint:** tooling operates on the same project/runtime contracts instead of maintaining a second representation.

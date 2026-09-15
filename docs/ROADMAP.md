# EXGINE Roadmap

EXGINE is developed as connected phases. A phase is complete only after its repository-wide checkpoint passes.

## Phase 0 — Foundation

Establish project contracts: C++20 build, public API boundaries, diagnostics, version identity, deterministic core behavior, tests, documentation, and repository structure.

**Checkpoint:** full-tree structural audit + clean build + automated tests.

## Phase 1 — Language — IMPLEMENTED

Build the source pipeline: `source -> lexer -> parser -> AST -> semantic validation -> IR`.

**Checkpoint:** source programs compile into validated IR and invalid programs fail through diagnostics. CI is green for the Phase 1/2 baseline.

## Phase 2 — Runtime — IMPLEMENTED

Turn validated IR into live runtime state with entity identity, world state, registry ownership, loading, updating, reset, and one engine lifecycle facade.

**Checkpoint:** complete programs create live entities, advance runtime state, reset cleanly, and failed loads cannot leave stale runtime state. CI is green on the Phase 2 head.

## Phase 3 — Geometry Foundation — IN PROGRESS

Build the continuous 3D geometry foundation used by every visual object. Primitive shapes are reusable mesh generators, not a block-world abstraction. Add boxes, spheres, cylinders, capsules, mesh assemblies, and deterministic procedural character parts. Extend the IR/runtime vocabulary for players, NPCs, bridges and props.

**Checkpoint:** geometry is generated through reusable public APIs, attached to runtime entities, procedural character generation is deterministic, and the full repository passes CI.

## Phase 4 — Materials and Procedural Textures

Build physically meaningful material definitions and procedural texture layers for wood, stone, concrete, gravel, asphalt, grass, sand, metal, glass, rubber, fabric, leather, water, snow and other game materials. Add texture coordinates, normals/tangents, surface variation and material instances.

**Checkpoint:** one managed material pipeline can describe and supply surface appearance to every geometry type.

## Phase 5 — World and Scenery

Build deterministic terrain, mountains, grasslands, forests, rivers, lakes, oceans, vegetation, rocks, bridges and world coordinates. Add chunks, generation, streaming, unloading and regeneration.

**Checkpoint:** a complete seeded scenery world can be generated, streamed, unloaded, regenerated and reproduced.

## Phase 6 — Renderer and Realistic Lighting

Build the GPU abstraction, cameras, culling, instancing, LOD, physically based rendering, shadows, ambient lighting, reflections, atmospheric effects, water rendering and post-processing. Shader code remains backend-aware but renderer-independent at the public boundary.

**Checkpoint:** the actual runtime world and its generated geometry/materials are rendered through one production renderer; no disconnected graphics demo counts.

## Phase 7 — Interactive Buildings

Generate buildings from reusable geometry, curves and constraints, including varied floorplans, rooms, doors, windows, stairs, furniture, lighting and collision. Support entering/exiting and interior exploration as normal runtime state.

**Checkpoint:** generated buildings have unique reproducible layouts and are enterable/interactable in the same world as their exteriors.

## Phase 8 — Vehicles and Complex Objects

Generate curved vehicle bodies and mechanical assemblies for cars, trucks, buses, motorcycles and other vehicles. Add wheels, interiors, lights, collision and later vehicle physics.

**Checkpoint:** complex generated vehicles are ordinary runtime entities with geometry/materials and can be driven once gameplay/physics layers are available.

## Phase 9 — Characters, Clothing and Equipment

Expand procedural players and NPCs into full character generation: body variation, faces, hair, clothing, watches, bags, shoes, uniforms, accessories, animation attachment points and equipment slots. Items share the same geometry/material/resource pipeline.

Combat-capable games can represent firearms, melee weapons and other equipment as ordinary game assets/components governed by game rules.

**Checkpoint:** players and NPCs can be generated from seeds, equipped from reusable item definitions, and remain compatible with animation, physics and rendering.

## Phase 10 — Physics and Gameplay

Add collision, bodies, character movement, vehicle physics, interaction, triggers, animation systems, combat/gameplay components and damage/state systems.

**Checkpoint:** gameplay operates on the same entities, geometry and world state rendered by the engine.

## Phase 11 — Resources and Assets

Implement asset discovery, import, caching, lifetime management, streaming, meshes, materials, textures, animation, audio and other resource contracts.

**Checkpoint:** source asset to runtime resource uses one managed pipeline.

## Phase 12 — Mobile Runtime and Performance

Make Android first-class with input, lifecycle, GPU integration, packaging, memory limits, mobile controls, capability detection, frame-time budgeting, dynamic resolution, adaptive LOD and profiling.

**Checkpoint:** the same game architecture runs on Android and quality adapts to device capability without changing game logic.

## Phase 13 — Editor and Tooling

Build project tooling, scene/world inspection, asset management, debugging and profiling on the same engine contracts.

**Checkpoint:** tooling never maintains a parallel representation of the runtime world.

# EXGINE

**EXGINE** is a native, mobile-first game engine designed around a code-to-world pipeline: game intent is described at a high level, transformed into a validated intermediate representation, optimized, and executed by a lightweight runtime.

EXGINE is being built as a complete engine, not as a collection of disconnected demos. Every major phase ends with a structural checkpoint that reviews the whole repository and verifies that the new subsystem is correctly connected to the architecture.

## Vision

A developer should be able to describe game-world intent without manually implementing every low-level representation.

For example:

```text
game "MyWorld" {
    world {
        terrain = procedural
        terrain_height = 20
        trees = procedural
        exploration = infinite
    }
}
```

The long-term pipeline is:

```text
Game source / EXGINE DSL / language adapters
                    |
               Front ends
                    |
                  AST
                    |
          Semantic validation
                    |
                EXGINE IR
                    |
                Optimizer
                    |
             Native runtime
       /        |          |        \
    World   Entities   Resources    Scene
                                  /    \
                            Lighting  Camera
                                  \    /
                                Renderer
                                    |
                           Android / Desktop
```

## Development model

EXGINE is developed in large, connected phases rather than tiny feature drops.

Each phase has a **checkpoint**. A checkpoint is not a progress counter. It is a repository-wide engineering gate:

1. Inspect the complete source tree.
2. Trace data flow and dependencies across subsystems.
3. Check public APIs and ownership boundaries.
4. Build and test the complete project.
5. Check that new code is used by the systems around it.
6. Remove temporary architecture and duplicated paths.
7. Record the result before starting the next phase.

A phase is complete only when its checkpoint passes.

## Phases

| Phase | Goal | Status |
|---|---|---|
| 0 | Architecture and production-grade foundation | Implemented |
| 1 | EXGINE language: lexer, parser, AST and semantic validation | Implemented |
| 2 | Runtime and entity lifecycle | Implemented |
| 3 | Continuous geometry and reusable object foundations | Implemented foundation |
| 4 | Materials, procedural textures and resource cache | Implemented |
| 5 | Procedural world, terrain, water, biomes and streaming | Implemented |
| 6 | Scene graph, lighting and camera state | Implemented |
| 7 | Renderer frame pipeline and shader contracts | Implemented |
| 8 | Interactive buildings | Implemented |
| 9 | Vehicles and complex objects | Implemented |
| 10 | Full physics | Planned / advanced extensions |
| 11 | Characters, NPCs, items and gameplay | Planned |
| 12 | Open-world scale and streaming | Planned |
| 13 | Android/mobile runtime and performance | Planned |
| 14 | Editor and production tooling | Planned |

See [`docs/ROADMAP.md`](docs/ROADMAP.md) for the full plan and the phase checkpoint documents for implementation boundaries.

## Current architecture

```text
include/exgine/   Public engine API
src/              Engine implementation
examples/         Executable examples
tests/            Automated verification
docs/             Architecture and development contracts
```

The core is C++20. Platform-specific rendering and mobile code are introduced behind explicit interfaces so graphics and device details do not leak into the portable engine core.

## Design goals

- Native C++ core
- Small and predictable memory footprint
- Mobile-first performance
- Deterministic procedural generation
- Chunked world streaming
- Scalable rendering quality
- Declarative game descriptions
- Shared IR for multiple source languages
- Android and desktop targets
- Clear subsystem boundaries
- Testable, incremental architecture without disposable placeholders

## Current renderer boundary

Phase 7 converts live runtime state into a validated `RenderFrame` containing camera matrices, visible mesh draw calls, material/texture bindings, and active lighting. The core also contains deterministic bounds/frustum culling and built-in GLSL ES 3.10 PBR/unlit shader sources. GPU objects, platform presentation and device-specific backend implementations remain behind the renderer contract.

## Current building boundary

Phase 8 adds deterministic multi-floor buildings using the shared continuous geometry system. A building owns reproducible rooms, wall partitions, doors, windows, stairs, furniture, interaction points, room queries and collision-volume metadata. Runtime building instances use normal entity IDs, scene nodes and material resources, so generated interiors enter the same renderer path as every other runtime object.

Building dimensions are configuration inputs: room bounds are computed from the configured footprint and wall thickness, then partitioned according to the configured room count. There is no hidden building expansion factor, and the generator preserves the caller's seed value including zero.

## Current vehicle boundary

Phase 9 adds configuration-driven vehicles and complex object assemblies for cars, SUVs, sports cars, pickups, trucks, buses, motorcycles, construction vehicles, emergency vehicles, boats and aircraft. Length, width, height, wheelbase, track, wheel dimensions, seating, cabin proportions and other structural settings feed one deterministic generator. Caller-provided seeds drive reproducible variation without an implicit private seed.

Vehicles are ordinary runtime entities using the shared geometry/material/resource/renderer path. Wheels, doors, seats, lights, collision volumes and physics attachment points are exported as engine data rather than being baked into renderer-only code.

## Current physics boundary

`include/exgine/physics.hpp` is the stable 3D physics contract and `src/physics.cpp` now provides a deterministic CPU rigid-body core with fixed-step integration, gravity/forces/torques, broadphase candidate generation, basic contact resolution, constraints, sleeping/waking, contact callbacks and spatial queries. Runtime owns the physics world and advances it from the normal update lifecycle.

The advanced physics roadmap remains broader than this current core: production-grade CCD/TOI, high-fidelity convex and triangle-mesh contact generation, full vehicle tire/drivetrain dynamics, character controllers, buoyancy/hydrodynamics, articulated/soft-body simulation and specialized mobile parallel backends will extend the same contract in Phase 10 rather than bypassing it.

## Building

EXGINE uses CMake and C++20.

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The repository must remain buildable as the engine grows.

## License

EXGINE is licensed under the **Apache License 2.0**. See [`LICENSE`](LICENSE).

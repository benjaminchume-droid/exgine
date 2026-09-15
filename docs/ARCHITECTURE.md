# EXGINE Architecture

## Purpose

This document defines the architectural contracts that keep EXGINE connected as the engine grows. New systems must fit these boundaries instead of creating parallel, one-off paths.

## System layers

```text
                 Source languages
                       |
                 Front-end adapters
                       |
                      AST
                       |
              Semantic validation
                       |
                    EXGINE IR
                       |
                  Optimization
                       |
                 Runtime systems
              /        |        \
           World     Entities    Resources
                       |
                   Buildings
                       |
                    Scene
                 /        \
            Lighting    Camera
                 \        /
                  Render API
                 /          \
          RenderFrame      Shaders
                 |            |
                 +-------> Platform/GPU backend
```

## Dependency direction

Dependencies flow downward toward lower-level services. A lower-level subsystem must not depend on a higher-level feature just to function.

- Front ends may produce AST/IR.
- IR contains engine-neutral game intent and validated data.
- Runtime consumes validated IR and owns live game state.
- World systems generate and stream world state.
- Entity systems own stable runtime identities.
- Building generation consumes configuration and existing geometry/material APIs to create building instances keyed by existing `EntityId` values.
- Scene graph owns runtime spatial hierarchy through stable node IDs.
- Lighting/camera systems describe renderer-facing scene state without depending on a graphics API.
- Renderer snapshots runtime state into validated `RenderFrame` data and does not define gameplay rules.
- Shader programs define the binding/source contract consumed by platform shader compilers.
- Platform backends own GPU resources, command submission, presentation, and API-specific synchronization.
- Examples and tools consume public APIs; engine internals do not depend on examples.

## Public API rule

Headers under `include/exgine/` are the public engine contract. Implementation details belong in `src/` unless there is a deliberate reason to expose them.

## Data ownership

Ownership must be explicit. Prefer value semantics for small immutable descriptions and RAII-owned objects/resources for runtime state. Avoid raw owning pointers. Cross-system references should use stable IDs or handles rather than undocumented pointer ownership.

Building instances are owned by `Runtime` and keyed by their existing entity IDs. Their generated geometry is shared with the normal entity render path; room/opening/interactable/collision metadata has one authoritative building representation.

Render frames own value snapshots of camera/light/material parameters and shared references to immutable texture resources. This keeps a submitted frame valid even when the caller releases its transient local references.

## Determinism

Procedural systems must be deterministic when given the same explicit seed and configuration. Platform-specific rendering may differ visually, but world generation and gameplay logic should not silently depend on frame timing or undefined platform state. Render-frame entity ordering is explicitly sorted by stable entity ID.

Building room IDs, opening placement, furniture placement, stairs and collision metadata derive from the explicit building seed/configuration and do not use runtime time or pointer identity.

## Threading

The core architecture should not assume that every system runs on the main thread. Thread-affine systems must state their constraints. Shared mutable state requires an explicit synchronization or ownership strategy.

## Error handling

Expected user/content errors should be represented through structured diagnostics. Programmer errors should be made visible during development. The engine should not use exceptions as an undocumented control-flow mechanism. Renderer validation fails closed when a geometry or material resource is missing.

Building generation fails cleanly when the target entity is not a building, the runtime is not loaded, required material resources are missing, or the generated definition is invalid.

## Platform separation

Android, desktop, Vulkan, OpenGL ES, audio backends, and input devices must be isolated behind platform/backend interfaces. Core world, IR, parser, gameplay, scene, lighting, building generation, render-frame construction, and shader source contracts should remain portable.

## Phase rule

A new phase may introduce implementation detail, but it may not weaken an existing architectural contract without updating this document and the phase checkpoint that depends on it.

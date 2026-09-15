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
              \        |        /
                    Scene
                 /        \
            Lighting    Camera
                 \        /
                  Render API
                       |
              Platform/GPU backend
```

## Dependency direction

Dependencies flow downward toward lower-level services. A lower-level subsystem must not depend on a higher-level feature just to function.

- Front ends may produce AST/IR.
- IR contains engine-neutral game intent and validated data.
- Runtime consumes validated IR and owns live game state.
- World systems generate and stream world state.
- Scene graph owns runtime spatial hierarchy through stable node IDs.
- Lighting/camera systems describe renderer-facing scene state without depending on a graphics API.
- Renderer consumes renderable state; it does not define gameplay rules.
- Platform backends provide OS/GPU integration behind interfaces.
- Examples and tools consume public APIs; engine internals do not depend on examples.

## Public API rule

Headers under `include/exgine/` are the public engine contract. Implementation details belong in `src/` unless there is a deliberate reason to expose them.

## Data ownership

Ownership must be explicit. Prefer value semantics for small immutable descriptions and RAII-owned objects/resources for runtime state. Avoid raw owning pointers. Cross-system references should use stable IDs or handles rather than undocumented pointer ownership.

## Determinism

Procedural systems must be deterministic when given the same explicit seed and configuration. Platform-specific rendering may differ visually, but world generation and gameplay logic should not silently depend on frame timing or undefined platform state.

## Threading

The core architecture should not assume that every system runs on the main thread. Thread-affine systems must state their constraints. Shared mutable state requires an explicit synchronization or ownership strategy.

## Error handling

Expected user/content errors should be represented through structured diagnostics. Programmer errors should be made visible during development. The engine should not use exceptions as an undocumented control-flow mechanism.

## Platform separation

Android, desktop, Vulkan, OpenGL ES, audio backends, and input devices must be isolated behind platform/backend interfaces. Core world, IR, parser, gameplay, scene and lighting code should remain portable.

## Phase rule

A new phase may introduce implementation detail, but it may not weaken an existing architectural contract without updating this document and the phase checkpoint that depends on it.

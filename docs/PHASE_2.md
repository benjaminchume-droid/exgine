# Phase 2 — Runtime

## Objective

Phase 2 turns validated EXGINE IR into live runtime state and establishes the engine lifecycle used by later systems.

```text
EXGINE source
    -> Compiler / IR
    -> Runtime::load
    -> WorldState + EntityRegistry
    -> Engine::update
    -> live runtime state
```

## Implemented

- Stable `EntityId` identity with an explicit invalid value.
- Entity registry with creation, lookup, destruction and reset.
- Runtime transform and active-state contracts.
- World state containing the world entity, entities, tick counter and elapsed time.
- Recursive IR-to-entity instantiation preserving hierarchy order and properties.
- Runtime load/reset lifecycle.
- Deterministic update clock with protection against negative delta time.
- Public `Engine` facade connecting source compilation directly to runtime loading and updates.
- Failed loads clear runtime state instead of leaving stale entities active.
- End-to-end tests covering source → compiler → IR → runtime → update and failure recovery.

## Architectural boundary

The compiler owns source and semantic concerns. The runtime owns live state. Runtime code does not parse source and does not depend on rendering, physics, platform APIs or asset backends.

The registry currently stores entities by stable ID. Later phases can attach components and systems without changing the source/compiler boundary.

## Current lifecycle

```cpp
exgine::Engine engine;
engine.load(source);
engine.update(delta_seconds);
engine.reset();
```

A successful load creates a world entity plus entities represented by the IR tree. Properties are copied into runtime entities so later systems operate on the compiled data rather than reparsing source.

## Checkpoint

Phase 2 is structurally complete when the complete source-to-runtime path builds, valid IR becomes live entities, updates advance runtime state, reset destroys live state, and failed loads cannot leave stale runtime state behind. External CI must be green-verified before formal checkpoint sign-off.

## Not included yet

Physics, rendering, transforms derived from world generation, component systems, scripting, resource management, networking, platform integration, and scheduling across multiple runtime systems belong to later phases.

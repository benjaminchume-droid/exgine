# Phase 8 — Interactive Buildings

## Goal

Phase 8 adds a deterministic building generator that produces real continuous 3D interior/exterior geometry and the metadata needed for exploration and interaction. Buildings remain ordinary EXGINE runtime entities and use the existing scene, material, resource and renderer paths.

## Implemented

- Configurable multi-floor building dimensions and floor heights.
- Deterministic room grids with stable room IDs and world-space room bounds.
- Exterior walls and foundations generated from reusable box geometry.
- Interior partition walls with deterministic doorway placement.
- Exterior entrance door and window openings with stable IDs.
- Door, window and light-switch interaction contracts.
- Furniture generation for beds, desks, chairs, tables, sofas, counters and cabinets through shared geometry primitives.
- Multi-floor stair flights with explicit collision-volume data.
- Collision AABB metadata for walls, foundations, stairs and closed doors.
- Door state changes the active collision set without creating a second building representation.
- Point-in-room queries for interior exploration systems.
- Runtime-owned building instances keyed by existing `EntityId` values.
- Building geometry attaches directly to `Entity::geometry` and therefore enters the Phase 7 renderer pipeline without a parallel rendering path.
- Explicit material/resource dependency: configured wall/floor/glass/door/furniture materials must already exist in `Runtime` before building generation succeeds.

## Connected data flow

```text
EXGINE source
    -> Compiler
    -> IR
    -> Runtime EntityRegistry
           |
      Building Entity
           |
   generate_building()
      /    |      \
 geometry rooms   interaction/collision metadata
      |      |          |
   Scene  room query  later Physics
      \      |         /
       Renderer / Gameplay
```

## Interaction boundary

Doors expose stable IDs and an open/closed state. `active_building_collision()` filters door-owned collision volumes when a door is open. This is deliberately data-driven so the later physics system can consume the result without the building generator owning a physics solver.

Windows currently have visual/opening metadata but remain solid in the collision model. Breaking windows and dynamic portal traversal belong to later gameplay/physics work.

## Runtime lifecycle

Building generation is explicit rather than a hidden side effect of `Runtime::load()`. This avoids making source loading depend on resource-generation order and keeps content errors deterministic: required materials must be defined first, then the building is generated and attached to its existing entity.

Reset clears all building instances together with the existing runtime state, resources, scene and lighting systems.

## Production boundary

Phase 8 does not implement the final physics solver, character controller, navmesh/pathfinding, door animation, destruction simulation or asynchronous building generation. It provides the stable building representation those later systems consume. The core remains portable and backend-independent.

## Checkpoint

- Building generation is deterministic for an explicit seed/configuration.
- Multi-floor rooms, openings, stairs and furniture are real reusable geometry.
- Door interaction changes active collision data deterministically.
- Building instances belong to existing runtime entities.
- Generated geometry is compatible with the Phase 7 renderer and material/resource system.
- Complete build and test suite passes in CI.

# Phase 6 — Scene & Lighting

## Objective
Phase 6 establishes the engine's renderer-facing scene contract without coupling the core to a graphics API. Runtime entities now receive stable scene nodes, hierarchical transforms are evaluated centrally, and lighting/camera/environment state is owned by the runtime.

## Implemented
- Hierarchical `SceneGraph` with a stable root node.
- Parent/child relationships with cycle prevention.
- Local and calculated world transforms.
- Subtree destruction and parent reassignment.
- Runtime entities carry their associated scene-node ID.
- Scene graph is rebuilt from the compiled IR during runtime load.
- Scene transforms are refreshed during runtime updates.
- Directional, point, spot and area light types.
- Validated light ranges, cone limits, intensities and directions.
- Main perspective/orthographic camera contract with clipping and culling configuration.
- Environment lighting values for sky, horizon, ground, sun, ambient and environment strength.
- Fog contract with density and distance controls.
- Exposure, gamma and HDR output policy.
- Runtime-owned lighting lifecycle and main-camera API.
- Full tests for scene hierarchy, transform composition, cycle rejection, destruction, cameras, light validation and runtime integration.

## Connected data flow

```text
EXGINE source
      |
      v
Compiler -> validated IR
                |
                v
           Runtime::load
             /        \
            /          \
     EntityRegistry   SceneGraph
           |              |
        entities       transforms
           |              |
           +-------+------+ 
                   |
                   v
              Runtime Scene
                   |
          +--------+--------+
          |        |        |
       Camera    Lights   Environment
          |        |        |
          +--------+--------+
                   |
                   v
             future renderer
```

## Design boundary
The lighting subsystem describes scene lighting semantics; it does not execute GPU shaders, allocate graphics API resources, rasterize shadow maps or perform final post-processing. Those responsibilities belong to Phase 7. Likewise, the scene graph is renderer-independent and uses stable IDs so the future renderer can consume scene data without taking ownership of gameplay entities.

## Transform contract
Phase 6 uses position/rotation/scale scene transforms. Child position is scaled by the parent's world scale, rotations accumulate component-wise, and scales multiply component-wise. This gives the runtime a deterministic hierarchy today while leaving room for a matrix/quaternion backend when the renderer and animation systems are introduced.

## Checkpoint
- Existing compiler → IR → runtime pipeline remains intact.
- Entity-to-scene ownership is explicit through stable IDs.
- Scene hierarchy rejects cycles and invalid parents.
- World transforms are deterministic after update.
- Invalid cameras/lights are rejected.
- Lighting state is reset with runtime state.
- CMake includes all Phase 6 sources.
- Complete Release build and test suite pass in CI.

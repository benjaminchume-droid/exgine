# Phase 10 — Physics

## Status

**Core checkpoint: implemented and validated.**

Phase 10 establishes the runtime physics layer used by buildings, vehicles, terrain and future character/gameplay systems. The public contract is `include/exgine/physics.hpp`; the connected CPU implementation is `src/physics.cpp`.

## Runtime contract

- Fixed-step real-time simulation with a bounded accumulator.
- Deterministic ordering is available through the deterministic world setting.
- Static, dynamic and kinematic body modes.
- Stable 64-bit body, collider and constraint handles.
- Gravity, forces, torques, damping, mass properties and velocity limits.
- Sleeping and waking with callbacks.
- Sphere, box, capsule, cylinder, convex-hull, triangle-mesh, height-field and compound shape contracts.
- Physics materials with density, static/dynamic friction, rolling/spinning friction and restitution.
- Layer/mask filtering and sensor colliders.
- Contact begin/persist/end events.
- Fixed, ball-socket, hinge, slider, distance, spring, cone-twist and six-degree-of-freedom constraint contracts, including motors and break thresholds.
- Raycast, shape-cast and overlap query contracts.
- Building collision volumes and vehicle attachment/collision data can be consumed without duplicating geometry ownership.

## Checkpoint validation

The Phase 10 test suite validates:

1. Fixed-step accumulation and bounded catch-up.
2. Gravity and dynamic-body integration.
3. Static-vs-dynamic collision response.
4. Contact begin/persist events.
5. Sleeping and waking.
6. Layer/mask and sensor query filtering.
7. Raycast, overlap and shape-cast query contracts.
8. Compound, convex, triangle-mesh and height-field shape validation.
9. Constraint creation, motor mutation and body-owned cleanup.
10. Deterministic simulation for identical inputs.
11. Runtime ownership: loading a world creates one physics world and runtime updates advance it.
12. Clear/reset behavior and stable handle lifecycle.

## Architectural boundary

Physics does not own render meshes, building definitions, vehicle definitions, characters or gameplay state. It owns simulation state and consumes shape/material descriptors. This keeps character controllers, vehicle dynamics, combat, swimming, interaction and NPC systems in later phases while allowing them to share the same physical world.

## Fidelity boundary

This checkpoint is production-oriented core infrastructure, not a claim that every advanced physical phenomenon is already implemented. High-fidelity continuous collision/TOI, exact convex and triangle-mesh narrowphase, production vehicle tire/drivetrain dynamics, articulated/soft-body simulation, buoyancy/hydrodynamics and specialized mobile parallel backends remain explicit follow-on work. They must extend this contract rather than introduce a second physics architecture.

## Definition of done

Phase 10 is accepted when the entire C++ project builds, the core suite passes, the dedicated physics suite passes, and the physics implementation remains connected to `Runtime::load()` and `Runtime::update()`.

# EXGINE Physics Contract

`include/exgine/physics.hpp` is the stable public contract for the EXGINE physics subsystem. It is deliberately separate from the Phase 9 vehicle generator so generated objects can expose physically meaningful attachment points without inventing a fake solver.

## Simulation model

The contract is designed for real-time three-dimensional rigid-body simulation with a fixed-step accumulator. The default contract targets a 120 Hz fixed step, bounded substeps, iterative velocity/position solving, sleeping and deterministic mode.

## Bodies

Bodies support `Static`, `Dynamic` and `Kinematic` modes. A body description carries world transform, linear/angular velocity, anisotropic damping, gravity scaling, mass properties, motion quality, sleep/gravity/CCD flags and speed limits.

## Shapes

The shape contract covers sphere, box, capsule, cylinder, convex hull, triangle mesh, height field and compound shapes. Shapes are independent from rendered mesh ownership and can therefore use specialized collision representations while sharing the same world/entity coordinates.

## Materials and contacts

Physics materials expose density, static/dynamic friction, rolling/spinning friction and restitution. Contact manifolds expose body/collider identity, up to four contact points, normals, penetration and accumulated normal/tangent impulses. Contact events distinguish begin, persist and end transitions.

## CCD and TOI

Motion quality explicitly distinguishes discrete, linear-cast, continuous and continuous+angular modes. The world settings reserve dedicated time-of-impact solver iterations so high-speed bodies do not depend on frame rate for tunnelling behavior.

## Constraints

The contract supports fixed, ball-socket, hinge, slider, distance, spring, cone-twist and six-degree-of-freedom constraints. Linear and angular limits, motors and break force/torque are first-class data.

## Queries

Raycasts, shape casts and overlap queries share a common layer/mask filter with body/collider exclusion and sensor policy. Results contain stable body/collider IDs, fraction/distance, hit position and normal.

## Runtime integration boundary

Physics bodies and colliders are intended to bind to ordinary EXGINE runtime entities. Buildings expose collision volumes; vehicles expose wheel/axle/center-of-mass attachment points. The full implementation in Phase 10 will consume these existing representations instead of creating parallel object systems.

## Determinism and threading

Deterministic mode is explicit. Parallel broadphase is a performance option and must not change externally visible simulation results when deterministic mode is enabled. Callback delivery is part of the contract but application-owned behavior remains outside the physics core.

## Phase boundary

Phase 9 establishes this solver-facing contract and physically useful vehicle/building metadata. Phase 10 is the implementation phase for broadphase/narrowphase collision, constraint solving, CCD, character controllers, vehicle dynamics, triggers, sleeping, buoyancy and deterministic simulation.

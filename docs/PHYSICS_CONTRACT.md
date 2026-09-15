# EXGINE Physics Contract

`include/exgine/physics.hpp` is the public contract for EXGINE's three-dimensional physics subsystem. It is deliberately independent from render geometry ownership: collision shapes can be specialized while still using the same world/entity coordinate system.

## Simulation model

The contract targets real-time rigid-body simulation with a fixed-step accumulator. The default is a 120 Hz fixed step with bounded catch-up substeps, iterative velocity/position solving, sleeping, interpolation state and an explicit deterministic mode.

## Bodies

Bodies support `Static`, `Dynamic` and `Kinematic`. Descriptions carry transform, linear/angular velocity, anisotropic damping, gravity scaling, mass properties, motion quality, sleep/gravity/CCD flags and velocity limits. Runtime owns the physics world; callers retain stable body handles rather than owning solver internals.

## Shapes

Supported representations are sphere, box, capsule, cylinder, convex hull, triangle mesh, height field and compound shapes. Render meshes and collision shapes are separate contracts so mobile/runtime optimization can select an appropriate collision representation without changing visual assets.

## Materials and contacts

Physics materials expose density, static/dynamic friction, rolling/spinning friction and restitution. Contact manifolds expose body/collider identity, up to four points, normals, penetration and accumulated impulses. Contact callbacks report begin/persist/end transitions, while sensors generate events without solver response.

## Constraints

The public contract supports fixed, ball-socket, hinge, slider, distance, spring, cone-twist and six-degree-of-freedom constraint descriptions, including linear/angular limits, motors and break thresholds.

## Queries

Raycasts, shape casts and overlap queries use one layer/mask filter model with body/collider exclusions and sensor policy. Results contain stable handles, normalized fraction/distance, world hit position and hit normal.

## Runtime implementation boundary

The repository now contains a deterministic CPU rigid-body runtime behind this contract. It includes fixed-step integration, gravity/force/torque application, sleeping/waking, broadphase candidate generation, basic shape narrowphase, sequential impulse contacts, friction/restitution, constraint processing, callbacks and spatial queries. `Runtime::load()` owns one physics world and `Runtime::update()` advances it through the same engine lifecycle.

This is a production-oriented core, not a claim that every advanced physical phenomenon already exists. Continuous collision detection/TOI behavior, high-fidelity convex/mesh contact generation, full vehicle tire/drivetrain dynamics, character controllers, buoyancy/hydrodynamics, articulated soft-body systems and specialized mobile parallel backends remain future physics work and must extend this contract without bypassing it.

## Determinism and threading

Determinism is explicit. IDs and contact processing order are stable, and the public contract forbids pointer/time-based simulation identity. Parallel broadphase is a policy option; deterministic mode must remain externally reproducible. Callbacks are application-owned and are invoked by the simulation owner.

## Integration with generated objects

Buildings provide collision-volume metadata; vehicles provide wheel, axle and center-of-mass attachment points. The physics layer consumes these representations through physics shapes/bodies instead of maintaining a second building or vehicle world.

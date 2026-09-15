# EXGINE Phase 9 — Vehicles and Complex Objects

## Goal

Phase 9 makes vehicles and complex mechanical objects first-class generated assets. They use the same continuous geometry, material, texture/resource, runtime entity, scene and renderer contracts already used by buildings and other engine objects.

## Configuration-first generation

`VehicleConfig` is the authoritative input. Length, width, height, ground clearance, wheelbase, track width, wheel size, axle count, wheel count, seats, doors, cabin dimensions, overhangs, wingspan and waterline determine the generated proportions.

Generation normalizes physically impossible values only to safe bounds. It does not silently enlarge the requested object or replace explicit dimensions with a hidden canonical size. The returned `VehicleDefinition::config` records the normalized configuration actually used.

`make_vehicle_config(VehicleType)` is an explicit convenience function that supplies type-specific starting values. Applications may override those values before generation.

## Determinism

The caller controls the seed. A zero seed remains zero; the generator does not substitute a private hard-coded seed. Procedural variation derives from deterministic seed mixing and does not use time, pointer identity or `std::hash`.

## Generated structure

Land vehicles receive reusable chassis/body/cabin/interior components, configurable doors and seats, wheel assemblies, lights and physics attachment points. Motorcycles, boats and aircraft use type-specific structural layouts. Wheels record axle index, role, driving/steering state, dimensions and suspension attachment data.

The current phase intentionally keeps collision representation separate from rendering geometry. That lets Phase 10 choose appropriate primitive/compound collision shapes without requiring renderer geometry to become the physics representation.

## Runtime path

A vehicle is generated only for an existing `NodeKind::Vehicle` entity. Runtime requires the configured body, glass, rubber, metal and interior material resources, stores one authoritative `VehicleInstance` keyed by the entity ID, and attaches its generated geometry to the normal entity render path.

Door state is explicit metadata today. Vehicle collision data and physics attachment points are ready for Phase 10; final suspension solving, tire forces, steering dynamics, drivetrain simulation, buoyancy and flight dynamics belong to the physics implementation phase.

## Building compatibility check

Phase 8 building generation remains configuration-driven. Its room bounds are computed inside the configured building width/depth after the configured wall thickness is accounted for. Floors use the configured floor height. No separate hidden expansion factor is introduced.

## Checkpoint

Repository-wide checkpoint for Phase 9:

- Public vehicle contract exists.
- All supported vehicle families generate through one shared API.
- Explicit dimensions and caller-provided seeds control generation.
- Vehicle geometry uses the shared `MeshAssembly` and `MeshPart` rotation path.
- Runtime owns generated vehicle instances by ordinary entity IDs.
- Material resources and renderer integration use existing engine pipelines.
- Regression tests cover all vehicle families, determinism, dimensional configuration and runtime rendering.
- `physics.hpp` establishes the production solver-facing contract without pretending the later Phase 10 solver has already been implemented.

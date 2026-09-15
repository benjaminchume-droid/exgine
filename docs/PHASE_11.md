# Phase 11 — Characters & Gameplay

## Status

**Core checkpoint: implemented.**

Phase 11 adds the reusable character/gameplay layer above the existing entity, world and physics architecture. It deliberately avoids hard-coded world layouts, magic building expansion, or game-specific rules in the physics layer.

## Character foundation

- Player and NPC character definitions.
- Deterministic character appearance remains supplied by the existing character generator.
- Physics-backed humanoid controller with standing/crouching dimensions.
- Walk, run, crouch, jump/fall state, climb and swim movement states.
- Acceleration/braking and configurable movement speeds.
- Stable character handles.

## Gameplay foundation

- Health and recovery.
- Stamina, hunger, thirst, fatigue and oxygen state.
- Typed damage events and combat result reporting.
- Generic item definitions.
- Stack-aware inventories with deterministic slot operations.
- Equip and consume operations.
- Generic interaction targets for use, pickup, drop, open/close, sit, vehicle entry/exit, talk and attack.
- NPC schedules with daily time windows and activity state.
- Scaled simulation time and game-minute tracking.

## Architectural connection

```text
Character / NPC
       |
       +-- Controller
       +-- Vitals
       +-- Inventory
       +-- Equipment
       +-- Interactions
       +-- Schedule
       +-- Combat events
       |
       v
   PhysicsWorld
       |
       +-- Buildings
       +-- Vehicles
       +-- Terrain / water
       |
       v
   EXGINE Runtime
       |
       v
   Renderer / World Streamer
```

`Runtime::load()` creates and binds one `GameplayWorld` to the same `PhysicsWorld`. `Runtime::update()` advances both. Character creation therefore does not create a private physics simulation.

## Data-driven boundary

Character movement parameters, appearance, item definitions, schedules and damage types are explicit data. Building and vehicle dimensions remain owned by their respective generators/configurations. Gameplay does not silently resize or relocate those assets.

## Fidelity boundary

This checkpoint establishes production-oriented gameplay contracts and integration, not a claim that the complete game is finished. Animation/skeletal runtime, navmesh/pathfinding, sophisticated locomotion over arbitrary stairs/slopes, vehicle possession/driving controllers, authoritative ranged-ballistics simulation, advanced melee hit volumes, clothing simulation, social AI, jobs, quest systems, needs behaviors, swimming hydrodynamics and full interaction routing are follow-on systems. They should consume these stable contracts rather than bypass them.

## Checkpoint validation

The dedicated Phase 11 suite verifies controller movement, inventories, item consumption/equipment, damage/death/recovery, proximity interactions, NPC schedules, simulation time, and Runtime-to-Physics-to-Gameplay lifecycle integration.

Definition of done: the complete C++ project builds, all existing suites remain passing, the dedicated Phase 10 physics suite remains passing, and the Phase 11 suite passes.

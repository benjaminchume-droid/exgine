# Phase 93 — Integrated Runtime Checkpoint

Phase 93 integrates the general-purpose ExAnimation and ExSound systems into the existing ShowcaseGame runtime path.

## Integrated path

```text
PlayableGame
   ├── Runtime
   │    ├── Physics / Gameplay
   │    ├── Skeleton + AnimationController
   │    ├── ExAnimation procedural graph generation
   │    ├── ExSound procedural synthesis
   │    └── AndroidAudioBackend
   ├── Weather / Particles
   ├── AudioWorld / spatial audio
   ├── UI
   └── Renderer / NextGen frame path
```

## Animation

The showcase selects an existing Player/NPC entity, attaches a real humanoid skeleton hierarchy, generates an animation clip from a procedural MotionGraph, registers it with Runtime, and plays/updates it from the game loop. Per-frame player displacement is converted into `MotionState` values (speed, vertical speed, grounded state and sprint state), providing the runtime feedback channel for procedural animation.

No motion-capture asset is required for this path.

## Audio

The showcase starts the Runtime audio backend when available and generates audio directly through ExSound. The update loop emits procedural gameplay-driven categories for locomotion, environment, impacts and vehicles. These are generated PCM buffers and submitted to the Runtime audio backend; authored audio samples are not required for the generated path.

The existing AudioWorld remains available for spatial cue processing alongside generated PCM.

## Integration test

`tests/showcase_integration_tests.cpp` now supplies a complete in-memory showcase project and exercises 180 update/render frames, save/restore, procedural animation updates and procedural audio event generation.

## Scope

This phase connects the systems; it does not turn event names into the underlying architecture. ExSound remains graph-based and ExAnimation remains motion-graph-based, so future game-specific behaviors can be constructed from the same primitives without adding one hard-coded subsystem per behavior.

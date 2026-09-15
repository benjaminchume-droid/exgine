# Phase 16 — Animation & Skeletal Runtime

Phase 16 adds a reusable animation layer for characters and other deformable assets.

## Delivered
- Hierarchical skeletons with stable bone IDs and parent validation.
- Local/model pose evaluation.
- Quaternion normalization, multiplication and spherical interpolation.
- Keyframed animation clips with per-bone tracks.
- Looping and non-looping playback, playback rate and deterministic clip sampling.
- Cross-fade blending between clips.
- Four-influence vertex skin weights and CPU skinning.
- Animation library and controller APIs independent of rendering backends.
- Deterministic humanoid skeleton plus idle/walk/run/jump starter clips.
- Regression coverage for hierarchy, interpolation, validation, skinning, playback and blending.

## Architecture
```text
Character / Asset
      |
   Skeleton
      |
  AnimationClip(s)
      |
 AnimationController
      |
  SkeletonPose
      |
 CPU Skinning / future GPU Skinning
      |
 Mesh / Renderer
```

The runtime deliberately keeps animation data separate from `Mesh`, physics and GPU ownership. This lets the same animation controller feed CPU tools now and GPU skinning later without changing the public geometry contract.

## Checkpoint
This phase is complete only when the complete CMake/CTest suite passes on the final `main` commit.

## Follow-ons
GPU skinning, animation importers, additive layers, animation state machines, inverse kinematics, root-motion extraction, compression, and motion matching build naturally on this contract and are intentionally subsequent phases/features rather than placeholders.

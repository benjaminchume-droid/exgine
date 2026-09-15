# Phase 3 — Geometry Foundation

## Goal

Phase 3 establishes EXGINE's reusable continuous-3D geometry layer. It deliberately does not use a block-world representation. Terrain, buildings, vehicles, bridges, characters, props, and future weapons will share the same mesh and material boundaries.

## Implemented

- `Vec2`, `Vec3`, vertex and indexed triangle mesh contracts.
- Reusable mesh assemblies made from named parts.
- Box, sphere, cylinder, and capsule generation.
- Deterministic procedural humanoid generation for both Player and NPC definitions.
- Appearance parameters for seed, height, body build, skin/hair/clothing material slots, watches, and backpacks.
- IR categories for Player, NPC, Bridge, and generic Prop so future source-language support can lower them into the same runtime.
- Geometry tests for topology validity and deterministic procedural character structure.

## Architecture rule

Geometry generation is data-oriented and independent of rendering. A generated mesh does not know about OpenGL, Vulkan, Metal, Direct3D, Android, or a specific shader. Rendering will consume the geometry through a renderer interface later.

The same rule applies to characters: a character is an assembly of addressable parts. Animation, clothing, materials, equipment, collision, and rendering can attach to those parts without duplicating the generator.

## Planned next layers

1. Mesh transforms, normals/tangents, bounds, LOD and mesh composition operations.
2. Material and procedural texture system.
3. Terrain, mountains, grasslands, water, vegetation and world streaming.
4. Renderer/GPU abstraction and physically based shader pipeline.
5. Building generation with interiors and enter/exit interaction.
6. Vehicle construction, complex curved bodies and vehicle physics.
7. Full character generation, clothing/accessories, animation and equipment.
8. Gameplay items and combat systems. Weapons will be represented as ordinary engine assets/components and governed by the game's gameplay rules rather than being baked into geometry.

## Checkpoint

A Phase 3 checkpoint requires the complete repository to build and test in CI, and requires the geometry APIs to remain connected to the existing Source → Lexer → Parser → AST → IR → Runtime architecture. No rendering or gameplay system is considered complete merely because a mesh can be generated in isolation.

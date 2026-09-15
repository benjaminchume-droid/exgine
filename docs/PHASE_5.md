# Phase 5 — World, Terrain & Water

Phase 5 replaces the original terrain prototype with a connected continuous-world subsystem. Terrain is sampled in world space and generated into deterministic reusable chunks. Biomes, water and vegetation are derived from the same world configuration and seed, while the runtime owns the world and chunk streamer.

## Implemented
- Seeded continuous terrain height sampling with broad, continental and detail scales.
- Configurable amplitude, chunk size, resolution, frequencies and sea level.
- Terrain chunk meshes with normals and UVs.
- Biome classification from elevation, moisture, temperature and slope.
- Ocean, beach, plains, forest, desert, rocky, snow and wetland categories.
- Water-depth sampling and submerged-cell water mesh generation.
- Water-body metadata with deterministic IDs and future flow/type extension points.
- Deterministic biome-aware vegetation instances with stable IDs, species, scale and rotation.
- Radius-based chunk streaming with deterministic load/unload behavior.
- Runtime creation of the world from compiled terrain properties.
- Runtime API for sampling and streaming the generated world.

## Connected pipeline

EXGINE source → Compiler → validated IR → Runtime::load → TerrainConfig → ProceduralWorld → WorldChunk → WorldStreamer → future scene/renderer.

Phase 4's procedural noise layer is reused by world generation, so the world subsystem does not introduce a second incompatible noise implementation.

## Determinism

The same world configuration, seed and chunk coordinate produce the same terrain vertices, indices, biome-driven vegetation placement and water metadata. This makes chunks safe to discard and regenerate during streaming without serializing generated geometry as the primary source of truth.

## Production boundary

Phase 5 establishes the production-facing world-generation contracts, but it does not claim final hydrodynamics, asynchronous worker scheduling, GPU terrain residency, erosion simulation, save persistence, swimming/submersion physics or final water shading. Those are deliberately owned by later phases so the world model stays independent from renderer and physics backends.

## Checkpoint

- The old terrain prototype is replaced, not duplicated.
- Terrain and vegetation generation are deterministic.
- Terrain chunks are valid triangle meshes and share world-space samples at boundaries.
- Water geometry is emitted only for submerged cells.
- Runtime owns the world and streamer.
- Complete build and test suite passes in CI.

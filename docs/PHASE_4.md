# Phase 4 — Materials & Procedural Textures

## Goal
Phase 4 establishes a deterministic, CPU-side material and texture pipeline that is directly usable by the runtime and ready to feed the future renderer. Materials describe physically meaningful surface properties; texture generation derives repeatable channel data from seeded procedural layers.

## Implemented
- PBR-oriented material contract: base color, roughness, metallic, specular, emission and opacity.
- Extensible material layers with generator name, scale, strength and seed.
- Material library with validation, replacement, lookup and removal.
- Real-world material presets including wood, steel, aluminium, copper, glass, rubber, water, grass, sand, concrete and snow.
- Texture2D CPU resource with explicit dimensions, channel count and floating-point storage.
- Texture sets for base color, roughness, metallic, normal, AO, emission, opacity and height.
- Deterministic value, Perlin-style gradient, simplex-compatible domain variation, FBM, ridged/turbulence and Voronoi-style procedural sampling.
- Seeded procedural material synthesis and channel generation.
- Material resource cache with deterministic generation keys.
- Runtime ownership of material definitions and generated material resources.
- End-to-end tests covering deterministic generation, validation, texture dimensions/channels, resource construction and runtime integration.

## Architecture
```text
Material Definition
       |
       +--> MaterialLibrary
       |
       +--> Procedural Layers
                |
                v
        Noise / FBM / Voronoi
                |
                v
          Texture Generator
                |
                v
       TextureSet (8 channels)
                |
                v
          ResourceCache
                |
                v
             Runtime
                |
                v
        Future GPU Renderer
```

## Production boundary
Phase 4 is deliberately renderer-independent. It does not pretend to be a GPU texture backend, image codec, PBR shader, virtual texture system or streaming system. Those belong to later renderer/platform phases. The contracts here make those systems consumers rather than owners of material semantics.

## Checkpoint requirements
1. Full repository/build graph inspection.
2. Clean CMake configure/build.
3. Complete test suite passes.
4. Material definitions reject invalid physical ranges.
5. Same material + same seed + same settings produces the same data.
6. Resource generation is keyed from material and generation settings.
7. Runtime owns material resources through the same resource boundary as geometry.
8. No Phase 4 code bypasses the compiler/runtime architecture.

# EXGINE

**EXGINE** is a native, mobile-first 3D game engine built around a code-to-world pipeline. Game intent is compiled into validated EXGINE IR, instantiated by a native runtime, and presented through shared world, simulation, rendering, audio, UI, and platform systems.

EXGINE is being built as a complete engine rather than a collection of disconnected demos. Each major milestone ends in a repository-wide checkpoint: the complete source tree is inspected, dependencies are traced, APIs are checked, the full project is built and tested, integration is exercised, and temporary duplicate paths are removed.

## Architecture

```text
Game source / EXGINE DSL
          |
       Front ends
          |
         AST
          |
 Semantic validation
          |
       EXGINE IR
          |
      Native runtime
    /       |        \
 World   Simulation  Resources
    |         |          |
 Scene    Physics/AI   Assets
    |         |          |
 Lighting Animation   GPU Residency
    \         |          /
         Renderer
             |
      Android / Desktop
```

## High-fidelity target

EXGINE is intentionally **not** a voxel/Minecraft-style renderer. The target is a continuous 3D world with realistic materials and lighting, detailed terrain, vegetation, water, enterable buildings, physically simulated vehicles, characters/NPCs, weather, audio, UI, streaming, persistence, and scalable rendering paths for mobile and higher-end hardware.

The production track now extends through Phase 70. Phases 61 onward deepen the integrated high-fidelity world and presentation pipeline rather than merely adding API surfaces. AAA quality remains an acceptance target measured on the running game: code existing by itself does not constitute completion.

## Phases

The detailed connected roadmap is maintained in [`docs/ROADMAP.md`](docs/ROADMAP.md), currently extending through the Phase 70 AAA acceptance track.

Core milestones include:

- continuous geometry, reusable object generation, and procedural materials
- terrain, biomes, water, vegetation, buildings, vehicles and open-world streaming
- rigid-body physics, character gameplay, animation and GPU skinning
- OpenGL ES 3.1 rendering and Android EGL/mobile runtime integration
- asset packaging/import, glTF/GLB ingestion, GPU asset residency and asynchronous streaming
- game projects, environment/time/weather, navigation, audio, UI, persistence and Android packaging
- high-fidelity world orchestration and rendering/simulation acceptance systems

## Build

EXGINE uses C++20 and CMake:

```bash
cmake -S . -B build
cmake --build build
ctest --test-dir build --output-on-failure
```

The repository's CI performs the same configure/build/test gate on `main`.

## Repository layout

```text
include/exgine/   Public engine API
src/              Engine implementation
examples/         Executable examples
tests/            Automated verification
docs/             Architecture, roadmap and checkpoint contracts
```

## License

EXGINE is licensed under the **Apache License 2.0**. See [`LICENSE`](LICENSE).

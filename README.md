# EXGINE

**EXGINE** is a native, mobile-first game engine designed around a code-to-world pipeline: game intent is described at a high level, transformed into a validated intermediate representation, optimized, and executed by a lightweight runtime.

EXGINE is being built as a complete engine, not as a collection of disconnected demos. Every major phase ends with a structural checkpoint that reviews the whole repository and verifies that the new subsystem is correctly connected to the architecture.

## Vision

A developer should be able to describe game-world intent without manually implementing every low-level representation.

For example:

```text
game "MyWorld" {
    world {
        terrain = procedural
        terrain_height = 20
        trees = procedural
        exploration = infinite
    }
}
```

The long-term pipeline is:

```text
Game source / EXGINE DSL / language adapters
                    |
               Front ends
                    |
                  AST
                    |
          Semantic validation
                    |
                EXGINE IR
                    |
                Optimizer
                    |
             Native runtime
          /        |        \
      World     Physics    Scene
          \        |        /
              Renderer
                    |
          Android / Desktop
```

## Development model

EXGINE is developed in large, connected phases rather than tiny feature drops.

Each phase has a **checkpoint**. A checkpoint is not a progress counter. It is a repository-wide engineering gate:

1. Inspect the complete source tree.
2. Trace data flow and dependencies across subsystems.
3. Check public APIs and ownership boundaries.
4. Build and test the complete project.
5. Check that new code is used by the systems around it.
6. Remove temporary architecture and duplicated paths.
7. Record the result before starting the next phase.

A phase is complete only when its checkpoint passes.

## Phases

| Phase | Goal | Status |
|---|---|---|
| 0 | Architecture and production-grade foundation | In progress |
| 1 | EXGINE language: lexer, parser, AST and semantic validation | Planned |
| 2 | Runtime and scene architecture | Planned |
| 3 | Procedural world generation and streaming | Planned |
| 4 | Procedural objects and geometry | Planned |
| 5 | 3D renderer and GPU abstraction | Planned |
| 6 | Physics and gameplay systems | Planned |
| 7 | Asset and resource pipeline | Planned |
| 8 | Android/mobile runtime | Planned |
| 9 | Automatic performance system | Planned |
| 10 | Editor and production tooling | Planned |

See [`docs/ROADMAP.md`](docs/ROADMAP.md) for the full plan and [`docs/PHASE_0.md`](docs/PHASE_0.md) for the current foundation checkpoint.

## Current architecture

Phase 0 establishes these boundaries:

```text
include/exgine/   Public engine API
src/              Engine implementation
examples/         Executable examples
 tests/            Automated verification
 docs/             Architecture and development contracts
```

The core is C++20. Platform-specific rendering and mobile code will be introduced behind explicit interfaces instead of leaking platform details into the core.

## Design goals

- Native C++ core
- Small and predictable memory footprint
- Mobile-first performance
- Deterministic procedural generation
- Chunked world streaming
- Automatic LOD and scalable rendering quality
- Declarative game descriptions
- Shared IR for multiple source languages
- Android and desktop targets
- Clear subsystem boundaries
- Testable, incremental architecture without disposable placeholders

## Non-goals for the early engine

EXGINE will not initially attempt to support every programming language, photorealistic AAA rendering, or a full editor. Those systems will be added after the core architecture can support them cleanly.

## Building

EXGINE uses CMake and C++20.

```bash
cmake -S . -B build
cmake --build build
```

The repository must remain buildable as the engine grows.

## License

EXGINE is licensed under the **Apache License 2.0**. See [`LICENSE`](LICENSE).

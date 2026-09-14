# Phase 0 — Foundation

## Objective

Create a foundation strong enough that later EXGINE systems can be built on one architecture rather than repeatedly replacing prototypes.

## Scope

Phase 0 establishes:

- C++20 build configuration
- public API boundary under `include/exgine/`
- engine version identity
- structured diagnostic primitives
- stable core type conventions
- deterministic procedural-world foundation
- automated tests
- repository documentation and licensing
- a clear path for future platform/backend code

Phase 0 does **not** implement the language parser, renderer, physics engine, Android backend, or editor. Those systems belong to later phases and must consume the contracts established here.

## Required repository shape

```text
exgine/
├── CMakeLists.txt
├── LICENSE
├── README.md
├── docs/
│   ├── ARCHITECTURE.md
│   ├── PHASE_0.md
│   └── ROADMAP.md
├── include/exgine/
├── src/
├── tests/
└── examples/
```

## Architectural checkpoint

Before Phase 1 begins, verify all of the following:

- The complete tree has been inspected.
- The project builds from a clean directory with CMake.
- Tests exercise public behavior rather than implementation details.
- Public headers do not accidentally depend on source-only paths.
- Runtime/world code has no dependency on the future parser or renderer.
- IR is a description format, not a second runtime object system.
- Procedural generation is deterministic for identical configuration.
- No raw owning pointers are required by the foundation.
- Version information has one authoritative definition.
- Diagnostics have a defined representation that later parsers can reuse.
- Documentation describes the code that actually exists.
- No placeholder API is introduced merely to make a future phase look complete.

## Exit criteria

Phase 0 passes only when the foundation can be used as the base for Phase 1 without architectural rewrites caused by missing ownership, diagnostics, versioning, testing, or build contracts.

The next phase begins with the language frontend and must connect directly to the existing IR rather than creating a parallel representation.

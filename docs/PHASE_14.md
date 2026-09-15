# Phase 14 — Editor & Tools

## Goal

Provide a production-grade editor-core layer for authoring EXGINE worlds without coupling the authoring model to a particular desktop UI toolkit or GPU backend.

## Implemented

- Stable editor project/node/asset data model.
- Hierarchical scene authoring with parent changes and cycle protection.
- Transform, active-state, naming, and arbitrary property editing.
- Asset catalog for meshes, textures, materials, audio, scripts, and scenes.
- Deterministic human-readable `.exproj` persistence with format versioning.
- EXGINE source emission from the authored hierarchy.
- Runtime preview integration through the existing `Runtime` and IR boundary.
- Selection state and bounded undo/redo history using complete project snapshots.
- Viewport state for camera, grid, gizmos, and wireframe preferences.
- Full Phase 14 regression test coverage.

## Architecture

```text
EditorSession
    |
    +-- EditorProject
    |     +-- Scene nodes
    |     +-- Properties
    |     +-- Asset catalog
    |
    +-- EditorViewport
    |
    +-- Undo / Redo
    |
    +-- Source / Project serialization
    |
    +-- Runtime Preview
              |
              +-- EXGINE IR
              +-- Runtime
              +-- Scene / Physics / Gameplay / World
```

The editor owns authoring state. Runtime systems remain the execution layer. This keeps the editor usable with future Vulkan/OpenGL ES/Metal/D3D12 frontends and prevents UI concerns from leaking into engine subsystems.

## Checkpoint

- Editor state has explicit ownership and validation boundaries.
- Hierarchy mutations reject invalid parents and cycles.
- Project loading validates the complete document before replacing the live document.
- Persistence is versioned so incompatible project formats can be rejected safely.
- Undo/redo is bounded to 128 complete snapshots.
- Runtime preview is built through the existing IR/runtime boundary.
- CI must compile and run the complete existing suite plus `exgine_editor_phase14_tests` before Phase 14 is considered complete.

## Deliberate boundary

Phase 14 does not pretend to be a native desktop application. The UI shell, GPU viewport, drag/drop asset browser, model importers, and platform windowing belong above this editor core and will connect to it in the following rendering/tooling work. No fake viewport or placeholder renderer is introduced.

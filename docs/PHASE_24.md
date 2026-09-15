# Phase 24 — GPU Asset Residency and Streaming

Phase 24 adds persistent OpenGL ES mesh residency with explicit memory budgeting and least-recently-used-style eviction. It is an optional GPU resource layer; it does not replace `OpenGLESRenderer` and does not own runtime entities or scene nodes.

## Delivered

- Persistent VAO/VBO/IBO allocation for imported/static `Mesh` resources.
- Configurable byte and mesh-count budgets.
- Frame-based touch timestamps.
- Automatic eviction of the least recently used resident mesh when capacity is required.
- Explicit per-asset eviction and complete teardown.
- Upload/rejection/eviction telemetry.
- Host-safe behavior when an OpenGL ES API table is unavailable.

## Ownership

```text
Asset / Runtime mesh
        |
 GPU residency manager
   /      |       \
 VAO     VBO      IBO

Runtime entity -> SceneGraph -> RenderFrame -> OpenGLESRenderer
```

The residency manager owns only GPU mesh objects. Android EGL, runtime entities, scene nodes, materials and package bytes remain owned by their existing subsystems.

## Checkpoint

The full repository must build, Phase 24 tests must verify configuration and host-safe failure behavior, and GPU resources must be released deterministically on eviction/destruction.

# Phase 22 — Runtime Asset Instancing

Phase 22 connects imported mesh assets to live runtime entities.

## Delivered

- `AssetRuntime` stores decoded mesh instances by stable `AssetId`.
- URI and ID lookup reuse Phase 20 identity.
- Imported meshes become normal `MeshAssembly` geometry.
- Runtime attachment calls the existing `Runtime::attach_geometry()` path.
- Material-slot overrides are applied at instance attachment time.
- Repeated imports of the same URI reuse the existing decoded asset.

## Architecture

```text
OBJ / future glTF importer
          |
     ImportedMesh
          |
      AssetRuntime
          |
    MeshAssembly cache
          |
 Runtime::attach_geometry()
          |
 Entity -> SceneGraph -> Renderer -> GPU
```

## Checkpoint

An authored mesh can be imported, registered once, instantiated on a live entity and rendered through the existing runtime ownership path without duplicating world or renderer state.

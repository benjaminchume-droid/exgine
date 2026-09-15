# Phase 20 — Asset Pipeline and Runtime Packaging

Phase 20 establishes the portable asset boundary needed to move authored content into EXGINE without coupling the engine core to a filesystem, Android storage API, or editor-specific representation.

## Delivered

- `AssetType` covers mesh, texture, material, skeleton, animation, scene, audio, script and opaque binary resources.
- `AssetRecord` carries a stable content-derived ID, canonical URI, raw payload and dependency IDs.
- `make_asset_id()` provides deterministic content-addressed identity from URI and bytes.
- `pack_assets()` and `unpack_assets()` implement a versioned little-endian `XGPK` package format.
- `AssetDatabase` provides duplicate-safe registration, URI/ID lookup, dependency validation, dependency-first load ordering, removal and package round trips.
- Missing dependencies, duplicate IDs, self-dependencies, dependency cycles and malformed/trailing package bytes are rejected.
- The implementation is platform-independent and uses no second resource ownership model.

## Architecture

```text
Editor / importer / downloader
            |
       AssetRecord
            |
      AssetDatabase
       /         \
 dependency DAG   package
       |             |
   load_order     XGPK bytes
       |             |
       +-------> Resource systems
                    |
          Mesh / Material / Texture
          Skeleton / Animation / Scene
                    |
                 Runtime
```

## Package format

The package starts with a 16-byte header: magic, format version, asset count and a reserved field. Each record contains its type, 64-bit asset ID, URI length/string, payload size, dependency count, payload bytes and dependency IDs. The format is deterministic for a deterministic asset ordering and deliberately carries no platform pointers or renderer handles.

## Ownership rules

The asset database owns package metadata and bytes. It does not own GPU objects, EGL surfaces, Android windows, scene nodes or runtime entities. Runtime resource caches remain responsible for decoded/render-ready representations. This keeps import/packaging separate from execution while allowing both to share stable asset identity.

## Checkpoint

The phase is complete when `exgine_core` builds the new package implementation, the repository test suite covers stable identity, dependency ordering, round trips, malformed packages and cycle rejection, and no existing renderer/runtime ownership contract is duplicated.

## Next extension

The next importer phase can add concrete decoders such as glTF/GLB while targeting `AssetRecord` and the existing geometry/material/skeleton/animation resource contracts established by earlier phases.

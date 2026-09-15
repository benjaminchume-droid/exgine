# Phase 21 — Asset Import Framework

Phase 21 turns packaged asset bytes into decoded engine geometry without coupling import code to Android, the editor or the renderer.

## Delivered

- `AssetImporter` interface for format-specific decoders.
- OBJ format detection and import.
- OBJ positions, texture coordinates and normals.
- Positive and negative OBJ indices.
- Polygon triangulation using a deterministic fan.
- Normal generation when normals are not supplied.
- Content-derived asset IDs shared with Phase 20.

## Checkpoint

A supported authored mesh can be decoded into the existing `Mesh` contract with deterministic identity and no renderer-specific ownership.

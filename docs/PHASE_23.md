# Phase 23 — glTF / GLB Ingestion

Phase 23 adds a memory-based glTF 2.0 importer on top of the Phase 21 importer contract. glTF remains the interchange boundary; decoded geometry and material factors are converted into existing EXGINE `Mesh`, `Material` and imported-node structures.

## Delivered

- glTF 2.0 JSON parsing without a third-party runtime dependency.
- GLB v2 header and JSON chunk decoding.
- Embedded base64 buffer ingestion.
- POSITION, NORMAL and TEXCOORD_0 accessors.
- Indexed and non-indexed triangle primitives.
- Basic PBR metallic/roughness material factors and opacity.
- Node names, mesh references, translation and scale.
- Generated normals when source normals are absent.
- Deterministic `AssetId` generation for imported primitives.
- Format detection for `.gltf`, `.glb` and GLB magic.

## Intentional boundary

The Phase 23 memory importer rejects external file references rather than silently depending on a filesystem. Image decoding, animation/skeleton import, skins, morph targets, sparse accessors and extensions remain future extensions over this importer contract.

## Architecture

```text
GLTF / GLB bytes
       |
   Import parser
       |
 Imported meshes/materials/nodes
       |
 AssetId + existing geometry/material contracts
       |
 Phase 22 AssetRuntime / Runtime
       |
 SceneGraph -> Renderer -> GPU
```

## Checkpoint

The full repository must build and the Phase 23 tests must verify format detection, valid mesh/material/node extraction and malformed/version rejection.

## Reference

EXGINE follows the Khronos glTF 2.0 interchange model and its PBR metallic/roughness material semantics. citeturn959773search0turn959773search17

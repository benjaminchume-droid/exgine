# `.exg` compiled content format

EXGINE already has a dependency-aware binary asset container. The container header uses `XGPK` and version `1`; the file extension is intentionally `.exg` when used as game content.

An `.exg` package can contain:

- mesh payloads
- textures
- materials
- skeletons
- animations
- scenes
- audio
- scripts
- arbitrary binary payloads

Each record has a stable content-derived `AssetId`, a URI, raw payload bytes, and dependency IDs. The runtime can therefore load a base world without regenerating its authored assets and stream additional records as the player moves.

## Authoring model

```text
source assets
   ↓
importers / processors
   ↓
EXG asset records
   ↓
XGPK container (`world.exg`)
   ↓
APK / downloadable content
   ↓
AssetDatabase
   ↓
AssetRuntime
   ↓
GPU residency
   ↓
Scene entities / render graph
```

The `.exg` file is **not** intended to be a screenshot, a fake configuration, or a predetermined game path. It is a compiled content container containing real asset payloads and relationships.

The runtime can then combine those authored records with procedural world generation, streaming and gameplay systems. This means a developer can ship a base city/track/world and let EXGINE continue generating or streaming compatible content rather than requiring the entire world to exist as one monolithic file or one RAM allocation.

`tools/exgpack` is the first concrete command-line compiler for this container.

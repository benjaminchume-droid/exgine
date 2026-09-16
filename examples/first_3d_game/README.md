# EXGINE: First Light

The first game authored on top of EXGINE's completed runtime stack.

## Game loop

First Light is a third-person/first-person exploration test world built around the engine primitives rather than a one-off renderer demo:

- continuous procedural terrain
- player and NPC entities
- authored building/outpost
- drivable vehicle entity
- dynamic physics crate
- procedural rain/weather
- directional + point lighting
- procedural character animation
- spatial/procedural audio
- HUD/crosshair and health presentation
- save/restore through the existing playable runtime
- OpenGL ES presentation on Android

The `.exg` manifest is the project-level artifact consumed by the EXGINE game runtime. The scene remains data-authored so the engine is exercising its general scene/runtime contracts rather than hard-coded game geometry.

## Android controls

The Android shell exposes EXGINE's native touch/key event queue. The current game shell is intentionally engine-owned; input is kept outside the renderer so a later control layer can add virtual sticks, camera gestures, keyboard and controller mappings without changing the rendering backend.

## Build

Desktop engine/game tests are built through the root CMake project. The Android artifact is built by `.github/workflows/build-first-light-apk.yml`.

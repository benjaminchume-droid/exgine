# Phase 25 — Universal Game Project and Runtime

Phase 25 turns EXGINE into an application-ready game runtime without introducing game-specific engine branches. A project manifest describes the game name/version, startup scene, tick rate, asset references, runtime settings and calendar configuration.

## Project layout

```text
Game/
  project.exg
  scenes/
  assets/
  scripts/
  materials/
  worlds/
  game/
```

The directories are conventions for project organization; the engine does not require fixed content types beyond the typed asset references in the manifest.

## Runtime

`GameRuntime` owns one existing `Runtime` plus `EnvironmentSystem`, scene definitions, variables and save-state translation. Scene loading uses an application-provided callback, so storage can be a filesystem, package, network store or editor database.

The core does not hardcode player types, maps, quests, items or other game-specific rules.

## Manifest format

`project.exg` is intentionally plain text and deterministic:

```text
name = My Game
version = 1.0
startup_scene = Main
tick_rate = 60
seconds_per_day = 1200
days_per_year = 360
scene = Main
asset = scene,MainScene,scenes/main.scene
setting.language = en
```

Seasons use:

```text
season = Spring,0,90,0.05,1.1,1.0
```

## Checkpoint

Project parsing/serialization, scene activation, runtime ticking, variables, save/restore and generic scene loading must pass the Phase 25 regression suite.

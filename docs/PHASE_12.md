# Phase 12 — Open World Streaming

## Status
Core checkpoint implemented and integrated into Runtime.

Phase 12 makes EXGINE's continuous world practical at runtime scale without changing the procedural world contract. It adds a bounded, deterministic streaming coordinator, level-of-detail selection, load/unload budgets, hysteresis, and a floating-origin contract for large worlds.

## Architecture
```text
ProceduralWorld
      |
      +--> OpenWorldStreamer
      |       +-- deterministic chunk priority
      |       +-- load/prefetch/unload radii
      |       +-- LOD selection
      |       +-- generation/unload budgets
      |       +-- stable chunk keys
      |
      +--> WorldChunk

Absolute world coordinates
      |
      v
FloatingOrigin
      |
      +-- grid-snapped origin
      +-- revisioned origin shifts
      +-- explicit shift delta
```

## Streaming guarantees
- No hard-coded terrain layout, building layout, vehicle dimensions, or gameplay placement.
- The terrain seed and `TerrainConfig` remain the source of procedural truth.
- Chunk priority is deterministic: nearest chunks first, then stable coordinate ordering.
- Streaming has explicit memory/work budgets instead of unbounded generation.
- Prefetch and unload radii are separate, preventing rapid load/unload thrashing while moving near a boundary.
- LOD is deterministic from chunk distance and derives its mesh resolution from the world configuration.
- Repeated generation with the same world configuration produces equivalent chunk topology.
- Invalid/non-finite focus positions are rejected rather than poisoning the streamer state.

## Public API
`OpenWorldStreamingConfig` controls active, prefetch and unload radii, maximum resident chunks, generation/unload budgets, and LOD thresholds.

`OpenWorldStreamer` exposes deterministic update, lookup, configuration, resident chunks and streaming statistics. `Runtime::stream_open_world()` and `Runtime::open_world_streamer()` expose it without removing the earlier low-level `WorldStreamer` API.

`FloatingOrigin` provides a separate large-world coordinate-management contract. It does not silently mutate physics, scene, or renderer coordinates; consumers receive an explicit `OriginShift` containing old origin, new origin, delta and revision. This keeps rebasing safe across systems that may hold world-space state.

## Production boundary
This checkpoint is a bounded CPU streaming coordinator. It deliberately does not claim asynchronous job scheduling, GPU residency, disk-backed world packs, network replication, or final renderer-origin rebasing. Those require platform/resource-system work and belong to later optimization/tooling phases rather than hidden behavior in the world generator.

## Checkpoint validation
- Deterministic generation and LOD selection.
- Resident-chunk budget enforcement.
- Streaming generation and eviction budgets.
- Hysteresis behavior.
- Runtime integration contract.
- Floating-origin threshold, snapping, revision and reset semantics.
- Full existing core, physics, Phase 10 and Phase 11 test suites remain part of CI.

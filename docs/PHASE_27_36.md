# EXGINE Phases 27–36 — Game Production Vertical

This milestone connects the engine from project loading to a persistent playable-game runtime without game-specific C++ branches.

- Phase 27: image decoding and glTF texture-resource binding contracts.
- Phase 28: prioritized asynchronous asset streaming with cancellation and telemetry.
- Phase 29: deterministic grid-backed navigation mesh and A* paths.
- Phase 30: data-driven animation state machines and transition parameters.
- Phase 31: environment-driven sky, weather, precipitation, cloud, fog and snow-cover state.
- Phase 32: backend-neutral spatial audio source/listener graph.
- Phase 33: backend-neutral UI model for HUD, menus, inventory and dialogue widgets.
- Phase 34: versioned binary save container for runtime variables and extension blobs.
- Phase 35: integrated `GameSession` project open/update/save/restore lifecycle.
- Phase 36: Android Gradle application packaging around the existing NativeActivity/EGL target.

The contracts intentionally expose loaders/backends instead of hardcoding a filesystem, audio device, UI renderer, or image library. The Android Gradle module points at the existing `platform/android/CMakeLists.txt` target so the native renderer/runtime remains the single engine implementation.

The vehicle-dynamics controller is provided by `vehicle_dynamics.hpp/.cpp` and connects throttle, brake, steering, wheel setup and possession to the existing physics world.

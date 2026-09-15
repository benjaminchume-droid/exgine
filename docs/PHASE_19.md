# Phase 19 — Android Mobile Runtime Integration

Phase 19 turns the Phase 18 Android presentation sample into a reusable mobile runtime boundary. It does not create a second world, scene or rendering path. Lifecycle, input and frame timing remain adapters around the existing EXGINE runtime and renderer contracts.

## Delivered

- Cross-platform `MobileRuntimeBridge` with explicit Android lifecycle states and surface availability.
- Bounded multi-touch/key input queue with deterministic overflow behavior and active-touch tracking.
- Android NDK input translation for pointer and key events through `android_native_app_glue`.
- Pause/resume-aware mobile frame timing with bounded frame deltas and a configurable 15–120 FPS target.
- Android NativeActivity lifecycle wiring for start/resume/pause/stop, surface attach/detach/resize and destroy handling.
- Desktop CI coverage for the mobile lifecycle/input/frame-pacing contract.

## Architecture

```text
Android lifecycle/input
        ↓
MobileRuntimeBridge
   ↙           ↘
InputQueue   FramePacer
        ↓       ↓
   Game/Application layer
             ↓
          Runtime
             ↓
        Scene/Renderer
             ↓
       AndroidEglPresenter
             ↓
        OpenGLESRenderer
```

The bridge carries platform events into engine-native data types. It does not own entities, physics, scenes, resources or rendering state.

## Lifecycle

`Created → Started → Resumed → Paused → Stopped → Destroyed` is modeled explicitly. A surface is an independent renderability condition, so the runtime can remain alive while the Android window is temporarily unavailable.

A frame is renderable only when the application is resumed and an EGL surface is available. Surface loss therefore stops presentation without destroying engine state.

## Input

`MobileInputEvent` represents touch down/move/up/cancel and key down/up. Touch events carry pointer identity, coordinates and pressure. The queue has a fixed capacity of 256 events and tracks up to 16 simultaneous touch pointers.

When the queue is full, the oldest event is discarded and the newest event is retained. This bounds memory and prevents an input flood from blocking the rendering loop.

## Frame timing

`MobileFramePacer` clamps elapsed frame time to 250 ms so an app resume or scheduling stall cannot inject an unbounded simulation step. The first frame and paused frames report zero delta. Target FPS is clamped to 15–120; presentation synchronization remains owned by the Phase 18 EGL swap interval.

## Intentional boundaries

Phase 19 does not invent a mobile gameplay runtime. The application still decides how touch/key events map to gameplay, UI, camera movement or controls. It also does not add Java/Kotlin rendering, sensor APIs, a parallel event bus, or an alternative renderer.

## Checkpoint

The phase is complete when the desktop repository suite is green and the Android NativeActivity target connects lifecycle, input and frame timing to the same `AndroidEglPresenter` and EXGINE render-frame contract established by Phases 15–18.

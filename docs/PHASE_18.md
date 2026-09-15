# Phase 18 — Android EGL + Surface + Swapchain + Real Mobile Presentation

## Scope

Phase 18 connects the Phase 15 OpenGL ES renderer to an actual Android window surface. The engine now owns the EGL lifecycle needed to turn a validated `RenderFrame` into visible Android presentation buffers.

## Delivered

- `include/exgine/android.hpp` defines a platform-safe Android EGL presentation boundary.
- `src/android.cpp` implements the Android path with `EGLDisplay`, `EGLContext`, `EGLSurface` and `ANativeWindow` ownership.
- EGL chooses an OpenGL ES 3.x window configuration with explicit color, depth and stencil requirements.
- The presentation context requests OpenGL ES 3.x and is required to run the Phase 15/17 renderer path.
- All Phase 15 OpenGL ES function pointers are resolved from the active Android EGL context.
- The existing `OpenGLESRenderer` remains the submission owner; Phase 18 only supplies the real context/surface and presentation lifecycle.
- Surface dimensions are queried every presentation cycle so orientation/window-size changes update the render target.
- `eglSwapInterval(1)` enables synchronized buffer presentation.
- `eglSwapBuffers` completes real buffer presentation to Android's native window queue.
- `ANativeWindow_acquire/release` prevents use-after-free across surface callbacks.
- EGL/context/surface destruction is ordered and idempotent.
- Failed initialization tears down partially-created resources without leaving a live native window reference.
- Non-Android desktop builds receive a deterministic unsupported-platform implementation and remain CI-testable.
- `platform/android/` contains a native NDK build target, `NativeActivity` manifest and native presentation loop.

## Data flow

```text
Android Surface
      |
      v
ANativeWindow
      |
      v
EGLDisplay -> EGLConfig -> EGLContext
      |
      v
EGLSurface (window buffer queue)
      |
      v
OpenGLESApi procedure loading
      |
      v
OpenGLESRenderer
      |
      v
RenderFrame
      |
      v
eglSwapBuffers()
      |
      v
Android display
```

## Build integration

The root CMake project exposes `EXGINE_BUILD_DEMO` and `EXGINE_BUILD_TESTS` so an Android application can consume the engine without building desktop examples/tests.

The native sample under `platform/android/` expects an Android NDK toolchain and uses the NDK `android_native_app_glue` implementation. The manifest requires OpenGL ES 3.1 and launches Android's `NativeActivity` into the `exgine_android` shared library.

Example native configuration for an NDK installation:

```bash
cmake -S platform/android -B build-android \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26
cmake --build build-android --parallel
```

Packaging into an APK remains the application/build-system responsibility; the native target and manifest supplied here are the engine-facing Android presentation slice.

## Intentional boundaries

Phase 18 does not add a second renderer, a Java/Kotlin rendering stack, or an alternate swapchain abstraction. EGL's Android window surface and buffer queue are the platform presentation layer, while `OpenGLESRenderer` remains the single GPU submission path.

Future Android work can build on this seam for input, lifecycle-aware render pausing, surface-loss recovery policy, frame pacing telemetry, Android asset loading, and application-level APK packaging.

## Checkpoint

The phase is complete only when the desktop contract tests pass, the existing Phase 0–17 tests remain green, and the Android-specific CMake target consumes the same `exgine_core` + OpenGL ES presentation boundary without duplicating rendering logic.

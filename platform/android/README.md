# EXGINE Android Platform Sample

Phase 18 establishes EGL + `ANativeWindow` presentation. Phase 19 adds the mobile runtime adapter around that presenter.

Build with the Android NDK CMake toolchain:

```bash
cmake -S platform/android -B build-android \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26
cmake --build build-android --parallel
```

The sample application uses the same `exgine_core` target as the engine. Its NativeActivity now handles Android start/resume/pause/stop, window attach/detach/resize, pointer/key input and frame timing through `MobileRuntimeBridge` before presenting through `AndroidEglPresenter`.

Input is intentionally exposed as engine-native events. A real game/application layer should consume `MobileInputQueue` and map those events to gameplay, UI, camera and controls instead of adding platform-specific state to the renderer.

The sample still presents a clear render frame. It is an engine/platform integration target, not a finished game shell or APK packaging project.

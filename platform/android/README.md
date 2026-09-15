# EXGINE Android Presentation

This directory is the native Android presentation slice for EXGINE Phase 18.

## Components

`CMakeLists.txt` builds the existing `exgine_core` library plus the Android `NativeActivity` shared library.

`src/main/cpp/native_activity.cpp` receives Android lifecycle commands, attaches/detaches the engine's `AndroidEglPresenter` to the current `ANativeWindow`, responds to surface-size changes, and drives a render/present loop.

`AndroidManifest.xml` declares a native activity and requires OpenGL ES 3.1.

## Native build

Configure with an Android NDK CMake toolchain:

```bash
cmake -S platform/android -B build-android \
  -DCMAKE_TOOLCHAIN_FILE="$ANDROID_NDK/build/cmake/android.toolchain.cmake" \
  -DANDROID_ABI=arm64-v8a \
  -DANDROID_PLATFORM=android-26
cmake --build build-android --parallel
```

The target name is `exgine_android`. An Android application packaging layer (Gradle/Android Studio or another APK packager) should include the generated shared library and the supplied manifest.

## Runtime ownership

The NativeActivity never creates a second renderer. It creates an `AndroidEglPresenter`, which creates the EGL display/context/window surface and then supplies a resolved `OpenGLESApi` to the existing Phase 15/17 `OpenGLESRenderer`.

The engine therefore keeps one rendering path:

```text
Runtime -> Renderer -> RenderFrame -> OpenGLESRenderer
                                           |
                                  AndroidEglPresenter
                                  /      |       \
                              EGLDisplay Context Surface
                                           |
                                      SwapBuffers
```

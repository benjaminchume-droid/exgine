#include "exgine/android.hpp"

#include <algorithm>
#include <cstdio>
#include <new>

#if defined(__ANDROID__)
#include <EGL/egl.h>
#include <GLES3/gl31.h>
#include <android/native_window.h>
#endif

namespace exgine {

bool AndroidEglConfig::valid() const noexcept {
    return red_bits >= 0 && green_bits >= 0 && blue_bits >= 0 && alpha_bits >= 0 &&
           depth_bits >= 0 && stencil_bits >= 0 && gl_major >= 2 && gl_minor >= 0 &&
           swap_interval >= 0 && swap_interval <= 1;
}

bool AndroidPresentationStats::valid() const noexcept {
    return width >= 0 && height >= 0 && (!attached || (width > 0 && height > 0));
}

AndroidEglPresenter::AndroidEglPresenter(AndroidEglConfig config) noexcept : config_(config) {
    if (!config_.valid()) {
        config_ = {};
    }
}

AndroidEglPresenter::~AndroidEglPresenter() {
    detach();
}

bool AndroidEglPresenter::ready() const noexcept {
#if defined(__ANDROID__)
    return stats_.attached && display_ != nullptr && surface_ != nullptr && context_ != nullptr;
#else
    return false;
#endif
}

#if defined(__ANDROID__)
namespace {

template <typename T>
T load_gl_proc(const char* name) noexcept {
    return reinterpret_cast<T>(eglGetProcAddress(name));
}

void* as_void(EGLDisplay value) noexcept { return reinterpret_cast<void*>(value); }
void* as_void(EGLSurface value) noexcept { return reinterpret_cast<void*>(value); }
void* as_void(EGLContext value) noexcept { return reinterpret_cast<void*>(value); }
EGLDisplay as_display(void* value) noexcept { return reinterpret_cast<EGLDisplay>(value); }
EGLSurface as_surface(void* value) noexcept { return reinterpret_cast<EGLSurface>(value); }
EGLContext as_context(void* value) noexcept { return reinterpret_cast<EGLContext>(value); }

std::string egl_error(const char* prefix) {
    const EGLint error = eglGetError();
    char buffer[80]{};
    std::snprintf(buffer, sizeof(buffer), "%s (EGL error 0x%04x)", prefix, error);
    return buffer;
}

OpenGLESApi make_android_gles_api() noexcept {
    OpenGLESApi api;
    api.Clear = load_gl_proc<PFNGLCLEARPROC>("glClear");
    api.ClearColor = load_gl_proc<PFNGLCLEARCOLORPROC>("glClearColor");
    api.Viewport = load_gl_proc<PFNGLVIEWPORTPROC>("glViewport");
    api.Enable = load_gl_proc<PFNGLENABLEPROC>("glEnable");
    api.Disable = load_gl_proc<PFNGLDISABLEPROC>("glDisable");
    api.DepthFunc = load_gl_proc<PFNGLDEPTHFUNCPROC>("glDepthFunc");
    api.BlendFunc = load_gl_proc<PFNGLBLENDFUNCPROC>("glBlendFunc");
    api.CreateShader = load_gl_proc<PFNGLCREATESHADERPROC>("glCreateShader");
    api.ShaderSource = load_gl_proc<PFNGLSHADERSOURCEPROC>("glShaderSource");
    api.CompileShader = load_gl_proc<PFNGLCOMPILESHADERPROC>("glCompileShader");
    api.GetShaderiv = load_gl_proc<PFNGLGETSHADERIVPROC>("glGetShaderiv");
    api.GetShaderInfoLog = load_gl_proc<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");
    api.DeleteShader = load_gl_proc<PFNGLDELETESHADERPROC>("glDeleteShader");
    api.CreateProgram = load_gl_proc<PFNGLCREATEPROGRAMPROC>("glCreateProgram");
    api.AttachShader = load_gl_proc<PFNGLATTACHSHADERPROC>("glAttachShader");
    api.LinkProgram = load_gl_proc<PFNGLLINKPROGRAMPROC>("glLinkProgram");
    api.GetProgramiv = load_gl_proc<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");
    api.GetProgramInfoLog = load_gl_proc<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");
    api.UseProgram = load_gl_proc<PFNGLUSEPROGRAMPROC>("glUseProgram");
    api.DeleteProgram = load_gl_proc<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");
    api.GetUniformLocation = load_gl_proc<PFNGLGETUNIFORMLOCATIONPROC>("glGetUniformLocation");
    api.UniformMatrix4fv = load_gl_proc<PFNGLUNIFORMMATRIX4FVPROC>("glUniformMatrix4fv");
    api.Uniform3f = load_gl_proc<PFNGLUNIFORM3FPROC>("glUniform3f");
    api.Uniform4f = load_gl_proc<PFNGLUNIFORM4FPROC>("glUniform4f");
    api.Uniform1f = load_gl_proc<PFNGLUNIFORM1FPROC>("glUniform1f");
    api.Uniform1i = load_gl_proc<PFNGLUNIFORM1IPROC>("glUniform1i");
    api.GenBuffers = load_gl_proc<PFNGLGENBUFFERSPROC>("glGenBuffers");
    api.BindBuffer = load_gl_proc<PFNGLBINDBUFFERPROC>("glBindBuffer");
    api.BufferData = load_gl_proc<PFNGLBUFFERDATAPROC>("glBufferData");
    api.DeleteBuffers = load_gl_proc<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");
    api.BindBufferBase = load_gl_proc<PFNGLBINDBUFFERBASEPROC>("glBindBufferBase");
    api.GenVertexArrays = load_gl_proc<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");
    api.BindVertexArray = load_gl_proc<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");
    api.DeleteVertexArrays = load_gl_proc<PFNGLDELETEVERTEXARRAYSPROC>("glDeleteVertexArrays");
    api.EnableVertexAttribArray = load_gl_proc<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");
    api.VertexAttribPointer = load_gl_proc<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");
    api.DrawElements = load_gl_proc<PFNGLDRAWELEMENTSPROC>("glDrawElements");
    return api;
}

} // namespace
#endif

bool AndroidEglPresenter::attach(AndroidNativeWindow* window) {
#if defined(__ANDROID__)
    last_error_.clear();
    if (!config_.valid()) {
        last_error_ = "invalid Android EGL configuration";
        return false;
    }
    if (window == nullptr) {
        last_error_ = "Android native window is null";
        return false;
    }
    if (ready() && window_ == window) {
        return resize();
    }
    detach();

    ANativeWindow_acquire(window);
    window_ = window;

    const EGLDisplay display = eglGetDisplay(EGL_DEFAULT_DISPLAY);
    if (display == EGL_NO_DISPLAY) {
        last_error_ = egl_error("eglGetDisplay failed");
        detach();
        return false;
    }
    EGLint major = 0;
    EGLint minor = 0;
    if (eglInitialize(display, &major, &minor) != EGL_TRUE) {
        last_error_ = egl_error("eglInitialize failed");
        ANativeWindow_release(window_);
        window_ = nullptr;
        return false;
    }

    const EGLint attributes[] = {
        EGL_SURFACE_TYPE, EGL_WINDOW_BIT,
        EGL_RENDERABLE_TYPE, EGL_OPENGL_ES3_BIT,
        EGL_RED_SIZE, config_.red_bits,
        EGL_GREEN_SIZE, config_.green_bits,
        EGL_BLUE_SIZE, config_.blue_bits,
        EGL_ALPHA_SIZE, config_.alpha_bits,
        EGL_DEPTH_SIZE, config_.depth_bits,
        EGL_STENCIL_SIZE, config_.stencil_bits,
        EGL_NONE
    };
    EGLConfig egl_config = nullptr;
    EGLint count = 0;
    if (eglChooseConfig(display, attributes, &egl_config, 1, &count) != EGL_TRUE || count != 1) {
        last_error_ = egl_error("eglChooseConfig failed");
        eglTerminate(display);
        ANativeWindow_release(window_);
        window_ = nullptr;
        return false;
    }

    const EGLint context_attributes[] = {
        EGL_CONTEXT_CLIENT_VERSION, config_.gl_major, EGL_NONE
    };
    const EGLContext context = eglCreateContext(display, egl_config, EGL_NO_CONTEXT, context_attributes);
    if (context == EGL_NO_CONTEXT) {
        last_error_ = egl_error("eglCreateContext failed");
        eglTerminate(display);
        ANativeWindow_release(window_);
        window_ = nullptr;
        return false;
    }

    const EGLSurface surface = eglCreateWindowSurface(display, egl_config, window, nullptr);
    if (surface == EGL_NO_SURFACE) {
        last_error_ = egl_error("eglCreateWindowSurface failed");
        eglDestroyContext(display, context);
        eglTerminate(display);
        ANativeWindow_release(window_);
        window_ = nullptr;
        return false;
    }

    if (eglMakeCurrent(display, surface, surface, context) != EGL_TRUE) {
        last_error_ = egl_error("eglMakeCurrent failed");
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        ANativeWindow_release(window_);
        window_ = nullptr;
        return false;
    }

    if (eglSwapInterval(display, config_.swap_interval) != EGL_TRUE) {
        last_error_ = egl_error("eglSwapInterval failed");
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        eglDestroySurface(display, surface);
        eglDestroyContext(display, context);
        eglTerminate(display);
        ANativeWindow_release(window_);
        window_ = nullptr;
        return false;
    }

    display_ = as_void(display);
    surface_ = as_void(surface);
    context_ = as_void(context);

    const OpenGLESApi api = make_android_gles_api();
    if (!api.complete()) {
        last_error_ = "required OpenGL ES 3.x procedures are unavailable";
        detach();
        return false;
    }
    native_ = new (std::nothrow) OpenGLESRenderer(api);
    if (native_ == nullptr) {
        last_error_ = "failed to allocate OpenGLESRenderer";
        detach();
        return false;
    }

    stats_.attached = true;
    return resize();
#else
    (void)window;
    last_error_ = "Android EGL presentation is available only on Android";
    return false;
#endif
}

bool AndroidEglPresenter::resize() {
#if defined(__ANDROID__)
    if (!ready()) {
        last_error_ = "Android EGL presenter is not attached";
        return false;
    }
    EGLint width = 0;
    EGLint height = 0;
    const EGLDisplay display = as_display(display_);
    const EGLSurface surface = as_surface(surface_);
    if (eglQuerySurface(display, surface, EGL_WIDTH, &width) != EGL_TRUE ||
        eglQuerySurface(display, surface, EGL_HEIGHT, &height) != EGL_TRUE ||
        width <= 0 || height <= 0) {
        last_error_ = egl_error("eglQuerySurface failed");
        return false;
    }
    stats_.width = width;
    stats_.height = height;
    return true;
#else
    last_error_ = "Android EGL presentation is available only on Android";
    return false;
#endif
}

bool AndroidEglPresenter::present(const RenderFrame& frame) {
#if defined(__ANDROID__)
    last_error_.clear();
    if (!ready() || native_ == nullptr) {
        last_error_ = "Android EGL presenter is not ready";
        return false;
    }
    if (frame.config.backend != RenderBackend::OpenGLES) {
        last_error_ = "render frame backend must be OpenGLES";
        return false;
    }
    if (!frame.valid()) {
        last_error_ = "render frame is invalid";
        return false;
    }
    if (!resize()) {
        return false;
    }
    RenderFrame presented = frame;
    presented.config.width = static_cast<std::uint32_t>(stats_.width);
    presented.config.height = static_cast<std::uint32_t>(stats_.height);
    auto* renderer = static_cast<OpenGLESRenderer*>(native_);
    const GpuSubmitResult result = renderer->submit(presented);
    if (!result.success) {
        last_error_ = result.error.empty() ? "OpenGL ES submission failed" : result.error;
        return false;
    }
    if (eglSwapBuffers(as_display(display_), as_surface(surface_)) != EGL_TRUE) {
        last_error_ = egl_error("eglSwapBuffers failed");
        return false;
    }
    ++stats_.presents;
    return true;
#else
    (void)frame;
    last_error_ = "Android EGL presentation is available only on Android";
    return false;
#endif
}

void AndroidEglPresenter::detach() noexcept {
#if defined(__ANDROID__)
    if (native_ != nullptr) {
        auto* renderer = static_cast<OpenGLESRenderer*>(native_);
        renderer->release();
        delete renderer;
        native_ = nullptr;
    }
    const EGLDisplay display = as_display(display_);
    if (display != EGL_NO_DISPLAY) {
        eglMakeCurrent(display, EGL_NO_SURFACE, EGL_NO_SURFACE, EGL_NO_CONTEXT);
        if (surface_ != nullptr) {
            eglDestroySurface(display, as_surface(surface_));
        }
        if (context_ != nullptr) {
            eglDestroyContext(display, as_context(context_));
        }
        eglTerminate(display);
    }
    display_ = nullptr;
    surface_ = nullptr;
    context_ = nullptr;
    if (window_ != nullptr) {
        ANativeWindow_release(window_);
        window_ = nullptr;
    }
#endif
    stats_.attached = false;
    stats_.width = 0;
    stats_.height = 0;
    last_error_.clear();
}

} // namespace exgine

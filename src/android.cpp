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
bool AndroidEglConfig::valid() const noexcept{return red_bits>=0&&green_bits>=0&&blue_bits>=0&&alpha_bits>=0&&depth_bits>=0&&stencil_bits>=0&&gl_major>=2&&gl_minor>=0&&swap_interval>=0&&swap_interval<=1;}
bool AndroidPresentationStats::valid() const noexcept{return width>=0&&height>=0&&(!attached||(width>0&&height>0));}
AndroidEglPresenter::AndroidEglPresenter(AndroidEglConfig config)noexcept:config_(config){if(!config_.valid())config_={};}
AndroidEglPresenter::~AndroidEglPresenter(){detach();}
bool AndroidEglPresenter::ready()const noexcept{
#if defined(__ANDROID__)
return stats_.attached&&display_&&surface_&&context_;
#else
return false;
#endif
}
#if defined(__ANDROID__)
namespace {template<typename T>T load_gl_proc(const char*n)noexcept{return reinterpret_cast<T>(eglGetProcAddress(n));}void*dv(EGLDisplay v)noexcept{return reinterpret_cast<void*>(v);}void*sv(EGLSurface v)noexcept{return reinterpret_cast<void*>(v);}void*cv(EGLContext v)noexcept{return reinterpret_cast<void*>(v);}EGLDisplay d(void*v)noexcept{return reinterpret_cast<EGLDisplay>(v);}EGLSurface s(void*v)noexcept{return reinterpret_cast<EGLSurface>(v);}EGLContext c(void*v)noexcept{return reinterpret_cast<EGLContext>(v);}std::string egl_error(const char*p){char b[80]{};std::snprintf(b,sizeof(b),"%s (EGL error 0x%04x)",p,eglGetError());return b;}OpenGLESApi make_api()noexcept{OpenGLESApi a;a.Clear=load_gl_proc<PFNGLCLEARPROC>("glClear");a.ClearColor=load_gl_proc<PFNGLCLEARCOLORPROC>("glClearColor");a.Viewport=load_gl_proc<PFNGLVIEWPORTPROC>("glViewport");a.Enable=load_gl_proc<PFNGLENABLEPROC>("glEnable");a.Disable=load_gl_proc<PFNGLDISABLEPROC>("glDisable");a.DepthFunc=load_gl_proc<PFNGLDEPTHFUNCPROC>("glDepthFunc");a.BlendFunc=load_gl_proc<PFNGLBLENDFUNCPROC>("glBlendFunc");a.GetError=load_gl_proc<PFNGLGETERRORPROC>("glGetError");a.CreateShader=load_gl_proc<PFNGLCREATESHADERPROC>("glCreateShader");a.ShaderSource=load_gl_proc<PFNGLSHADERSOURCEPROC>("glShaderSource");a.CompileShader=load_gl_proc<PFNGLCOMPILESHADERPROC>("glCompileShader");a.GetShaderiv=load_gl_proc<PFNGLGETSHADERIVPROC>("glGetShaderiv");a.GetShaderInfoLog=load_gl_proc<PFNGLGETSHADERINFOLOGPROC>("glGetShaderInfoLog");a.DeleteShader=load_gl_proc<PFNGLDELETESHADERPROC>("glDeleteShader");a.CreateProgram=load_gl_proc<PFNGLCREATEPROGRAMPROC>("glCreateProgram");a.AttachShader=load_gl_proc<PFNGLATTACHSHADERPROC>("glAttachShader");a.LinkProgram=load_gl_proc<PFNGLLINKPROGRAMPROC>("glLinkProgram");a.GetProgramiv=load_gl_proc<PFNGLGETPROGRAMIVPROC>("glGetProgramiv");a.GetProgramInfoLog=load_gl_proc<PFNGLGETPROGRAMINFOLOGPROC>("glGetProgramInfoLog");a.UseProgram=load_gl_proc<PFNGLUSEPROGRAMPROC>("glUseProgram");a.DeleteProgram=load_gl_proc<PFNGLDELETEPROGRAMPROC>("glDeleteProgram");a.GetUniformLocation=load_gl_proc<PFNGLGETUNIFORMLOCATIONPROC>("glGetUniformLocation");a.UniformMatrix4fv=load_gl_proc<PFNGLUNIFORMMATRIX4FVPROC>("glUniformMatrix4fv");a.Uniform3f=load_gl_proc<PFNGLUNIFORM3FPROC>("glUniform3f");a.Uniform4f=load_gl_proc<PFNGLUNIFORM4FPROC>("glUniform4f");a.Uniform1f=load_gl_proc<PFNGLUNIFORM1FPROC>("glUniform1f");a.Uniform1i=load_gl_proc<PFNGLUNIFORM1IPROC>("glUniform1i");a.GenBuffers=load_gl_proc<PFNGLGENBUFFERSPROC>("glGenBuffers");a.BindBuffer=load_gl_proc<PFNGLBINDBUFFERPROC>("glBindBuffer");a.BufferData=load_gl_proc<PFNGLBUFFERDATAPROC>("glBufferData");a.DeleteBuffers=load_gl_proc<PFNGLDELETEBUFFERSPROC>("glDeleteBuffers");a.BindBufferBase=load_gl_proc<PFNGLBINDBUFFERBASEPROC>("glBindBufferBase");a.GenVertexArrays=load_gl_proc<PFNGLGENVERTEXARRAYSPROC>("glGenVertexArrays");a.BindVertexArray=load_gl_proc<PFNGLBINDVERTEXARRAYPROC>("glBindVertexArray");a.DeleteVertexArrays=load_gl_proc<PFNGLDELETEVERTEXARRAYPROC>("glDeleteVertexArrays");a.EnableVertexAttribArray=load_gl_proc<PFNGLENABLEVERTEXATTRIBARRAYPROC>("glEnableVertexAttribArray");a.VertexAttribPointer=load_gl_proc<PFNGLVERTEXATTRIBPOINTERPROC>("glVertexAttribPointer");a.DrawElements=load_gl_proc<PFNGLDRAWELEMENTSPROC>("glDrawElements");a.GenTextures=load_gl_proc<PFNGLGENTEXTURESPROC>("glGenTextures");a.BindTexture=load_gl_proc<PFNGLBINDTEXTUREPROC>("glBindTexture");a.TexParameteri=load_gl_proc<PFNGLTEXPARAMETERIPROC>("glTexParameteri");a.TexImage2D=load_gl_proc<PFNGLTEXIMAGE2DPROC>("glTexImage2D");a.GenerateMipmap=load_gl_proc<PFNGLGENERATEMIPMAPPROC>("glGenerateMipmap");a.ActiveTexture=load_gl_proc<PFNGLACTIVETEXTUREPROC>("glActiveTexture");a.DeleteTextures=load_gl_proc<PFNGLDELETETEXTURESPROC>("glDeleteTextures");return a;}}
#endif
bool AndroidEglPresenter::attach(AndroidNativeWindow*w){
#if defined(__ANDROID__)
last_error_.clear();if(!config_.valid()){last_error_="invalid Android EGL configuration";return false;}if(!w){last_error_="Android native window is null";return false;}if(ready()&&window_==w)return resize();detach();ANativeWindow_acquire(w);window_=w;const EGLDisplay display=eglGetDisplay(EGL_DEFAULT_DISPLAY);if(display==EGL_NO_DISPLAY){last_error_=egl_error("eglGetDisplay failed");detach();return false;}EGLint major=0,minor=0;if(eglInitialize(display,&major,&minor)!=EGL_TRUE){last_error_=egl_error("eglInitialize failed");detach();return false;}const EGLint attrs[]={EGL_SURFACE_TYPE,EGL_WINDOW_BIT,EGL_RENDERABLE_TYPE,EGL_OPENGL_ES3_BIT,EGL_RED_SIZE,config_.red_bits,EGL_GREEN_SIZE,config_.green_bits,EGL_BLUE_SIZE,config_.blue_bits,EGL_ALPHA_SIZE,config_.alpha_bits,EGL_DEPTH_SIZE,config_.depth_bits,EGL_STENCIL_SIZE,config_.stencil_bits,EGL_NONE};EGLConfig ec=nullptr;EGLint count=0;if(eglChooseConfig(display,attrs,&ec,1,&count)!=EGL_TRUE||count!=1){last_error_=egl_error("eglChooseConfig failed");eglTerminate(display);ANativeWindow_release(window_);window_=nullptr;return false;}const EGLint ca[]={EGL_CONTEXT_CLIENT_VERSION,config_.gl_major,EGL_NONE};const EGLContext ctx=eglCreateContext(display,ec,EGL_NO_CONTEXT,ca);if(ctx==EGL_NO_CONTEXT){last_error_=egl_error("eglCreateContext failed");eglTerminate(display);ANativeWindow_release(window_);window_=nullptr;return false;}const EGLSurface surf=eglCreateWindowSurface(display,ec,w,nullptr);if(surf==EGL_NO_SURFACE){last_error_=egl_error("eglCreateWindowSurface failed");eglDestroyContext(display,ctx);eglTerminate(display);ANativeWindow_release(window_);window_=nullptr;return false;}if(eglMakeCurrent(display,surf,surf,ctx)!=EGL_TRUE){last_error_=egl_error("eglMakeCurrent failed");eglDestroySurface(display,surf);eglDestroyContext(display,ctx);eglTerminate(display);ANativeWindow_release(window_);window_=nullptr;return false;}if(eglSwapInterval(display,config_.swap_interval)!=EGL_TRUE){last_error_=egl_error("eglSwapInterval failed");eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);eglDestroySurface(display,surf);eglDestroyContext(display,ctx);eglTerminate(display);ANativeWindow_release(window_);window_=nullptr;return false;}display_=dv(display);surface_=sv(surf);context_=cv(ctx);const auto api=make_api();if(!api.complete()){last_error_="required OpenGL ES procedures are unavailable";detach();return false;}native_=new(std::nothrow)OpenGLESRenderer(api);if(!native_){last_error_="failed to allocate OpenGLESRenderer";detach();return false;}stats_.attached=true;return resize();
#else
(void)w;last_error_="Android EGL presentation is available only on Android";return false;
#endif
}
bool AndroidEglPresenter::resize(){
#if defined(__ANDROID__)
if(!ready()){last_error_="Android EGL presenter is not attached";return false;}EGLint width=0,height=0;const EGLDisplay display=d(display_),surface=s(surface_);if(eglQuerySurface(display,surface,EGL_WIDTH,&width)!=EGL_TRUE||eglQuerySurface(display,surface,EGL_HEIGHT,&height)!=EGL_TRUE||width<=0||height<=0){last_error_=egl_error("eglQuerySurface failed");return false;}stats_.width=width;stats_.height=height;return true;
#else
last_error_="Android EGL presentation is available only on Android";return false;
#endif
}
bool AndroidEglPresenter::present(const RenderFrame&frame){
#if defined(__ANDROID__)
last_error_.clear();if(!ready()||!native_){last_error_="Android EGL presenter is not ready";return false;}if(frame.config.backend!=RenderBackend::OpenGLES){last_error_="render frame backend must be OpenGLES";return false;}if(!frame.valid()){last_error_="render frame is invalid";return false;}if(!resize())return false;RenderFrame presented=frame;presented.config.width=static_cast<std::uint32_t>(stats_.width);presented.config.height=static_cast<std::uint32_t>(stats_.height);auto*r=static_cast<OpenGLESRenderer*>(native_);const auto result=r->submit(presented);if(!result.success){last_error_=result.error.empty()?"OpenGL ES submission failed":result.error;return false;}if(eglSwapBuffers(d(display_),s(surface_))!=EGL_TRUE){last_error_=egl_error("eglSwapBuffers failed");return false;}++stats_.presents;return true;
#else
(void)frame;last_error_="Android EGL presentation is available only on Android";return false;
#endif
}
void AndroidEglPresenter::detach()noexcept{
#if defined(__ANDROID__)
if(native_){auto*r=static_cast<OpenGLESRenderer*>(native_);r->release();delete r;native_=nullptr;}const EGLDisplay display=d(display_);if(display!=EGL_NO_DISPLAY){eglMakeCurrent(display,EGL_NO_SURFACE,EGL_NO_SURFACE,EGL_NO_CONTEXT);if(surface_)eglDestroySurface(display,s(surface_));if(context_)eglDestroyContext(display,c(context_));eglTerminate(display);}display_=surface_=context_=nullptr;if(window_){ANativeWindow_release(window_);window_=nullptr;}
#endif
stats_.attached=false;stats_.width=stats_.height=0;last_error_.clear();
}
} // namespace exgine

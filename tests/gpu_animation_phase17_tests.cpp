#include "exgine/gpu.hpp"
#include "exgine/runtime.hpp"
#include "exgine/shaders.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <string>
using namespace exgine;
namespace {
std::uint32_t next_id=1;
int shader_compile=0, shader_link=0, draw_calls=0, matrix_uploads=0;
void Clear(GlUInt){} void ClearColor(GlFloat,GlFloat,GlFloat,GlFloat){} void Viewport(GlInt,GlInt,GlInt,GlInt){} void Enable(GlEnum){} void Disable(GlEnum){} void DepthFunc(GlEnum){} void BlendFunc(GlEnum,GlEnum){}
GlUInt CreateShader(GlEnum){return next_id++;} void ShaderSource(GlUInt,GlInt,const char* const*,const GlInt*){} void CompileShader(GlUInt){++shader_compile;} void GetShaderiv(GlUInt,GlEnum,GlInt*v){*v=1;} void GetShaderInfoLog(GlUInt,GlInt,GlInt*w,char*){*w=0;} void DeleteShader(GlUInt){}
GlUInt CreateProgram(){return next_id++;} void AttachShader(GlUInt,GlUInt){} void LinkProgram(GlUInt){++shader_link;} void GetProgramiv(GlUInt,GlEnum,GlInt*v){*v=1;} void GetProgramInfoLog(GlUInt,GlInt,GlInt*w,char*){*w=0;} void UseProgram(GlUInt){} void DeleteProgram(GlUInt){}
GlInt GetUniformLocation(GlUInt,const char* n){ return std::string{n}.empty()?-1:1; }
void UniformMatrix4fv(GlInt,GlInt,GlBool,const GlFloat*){++matrix_uploads;} void Uniform3f(GlInt,GlFloat,GlFloat,GlFloat){} void Uniform4f(GlInt,GlFloat,GlFloat,GlFloat,GlFloat){} void Uniform1f(GlInt,GlFloat){} void Uniform1i(GlInt,GlInt){}
void GenBuffers(GlInt n,GlUInt*v){for(int i=0;i<n;++i)v[i]=next_id++;} void BindBuffer(GlEnum,GlUInt){} void BufferData(GlEnum,GlSize,const void*,GlEnum){} void DeleteBuffers(GlInt,const GlUInt*){} void BindBufferBase(GlEnum,GlUInt,GlUInt){}
void GenVertexArrays(GlInt n,GlUInt*v){for(int i=0;i<n;++i)v[i]=next_id++;} void BindVertexArray(GlUInt){} void DeleteVertexArrays(GlInt,const GlUInt*){} void EnableVertexAttribArray(GlUInt){} void VertexAttribPointer(GlUInt,GlInt,GlEnum,GlBool,GlInt,const void*){} void DrawElements(GlEnum,GlInt,GlEnum,const void*){++draw_calls;}
OpenGLESApi api(){OpenGLESApi a;a.Clear=Clear;a.ClearColor=ClearColor;a.Viewport=Viewport;a.Enable=Enable;a.Disable=Disable;a.DepthFunc=DepthFunc;a.BlendFunc=BlendFunc;a.CreateShader=CreateShader;a.ShaderSource=ShaderSource;a.CompileShader=CompileShader;a.GetShaderiv=GetShaderiv;a.GetShaderInfoLog=GetShaderInfoLog;a.DeleteShader=DeleteShader;a.CreateProgram=CreateProgram;a.AttachShader=AttachShader;a.LinkProgram=LinkProgram;a.GetProgramiv=GetProgramiv;a.GetProgramInfoLog=GetProgramInfoLog;a.UseProgram=UseProgram;a.DeleteProgram=DeleteProgram;a.GetUniformLocation=GetUniformLocation;a.UniformMatrix4fv=UniformMatrix4fv;a.Uniform3f=Uniform3f;a.Uniform4f=Uniform4f;a.Uniform1f=Uniform1f;a.Uniform1i=Uniform1i;a.GenBuffers=GenBuffers;a.BindBuffer=BindBuffer;a.BufferData=BufferData;a.DeleteBuffers=DeleteBuffers;a.BindBufferBase=BindBufferBase;a.GenVertexArrays=GenVertexArrays;a.BindVertexArray=BindVertexArray;a.DeleteVertexArrays=DeleteVertexArrays;a.EnableVertexAttribArray=EnableVertexAttribArray;a.VertexAttribPointer=VertexAttribPointer;a.DrawElements=DrawElements;return a;}
IR make_ir(){IR ir;auto&t=ir.add_child(NodeKind::Terrain,"terrain");t.add_property("resolution",std::int64_t{8});return ir;}
}
int main(){
    const auto shader=make_mobile_skinned_pbr_shader();
    assert(shader.valid());
    assert(shader.vertex_source.find("a_bone_ids")!=std::string::npos);
    assert(shader.vertex_source.find("u_bones")!=std::string::npos);
    Runtime rt; assert(rt.load(make_ir()));
    auto e=rt.state().entities.create(NodeKind::Player,"animated"); assert(e!=0);
    auto sk=make_humanoid_skeleton(); assert(rt.attach_skeleton(e,sk));
    auto clip=make_walk_clip(sk,7); assert(rt.define_animation(clip));
    SkinnedMesh sm; sm.vertices={{{0,0,0},{0,1,0},{0,0},{ {1,0,0,0},{1,0,0,0}}},{{0,1,0},{0,1,0},{0,1},{ {2,0,0,0},{1,0,0,0}}},{{0,0,1},{0,1,0},{1,0},{ {1,0,0,0},{1,0,0,0}}}}; sm.indices={0,1,2};
    assert(rt.attach_skinned_mesh(e,sm,"skin"));
    Material m=make_real_world_material("steel",7); m.name="skin"; assert(rt.define_material(m));
    assert(rt.play_animation(e,7,0)); rt.update(0.1); RenderConfig rc; rc.backend=RenderBackend::OpenGLES; Renderer r(rc); RenderFrame f; assert(r.build_frame(rt,f)); assert(f.draws.size()==1); assert(f.draws[0].skinned_mesh); assert(!f.draws[0].bone_palette.empty());
    OpenGLESRenderer gpu(api()); auto out=gpu.submit(f); assert(out.success); assert(out.draw_calls==1); assert(out.triangles==1); assert(out.animated_draw_calls==1); assert(shader_compile>=4); assert(shader_link>=2); assert(matrix_uploads>=2); assert(draw_calls==1); gpu.release();
    return 0;
}

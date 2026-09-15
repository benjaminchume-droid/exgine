#include "exgine/gles_gpu_driven.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>
#if defined(__ANDROID__)
#include <EGL/egl.h>
#endif
namespace exgine { namespace {
constexpr GlEnum ARRAY=0x8892,ELEMENT=0x8893,INDIRECT=0x8F3F,SSBO=0x90D2,STATIC_DRAW=0x88E4,DYNAMIC_DRAW=0x88E8,FLOAT=0x1406,U32=0x1405,TRIS=0x0004,VS=0x8B31,FS=0x8B30,CS=0x91B9,COMPILE=0x8B81,LINK=0x8B82;
using DispatchProc=void(*)(GlUInt,GlUInt,GlUInt); using BarrierProc=void(*)(GlUInt); using IndirectProc=void(*)(GlEnum,GlEnum,const void*);
struct Batch { GlUInt vao=0,vb=0,ib=0,models=0,spheres=0,visible=0,indirect=0,program=0,compute=0; std::size_t index_count=0; };
struct State { std::unordered_map<std::uint64_t,Batch> batches; };
std::unordered_map<const OpenGLESRenderer*,std::unique_ptr<State>> states;
std::uint32_t bits(float v)noexcept{std::uint32_t x=0;std::memcpy(&x,&v,sizeof(x));return x;}
std::uint64_t key_for(const RenderDrawCall&d)noexcept{auto h=static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(d.geometry.get()));h^=static_cast<std::uint64_t>(d.part_index+0x9e37);h^=static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(d.material.textures.get()))<<1;h^=static_cast<std::uint64_t>(bits(d.material.material.base_color.r))*0x9e3779b185ebca87ULL;h^=static_cast<std::uint64_t>(bits(d.material.material.base_color.g))*0xc2b2ae3d27d4eb4fULL;h^=static_cast<std::uint64_t>(bits(d.material.material.base_color.b))*0x165667b19e3779f9ULL;h^=static_cast<std::uint64_t>(bits(d.material.material.roughness))<<17;h^=static_cast<std::uint64_t>(bits(d.material.material.metallic))<<29;return h;}
#if defined(__ANDROID__)
template<class T>T proc(const char*name){return reinterpret_cast<T>(eglGetProcAddress(name));}
#endif
bool compile(OpenGLESApi&a,GlEnum type,const char*src,GlUInt&out){out=a.CreateShader(type);if(!out)return false;GlInt len=(GlInt)std::strlen(src);a.ShaderSource(out,1,&src,&len);a.CompileShader(out);GlInt ok=0;a.GetShaderiv(out,COMPILE,&ok);if(ok)return true;a.DeleteShader(out);out=0;return false;}
bool link(OpenGLESApi&a,const char*vs,const char*fs,GlUInt&out){GlUInt v=0,f=0;if(!compile(a,VS,vs,v)||!compile(a,FS,fs,f)){if(v)a.DeleteShader(v);if(f)a.DeleteShader(f);return false;}out=a.CreateProgram();if(!out)return false;a.AttachShader(out,v);a.AttachShader(out,f);a.LinkProgram(out);a.DeleteShader(v);a.DeleteShader(f);GlInt ok=0;a.GetProgramiv(out,LINK,&ok);if(ok)return true;a.DeleteProgram(out);out=0;return false;}
bool link_compute(OpenGLESApi&a,const char*src,GlUInt&out){GlUInt c=0;if(!compile(a,CS,src,c))return false;out=a.CreateProgram();if(!out){a.DeleteShader(c);return false;}a.AttachShader(out,c);a.LinkProgram(out);a.DeleteShader(c);GlInt ok=0;a.GetProgramiv(out,LINK,&ok);if(ok)return true;a.DeleteProgram(out);out=0;return false;}
const char* rvs=R"GLSL(#version 310 es
precision highp float;
layout(location=0)in vec3 a_position;layout(location=1)in vec3 a_normal;
layout(std430,binding=0)readonly buffer Models{mat4 models[];};layout(std430,binding=1)readonly buffer Visible{uint ids[];};
uniform mat4 u_view_projection;out vec3 v_n;
void main(){uint id=ids[gl_InstanceID];mat4 m=models[id];v_n=normalize(mat3(m)*a_normal);gl_Position=u_view_projection*m*vec4(a_position,1.0);}
)GLSL";
const char* rfs=R"GLSL(#version 310 es
precision highp float;in vec3 v_n;out vec4 o;uniform vec4 u_base_roughness;uniform vec3 u_light_color;uniform float u_light_intensity;
void main(){float nd=max(dot(normalize(v_n),normalize(vec3(.3,.8,.2))),0.0);vec3 c=u_base_roughness.rgb*(.2+nd*u_light_intensity)*u_light_color;o=vec4(c/(c+1.0),1.0);}
)GLSL";
const char* csrc=R"GLSL(#version 310 es
layout(local_size_x=64)in;layout(std430,binding=0)readonly buffer Models{mat4 models[];};layout(std430,binding=1)readonly buffer Spheres{vec4 spheres[];};layout(std430,binding=2)buffer Visible{uint ids[];};layout(std430,binding=3)buffer Command{uint count;uint instanceCount;uint firstIndex;uint baseVertex;uint baseInstance;};uniform mat4 u_view_projection;uniform int u_count;
void main(){uint id=gl_GlobalInvocationID.x;if(int(id)>=u_count)return;vec4 c=u_view_projection*vec4(spheres[id].xyz,1.0);float r=max(spheres[id].w,.001);if(c.w<=0.0||abs(c.x)>c.w+r||abs(c.y)>c.w+r||abs(c.z)>c.w+r)return;uint dst=atomicAdd(instanceCount,1u);ids[dst]=id;}
)GLSL";
void release_batch(OpenGLESApi&a,Batch&b){if(a.DeleteProgram){if(b.program)a.DeleteProgram(b.program);if(b.compute)a.DeleteProgram(b.compute);}if(a.DeleteBuffers)for(auto x:{b.vb,b.ib,b.models,b.spheres,b.visible,b.indirect})if(x)a.DeleteBuffers(1,&x);if(a.DeleteVertexArrays&&b.vao)a.DeleteVertexArrays(1,&b.vao);b={};}
Batch& ensure_batch(OpenGLESApi&a,State&s,const RenderDrawCall&d,std::string&error){auto key=key_for(d);auto it=s.batches.find(key);if(it!=s.batches.end())return it->second;Batch b{};const auto&m=d.geometry->parts[d.part_index].mesh;std::vector<float>v;v.reserve(m.vertices.size()*6);for(const auto&x:m.vertices)v.insert(v.end(),{x.position.x,x.position.y,x.position.z,x.normal.x,x.normal.y,x.normal.z});a.GenVertexArrays(1,&b.vao);a.GenBuffers(1,&b.vb);a.GenBuffers(1,&b.ib);a.GenBuffers(1,&b.models);a.GenBuffers(1,&b.spheres);a.GenBuffers(1,&b.visible);a.GenBuffers(1,&b.indirect);if(!b.vao||!b.vb||!b.ib){error="GPU batch allocation failed";release_batch(a,b);return s.batches.emplace(key,b).first->second;}a.BindVertexArray(b.vao);a.BindBuffer(ARRAY,b.vb);a.BufferData(ARRAY,v.size()*sizeof(float),v.data(),STATIC_DRAW);a.BindBuffer(ELEMENT,b.ib);a.BufferData(ELEMENT,m.indices.size()*sizeof(std::uint32_t),m.indices.data(),STATIC_DRAW);a.EnableVertexAttribArray(0);a.VertexAttribPointer(0,3,FLOAT,0,6*sizeof(float),nullptr);a.EnableVertexAttribArray(1);a.VertexAttribPointer(1,3,FLOAT,0,6*sizeof(float),(const void*)(3*sizeof(float)));if(!link(a,rvs,rfs,b.program))error="GPU instanced shader link failed";
#if defined(__ANDROID__)
if(!link_compute(a,csrc,b.compute))error="GPU visibility shader link failed";
#endif
b.index_count=m.indices.size();auto[it2,_]=s.batches.emplace(key,b);return it2->second;}
}
GpuSubmitResult submit_gpu_driven_batches(OpenGLESRenderer&r,RenderFrame&frame){GpuSubmitResult result{};if(frame.draws.size()<2)return result;auto&a=const_cast<OpenGLESApi&>(r.api());auto&state=states[&r];if(!state)state=std::make_unique<State>();
#if defined(__ANDROID__)
const auto dispatch=proc<DispatchProc>("glDispatchCompute");const auto barrier=proc<BarrierProc>("glMemoryBarrier");const auto draw_indirect=proc<IndirectProc>("glDrawElementsIndirect");
#endif
std::unordered_map<std::uint64_t,std::vector<std::size_t>>groups;for(std::size_t i=0;i<frame.draws.size();++i){const auto&d=frame.draws[i];if(d.animated()||d.pass!=RenderPass::Opaque||!d.geometry||d.part_index>=d.geometry->parts.size())continue;groups[key_for(d)].push_back(i);}std::vector<bool>consume(frame.draws.size(),false);
for(const auto&entry:groups){const auto&ids=entry.second;if(ids.size()<2)continue;const auto&first=frame.draws[ids.front()];std::string error;auto&b=ensure_batch(a,*state,first,error);if(!b.program)continue;std::vector<float>models,spheres;models.reserve(ids.size()*16);spheres.reserve(ids.size()*4);for(auto idx:ids){const auto&d=frame.draws[idx];models.insert(models.end(),d.model.m.begin(),d.model.m.end());Vec3 c{(d.world_bounds.min.x+d.world_bounds.max.x)*.5f,(d.world_bounds.min.y+d.world_bounds.max.y)*.5f,(d.world_bounds.min.z+d.world_bounds.max.z)*.5f};float x=d.world_bounds.max.x-d.world_bounds.min.x,y=d.world_bounds.max.y-d.world_bounds.min.y,z=d.world_bounds.max.z-d.world_bounds.min.z;float r=.5f*std::sqrt(x*x+y*y+z*z);spheres.insert(spheres.end(),{c.x,c.y,c.z,r});consume[idx]=true;}
a.BindBuffer(SSBO,b.models);a.BufferData(SSBO,models.size()*sizeof(float),models.data(),DYNAMIC_DRAW);a.BindBufferBase(SSBO,0,b.models);a.BindBuffer(SSBO,b.spheres);a.BufferData(SSBO,spheres.size()*sizeof(float),spheres.data(),DYNAMIC_DRAW);a.BindBufferBase(SSBO,1,b.spheres);a.BindBuffer(SSBO,b.visible);a.BufferData(SSBO,ids.size()*sizeof(std::uint32_t),nullptr,DYNAMIC_DRAW);a.BindBufferBase(SSBO,2,b.visible);std::uint32_t command[5]={(std::uint32_t)b.index_count,0,0,0,0};a.BindBuffer(INDIRECT,b.indirect);a.BufferData(INDIRECT,sizeof(command),command,DYNAMIC_DRAW);
#if defined(__ANDROID__)
if(dispatch&&barrier&&draw_indirect&&b.compute){a.BindBufferBase(SSBO,3,b.indirect);a.UseProgram(b.compute);if(auto p=a.GetUniformLocation(b.compute,"u_view_projection");p>=0)a.UniformMatrix4fv(p,1,0,frame.view_projection.m.data());if(auto p=a.GetUniformLocation(b.compute,"u_count");p>=0)a.Uniform1i(p,(GlInt)ids.size());dispatch((GlUInt)((ids.size()+63)/64),1,1);barrier(0x2000u|0x00000040u);a.UseProgram(b.program);if(auto p=a.GetUniformLocation(b.program,"u_view_projection");p>=0)a.UniformMatrix4fv(p,1,0,frame.view_projection.m.data());if(auto p=a.GetUniformLocation(b.program,"u_base_roughness");p>=0)a.Uniform4f(p,first.material.material.base_color.r,first.material.material.base_color.g,first.material.material.base_color.b,std::clamp(first.material.material.roughness,.045f,1.f));if(auto p=a.GetUniformLocation(b.program,"u_light_color");p>=0&&!frame.lights.empty())a.Uniform3f(p,frame.lights.front().color.r,frame.lights.front().color.g,frame.lights.front().color.b);if(auto p=a.GetUniformLocation(b.program,"u_light_intensity");p>=0&&!frame.lights.empty())a.Uniform1f(p,frame.lights.front().intensity);a.BindVertexArray(b.vao);draw_indirect(TRIS,U32,nullptr);a.BindVertexArray(0);result.success=true;result.draw_calls++;result.triangles+=b.index_count/3;}
else
#endif
{if(!a.DrawElementsInstanced)continue;a.BindBuffer(SSBO,b.visible);std::vector<std::uint32_t>ids_cpu(ids.size());for(std::size_t i=0;i<ids.size();++i)ids_cpu[i]=(std::uint32_t)i;a.BufferData(SSBO,ids_cpu.size()*sizeof(std::uint32_t),ids_cpu.data(),DYNAMIC_DRAW);a.BindBufferBase(SSBO,1,b.visible);a.UseProgram(b.program);if(auto p=a.GetUniformLocation(b.program,"u_view_projection");p>=0)a.UniformMatrix4fv(p,1,0,frame.view_projection.m.data());if(auto p=a.GetUniformLocation(b.program,"u_base_roughness");p>=0)a.Uniform4f(p,first.material.material.base_color.r,first.material.material.base_color.g,first.material.material.base_color.b,std::clamp(first.material.material.roughness,.045f,1.f));if(auto p=a.GetUniformLocation(b.program,"u_light_color");p>=0&&!frame.lights.empty())a.Uniform3f(p,frame.lights.front().color.r,frame.lights.front().color.g,frame.lights.front().color.b);if(auto p=a.GetUniformLocation(b.program,"u_light_intensity");p>=0&&!frame.lights.empty())a.Uniform1f(p,frame.lights.front().intensity);a.BindVertexArray(b.vao);a.DrawElementsInstanced(TRIS,(GlInt)b.index_count,U32,nullptr,(GlInt)ids.size());a.BindVertexArray(0);result.success=true;result.draw_calls++;result.triangles+=b.index_count/3;}
}
if(result.success){std::vector<RenderDrawCall>keep;keep.reserve(frame.draws.size());for(std::size_t i=0;i<frame.draws.size();++i)if(!consume[i])keep.push_back(std::move(frame.draws[i]));frame.draws.swap(keep);}return result;}
}
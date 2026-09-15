#include "exgine/gpu.hpp"
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <string>
#include <vector>

namespace exgine { namespace {
constexpr GlEnum GL_ARRAY_BUFFER=0x8892, GL_ELEMENT_ARRAY_BUFFER=0x8893, GL_STATIC_DRAW=0x88E4, GL_DYNAMIC_DRAW=0x88E8;
constexpr GlEnum GL_FLOAT=0x1406, GL_UNSIGNED_INT=0x1405, GL_TRIANGLES=0x0004, GL_DEPTH_TEST=0x0B71, GL_BLEND=0x0BE2;
constexpr GlEnum GL_SRC_ALPHA=0x0302, GL_ONE_MINUS_SRC_ALPHA=0x0303, GL_VERTEX_SHADER=0x8B31, GL_FRAGMENT_SHADER=0x8B30;
struct V { float x,y,z,nx,ny,nz,u,v; };
struct P { float x,y,z,s; };

bool shader(OpenGLESApi&a,GlEnum type,const char*src,GlUInt&out,std::string&e){out=a.CreateShader(type);a.ShaderSource(out,1,&src,nullptr);a.CompileShader(out);GlInt ok=0;a.GetShaderiv(out,0x8B81,&ok);if(!ok){char log[2048]{};GlInt n=0;a.GetShaderInfoLog(out,2047,&n,log);e=log;return false;}return true;}
bool program(OpenGLESApi&a,const char*vs,const char*fs,GlUInt&out,std::string&e){GlUInt v=0,f=0;if(!shader(a,GL_VERTEX_SHADER,vs,v,e))return false;if(!shader(a,GL_FRAGMENT_SHADER,fs,f,e))return false;out=a.CreateProgram();a.AttachShader(out,v);a.AttachShader(out,f);a.LinkProgram(out);a.DeleteShader(v);a.DeleteShader(f);GlInt ok=0;a.GetProgramiv(out,0x8B82,&ok);if(!ok){char log[2048]{};GlInt n=0;a.GetProgramInfoLog(out,2047,&n,log);e=log;return false;}return true;}
void setup_mesh(OpenGLESApi&a,GlUInt vao,GlUInt vb,GlUInt ib){a.BindVertexArray(vao);a.BindBuffer(GL_ARRAY_BUFFER,vb);a.EnableVertexAttribArray(0);a.VertexAttribPointer(0,3,GL_FLOAT,0,sizeof(V),(void*)0);a.EnableVertexAttribArray(1);a.VertexAttribPointer(1,3,GL_FLOAT,0,sizeof(V),(void*)(3*sizeof(float)));a.EnableVertexAttribArray(2);a.VertexAttribPointer(2,2,GL_FLOAT,0,sizeof(V),(void*)(6*sizeof(float)));a.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);}
const char* terrain_vs=R"GLSL(#version 310 es
precision highp float;layout(location=0)in vec3 a_position;layout(location=1)in vec3 a_normal;layout(location=2)in vec2 a_uv;uniform mat4 u_vp;uniform vec3 u_origin;out vec3 v_n;out vec3 v_p;void main(){vec3 p=a_position+u_origin;p.y+=sin(p.x*.025)*3.0+cos(p.z*.021)*2.0+sin((p.x+p.z)*.009)*8.0;v_p=p;v_n=normalize(vec3(-.075*cos(p.x*.025)-.072*sin((p.x+p.z)*.009),1.,.063*sin(p.z*.021)-.072*sin((p.x+p.z)*.009)));gl_Position=u_vp*vec4(p,1.);}
)GLSL";
const char* terrain_fs=R"GLSL(#version 310 es
precision highp float;in vec3 v_n;in vec3 v_p;layout(location=0)out vec4 o;void main(){float h=clamp(v_p.y/20.,0.,1.);vec3 grass=mix(vec3(.08,.20,.06),vec3(.25,.36,.12),h);float l=max(dot(normalize(v_n),normalize(vec3(.4,.8,.25))),0.);o=vec4(grass*(.35+.65*l),1.);}
)GLSL";
const char* water_vs=R"GLSL(#version 310 es
precision highp float;layout(location=0)in vec3 a_position;layout(location=2)in vec2 a_uv;uniform mat4 u_vp;uniform vec3 u_origin;uniform float u_time;out vec3 v_p;void main(){vec3 p=a_position+u_origin;p.y+=sin(p.x*.7+u_time)*.15+cos(p.z*.55+u_time*.8)*.12;v_p=p;gl_Position=u_vp*vec4(p,1.);}
)GLSL";
const char* water_fs=R"GLSL(#version 310 es
precision highp float;in vec3 v_p;layout(location=0)out vec4 o;void main(){vec3 c=vec3(.025,.22,.30);float fres=pow(1.-clamp(abs(v_p.y),0.,1.),2.);o=vec4(c+.15*fres,.78);}
)GLSL";
const char* veg_vs=R"GLSL(#version 310 es
precision highp float;layout(location=0)in vec3 a_position;layout(location=2)in vec2 a_uv;layout(location=3)in vec4 a_instance;uniform mat4 u_vp;uniform vec3 u_camera;out vec2 v_uv;void main(){vec3 p=a_position*a_instance.w+vec3(a_instance.xyz);float d=distance(p,u_camera);if(d>300.)p=vec3(1e6);float lod=d<50.?1.:d<140?.75:.45;p.y+=sin(p.x*.7)*.08;gl_Position=u_vp*vec4(p,1.);v_uv=a_uv;}
)GLSL";
const char* veg_fs=R"GLSL(#version 310 es
precision highp float;in vec2 v_uv;layout(location=0)out vec4 o;float hash(vec2 p){return fract(sin(dot(p,vec2(12.9898,78.233)))*43758.5453);}void main(){if(v_uv.y<.08)discard;vec3 c=mix(vec3(.06,.20,.03),vec3(.18,.45,.08),hash(v_uv*41.));o=vec4(c,1.);}
)GLSL";

std::vector<V> grid_vertices(int n,float extent){std::vector<V> v;v.reserve((n+1)*(n+1));for(int z=0;z<=n;++z)for(int x=0;x<=n;++x){float u=float(x)/n,w=float(z)/n;v.push_back({(u-.5f)*extent,0,(w-.5f)*extent,0,1,0,u,w});}return v;}
std::vector<std::uint32_t> grid_indices(int n){std::vector<std::uint32_t> i;i.reserve(n*n*6);for(int z=0;z<n;++z)for(int x=0;x<n;++x){std::uint32_t a=z*(n+1)+x,b=a+1,c=a+n+1,d=c+1;i.insert(i.end(),{a,c,b,b,c,d});}return i;}

}

GpuSubmitResult render_world_systems(OpenGLESApi&a,const RenderFrame&frame,const AdvancedRenderPipeline&pipeline){GpuSubmitResult r{};if(!a.GenBuffers||!a.VertexAttribDivisor||!a.DrawElementsInstanced){r.success=true;return r;}std::string e;GlUInt tp=0,wp=0,vp=0; if(!program(a,terrain_vs,terrain_fs,tp,e)||!program(a,water_vs,water_fs,wp,e)||!program(a,veg_vs,veg_fs,vp,e)){r.error=e;return r;}GlUInt vao=0,vb=0,ib=0,wvao=0,wvb=0,wib=0,vvao=0,vvb=0,vib=0,inst=0;a.GenVertexArrays(1,&vao);a.GenBuffers(1,&vb);a.GenBuffers(1,&ib);auto tv=grid_vertices(64,256.f);auto ti=grid_indices(64);a.BindBuffer(GL_ARRAY_BUFFER,vb);a.BufferData(GL_ARRAY_BUFFER,GlSize(tv.size()*sizeof(V)),tv.data(),GL_STATIC_DRAW);a.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,ib);a.BufferData(GL_ELEMENT_ARRAY_BUFFER,GlSize(ti.size()*sizeof(std::uint32_t)),ti.data(),GL_STATIC_DRAW);setup_mesh(a,vao,vb,ib);
a.GenVertexArrays(1,&wvao);a.GenBuffers(1,&wvb);a.GenBuffers(1,&wib);auto wv=grid_vertices(64,256.f);auto wi=grid_indices(64);a.BindBuffer(GL_ARRAY_BUFFER,wvb);a.BufferData(GL_ARRAY_BUFFER,GlSize(wv.size()*sizeof(V)),wv.data(),GL_STATIC_DRAW);a.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,wib);a.BufferData(GL_ELEMENT_ARRAY_BUFFER,GlSize(wi.size()*sizeof(std::uint32_t)),wi.data(),GL_STATIC_DRAW);setup_mesh(a,wvao,wvb,wib);
std::vector<P> instances;instances.reserve(2048);WorldStreamingManager streaming;const auto cells=streaming.desired_cells(frame.camera.position,2);for(const auto&c:cells)streaming.mark_loaded(c);const int radius=96;for(int z=-radius;z<=radius;z+=8)for(int x=-radius;x<=radius;x+=8){float wx=frame.camera.position.x+x,wz=frame.camera.position.z+z;float jitter=std::sin(wx*12.7f+wz*4.1f);if(std::fmod(std::abs(wx+wz),17.f)<3.f)instances.push_back({wx,1.5f+std::abs(std::sin(wx*.04f+wz*.03f)),wz,.7f+std::abs(jitter)*.6f});}
a.GenVertexArrays(1,&vvao);a.GenBuffers(1,&vvb);a.GenBuffers(1,&vib);a.GenBuffers(1,&inst);std::vector<V> quad={{-.7f,0,0,0,1,0,0,0},{.7f,0,0,0,1,0,1,0},{.7f,3,0,0,1,0,1,1},{-.7f,3,0,0,1,0,0,1}};std::vector<std::uint32_t> qi={0,1,2,0,2,3};a.BindBuffer(GL_ARRAY_BUFFER,vvb);a.BufferData(GL_ARRAY_BUFFER,GlSize(quad.size()*sizeof(V)),quad.data(),GL_STATIC_DRAW);a.BindBuffer(GL_ELEMENT_ARRAY_BUFFER,vib);a.BufferData(GL_ELEMENT_ARRAY_BUFFER,GlSize(qi.size()*sizeof(std::uint32_t)),qi.data(),GL_STATIC_DRAW);setup_mesh(a,vvao,vvb,vib);a.BindBuffer(GL_ARRAY_BUFFER,inst);a.BufferData(GL_ARRAY_BUFFER,GlSize(instances.size()*sizeof(P)),instances.data(),GL_DYNAMIC_DRAW);a.EnableVertexAttribArray(3);a.VertexAttribPointer(3,4,GL_FLOAT,0,sizeof(P),(void*)0);a.VertexAttribDivisor(3,1);
Mat4 vp=frame.view_projection;auto set_vp=[&](GlUInt p){a.UseProgram(p);GlInt u=a.GetUniformLocation(p,"u_vp");if(u>=0)a.UniformMatrix4fv(u,1,0,vp.m.data());};
if(pipeline.build(frame).features.enabled(RenderFeature::Terrain)){set_vp(tp);GlInt o=a.GetUniformLocation(tp,"u_origin");if(o>=0)a.Uniform3f(o,frame.camera.position.x,0,frame.camera.position.z);a.BindVertexArray(vao);a.DrawElements(GL_TRIANGLES,int(ti.size()),GL_UNSIGNED_INT,nullptr);r.draw_calls++;r.triangles+=ti.size()/3;}
if(pipeline.build(frame).features.enabled(RenderFeature::Water)){set_vp(wp);GlInt o=a.GetUniformLocation(wp,"u_origin"),t=a.GetUniformLocation(wp,"u_time");if(o>=0)a.Uniform3f(o,frame.camera.position.x,0,frame.camera.position.z);if(t>=0)a.Uniform1f(t,float(frame.frame_id)/60.f);a.Enable(GL_BLEND);a.BlendFunc(GL_SRC_ALPHA,GL_ONE_MINUS_SRC_ALPHA);a.BindVertexArray(wvao);a.DrawElements(GL_TRIANGLES,int(wi.size()),GL_UNSIGNED_INT,nullptr);a.Disable(GL_BLEND);r.draw_calls++;r.triangles+=wi.size()/3;}
if(pipeline.build(frame).features.enabled(RenderFeature::Vegetation)){a.UseProgram(vp);GlInt u=a.GetUniformLocation(vp,"u_vp"),c=a.GetUniformLocation(vp,"u_camera");if(u>=0)a.UniformMatrix4fv(u,1,0,vp.m.data());if(c>=0)a.Uniform3f(c,frame.camera.position.x,frame.camera.position.y,frame.camera.position.z);a.BindVertexArray(vvao);a.DrawElementsInstanced(GL_TRIANGLES,int(qi.size()),GL_UNSIGNED_INT,nullptr,int(instances.size()));r.draw_calls++;r.triangles+=instances.size()*2;}
a.DeleteBuffers(1,&vb);a.DeleteBuffers(1,&ib);a.DeleteVertexArrays(1,&vao);a.DeleteBuffers(1,&wvb);a.DeleteBuffers(1,&wib);a.DeleteVertexArrays(1,&wvao);a.DeleteBuffers(1,&vvb);a.DeleteBuffers(1,&vib);a.DeleteBuffers(1,&inst);a.DeleteVertexArrays(1,&vvao);a.DeleteProgram(tp);a.DeleteProgram(wp);a.DeleteProgram(vp);r.success=true;return r;}
}

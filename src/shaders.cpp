#include "exgine/shaders.hpp"
namespace exgine { namespace {
const char* pbr_vs=R"GLSL(#version 310 es
precision highp float;
layout(location=0) in vec3 a_position;
layout(location=1) in vec3 a_normal;
layout(location=2) in vec2 a_uv;
layout(std140,binding=0) uniform Camera { mat4 u_view_projection; };
uniform mat4 u_model;
out vec3 v_world_position;
out vec3 v_normal;
out vec2 v_uv;
void main(){vec4 p=u_model*vec4(a_position,1.0);v_world_position=p.xyz;v_normal=normalize(mat3(u_model)*a_normal);v_uv=a_uv;gl_Position=u_view_projection*p;}
)GLSL";
const char* pbr_fs=R"GLSL(#version 310 es
precision highp float;
layout(location=0) out vec4 o_color;
struct Light { vec4 position_type; vec4 direction_intensity; vec4 color_range; };
layout(std140,binding=1) uniform Lights { Light u_lights[256]; uint u_light_count; };
layout(std140,binding=2) uniform Material { vec4 u_base_roughness; vec4 u_metal_specular; vec4 u_emission_opacity; };
uniform vec3 u_camera_position;
uniform float u_ambient_intensity;
in vec3 v_world_position;
in vec3 v_normal;
in vec2 v_uv;
const float PI=3.14159265359;
float saturate(float x){return clamp(x,0.0,1.0);}
vec3 fresnel(vec3 f0,float cos_theta){return f0+(1.0-f0)*pow(1.0-cos_theta,5.0);}
void main(){vec3 n=normalize(v_normal);vec3 v=normalize(u_camera_position-v_world_position);vec3 base=u_base_roughness.rgb;float rough=max(u_base_roughness.a,0.045);float metallic=saturate(u_metal_specular.x);vec3 f0=mix(vec3(0.04),base,metallic);vec3 Lo=vec3(0.0);for(uint i=0u;i<u_light_count&&i<256u;++i){Light l=u_lights[i];float type=l.position_type.w;vec3 lp=l.position_type.xyz;vec3 L=type<0.5?normalize(-l.direction_intensity.xyz):normalize(lp-v_world_position);float dist=length(lp-v_world_position);float atten=type<0.5?1.0:saturate(1.0-dist/max(l.color_range.w,0.001));float ndl=saturate(dot(n,L));if(ndl<=0.0)continue;vec3 h=normalize(v+L);float ndh=saturate(dot(n,h));float vdH=saturate(dot(v,h));float a=rough*rough;float a2=a*a;float denom=ndh*ndh*(a2-1.0)+1.0;float D=a2/(PI*denom*denom);float k=(rough+1.0);k=k*k/8.0;float Gv=saturate(dot(n,v))/(saturate(dot(n,v))*(1.0-k)+k);float Gl=ndl/(ndl*(1.0-k)+k);vec3 F=fresnel(f0,vdH);vec3 spec=(D*Gv*Gl*F)/max(4.0*saturate(dot(n,v))*ndl,0.001);vec3 kd=(1.0-F)*(1.0-metallic);Lo+=(kd*base/PI+spec)*l.color_range.rgb*l.direction_intensity.w*ndl*atten;}
vec3 color=Lo+base*u_ambient_intensity+u_emission_opacity.rgb;float alpha=saturate(u_emission_opacity.a);color=color/(color+vec3(1.0));color=pow(max(color,vec3(0.0)),vec3(1.0/2.2));o_color=vec4(color,alpha);}
)GLSL";
const char* unlit_vs=R"GLSL(#version 310 es
precision highp float;
layout(location=0) in vec3 a_position;
layout(location=2) in vec2 a_uv;
layout(std140,binding=0) uniform Camera { mat4 u_view_projection; };
uniform mat4 u_model;
out vec2 v_uv;
void main(){v_uv=a_uv;gl_Position=u_view_projection*u_model*vec4(a_position,1.0);}
)GLSL";
const char* unlit_fs=R"GLSL(#version 310 es
precision highp float;
layout(location=0) out vec4 o_color;
uniform vec4 u_color;
void main(){o_color=u_color;}
)GLSL";
}
bool ShaderProgram::valid()const noexcept{return !name.empty()&&!vertex_source.empty()&&!fragment_source.empty()&&vertex_source.find("#version 310 es")!=std::string::npos&&fragment_source.find("#version 310 es")!=std::string::npos;}
ShaderProgram make_pbr_shader(){return {"pbr",pbr_vs,pbr_fs};}
ShaderProgram make_unlit_shader(){return {"unlit",unlit_vs,unlit_fs};}
ShaderLibrary::ShaderLibrary(){auto p=make_pbr_shader(),u=make_unlit_shader();if(p.valid())programs_.emplace(p.name,std::move(p));if(u.valid())programs_.emplace(u.name,std::move(u));}
const ShaderProgram* ShaderLibrary::find(std::string_view name)const noexcept{auto it=programs_.find(std::string{name});return it==programs_.end()?nullptr:&it->second;}
} // namespace exgine

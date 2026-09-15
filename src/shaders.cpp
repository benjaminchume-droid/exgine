#include "exgine/shaders.hpp"
namespace exgine { namespace {
const char* pbr_vs=R"GLSL(#version 310 es
precision highp float;
layout(location=0) in vec3 a_position; layout(location=1) in vec3 a_normal; layout(location=2) in vec2 a_uv;
layout(std140,binding=0) uniform Camera { mat4 u_view_projection; };
uniform mat4 u_model; out vec3 v_world_position; out vec3 v_normal; out vec2 v_uv;
void main(){vec4 p=u_model*vec4(a_position,1.0);v_world_position=p.xyz;v_normal=normalize(mat3(u_model)*a_normal);v_uv=a_uv;gl_Position=u_view_projection*p;}
)GLSL";
const char* pbr_fs=R"GLSL(#version 310 es
precision highp float; layout(location=0) out vec4 o_color;
struct Light{vec4 position_type;vec4 direction_intensity;vec4 color_range;};
layout(std140,binding=1) uniform Lights{Light u_lights[256];uint u_light_count;};
layout(std140,binding=2) uniform Material{vec4 u_base_roughness;vec4 u_metal_specular;vec4 u_emission_opacity;};
uniform vec3 u_camera_position;uniform float u_ambient_intensity;in vec3 v_world_position;in vec3 v_normal;in vec2 v_uv;
const float PI=3.14159265359;float saturate(float x){return clamp(x,0.0,1.0);}vec3 fresnel(vec3 f0,float c){return f0+(1.0-f0)*pow(1.0-c,5.0);}
void main(){vec3 n=normalize(v_normal);vec3 v=normalize(u_camera_position-v_world_position);vec3 base=u_base_roughness.rgb;float rough=max(u_base_roughness.a,.045);float metallic=saturate(u_metal_specular.x);vec3 f0=mix(vec3(.04),base,metallic);vec3 lo=vec3(0);for(uint i=0u;i<u_light_count&&i<256u;++i){Light l=u_lights[i];vec3 L=l.position_type.w<.5?normalize(-l.direction_intensity.xyz):normalize(l.position_type.xyz-v_world_position);float dist=length(l.position_type.xyz-v_world_position);float att=l.position_type.w<.5?1.0:saturate(1.0-dist/max(l.color_range.w,.001));float ndl=saturate(dot(n,L));if(ndl<=0.)continue;vec3 h=normalize(v+L);float ndh=saturate(dot(n,h));float vdh=saturate(dot(v,h));float a=rough*rough,a2=a*a;float den=ndh*ndh*(a2-1.)+1.;float D=a2/(PI*den*den);float k=(rough+1.);k=k*k/8.;float nv=saturate(dot(n,v));float Gv=nv/(nv*(1.-k)+k);float Gl=ndl/(ndl*(1.-k)+k);vec3 F=fresnel(f0,vdh);vec3 spec=(D*Gv*Gl*F)/max(4.*nv*ndl,.001);vec3 kd=(1.-F)*(1.-metallic);lo+=(kd*base/PI+spec)*l.color_range.rgb*l.direction_intensity.w*ndl*att;}vec3 color=lo+base*u_ambient_intensity+u_emission_opacity.rgb;color=color/(color+1.);color=pow(max(color,vec3(0)),vec3(1./2.2));o_color=vec4(color,saturate(u_emission_opacity.a));}
)GLSL";
const char* mobile_vs=R"GLSL(#version 310 es
precision highp float;layout(location=0)in vec3 a_position;layout(location=1)in vec3 a_normal;layout(location=2)in vec2 a_uv;uniform mat4 u_view_projection;uniform mat4 u_model;out vec3 v_normal;out vec3 v_world_position;void main(){vec4 p=u_model*vec4(a_position,1.);v_world_position=p.xyz;v_normal=normalize(mat3(u_model)*a_normal);gl_Position=u_view_projection*p;}
)GLSL";
const char* mobile_fs=R"GLSL(#version 310 es
precision highp float;layout(location=0)out vec4 o_color;uniform vec4 u_base_roughness;uniform vec4 u_metal_specular;uniform vec4 u_emission_opacity;uniform vec3 u_camera_position;uniform float u_ambient_intensity;uniform vec3 u_light_direction;uniform vec3 u_light_color;uniform float u_light_intensity;in vec3 v_normal;in vec3 v_world_position;void main(){vec3 n=normalize(v_normal);vec3 l=normalize(-u_light_direction);float ndl=max(dot(n,l),0.);vec3 base=u_base_roughness.rgb;float metallic=clamp(u_metal_specular.x,0.,1.);vec3 f0=mix(vec3(.04),base,metallic);vec3 v=normalize(u_camera_position-v_world_position);vec3 h=normalize(l+v);float spec=pow(max(dot(n,h),0.),mix(64.,4.,u_base_roughness.a))*mix(.04,1.,metallic);vec3 color=base*(u_ambient_intensity+ndl*u_light_intensity*u_light_color)+f0*spec+u_emission_opacity.rgb;color=color/(color+1.);color=pow(max(color,vec3(0)),vec3(1./2.2));o_color=vec4(color,clamp(u_emission_opacity.a,0.,1.));}
)GLSL";
const char* unlit_vs=R"GLSL(#version 310 es
precision highp float;layout(location=0)in vec3 a_position;layout(location=2)in vec2 a_uv;layout(std140,binding=0)uniform Camera{mat4 u_view_projection;};uniform mat4 u_model;out vec2 v_uv;void main(){v_uv=a_uv;gl_Position=u_view_projection*u_model*vec4(a_position,1.);}
)GLSL";
const char* unlit_fs=R"GLSL(#version 310 es
precision highp float;layout(location=0)out vec4 o_color;uniform vec4 u_color;void main(){o_color=u_color;}
)GLSL";
}
bool ShaderProgram::valid()const noexcept{return !name.empty()&&!vertex_source.empty()&&!fragment_source.empty()&&vertex_source.find("#version 310 es")!=std::string::npos&&fragment_source.find("#version 310 es")!=std::string::npos;}
ShaderProgram make_pbr_shader(){return {"pbr",pbr_vs,pbr_fs};} ShaderProgram make_unlit_shader(){return {"unlit",unlit_vs,unlit_fs};} ShaderProgram make_mobile_pbr_shader(){return {"mobile_pbr",mobile_vs,mobile_fs};}
ShaderLibrary::ShaderLibrary(){auto p=make_pbr_shader(),u=make_unlit_shader(),m=make_mobile_pbr_shader();if(p.valid())programs_.emplace(p.name,std::move(p));if(u.valid())programs_.emplace(u.name,std::move(u));if(m.valid())programs_.emplace(m.name,std::move(m));}
const ShaderProgram* ShaderLibrary::find(std::string_view name)const noexcept{auto it=programs_.find(std::string{name});return it==programs_.end()?nullptr:&it->second;}
} // namespace exgine
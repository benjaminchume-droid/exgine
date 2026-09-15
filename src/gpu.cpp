#include "exgine/gpu.hpp"
#include "exgine/shaders.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

namespace exgine {
namespace {
constexpr GlEnum COLOR=0x00004000u,DEPTH=0x00000100u,DEPTH_TEST=0x0B71u,CULL=0x0B44u,BLEND=0x0BE2u;
constexpr GlEnum LESS=0x0201u,SRC_ALPHA=0x0302u,ONE_MINUS_SRC_ALPHA=0x0303u;
constexpr GlEnum VS=0x8B31u,FS=0x8B30u,COMPILE=0x8B81u,LINK=0x8B82u,LOG_LEN=0x8B84u;
constexpr GlEnum ARRAY=0x8892u,ELEMENT=0x8893u,STATIC_DRAW=0x88E4u,FLOAT=0x1406u,UNSIGNED_INT=0x1405u,TRIANGLES=0x0004u;
constexpr GlEnum TEX2D=0x0DE1u,TEX0=0x84C0u,MIN_FILTER=0x2801u,MAG_FILTER=0x2800u,WRAP_S=0x2802u,WRAP_T=0x2803u;
constexpr GlEnum LINEAR=0x2601u,LINEAR_MIPMAP_LINEAR=0x2703u,REPEAT=0x2901u,RGBA8=0x8058u,RGBA=0x1908u,U8=0x1401u;

std::uint64_t mesh_key(const RenderDrawCall& draw) noexcept {
    const auto a=draw.geometry.get();
    const auto b=draw.skinned_mesh.get();
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(a))^
           (static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(b))<<1)^
           (static_cast<std::uint64_t>(draw.part_index)+0x9e3779b97f4a7c15ULL);
}
std::uint64_t tex_key(const std::shared_ptr<const Texture2D>& texture) noexcept {
    return static_cast<std::uint64_t>(reinterpret_cast<std::uintptr_t>(texture.get()));
}

GlInt uniform(const OpenGLESApi& api,GlUInt program,const char* name) noexcept {
    return api.GetUniformLocation?api.GetUniformLocation(program,name):-1;
}
void set_common_uniforms(const OpenGLESApi& api,GlUInt program,GlInt vp,GlInt model,GlInt camera,GlInt ambient,GlInt rough,GlInt metal,GlInt emission,GlInt light_dir,GlInt light_color,GlInt light_intensity,const RenderFrame& frame,const RenderDrawCall& draw) {
    if(vp>=0) api.UniformMatrix4fv(vp,1,0,frame.view_projection.m.data());
    if(model>=0) api.UniformMatrix4fv(model,1,0,draw.model.m.data());
    if(camera>=0) api.Uniform3f(camera,frame.camera.position.x,frame.camera.position.y,frame.camera.position.z);
    const auto& m=draw.material.material;
    if(ambient>=0) api.Uniform1f(ambient,frame.lighting.environment.ambient_intensity);
    if(rough>=0) api.Uniform1f(rough,std::clamp(m.roughness,0.045f,1.0f));
    if(metal>=0) api.Uniform1f(metal,std::clamp(m.metallic,0.0f,1.0f));
    if(emission>=0) api.Uniform4f(emission,m.emission.r,m.emission.g,m.emission.b,std::clamp(m.opacity,0.0f,1.0f));
    if(!frame.lights.empty()) {
        const auto& l=frame.lights.front();
        if(light_dir>=0) api.Uniform3f(light_dir,l.direction.x,l.direction.y,l.direction.z);
        if(light_color>=0) api.Uniform3f(light_color,l.color.r,l.color.g,l.color.b);
        if(light_intensity>=0) api.Uniform1f(light_intensity,l.intensity);
    }
}
}

bool OpenGLESApi::complete() const noexcept {
    return Clear&&ClearColor&&Viewport&&Enable&&Disable&&DepthFunc&&BlendFunc&&
           CreateShader&&ShaderSource&&CompileShader&&GetShaderiv&&GetShaderInfoLog&&DeleteShader&&
           CreateProgram&&AttachShader&&LinkProgram&&GetProgramiv&&GetProgramInfoLog&&UseProgram&&
           DeleteProgram&&GetUniformLocation&&UniformMatrix4fv&&Uniform3f&&Uniform4f&&Uniform1f&&Uniform1i&&
           GenBuffers&&BindBuffer&&BufferData&&DeleteBuffers&&GenVertexArrays&&BindVertexArray&&
           DeleteVertexArrays&&EnableVertexAttribArray&&VertexAttribPointer&&DrawElements;
}

bool OpenGLESRenderer::ready() const noexcept { return api_.complete()&&program_!=0&&skinned_program_!=0; }
bool OpenGLESRenderer::texture_ready() const noexcept {
    return ready()&&textured_program_!=0&&skinned_textured_program_!=0&&api_.GenTextures&&api_.BindTexture&&
           api_.TexParameteri&&api_.TexImage2D&&api_.GenerateMipmap&&api_.ActiveTexture&&api_.DeleteTextures;
}

bool OpenGLESRenderer::compile_shader(GlEnum type,const char* source,GlUInt& shader,std::string& error) {
    shader=api_.CreateShader(type);
    if(!shader){ error="glCreateShader failed"; return false; }
    const GlInt length=static_cast<GlInt>(std::strlen(source));
    api_.ShaderSource(shader,1,&source,&length);
    api_.CompileShader(shader);
    GlInt ok=0; api_.GetShaderiv(shader,COMPILE,&ok);
    if(ok) return true;
    GlInt log_len=0,written=0; api_.GetShaderiv(shader,LOG_LEN,&log_len);
    std::vector<char> log(static_cast<std::size_t>(std::max(1,log_len)),0);
    api_.GetShaderInfoLog(shader,log_len,&written,log.data());
    error.assign(log.data(),static_cast<std::size_t>(std::max(0,written)));
    api_.DeleteShader(shader); shader=0; return false;
}

bool OpenGLESRenderer::link_program(const char*,GlUInt vertex,const char* fragment,GlUInt& program,std::string& error) {
    GlUInt frag=0;
    if(!compile_shader(FS,fragment,frag,error)) { if(vertex) api_.DeleteShader(vertex); return false; }
    program=api_.CreateProgram();
    if(!program) { api_.DeleteShader(vertex); api_.DeleteShader(frag); error="glCreateProgram failed"; return false; }
    api_.AttachShader(program,vertex); api_.AttachShader(program,frag); api_.LinkProgram(program);
    api_.DeleteShader(vertex); api_.DeleteShader(frag);
    GlInt ok=0; api_.GetProgramiv(program,LINK,&ok);
    if(ok) return true;
    GlInt log_len=0,written=0; api_.GetProgramiv(program,LOG_LEN,&log_len);
    std::vector<char> log(static_cast<std::size_t>(std::max(1,log_len)),0);
    api_.GetProgramInfoLog(program,log_len,&written,log.data());
    error.assign(log.data(),static_cast<std::size_t>(std::max(0,written)));
    api_.DeleteProgram(program); program=0; return false;
}

bool OpenGLESRenderer::ensure_programs(std::string& error) {
    if(program_&&skinned_program_) {
        if(api_.GenTextures&&api_.BindTexture&&api_.TexParameteri&&api_.TexImage2D&&api_.GenerateMipmap&&api_.ActiveTexture&&api_.DeleteTextures&&(!textured_program_||!skinned_textured_program_)) {
            const auto textured=make_mobile_textured_pbr_shader();
            const auto skinned_textured=make_mobile_skinned_textured_pbr_shader();
            GlUInt vs=0;
            if(!compile_shader(VS,textured.vertex_source.c_str(),vs,error)||!link_program(textured.vertex_source.c_str(),vs,textured.fragment_source.c_str(),textured_program_,error)) return false;
            if(!compile_shader(VS,skinned_textured.vertex_source.c_str(),vs,error)||!link_program(skinned_textured.vertex_source.c_str(),vs,skinned_textured.fragment_source.c_str(),skinned_textured_program_,error)) return false;
        }
        return true;
    }
    if(!api_.complete()){ error="incomplete GLES API"; return false; }
    const auto p=make_mobile_pbr_shader();
    const auto sp=make_mobile_skinned_pbr_shader();
    GlUInt vs=0;
    if(!compile_shader(VS,p.vertex_source.c_str(),vs,error)||!link_program(p.vertex_source.c_str(),vs,p.fragment_source.c_str(),program_,error)) return false;
    if(!compile_shader(VS,sp.vertex_source.c_str(),vs,error)||!link_program(sp.vertex_source.c_str(),vs,sp.fragment_source.c_str(),skinned_program_,error)) { release(); return false; }

    u_vp_=uniform(api_,program_,"u_view_projection"); u_model_=uniform(api_,program_,"u_model"); u_camera_=uniform(api_,program_,"u_camera_position");
    u_ambient_=uniform(api_,program_,"u_ambient_intensity"); u_base_=uniform(api_,program_,"u_base_roughness"); u_rough_=uniform(api_,program_,"u_base_roughness");
    u_metal_=uniform(api_,program_,"u_metal_specular"); u_emission_=uniform(api_,program_,"u_emission_opacity"); u_opacity_=u_emission_;
    u_light_dir_=uniform(api_,program_,"u_light_direction"); u_light_color_=uniform(api_,program_,"u_light_color"); u_light_intensity_=uniform(api_,program_,"u_light_intensity");
    s_vp_=uniform(api_,skinned_program_,"u_view_projection"); s_model_=uniform(api_,skinned_program_,"u_model"); s_camera_=uniform(api_,skinned_program_,"u_camera_position");
    s_ambient_=uniform(api_,skinned_program_,"u_ambient_intensity"); s_base_=uniform(api_,skinned_program_,"u_base_roughness"); s_metal_=uniform(api_,skinned_program_,"u_metal_specular"); s_emission_=uniform(api_,skinned_program_,"u_emission_opacity");
    s_light_dir_=uniform(api_,skinned_program_,"u_light_direction"); s_light_color_=uniform(api_,skinned_program_,"u_light_color"); s_light_intensity_=uniform(api_,skinned_program_,"u_light_intensity");
    s_bones_=uniform(api_,skinned_program_,"u_bones[0]"); s_bone_count_=uniform(api_,skinned_program_,"u_bone_count");

    if(api_.GenTextures&&api_.BindTexture&&api_.TexParameteri&&api_.TexImage2D&&api_.GenerateMipmap&&api_.ActiveTexture&&api_.DeleteTextures) {
        const auto tp=make_mobile_textured_pbr_shader(); const auto stp=make_mobile_skinned_textured_pbr_shader();
        if(!compile_shader(VS,tp.vertex_source.c_str(),vs,error)||!link_program(tp.vertex_source.c_str(),vs,tp.fragment_source.c_str(),textured_program_,error)) { release(); return false; }
        if(!compile_shader(VS,stp.vertex_source.c_str(),vs,error)||!link_program(stp.vertex_source.c_str(),vs,stp.fragment_source.c_str(),skinned_textured_program_,error)) { release(); return false; }
        auto q=[&](GlUInt prog,const char* name){return uniform(api_,prog,name);};
        tu_vp_=q(textured_program_,"u_view_projection"); tu_model_=q(textured_program_,"u_model"); tu_camera_=q(textured_program_,"u_camera_position"); tu_ambient_=q(textured_program_,"u_ambient_intensity");
        tu_base_=q(textured_program_,"u_base_roughness"); tu_metal_=q(textured_program_,"u_metal_specular"); tu_emission_=q(textured_program_,"u_emission_opacity"); tu_light_dir_=q(textured_program_,"u_light_direction");
        tu_light_color_=q(textured_program_,"u_light_color"); tu_light_intensity_=q(textured_program_,"u_light_intensity");
        t_base_=q(textured_program_,"u_base_texture"); t_rough_=q(textured_program_,"u_roughness_texture"); t_metal_=q(textured_program_,"u_metallic_texture"); t_normal_=q(textured_program_,"u_normal_texture");
        t_ao_=q(textured_program_,"u_ao_texture"); t_emission_=q(textured_program_,"u_emission_texture"); t_opacity_=q(textured_program_,"u_opacity_texture"); t_use_=q(textured_program_,"u_use_textures");
        stu_vp_=q(skinned_textured_program_,"u_view_projection"); stu_model_=q(skinned_textured_program_,"u_model"); stu_camera_=q(skinned_textured_program_,"u_camera_position"); stu_ambient_=q(skinned_textured_program_,"u_ambient_intensity");
        stu_base_=q(skinned_textured_program_,"u_base_roughness"); stu_metal_=q(skinned_textured_program_,"u_metal_specular"); stu_emission_=q(skinned_textured_program_,"u_emission_opacity"); stu_light_dir_=q(skinned_textured_program_,"u_light_direction");
        stu_light_color_=q(skinned_textured_program_,"u_light_color"); stu_light_intensity_=q(skinned_textured_program_,"u_light_intensity"); stu_bones_=q(skinned_textured_program_,"u_bones[0]"); stu_bone_count_=q(skinned_textured_program_,"u_bone_count");
        st_base_=q(skinned_textured_program_,"u_base_texture"); st_rough_=q(skinned_textured_program_,"u_roughness_texture"); st_metal_=q(skinned_textured_program_,"u_metallic_texture"); st_normal_=q(skinned_textured_program_,"u_normal_texture");
        st_ao_=q(skinned_textured_program_,"u_ao_texture"); st_emission_=q(skinned_textured_program_,"u_emission_texture"); st_opacity_=q(skinned_textured_program_,"u_opacity_texture"); st_use_=q(skinned_textured_program_,"u_use_textures");
    }
    return true;
}

bool OpenGLESRenderer::ensure_mesh(const RenderDrawCall& draw,std::uint64_t frame,GpuCachedMesh*& out,std::string& error) {
    const auto key=mesh_key(draw);
    if(auto it=meshes_.find(key);it!=meshes_.end()){it->second.last_used_frame=frame;out=&it->second;return it->second.valid();}
    const bool skinned=draw.animated(); std::vector<float> vertices; std::vector<std::uint32_t> indices;
    if(skinned) {
        if(!draw.skinned_mesh){error="null skinned mesh";return false;}
        for(const auto& v:draw.skinned_mesh->vertices) {
            vertices.insert(vertices.end(),{v.position.x,v.position.y,v.position.z,v.normal.x,v.normal.y,v.normal.z,v.uv.x,v.uv.y,
                                             static_cast<float>(v.skin.bones[0]),static_cast<float>(v.skin.bones[1]),static_cast<float>(v.skin.bones[2]),static_cast<float>(v.skin.bones[3]),
                                             v.skin.weights[0],v.skin.weights[1],v.skin.weights[2],v.skin.weights[3]});
        }
        indices=draw.skinned_mesh->indices;
    } else {
        if(!draw.geometry||draw.part_index>=draw.geometry->parts.size()||!draw.geometry->parts[draw.part_index].mesh.valid()){error="invalid mesh";return false;}
        const auto& mesh=draw.geometry->parts[draw.part_index].mesh;
        for(const auto& v:mesh.vertices) vertices.insert(vertices.end(),{v.position.x,v.position.y,v.position.z,v.normal.x,v.normal.y,v.normal.z,v.uv.x,v.uv.y});
        indices=mesh.indices;
    }
    GpuCachedMesh cached; cached.animated=skinned; cached.last_used_frame=frame; cached.index_count=indices.size(); cached.vertex_count=vertices.size()/(skinned?16u:8u);
    api_.GenVertexArrays(1,&cached.vao); api_.GenBuffers(1,&cached.vertex_buffer); api_.GenBuffers(1,&cached.index_buffer);
    if(!cached.valid()){error="GPU buffer allocation failed";return false;}
    api_.BindVertexArray(cached.vao); api_.BindBuffer(ARRAY,cached.vertex_buffer); api_.BufferData(ARRAY,static_cast<GlSize>(vertices.size()*sizeof(float)),vertices.data(),STATIC_DRAW);
    api_.BindBuffer(ELEMENT,cached.index_buffer); api_.BufferData(ELEMENT,static_cast<GlSize>(indices.size()*sizeof(std::uint32_t)),indices.data(),STATIC_DRAW);
    const GlInt stride=static_cast<GlInt>((skinned?16u:8u)*sizeof(float));
    api_.EnableVertexAttribArray(0); api_.VertexAttribPointer(0,3,FLOAT,0,stride,nullptr);
    api_.EnableVertexAttribArray(1); api_.VertexAttribPointer(1,3,FLOAT,0,stride,reinterpret_cast<const void*>(3*sizeof(float)));
    api_.EnableVertexAttribArray(2); api_.VertexAttribPointer(2,2,FLOAT,0,stride,reinterpret_cast<const void*>(6*sizeof(float)));
    if(skinned){api_.EnableVertexAttribArray(3);api_.VertexAttribPointer(3,4,FLOAT,0,stride,reinterpret_cast<const void*>(8*sizeof(float)));api_.EnableVertexAttribArray(4);api_.VertexAttribPointer(4,4,FLOAT,0,stride,reinterpret_cast<const void*>(12*sizeof(float)));}
    const auto [it,inserted]=meshes_.emplace(key,cached); if(!inserted){error="mesh cache insert failed";return false;} out=&it->second; return true;
}

bool OpenGLESRenderer::ensure_texture(const std::shared_ptr<const Texture2D>& texture,std::uint64_t frame,GpuCachedTexture*& out,std::string& error) {
    (void)frame;
    if(!texture_ready()||!texture||!texture->valid()){error="texture residency unavailable";return false;}
    const auto key=tex_key(texture); if(auto it=textures_.find(key);it!=textures_.end()){out=&it->second;return it->second.valid();}
    std::vector<std::uint8_t> rgba(static_cast<std::size_t>(texture->width)*texture->height*4u,255u);
    for(std::uint32_t y=0;y<texture->height;++y) for(std::uint32_t x=0;x<texture->width;++x) {
        const auto src=texture->index(x,y,0); const auto dst=(static_cast<std::size_t>(y)*texture->width+x)*4u;
        const auto sample=[&](std::uint32_t c,std::uint8_t fallback){return c<texture->channels?static_cast<std::uint8_t>(std::clamp(texture->data[src+c],0.0f,1.0f)*255.0f+0.5f):fallback;};
        rgba[dst]=sample(0,255); rgba[dst+1]=sample(1,rgba[dst]); rgba[dst+2]=sample(2,rgba[dst]); rgba[dst+3]=sample(3,255);
    }
    GpuCachedTexture cached; cached.width=texture->width; cached.height=texture->height; cached.source_key=key; api_.GenTextures(1,&cached.handle);
    if(!cached.handle){error="texture allocation failed";return false;}
    api_.ActiveTexture(TEX0); api_.BindTexture(TEX2D,cached.handle); api_.TexParameteri(TEX2D,MIN_FILTER,static_cast<GlInt>(LINEAR_MIPMAP_LINEAR)); api_.TexParameteri(TEX2D,MAG_FILTER,static_cast<GlInt>(LINEAR)); api_.TexParameteri(TEX2D,WRAP_S,static_cast<GlInt>(REPEAT)); api_.TexParameteri(TEX2D,WRAP_T,static_cast<GlInt>(REPEAT));
    api_.TexImage2D(TEX2D,0,static_cast<GlInt>(RGBA8),static_cast<GlInt>(texture->width),static_cast<GlInt>(texture->height),0,RGBA,U8,rgba.data()); api_.GenerateMipmap(TEX2D);
    const auto [it,inserted]=textures_.emplace(key,cached); if(!inserted){api_.DeleteTextures(1,&cached.handle);error="texture cache insert failed";return false;} out=&it->second; return true;
}

GpuSubmitResult OpenGLESRenderer::submit(const RenderFrame& frame) {
    GpuSubmitResult result;
    if(!frame.valid()){result.error="invalid render frame";return result;}
    if(frame.config.backend!=RenderBackend::OpenGLES){result.error="wrong backend";return result;}
    std::string error; if(!ensure_programs(error)){result.error=error;return result;}
    api_.Viewport(0,0,static_cast<GlInt>(frame.config.width),static_cast<GlInt>(frame.config.height));
    const auto sky=frame.lighting.environment.sky_color; api_.ClearColor(sky.r,sky.g,sky.b,1.0f); api_.Clear(COLOR|DEPTH); api_.Enable(DEPTH_TEST); api_.DepthFunc(LESS); api_.Enable(CULL);
    for(const auto& draw:frame.draws) {
        GpuCachedMesh* mesh=nullptr; if(!ensure_mesh(draw,frame.frame_id,mesh,error)){result.error=error;return result;}
        const bool skinned=draw.animated(); const bool textured=texture_ready()&&draw.material.textures&&draw.material.textures->valid();
        GpuCachedTexture* maps[7]{};
        if(textured){const auto& set=draw.material.textures; const std::shared_ptr<const Texture2D> texs[7]={set->base_color,set->roughness,set->metallic,set->normal,set->ambient_occlusion,set->emission,set->opacity}; for(int i=0;i<7;++i) if(!ensure_texture(texs[i],frame.frame_id,maps[i],error)){result.error=error;return result;}}
        GlUInt program=0; GlInt vp=-1,model=-1,camera=-1,ambient=-1,rough=-1,metal=-1,emission=-1,ld=-1,lc=-1,li=-1,bones=-1,bone_count=-1;
        if(skinned){program=textured?skinned_textured_program_:skinned_program_;vp=textured?stu_vp_:s_vp_;model=textured?stu_model_:s_model_;camera=textured?stu_camera_:s_camera_;ambient=textured?stu_ambient_:s_ambient_;rough=textured?stu_base_:s_base_;metal=textured?stu_metal_:s_metal_;emission=textured?stu_emission_:s_emission_;ld=textured?stu_light_dir_:s_light_dir_;lc=textured?stu_light_color_:s_light_color_;li=textured?stu_light_intensity_:s_light_intensity_;bones=textured?stu_bones_:s_bones_;bone_count=textured?stu_bone_count_:s_bone_count_;}
        else {program=textured?textured_program_:program_;vp=textured?tu_vp_:u_vp_;model=textured?tu_model_:u_model_;camera=textured?tu_camera_:u_camera_;ambient=textured?tu_ambient_:u_ambient_;rough=textured?tu_base_:u_rough_;metal=textured?tu_metal_:u_metal_;emission=textured?tu_emission_:u_emission_;ld=textured?tu_light_dir_:u_light_dir_;lc=textured?tu_light_color_:u_light_color_;li=textured?tu_light_intensity_:u_light_intensity_;}
        if(program==0){result.error="shader program unavailable";return result;}
        api_.UseProgram(program); set_common_uniforms(api_,program,vp,model,camera,ambient,rough,metal,emission,ld,lc,li,frame,draw);
        if(skinned&&bones>=0&&!draw.bone_palette.empty()){api_.UniformMatrix4fv(bones,static_cast<GlInt>(std::min<std::size_t>(draw.bone_palette.size(),128)),0,draw.bone_palette.front().m.data());if(bone_count>=0)api_.Uniform1i(bone_count,static_cast<GlInt>(std::min<std::size_t>(draw.bone_palette.size(),128)));}
        if(textured){const GlInt locs[7]={skinned?st_base_:t_base_,skinned?st_rough_:t_rough_,skinned?st_metal_:t_metal_,skinned?st_normal_:t_normal_,skinned?st_ao_:t_ao_,skinned?st_emission_:t_emission_,skinned?st_opacity_:t_opacity_};for(int i=0;i<7;++i){api_.ActiveTexture(TEX0+static_cast<GlEnum>(i));api_.BindTexture(TEX2D,maps[i]->handle);if(locs[i]>=0)api_.Uniform1i(locs[i],i);}const GlInt use=skinned?st_use_:t_use_;if(use>=0)api_.Uniform1i(use,1);}
        api_.BindVertexArray(mesh->vao); const bool transparent=draw.pass==RenderPass::Transparent||draw.material.material.opacity<0.999f; if(transparent){api_.Enable(BLEND);api_.BlendFunc(SRC_ALPHA,ONE_MINUS_SRC_ALPHA);} else api_.Disable(BLEND);
        api_.DrawElements(TRIANGLES,static_cast<GlInt>(mesh->index_count),UNSIGNED_INT,nullptr); ++result.draw_calls; result.triangles+=mesh->index_count/3; if(skinned)++result.animated_draw_calls;
    }
    result.success=true;return result;
}

void OpenGLESRenderer::release_meshes() noexcept {
    if(!api_.DeleteVertexArrays||!api_.DeleteBuffers)return;
    for(auto& [key,mesh]:meshes_){(void)key;if(mesh.vao)api_.DeleteVertexArrays(1,&mesh.vao);if(mesh.vertex_buffer)api_.DeleteBuffers(1,&mesh.vertex_buffer);if(mesh.index_buffer)api_.DeleteBuffers(1,&mesh.index_buffer);}
    meshes_.clear();
}
void OpenGLESRenderer::release_textures() noexcept { if(!api_.DeleteTextures)return; for(auto& [key,texture]:textures_){(void)key;if(texture.handle)api_.DeleteTextures(1,&texture.handle);} textures_.clear(); }
void OpenGLESRenderer::release() noexcept {
    release_meshes(); release_textures();
    if(api_.DeleteProgram){if(program_)api_.DeleteProgram(program_);if(skinned_program_)api_.DeleteProgram(skinned_program_);if(textured_program_)api_.DeleteProgram(textured_program_);if(skinned_textured_program_)api_.DeleteProgram(skinned_textured_program_);}
    program_=skinned_program_=textured_program_=skinned_textured_program_=0;
}

} // namespace exgine

#include "exgine/gles_tracks.hpp"
#include <algorithm>
#include <cmath>
#include <utility>
namespace exgine {
namespace {
constexpr GlEnum GL_VERSION=0x1F02,GL_EXTENSIONS=0x1F03,GL_MAX_TEXTURE_SIZE=0x0D33,GL_MAX_TEXTURE_UNITS=0x8872,GL_MAX_SSBO_BINDINGS=0x90DD,GL_MAX_COMPUTE_INVOCATIONS=0x90EB,GL_FRAGMENT_SHADER=0x8B30,GL_HIGH_FLOAT=0x8DF2;
constexpr GlEnum GL_TEXTURE_CUBE_MAP=0x8513,GL_TEXTURE_CUBE_MAP_POSITIVE_X=0x8515,GL_TEXTURE_MIN_FILTER=0x2801,GL_TEXTURE_MAG_FILTER=0x2800,GL_TEXTURE_WRAP_S=0x2802,GL_TEXTURE_WRAP_T=0x2803,GL_TEXTURE_WRAP_R=0x8072,GL_CLAMP_TO_EDGE=0x812F,GL_LINEAR=0x2601,GL_LINEAR_MIPMAP_LINEAR=0x2703,GL_RGBA=0x1908,GL_RGBA16F=0x881A,GL_UNSIGNED_BYTE=0x1401,GL_HALF_FLOAT=0x140B;
std::pair<int,int> parse_version(std::string_view text) noexcept { int major=0,minor=0; const auto p=text.find("OpenGL ES "); std::size_t i=p==std::string_view::npos?0:p+10; while(i<text.size()&&text[i]>='0'&&text[i]<='9'){major=major*10+text[i]-'0';++i;} if(i<text.size()&&text[i]=='.'){++i;while(i<text.size()&&text[i]>='0'&&text[i]<='9'){minor=minor*10+text[i]-'0';++i;}} return {major,minor}; }
bool contains(std::string_view s,std::string_view n) noexcept { return s.find(n)!=std::string_view::npos; }
}
GlesCapabilities profile_gles(OpenGLESApi& api,std::string_view version,std::string_view extensions) noexcept {
    GlesCapabilities caps{};
    if(api.GetString){if(const auto* v=api.GetString(GL_VERSION))version=reinterpret_cast<const char*>(v);if(const auto* e=api.GetString(GL_EXTENSIONS))extensions=reinterpret_cast<const char*>(e);}
    const auto [major,minor]=parse_version(version); caps.major=major;caps.minor=minor;caps.es31=major>3||(major==3&&minor>=1);
    caps.max_texture_size=caps.es31?16384:4096; caps.max_texture_units=caps.es31?16:8;
    if(api.GetIntegerv){GlInt v=0;api.GetIntegerv(GL_MAX_TEXTURE_SIZE,&v);if(v>0)caps.max_texture_size=v;api.GetIntegerv(GL_MAX_TEXTURE_UNITS,&v);if(v>0)caps.max_texture_units=v;api.GetIntegerv(GL_MAX_SSBO_BINDINGS,&v);if(v>0)caps.max_ssbo_bindings=v;api.GetIntegerv(GL_MAX_COMPUTE_INVOCATIONS,&v);if(v>0)caps.max_compute_invocations=v;}
    caps.compute=caps.es31; caps.ssbo=caps.es31&&api.BindBufferBase; caps.indirect=caps.es31&&api.DrawElementsInstanced; caps.cubemap=api.GenTextures&&api.BindTexture&&api.TexImage2D; caps.color_buffer_float=caps.es31||contains(extensions,"EXT_color_buffer_float")||contains(extensions,"EXT_color_buffer_half_float");caps.depth24=caps.es31||contains(extensions,"OES_depth24");
    if(api.GetShaderPrecisionFormat){GlInt range[2]={0,0},precision=0;api.GetShaderPrecisionFormat(GL_FRAGMENT_SHADER,GL_HIGH_FLOAT,range,&precision);caps.highp_fragment=precision>0;}else caps.highp_fragment=caps.es31;
    return caps;
}
std::array<CubeFaceCapture,6> cube_faces() noexcept { return {{{GL_TEXTURE_CUBE_MAP_POSITIVE_X,{1,0,0},{0,-1,0}},{GL_TEXTURE_CUBE_MAP_POSITIVE_X+1,{-1,0,0},{0,-1,0}},{GL_TEXTURE_CUBE_MAP_POSITIVE_X+2,{0,1,0},{0,0,1}},{GL_TEXTURE_CUBE_MAP_POSITIVE_X+3,{0,-1,0},{0,0,-1}},{GL_TEXTURE_CUBE_MAP_POSITIVE_X+4,{0,0,1},{0,-1,0}},{GL_TEXTURE_CUBE_MAP_POSITIVE_X+5,{0,0,-1},{0,-1,0}}}}; }
bool create_cubemap(OpenGLESApi& api,std::uint32_t size,std::uint32_t mip_levels,GlInt internal_format,GlesCubeResource& out,std::string& error){
    if(!api.GenTextures||!api.BindTexture||!api.TexImage2D||size==0||mip_levels==0){error="cubemap API unavailable or dimensions invalid";return false;}
    if(out.texture&&api.DeleteTextures)api.DeleteTextures(1,&out.texture); out={}; api.GenTextures(1,&out.texture); if(!out.texture){error="cubemap allocation failed";return false;}
    api.BindTexture(GL_TEXTURE_CUBE_MAP,out.texture);api.TexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MIN_FILTER,mip_levels>1?GL_LINEAR_MIPMAP_LINEAR:GL_LINEAR);api.TexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_MAG_FILTER,GL_LINEAR);api.TexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_S,GL_CLAMP_TO_EDGE);api.TexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_T,GL_CLAMP_TO_EDGE);api.TexParameteri(GL_TEXTURE_CUBE_MAP,GL_TEXTURE_WRAP_R,GL_CLAMP_TO_EDGE);
    for(std::uint32_t mip=0;mip<mip_levels;++mip){const GlInt extent=(GlInt)std::max(1u,size>>mip);for(const auto& face:cube_faces())api.TexImage2D(face.target,(GlInt)mip,internal_format,extent,extent,0,GL_RGBA,internal_format==GL_RGBA16F?GL_HALF_FLOAT:GL_UNSIGNED_BYTE,nullptr);} out={out.texture,size,mip_levels,true}; return true;
}
bool upload_cubemap_face(OpenGLESApi& api,const GlesCubeResource& cube,std::uint32_t mip,GlEnum face,GlInt internal_format,GlEnum format,GlEnum type,const void* pixels,std::uint32_t size){if(!cube.valid||mip>=cube.mip_levels||!api.BindTexture||!api.TexImage2D)return false;api.BindTexture(GL_TEXTURE_CUBE_MAP,cube.texture);api.TexImage2D(face,(GlInt)mip,internal_format,(GlInt)size,(GlInt)size,0,format,type,pixels);return true;}
TerrainGpuPatch build_terrain_patch(std::uint32_t resolution,std::uint8_t stitch_mask,float morph) noexcept {
    TerrainGpuPatch p{};p.resolution=std::max(2u,resolution);p.stitch_mask=stitch_mask;p.morph=std::clamp(morph,0.f,1.f);const auto n=p.resolution;p.vertices.reserve(n*n);
    for(std::uint32_t z=0;z<n;++z)for(std::uint32_t x=0;x<n;++x){const float fx=float(x)/float(n-1),fz=float(z)/float(n-1);const bool edge=((stitch_mask&1u)&&z==n-1)||((stitch_mask&2u)&&x==n-1)||((stitch_mask&4u)&&z==0)||((stitch_mask&8u)&&x==0);p.vertices.push_back({{fx,0,fz},edge?p.morph:0.f});}
    p.indices.reserve((n-1)*(n-1)*6);auto id=[n](std::uint32_t x,std::uint32_t z){return z*n+x;};for(std::uint32_t z=0;z+1<n;++z)for(std::uint32_t x=0;x+1<n;++x)p.indices.insert(p.indices.end(),{id(x,z),id(x+1,z),id(x+1,z+1),id(x,z),id(x+1,z+1),id(x,z+1)});return p;
}
std::vector<VegetationBatch> build_vegetation_batches(const std::vector<RenderDrawCall>& draws){std::vector<VegetationBatch> batches;std::unordered_map<std::uint64_t,std::size_t> lookup;for(const auto& d:draws){if(d.animated()||d.pass!=RenderPass::Opaque||!d.geometry||d.part_index>=d.geometry->parts.size())continue;const auto key=(std::uint64_t)reinterpret_cast<std::uintptr_t>(d.geometry.get())^((std::uint64_t)reinterpret_cast<std::uintptr_t>(d.material.textures.get())<<1);auto it=lookup.find(key);if(it==lookup.end()){lookup.emplace(key,batches.size());batches.push_back({key,0,{}});it=lookup.find(key);}batches[it->second].instances.push_back({d.model,0});}return batches;}
IndirectElementsCommand make_indirect_command(std::uint32_t count,std::uint32_t instance_count,std::uint32_t first_index) noexcept{return{count,instance_count,first_index,0,0};}
bool GpuAssetStreamController::request(std::uint64_t id,float priority,std::uint32_t generation){if(!id)return false;queue_.request({id,priority,generation});states_[id]={StreamedAssetState::Stage::Requested,generation,0};return true;}
bool GpuAssetStreamController::mark_decoded(std::uint64_t id,std::uint32_t generation,std::size_t bytes){const auto it=states_.find(id);if(it==states_.end()||it->second.generation!=generation)return false;it->second.stage=StreamedAssetState::Stage::Decoded;it->second.decoded_bytes=bytes;return true;}
bool GpuAssetStreamController::make_resident(std::uint64_t id,const Mesh&mesh,std::uint64_t frame,std::string&error){const auto it=states_.find(id);if(it==states_.end())return false;if(!residency_.upload(id,mesh,frame,error)){it->second.stage=StreamedAssetState::Stage::Failed;return false;}it->second.stage=StreamedAssetState::Stage::Resident;return true;}
bool GpuAssetStreamController::touch(std::uint64_t id,std::uint64_t frame) noexcept{return residency_.touch(id,frame);}bool GpuAssetStreamController::evict(std::uint64_t id) noexcept{if(!residency_.evict(id))return false;const auto it=states_.find(id);if(it!=states_.end())it->second.stage=StreamedAssetState::Stage::Evicted;return true;}const StreamedAssetState*GpuAssetStreamController::state(std::uint64_t id)const noexcept{const auto it=states_.find(id);return it==states_.end()?nullptr:&it->second;}
} // namespace exgine

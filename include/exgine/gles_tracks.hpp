#pragma once
#include "exgine/advanced_render.hpp"
#include "exgine/gpu.hpp"
#include "exgine/gpu_residency.hpp"
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace exgine {
struct GlesCapabilities{int major=0,minor=0,max_texture_size=0,max_texture_units=0,max_ssbo_bindings=0,max_compute_invocations=0;bool es31=false,highp_fragment=false,color_buffer_float=false,depth24=false,cubemap=false,compute=false,ssbo=false,indirect=false;[[nodiscard]]bool valid()const noexcept{return major>=2&&minor>=0&&max_texture_size>0;}[[nodiscard]]bool gpu_driven()const noexcept{return compute&&ssbo&&indirect;}};
GlesCapabilities profile_gles(OpenGLESApi&,std::string_view version={},std::string_view extensions={})noexcept;
struct CubeFaceCapture{GlEnum target=0;Vec3 direction{};Vec3 up{};}; struct GlesCubeResource{GlUInt texture=0;std::uint32_t size=0,mip_levels=1;bool valid=false;};
[[nodiscard]]std::array<CubeFaceCapture,6>cube_faces()noexcept; bool create_cubemap(OpenGLESApi&,std::uint32_t,std::uint32_t,GlInt,GlesCubeResource&,std::string&); bool upload_cubemap_face(OpenGLESApi&,const GlesCubeResource&,std::uint32_t,GlEnum,GlInt,GlEnum,GlEnum,const void*,std::uint32_t);
struct TerrainGpuVertex{Vec3 position{};float morph=0;}; struct TerrainGpuPatch{std::uint32_t resolution=0;float morph=0;std::uint8_t stitch_mask=0;std::vector<TerrainGpuVertex>vertices;std::vector<std::uint32_t>indices;}; TerrainGpuPatch build_terrain_patch(std::uint32_t,std::uint8_t,float)noexcept;
struct VegetationInstanceGpu{Mat4 model=Mat4::identity();std::uint8_t lod=0;}; struct VegetationBatch{std::uint64_t mesh_key=0,material_key=0;std::vector<VegetationInstanceGpu>instances;}; std::vector<VegetationBatch>build_vegetation_batches(const std::vector<RenderDrawCall>&);
struct IndirectElementsCommand{std::uint32_t count=0,instance_count=0,first_index=0,base_vertex=0,base_instance=0;}; IndirectElementsCommand make_indirect_command(std::uint32_t,std::uint32_t,std::uint32_t=0)noexcept;
struct StreamedAssetState{enum class Stage:std::uint8_t{Requested,Decoded,Resident,Evicted,Failed};Stage stage=Stage::Requested;std::uint32_t generation=0;std::size_t decoded_bytes=0;};
class GpuAssetStreamController{public:explicit GpuAssetStreamController(OpenGLESAssetResidency&r)noexcept:residency_(r){}bool request(std::uint64_t,float,std::uint32_t=0);bool mark_decoded(std::uint64_t,std::uint32_t,std::size_t);bool make_resident(std::uint64_t,const Mesh&,std::uint64_t,std::string&);bool touch(std::uint64_t,std::uint64_t)noexcept;bool evict(std::uint64_t)noexcept;std::size_t trim()noexcept{return residency_.evict_until_within_budget();}const StreamedAssetState*state(std::uint64_t)const noexcept;private:OpenGLESAssetResidency&residency_;AssetStreamingQueue queue_;std::unordered_map<std::uint64_t,StreamedAssetState>states_;};
}
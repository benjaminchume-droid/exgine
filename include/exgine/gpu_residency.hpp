#pragma once
#include "exgine/gpu.hpp"
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>
namespace exgine {
struct GpuResidentMesh{std::uint64_t asset_id=0;GlUInt vao=0;GlUInt vertex_buffer=0;GlUInt index_buffer=0;std::size_t vertex_bytes=0;std::size_t index_bytes=0;std::size_t index_count=0;std::uint64_t last_used_frame=0;bool resident=false;[[nodiscard]]std::size_t bytes()const noexcept{return vertex_bytes+index_bytes;}};
struct GpuResidencyConfig{std::size_t budget_bytes=64u*1024u*1024u;std::size_t max_meshes=4096;[[nodiscard]]bool valid()const noexcept{return budget_bytes>0&&max_meshes>0;}};
struct GpuResidencyStats{std::size_t resident_meshes=0,resident_bytes=0,uploads=0,evictions=0,rejected_uploads=0,max_meshes=0;[[nodiscard]]bool valid()const noexcept{return resident_meshes<=max_meshes;}};
class OpenGLESAssetResidency{public:explicit OpenGLESAssetResidency(OpenGLESApi,GpuResidencyConfig={})noexcept;~OpenGLESAssetResidency();OpenGLESAssetResidency(const OpenGLESAssetResidency&)=delete;OpenGLESAssetResidency&operator=(const OpenGLESAssetResidency&)=delete;[[nodiscard]]bool ready()const noexcept;[[nodiscard]]const GpuResidencyConfig&config()const noexcept{return config_;}[[nodiscard]]const GpuResidencyStats&stats()const noexcept{return stats_;}[[nodiscard]]const GpuResidentMesh*find(std::uint64_t)const noexcept;bool upload(std::uint64_t,const Mesh&,std::uint64_t,std::string&);bool touch(std::uint64_t,std::uint64_t)noexcept;bool evict(std::uint64_t)noexcept;std::size_t evict_until_within_budget()noexcept;void clear()noexcept;private:OpenGLESApi api_{};GpuResidencyConfig config_{};GpuResidencyStats stats_{};std::vector<GpuResidentMesh> meshes_;bool ensure_capacity(std::size_t,std::uint64_t,std::string&);void release_mesh(GpuResidentMesh&)noexcept;};
}
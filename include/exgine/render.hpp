#pragma once
#include "exgine/animation.hpp"
#include "exgine/geometry.hpp"
#include "exgine/lighting.hpp"
#include "exgine/resources.hpp"
#include "exgine/scene.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <string>
#include <vector>
namespace exgine {
class Runtime;
enum class RenderBackend : std::uint8_t { Headless, OpenGLES, Vulkan, Metal, Direct3D12 };
enum class RenderPass : std::uint8_t { Opaque, Transparent, Overlay };
struct RenderConfig{RenderBackend backend=RenderBackend::Headless;std::uint32_t width=1280,height=720;bool frustum_culling=true;bool high_dynamic_range=true;std::uint32_t max_lights=256;std::uint32_t max_bones_per_draw=128;};
struct Mat4{std::array<float,16> m{};static Mat4 identity()noexcept;float& at(std::size_t r,std::size_t c)noexcept{return m[c*4+r];}const float& at(std::size_t r,std::size_t c)const noexcept{return m[c*4+r];}};
Mat4 multiply(const Mat4&,const Mat4&)noexcept;
Mat4 make_model_matrix(const SceneTransform&)noexcept;
Mat4 make_view_matrix(const Camera&)noexcept;
Mat4 make_projection_matrix(const Camera&,float aspect_ratio)noexcept;
bool finite_matrix(const Mat4&)noexcept;
struct Bounds3{Vec3 min{0,0,0},max{0,0,0};[[nodiscard]]bool valid()const noexcept;};
Bounds3 mesh_bounds(const Mesh&)noexcept;
Bounds3 transform_bounds(const Bounds3&,const Mat4&)noexcept;
struct RenderMaterialBinding{Material material{};std::shared_ptr<const TextureSet> textures;[[nodiscard]]bool valid()const noexcept;};
struct RenderDrawCall{std::uint64_t entity_id=0;SceneNodeId scene_node=invalid_scene_node;RenderPass pass=RenderPass::Opaque;std::shared_ptr<const MeshAssembly> geometry;std::size_t part_index=0;std::shared_ptr<const SkinnedMesh> skinned_mesh;std::vector<Mat4> bone_palette;Mat4 model=Mat4::identity();Bounds3 world_bounds{};RenderMaterialBinding material{};[[nodiscard]]bool animated()const noexcept{return static_cast<bool>(skinned_mesh);}[[nodiscard]]bool valid()const noexcept;};
struct RenderFrame{std::uint64_t frame_id=0;RenderConfig config{};Camera camera{};Mat4 view=Mat4::identity(),projection=Mat4::identity(),view_projection=Mat4::identity();RenderLightingSettings lighting{};std::vector<Light> lights;std::vector<RenderDrawCall> draws;[[nodiscard]]bool valid()const noexcept;};
struct RenderStats{std::size_t submitted_draws=0,visible_draws=0,culled_draws=0,submitted_lights=0,animated_draws=0;};
struct RenderResult{bool success=false;RenderStats stats{};std::string error;};
class Renderer{public:explicit Renderer(RenderConfig config={});[[nodiscard]]const RenderConfig& config()const noexcept{return config_;}[[nodiscard]]std::uint64_t next_frame_id()const noexcept{return next_frame_id_;}bool build_frame(const Runtime&,RenderFrame&)const;[[nodiscard]]RenderResult submit(const RenderFrame&);[[nodiscard]]bool validate(const RenderFrame&)const noexcept;private:RenderConfig config_{};std::uint64_t next_frame_id_=1;};
} // namespace exgine

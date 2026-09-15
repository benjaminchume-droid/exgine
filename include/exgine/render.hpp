#pragma once

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

struct RenderConfig {
    RenderBackend backend = RenderBackend::Headless;
    std::uint32_t width = 1280;
    std::uint32_t height = 720;
    bool frustum_culling = true;
    bool high_dynamic_range = true;
    std::uint32_t max_lights = 256;
};

struct Mat4 {
    std::array<float, 16> m{};

    [[nodiscard]] static Mat4 identity() noexcept;
    [[nodiscard]] float& at(std::size_t row, std::size_t column) noexcept {
        return m[column * 4U + row];
    }
    [[nodiscard]] const float& at(std::size_t row, std::size_t column) const noexcept {
        return m[column * 4U + row];
    }
};

[[nodiscard]] Mat4 multiply(const Mat4& a, const Mat4& b) noexcept;
[[nodiscard]] Mat4 make_model_matrix(const SceneTransform& transform) noexcept;
[[nodiscard]] Mat4 make_view_matrix(const Camera& camera) noexcept;
[[nodiscard]] Mat4 make_projection_matrix(const Camera& camera,
                                           float aspect_ratio) noexcept;
[[nodiscard]] bool finite_matrix(const Mat4& matrix) noexcept;

struct Bounds3 {
    Vec3 min{0, 0, 0};
    Vec3 max{0, 0, 0};

    [[nodiscard]] bool valid() const noexcept;
};

[[nodiscard]] Bounds3 mesh_bounds(const Mesh& mesh) noexcept;
[[nodiscard]] Bounds3 transform_bounds(const Bounds3& bounds,
                                       const Mat4& transform) noexcept;

struct RenderMaterialBinding {
    Material material{};
    std::shared_ptr<const TextureSet> textures;

    [[nodiscard]] bool valid() const noexcept;
};

struct RenderDrawCall {
    EntityId entity_id = invalid_entity;
    SceneNodeId scene_node = invalid_scene_node;
    RenderPass pass = RenderPass::Opaque;
    std::shared_ptr<const MeshAssembly> geometry;
    std::size_t part_index = 0;
    Mat4 model = Mat4::identity();
    Bounds3 world_bounds{};
    RenderMaterialBinding material{};

    [[nodiscard]] bool valid() const noexcept;
};

struct RenderFrame {
    std::uint64_t frame_id = 0;
    RenderConfig config{};
    Camera camera{};
    Mat4 view = Mat4::identity();
    Mat4 projection = Mat4::identity();
    Mat4 view_projection = Mat4::identity();
    RenderLightingSettings lighting{};
    std::vector<Light> lights;
    std::vector<RenderDrawCall> draws;

    [[nodiscard]] bool valid() const noexcept;
};

struct RenderStats {
    std::size_t submitted_draws = 0;
    std::size_t visible_draws = 0;
    std::size_t culled_draws = 0;
    std::size_t submitted_lights = 0;
};

struct RenderResult {
    bool success = false;
    RenderStats stats{};
    std::string error;
};

class Renderer {
public:
    explicit Renderer(RenderConfig config = {});

    [[nodiscard]] const RenderConfig& config() const noexcept { return config_; }
    [[nodiscard]] std::uint64_t next_frame_id() const noexcept { return next_frame_id_; }
    bool build_frame(const Runtime& runtime, RenderFrame& frame) const;
    [[nodiscard]] RenderResult submit(const RenderFrame& frame);
    [[nodiscard]] bool validate(const RenderFrame& frame) const noexcept;

private:
    RenderConfig config_{};
    std::uint64_t next_frame_id_ = 1;
};

} // namespace exgine

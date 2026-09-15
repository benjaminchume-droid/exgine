#pragma once

#include "exgine/geometry.hpp"

#include <cstdint>
#include <vector>

namespace exgine {
using SceneNodeId=std::uint64_t; inline constexpr SceneNodeId invalid_scene_node=0;
struct SceneTransform{Vec3 position{};Vec3 rotation{};Vec3 scale{1,1,1};};
struct SceneNode{SceneNodeId id=invalid_scene_node;SceneNodeId parent=invalid_scene_node;SceneTransform local{};SceneTransform world{};std::vector<SceneNodeId> children;bool active=true;};
class SceneGraph{public:SceneGraph();[[nodiscard]] SceneNodeId create(SceneNodeId parent=invalid_scene_node);bool destroy(SceneNodeId id) noexcept;bool set_parent(SceneNodeId id,SceneNodeId parent) noexcept;bool set_local_transform(SceneNodeId id,SceneTransform transform) noexcept;[[nodiscard]] SceneNode* get(SceneNodeId id) noexcept;[[nodiscard]] const SceneNode* get(SceneNodeId id) const noexcept;[[nodiscard]] SceneNodeId root() const noexcept{return root_;}[[nodiscard]] std::size_t size() const noexcept;void update_world_transforms() noexcept;void clear() noexcept;private:SceneNodeId next_id_=1;SceneNodeId root_=invalid_scene_node;std::vector<SceneNode> nodes_;void update_node(SceneNodeId id,const SceneTransform& parent_world) noexcept;[[nodiscard]] bool would_cycle(SceneNodeId id,SceneNodeId parent) const noexcept;};
} // namespace exgine

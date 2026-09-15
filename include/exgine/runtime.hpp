#pragma once
#include "exgine/building.hpp"
#include "exgine/geometry.hpp"
#include "exgine/ir.hpp"
#include "exgine/lighting.hpp"
#include "exgine/material.hpp"
#include "exgine/resources.hpp"
#include "exgine/scene.hpp"
#include "exgine/world.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace exgine {
using EntityId=std::uint64_t; inline constexpr EntityId invalid_entity=0;
struct Transform{float x=0,y=0,z=0;};
struct Entity{EntityId id=invalid_entity;NodeKind kind=NodeKind::Property;std::string name;Transform transform{};bool active=true;std::vector<Property> properties;std::shared_ptr<MeshAssembly> geometry;SceneNodeId scene_node=invalid_scene_node;};
class EntityRegistry{public:EntityId create(NodeKind,std::string);bool destroy(EntityId) noexcept;[[nodiscard]] Entity* get(EntityId) noexcept;[[nodiscard]] const Entity* get(EntityId) const noexcept;[[nodiscard]] std::vector<EntityId> ids() const;[[nodiscard]] std::size_t size() const noexcept{return entities_.size();}void clear() noexcept;private:EntityId next_id_=1;std::unordered_map<EntityId,Entity> entities_;};
struct WorldState{EntityId world_entity=invalid_entity;EntityRegistry entities;std::uint64_t tick=0;double elapsed_seconds=0;};
class Runtime{public:Runtime()=default;bool load(const IR&);void update(double) noexcept;void reset() noexcept;bool attach_geometry(EntityId,MeshAssembly);bool generate_building(EntityId,BuildingConfig config={});bool set_building_door(EntityId,std::uint64_t,bool) noexcept;[[nodiscard]] const BuildingInstance* building(EntityId) const noexcept;[[nodiscard]] std::vector<BuildingCollisionVolume> building_collision(EntityId) const;[[nodiscard]] const BuildingRoom* find_building_room(EntityId,Vec3) const noexcept;bool define_material(Material,TextureGenerationSettings settings={});bool stream_world(Vec3 focus_position,std::uint32_t view_radius_chunks);bool set_main_camera(Camera camera) noexcept;std::uint64_t create_light(Light light);bool destroy_light(std::uint64_t id) noexcept;[[nodiscard]] const SceneGraph& scene() const noexcept{return scene_;}[[nodiscard]] SceneGraph& scene() noexcept{return scene_;}[[nodiscard]] const LightingWorld& lighting() const noexcept{return lighting_;}[[nodiscard]] LightingWorld& lighting() noexcept{return lighting_;}[[nodiscard]] const ProceduralWorld* world() const noexcept{return world_.get();}[[nodiscard]] const WorldStreamer* world_streamer() const noexcept{return streamer_.get();}[[nodiscard]] const Material* material(std::string_view) const noexcept;[[nodiscard]] std::shared_ptr<const MaterialResource> material_resource(std::string_view) const noexcept;[[nodiscard]] const MaterialLibrary& materials() const noexcept{return materials_;}[[nodiscard]] const ResourceCache& resources() const noexcept{return resources_;}[[nodiscard]] const WorldState& state() const noexcept{return state_;}[[nodiscard]] WorldState& state() noexcept{return state_;}private:WorldState state_;MaterialLibrary materials_;ResourceCache resources_;std::unique_ptr<ProceduralWorld> world_;std::unique_ptr<WorldStreamer> streamer_;SceneGraph scene_;LightingWorld lighting_;std::unordered_map<EntityId,BuildingInstance> buildings_;bool loaded_=false;};
} // namespace exgine

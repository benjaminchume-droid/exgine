#pragma once
#include "exgine/geometry.hpp"
#include "exgine/ir.hpp"
#include "exgine/material.hpp"
#include "exgine/resources.hpp"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
namespace exgine {
using EntityId=std::uint64_t; inline constexpr EntityId invalid_entity=0;
struct Transform{float x=0,y=0,z=0;};
struct Entity{EntityId id=invalid_entity;NodeKind kind=NodeKind::Property;std::string name;Transform transform{};bool active=true;std::vector<Property> properties;std::shared_ptr<MeshAssembly> geometry;};
class EntityRegistry{public:EntityId create(NodeKind,std::string);bool destroy(EntityId) noexcept;[[nodiscard]] Entity* get(EntityId) noexcept;[[nodiscard]] const Entity* get(EntityId) const noexcept;[[nodiscard]] std::size_t size() const noexcept{return entities_.size();}void clear() noexcept;private:EntityId next_id_=1;std::unordered_map<EntityId,Entity> entities_;};
struct WorldState{EntityId world_entity=invalid_entity;EntityRegistry entities;std::uint64_t tick=0;double elapsed_seconds=0;};
class Runtime{public:Runtime()=default;bool load(const IR&);void update(double) noexcept;void reset() noexcept;bool attach_geometry(EntityId,MeshAssembly);bool define_material(Material,TextureGenerationSettings settings={});[[nodiscard]] const Material* material(std::string_view) const noexcept;[[nodiscard]] std::shared_ptr<const MaterialResource> material_resource(std::string_view) const noexcept;[[nodiscard]] const MaterialLibrary& materials() const noexcept{return materials_;}[[nodiscard]] const ResourceCache& resources() const noexcept{return resources_;}[[nodiscard]] const WorldState& state() const noexcept{return state_;}[[nodiscard]] WorldState& state() noexcept{return state_;}private:WorldState state_;MaterialLibrary materials_;ResourceCache resources_;bool loaded_=false;};
} // namespace exgine

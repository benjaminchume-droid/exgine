#include "exgine/runtime.hpp"
#include <limits>
#include <utility>
namespace exgine {
EntityId EntityRegistry::create(NodeKind kind,std::string name){if(next_id_==invalid_entity||next_id_==std::numeric_limits<EntityId>::max())return invalid_entity;const EntityId id=next_id_++;entities_.emplace(id,Entity{id,kind,std::move(name),{},true,{},nullptr});return id;}
bool EntityRegistry::destroy(EntityId id) noexcept{return entities_.erase(id)!=0;} Entity* EntityRegistry::get(EntityId id) noexcept{const auto it=entities_.find(id);return it==entities_.end()?nullptr:&it->second;} const Entity* EntityRegistry::get(EntityId id) const noexcept{const auto it=entities_.find(id);return it==entities_.end()?nullptr:&it->second;} void EntityRegistry::clear() noexcept{entities_.clear();next_id_=1;}
namespace {bool instantiate(const Node& n,EntityRegistry& r,EntityId& world){const EntityId id=r.create(n.kind,n.name);if(id==invalid_entity)return false;if(n.kind==NodeKind::World)world=id;auto* e=r.get(id);e->properties=n.properties;for(const auto& child:n.children)if(!instantiate(child,r,world))return false;return true;}}
bool Runtime::load(const IR& ir){reset();if(ir.root.kind!=NodeKind::World)return false;if(!instantiate(ir.root,state_.entities,state_.world_entity)){reset();return false;}loaded_=state_.world_entity!=invalid_entity;return loaded_;}
void Runtime::update(double dt) noexcept{if(!loaded_||dt<0)return;state_.elapsed_seconds+=dt;++state_.tick;}
bool Runtime::attach_geometry(EntityId id,MeshAssembly g){auto* e=state_.entities.get(id);if(!e||!g.valid())return false;e->geometry=std::make_shared<MeshAssembly>(std::move(g));return true;}
bool Runtime::define_material(Material m,TextureGenerationSettings settings){if(!loaded_||!m.valid())return false;const std::string name=m.name;if(!materials_.define(std::move(m)))return false;const Material* stored=materials_.find(name);if(!stored)return false;auto resource=build_material_resource(*stored,settings);if(!resource)return false;return resources_.store_material(std::move(resource));}
const Material* Runtime::material(std::string_view name) const noexcept{return materials_.find(name);} std::shared_ptr<const MaterialResource> Runtime::material_resource(std::string_view name) const noexcept{return resources_.find_material(name);} void Runtime::reset() noexcept{state_={};materials_.clear();resources_.clear();loaded_=false;}
} // namespace exgine

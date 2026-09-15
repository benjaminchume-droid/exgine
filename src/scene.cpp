#include "exgine/scene.hpp"

#include <algorithm>
#include <limits>

namespace exgine {
namespace {
SceneTransform identity_transform() noexcept { return {{0,0,0},{0,0,0},{1,1,1}}; }
SceneTransform compose(const SceneTransform& parent,const SceneTransform& local) noexcept {
    return {{parent.position.x + local.position.x * parent.scale.x,
             parent.position.y + local.position.y * parent.scale.y,
             parent.position.z + local.position.z * parent.scale.z},
            {parent.rotation.x + local.rotation.x,
             parent.rotation.y + local.rotation.y,
             parent.rotation.z + local.rotation.z},
            {parent.scale.x * local.scale.x,parent.scale.y * local.scale.y,parent.scale.z * local.scale.z}};
}
}
SceneGraph::SceneGraph(){ root_=create(invalid_scene_node); }
SceneNodeId SceneGraph::create(SceneNodeId parent){if(next_id_==invalid_scene_node||next_id_==std::numeric_limits<SceneNodeId>::max())return invalid_scene_node;if(parent!=invalid_scene_node&&!get(parent))return invalid_scene_node;const auto id=next_id_++;nodes_.push_back(SceneNode{id,parent,identity_transform(),identity_transform(),{},true});if(parent!=invalid_scene_node)get(parent)->children.push_back(id);return id;}
bool SceneGraph::would_cycle(SceneNodeId id,SceneNodeId parent) const noexcept{for(auto current=parent;current!=invalid_scene_node;){if(current==id)return true;const auto* node=get(current);if(!node)return false;current=node->parent;}return false;}
bool SceneGraph::set_parent(SceneNodeId id,SceneNodeId parent) noexcept{auto* node=get(id);if(!node||id==root_||parent==id||(parent!=invalid_scene_node&&!get(parent))||would_cycle(id,parent))return false;if(node->parent!=invalid_scene_node){auto* old=get(node->parent);if(old){old->children.erase(std::remove(old->children.begin(),old->children.end(),id),old->children.end());}}node->parent=parent;if(parent!=invalid_scene_node)get(parent)->children.push_back(id);return true;}
bool SceneGraph::destroy(SceneNodeId id) noexcept{if(id==invalid_scene_node||id==root_)return false;auto* node=get(id);if(!node)return false;auto children=node->children;for(const auto child:children)destroy(child);if(node->parent!=invalid_scene_node){auto* p=get(node->parent);if(p)p->children.erase(std::remove(p->children.begin(),p->children.end(),id),p->children.end());}nodes_.erase(std::remove_if(nodes_.begin(),nodes_.end(),[id](const SceneNode& n){return n.id==id;}),nodes_.end());return true;}
bool SceneGraph::set_local_transform(SceneNodeId id,SceneTransform transform) noexcept{auto* node=get(id);if(!node)return false;node->local=transform;return true;}
SceneNode* SceneGraph::get(SceneNodeId id) noexcept{for(auto& n:nodes_)if(n.id==id)return &n;return nullptr;} const SceneNode* SceneGraph::get(SceneNodeId id) const noexcept{for(const auto& n:nodes_)if(n.id==id)return &n;return nullptr;}
std::size_t SceneGraph::size() const noexcept{return nodes_.size();}
void SceneGraph::update_node(SceneNodeId id,const SceneTransform& parent_world) noexcept{auto* node=get(id);if(!node)return;node->world=node->parent==invalid_scene_node?node->local:compose(parent_world,node->local);const auto world=node->world;for(const auto child:node->children)update_node(child,world);}
void SceneGraph::update_world_transforms() noexcept{if(const auto* node=get(root_))update_node(node->id,identity_transform());}
void SceneGraph::clear() noexcept{nodes_.clear();next_id_=1;root_=create(invalid_scene_node);}
} // namespace exgine

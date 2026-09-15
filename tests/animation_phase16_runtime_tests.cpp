#include "exgine/animation.hpp"
#include "exgine/runtime.hpp"
#include <cassert>
int main(){using namespace exgine; AnimationLibrary lib; auto s=make_humanoid_skeleton(); auto walk=make_walk_clip(s,42); assert(lib.define(walk)); AnimationController c(s); assert(c.play(42,0)); c.update(.1f,lib); assert(c.pose().valid_for(s)); IR ir; ir.root.kind=NodeKind::World; ir.root.name="world"; Runtime rt; assert(rt.load(ir)); EntityId e=rt.state().entities.create(NodeKind::Player,"player"); assert(e!=invalid_entity); assert(rt.attach_skeleton(e,s)); assert(rt.define_animation(walk)); assert(rt.play_animation(e,42,0)); rt.update(.1); assert(rt.animation_pose(e)!=nullptr); assert(rt.animation_pose(e)->valid_for(s)); rt.reset(); return 0; }

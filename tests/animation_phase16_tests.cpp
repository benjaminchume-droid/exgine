#include "exgine/animation.hpp"
#include <cassert>
#include <cmath>
using namespace exgine;
int main(){
 auto s=make_humanoid_skeleton(); assert(s.valid()); assert(s.find_bone("head")!=invalid_bone);
 auto p=make_bind_pose(s); assert(p.valid_for(s));
 auto q=slerp({0,0,0,1},{0,1,0,0},.5f); assert(std::fabs(q.w)>0.6f&&std::fabs(q.y)>0.6f);
 SkinnedMesh m; m.vertices.push_back({{1,0,0},{1,0,0},{0,0},{{1,0,0,0},{1,0,0,0}}}); m.indices={0,0,0}; assert(m.valid(s)); auto v=skin_mesh(m,p); assert(v.size()==1&&std::fabs(v[0].position.x-1)<1e-4f);
 auto idle=make_idle_clip(s); auto walk=make_walk_clip(s); auto run=make_run_clip(s); auto jump=make_jump_clip(s); assert(idle.valid(s)&&walk.valid(s)&&run.valid(s)&&jump.valid(s));
 AnimationLibrary lib; assert(lib.define(idle)&&lib.define(walk)&&lib.define(run)&&lib.define(jump)); assert(lib.size()==4&&lib.find("walk"));
 AnimationController c(s); assert(c.play(walk.id,0)); c.update(.25f,lib); assert(c.player().playing); assert(c.pose().valid_for(s)); auto before=c.pose().local[11].rotation; c.update(.25f,lib); auto after=c.pose().local[11].rotation; assert(std::fabs(before.x-after.x)>1e-4f);
 assert(c.play(run.id,.2f)); c.update(.1f,lib); assert(c.player().clip==run.id); c.stop(); assert(!c.player().playing); return 0;
}

#include "exgine/gles_tracks.hpp"
#include <algorithm>
#include <cassert>
int main(){using namespace exgine;for(const auto&f:cube_faces())assert(f.target>=0x8515&&f.target<=0x851A);for(unsigned mask=0;mask<16;++mask){auto p=build_terrain_patch(17,(std::uint8_t)mask,1.25f);assert(p.resolution==17);assert(p.morph==1.f);assert(p.stitch_mask==mask);assert(p.vertices.size()==289);assert(!p.indices.empty());for(auto i:p.indices)assert(i<p.vertices.size());}auto c=make_indirect_command(36,128,4);assert(c.count==36&&c.instance_count==128&&c.first_index==4);OpenGLESApi api;auto caps=profile_gles(api,"OpenGL ES 3.1","");assert(caps.es31&&caps.cubemap);return 0;}

#include "exgine/gles_production_hardening.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <vector>

using namespace exgine;

static std::vector<std::byte> block(std::uint8_t codec, std::uint32_t decoded, std::initializer_list<std::uint8_t> payload) {
    std::vector<std::byte> b(16 + payload.size());
    auto put=[&](std::size_t o,std::uint32_t v){b[o]=std::byte(v&255);b[o+1]=std::byte((v>>8)&255);b[o+2]=std::byte((v>>16)&255);b[o+3]=std::byte((v>>24)&255);};
    put(0,0x4B415058); b[4]=std::byte(codec); put(8,decoded); put(12,(std::uint32_t)payload.size());
    std::size_t i=16; for(auto x:payload)b[i++]=std::byte(x); return b;
}

int main(){
    OpenGLESApi api; auto caps=profile_gles(api,"OpenGL ES 3.0",""); auto d=decide_gpu_submission(caps); assert(!d.enabled);
    auto p=build_stitched_terrain_patch(9,1u|8u,1.f); assert(p.resolution==9&&!p.indices.empty()); bool moved=false; for(const auto&v:p.vertices)if(v.morph_position.z!=v.position.z||v.morph_position.x!=v.position.x)moved=true; assert(moved);
    auto raw=block(0,3,{1,2,3}); std::vector<std::byte> out; std::string e; assert(decode_expak_block(raw,out)&&out.size()==3&&std::to_integer<unsigned>(out[1])==2);
    auto rle=block(1,5,{5,'A'}); assert(decode_expak_block(rle,out)&&out.size()==5); for(auto x:out)assert(x==std::byte{'A'});
    // LZ4: token 0x50 = five literals, followed by end-of-block.
    auto lz=block(2,5,{0x50,'H','e','l','l','o'}); assert(decode_expak_block(lz,out)&&out.size()==5);
    return 0;
}

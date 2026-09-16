#include "exgine/gles_production_hardening.hpp"
#include <algorithm>
#include <cstring>
#include <limits>

namespace exgine {

GpuSubmissionDecision decide_gpu_submission(const GlesCapabilities& caps) noexcept {
    GpuSubmissionDecision d{};
    d.compute = caps.compute;
    d.ssbo = caps.ssbo;
    d.indirect = caps.indirect;
    if (!caps.valid()) { d.reason = "invalid GLES profile"; return d; }
    if (!caps.es31) { d.reason = "GLES 3.1 unavailable"; return d; }
    if (!caps.compute) { d.reason = "compute unavailable"; return d; }
    if (!caps.ssbo) { d.reason = "SSBO unavailable"; return d; }
    if (!caps.indirect) { d.reason = "indirect draw unavailable"; return d; }
    if (caps.max_ssbo_bindings < 4) { d.reason = "insufficient SSBO bindings"; return d; }
    if (caps.max_compute_invocations < 1) { d.reason = "compute limits unavailable"; return d; }
    d.enabled = true; d.reason = "GLES 3.1 GPU-driven path enabled"; return d;
}

TerrainStitchedPatch build_stitched_terrain_patch(std::uint32_t resolution, std::uint8_t stitch_mask, float morph) noexcept {
    TerrainStitchedPatch p{};
    p.resolution = std::max<std::uint32_t>(2, resolution);
    p.stitch_mask = stitch_mask;
    morph = std::clamp(morph, 0.0f, 1.0f);
    const auto n = p.resolution;
    p.vertices.reserve(static_cast<std::size_t>(n) * n);
    auto coarse_coord = [n](std::uint32_t x, std::uint32_t z, std::uint8_t mask) {
        if ((mask & 8u) && x == 0 && (z & 1u)) return z - 1;
        if ((mask & 2u) && x + 1 == n && (z & 1u)) return z - 1;
        if ((mask & 4u) && z == 0 && (x & 1u)) return x - 1;
        if ((mask & 1u) && z + 1 == n && (x & 1u)) return x - 1;
        return z;
    };
    for (std::uint32_t z=0; z<n; ++z) for (std::uint32_t x=0; x<n; ++x) {
        const float fx=float(x)/float(n-1), fz=float(z)/float(n-1);
        float mx=fx, mz=fz;
        if ((stitch_mask&8u)&&x==0&&(z&1u)) mz=float(coarse_coord(x,z,8))/float(n-1);
        if ((stitch_mask&2u)&&x+1==n&&(z&1u)) mz=float(coarse_coord(x,z,2))/float(n-1);
        if ((stitch_mask&4u)&&z==0&&(x&1u)) mx=float(coarse_coord(x,z,4))/float(n-1);
        if ((stitch_mask&1u)&&z+1==n&&(x&1u)) mx=float(coarse_coord(x,z,1))/float(n-1);
        p.vertices.push_back({{fx,0,fz},{mx,0,mz},morph});
    }
    auto id=[n](std::uint32_t x,std::uint32_t z){return z*n+x;};
    auto edge_id=[&](std::uint32_t x,std::uint32_t z)->std::uint32_t {
        if ((stitch_mask&8u)&&x==0&&(z&1u)) --z;
        if ((stitch_mask&2u)&&x+1==n&&(z&1u)) --z;
        if ((stitch_mask&4u)&&z==0&&(x&1u)) --x;
        if ((stitch_mask&1u)&&z+1==n&&(x&1u)) --x;
        return id(x,z);
    };
    p.indices.reserve(static_cast<std::size_t>(n-1)*(n-1)*6);
    for(std::uint32_t z=0;z+1<n;++z) for(std::uint32_t x=0;x+1<n;++x) {
        const auto a=edge_id(x,z), b=edge_id(x+1,z), c=edge_id(x+1,z+1), d=edge_id(x,z+1);
        if(a!=b&&b!=c&&c!=a) p.indices.insert(p.indices.end(),{a,b,c});
        if(a!=c&&c!=d&&d!=a) p.indices.insert(p.indices.end(),{a,c,d});
    }
    return p;
}

bool decompress_rle(std::span<const std::byte> payload, std::span<std::byte> output) noexcept {
    std::size_t in=0,out=0;
    while(in+2<=payload.size()) {
        const auto count=static_cast<std::uint8_t>(payload[in++]);
        const auto value=payload[in++];
        if(count==0 || out+count>output.size()) return false;
        std::fill_n(output.data()+out,count,value); out+=count;
    }
    return in==payload.size() && out==output.size();
}

static std::uint32_t read32(const std::byte* p) noexcept {
    return std::uint32_t(std::to_integer<std::uint8_t>(p[0])) | (std::uint32_t(std::to_integer<std::uint8_t>(p[1]))<<8) |
           (std::uint32_t(std::to_integer<std::uint8_t>(p[2]))<<16) | (std::uint32_t(std::to_integer<std::uint8_t>(p[3]))<<24);
}

bool decompress_lz4_block(std::span<const std::byte> payload, std::span<std::byte> output) noexcept {
    std::size_t ip=0,op=0;
    while(ip<payload.size()) {
        const std::uint8_t token=std::to_integer<std::uint8_t>(payload[ip++]);
        std::size_t lit=token>>4;
        if(lit==15){std::uint8_t x=255;while(x==255){if(ip>=payload.size())return false;x=std::to_integer<std::uint8_t>(payload[ip++]);lit+=x;}}
        if(ip+lit>payload.size()||op+lit>output.size())return false;
        std::memcpy(output.data()+op,payload.data()+ip,lit);ip+=lit;op+=lit;
        if(ip==payload.size()) break;
        if(ip+2>payload.size())return false;
        const std::size_t off=std::size_t(std::to_integer<std::uint8_t>(payload[ip]))|(std::size_t(std::to_integer<std::uint8_t>(payload[ip+1]))<<8);ip+=2;
        if(off==0||off>op)return false;
        std::size_t len=token&15;
        if(len==15){std::uint8_t x=255;while(x==255){if(ip>=payload.size())return false;x=std::to_integer<std::uint8_t>(payload[ip++]);len+=x;}}
        len+=4;if(op+len>output.size())return false;
        for(std::size_t i=0;i<len;++i) output[op+i]=output[op-off+i]; op+=len;
    }
    return op==output.size();
}

bool decode_expak_block(std::span<const std::byte> block, std::vector<std::byte>& decoded, std::string& error) {
    if(block.size()<16){error="EXPAK block header truncated";return false;}
    ExPakBlockHeader h{};h.magic=read32(block.data());h.codec=std::to_integer<std::uint8_t>(block[4]);h.decoded_size=read32(block.data()+8);h.payload_size=read32(block.data()+12);
    if(h.magic!=0x4B415058){error="invalid EXPAK magic";return false;}
    if(h.payload_size!=block.size()-16 || h.decoded_size>std::numeric_limits<std::size_t>::max()){error="invalid EXPAK sizes";return false;}
    decoded.assign(h.decoded_size,std::byte{});const auto payload=block.subspan(16);
    bool ok=false;if(h.codec==0){if(payload.size()!=decoded.size()) {error="raw size mismatch";return false;}std::copy(payload.begin(),payload.end(),decoded.begin());ok=true;}
    else if(h.codec==1)ok=decompress_rle(payload,decoded);
    else if(h.codec==2)ok=decompress_lz4_block(payload,decoded);
    else {error="unsupported EXPAK codec";return false;}
    if(!ok){error="EXPAK decompression failed";decoded.clear();return false;}return true;
}

} // namespace exgine

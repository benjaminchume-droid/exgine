#pragma once
#include "exgine/gles_tracks.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>
namespace exgine {
struct GpuSubmissionDecision { bool enabled=false,compute=false,ssbo=false,indirect=false; const char* reason="unprofiled"; };
GpuSubmissionDecision decide_gpu_submission(const GlesCapabilities& caps) noexcept;
struct TerrainEdgeVertex { Vec3 position{},morph_position{}; float morph=0.0f; };
struct TerrainStitchedPatch { std::uint32_t resolution=0; std::uint8_t stitch_mask=0; std::vector<TerrainEdgeVertex> vertices; std::vector<std::uint32_t> indices; };
TerrainStitchedPatch build_stitched_terrain_patch(std::uint32_t resolution,std::uint8_t stitch_mask,float morph) noexcept;
struct ExPakBlockHeader { std::uint32_t magic=0x4B415058; std::uint8_t codec=0; std::uint8_t reserved[3]{}; std::uint32_t decoded_size=0; std::uint32_t payload_size=0; };
bool decompress_rle(std::span<const std::byte> payload,std::span<std::byte> output) noexcept;
bool decompress_lz4_block(std::span<const std::byte> payload,std::span<std::byte> output) noexcept;
bool decode_expak_block(std::span<const std::byte> block,std::vector<std::byte>& decoded,std::string& error);
// EXPAK uses raw/LZ4 blocks for runtime portability. Compression chooses the smaller encoding.
std::vector<std::byte> compress_expak_block(std::span<const std::byte> input);
std::vector<std::byte> build_expak_package(std::span<const std::span<const std::byte>> blocks);
}
#pragma once
#include "exgine/gles_tracks.hpp"
#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <vector>

namespace exgine {

// GPU submission must be explicitly armed by the live GLES profile. Pointer presence alone is insufficient.
struct GpuSubmissionDecision {
    bool enabled = false;
    bool compute = false;
    bool ssbo = false;
    bool indirect = false;
    const char* reason = "unprofiled";
};
GpuSubmissionDecision decide_gpu_submission(const GlesCapabilities& caps) noexcept;

// Crack-free terrain edge topology. Odd fine-grid edge vertices collapse onto their
// even neighbour when a coarser neighbour is present; the morph target follows the same rule.
struct TerrainEdgeVertex {
    Vec3 position{};
    Vec3 morph_position{};
    float morph = 0.0f;
};
struct TerrainStitchedPatch {
    std::uint32_t resolution = 0;
    std::uint8_t stitch_mask = 0;
    std::vector<TerrainEdgeVertex> vertices;
    std::vector<std::uint32_t> indices;
};
TerrainStitchedPatch build_stitched_terrain_patch(std::uint32_t resolution,
                                                  std::uint8_t stitch_mask,
                                                  float morph) noexcept;

// Self-contained EXPAK block codec. Raw blocks are always supported; RLE and
// LZ4 block payloads are supported without a third-party runtime dependency.
struct ExPakBlockHeader {
    std::uint32_t magic = 0x4B415058; // "XPAK"
    std::uint8_t codec = 0;           // 0 raw, 1 RLE, 2 LZ4 block
    std::uint8_t reserved[3]{};
    std::uint32_t decoded_size = 0;
    std::uint32_t payload_size = 0;
};

bool decompress_rle(std::span<const std::byte> payload, std::span<std::byte> output) noexcept;
bool decompress_lz4_block(std::span<const std::byte> payload, std::span<std::byte> output) noexcept;
bool decode_expak_block(std::span<const std::byte> block, std::vector<std::byte>& decoded, std::string& error);

} // namespace exgine

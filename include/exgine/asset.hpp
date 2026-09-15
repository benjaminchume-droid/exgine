#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class AssetType : std::uint8_t {
    Mesh = 0,
    Texture = 1,
    Material = 2,
    Skeleton = 3,
    Animation = 4,
    Scene = 5,
    Audio = 6,
    Script = 7,
    Binary = 8,
};

using AssetId = std::uint64_t;
inline constexpr AssetId invalid_asset = 0;

struct AssetRecord {
    AssetId id = invalid_asset;
    AssetType type = AssetType::Binary;
    std::string uri;
    std::vector<std::uint8_t> data;
    std::vector<AssetId> dependencies;

    [[nodiscard]] bool valid() const noexcept;
};

struct AssetPackageHeader {
    static constexpr std::uint32_t magic = 0x4B504758u; // "XGPK"
    static constexpr std::uint32_t version = 1;
    std::uint32_t asset_count = 0;
};

struct AssetPackageResult {
    bool success = false;
    std::vector<std::uint8_t> bytes;
    std::string error;
};

struct AssetLoadResult {
    bool success = false;
    std::string error;
};

[[nodiscard]] AssetId make_asset_id(std::string_view uri,
                                    const std::vector<std::uint8_t>& data) noexcept;
[[nodiscard]] AssetPackageResult pack_assets(const std::vector<AssetRecord>& assets);
[[nodiscard]] AssetLoadResult unpack_assets(const std::vector<std::uint8_t>& bytes,
                                            std::vector<AssetRecord>& assets);

class AssetDatabase {
public:
    bool add(AssetRecord asset);
    bool remove(AssetId id) noexcept;
    void clear() noexcept;

    [[nodiscard]] const AssetRecord* find(AssetId id) const noexcept;
    [[nodiscard]] const AssetRecord* find_uri(std::string_view uri) const noexcept;
    [[nodiscard]] std::size_t size() const noexcept { return assets_.size(); }
    [[nodiscard]] bool dependencies_resolved(AssetId id) const noexcept;
    [[nodiscard]] std::vector<AssetId> load_order() const;
    [[nodiscard]] AssetPackageResult pack() const;
    [[nodiscard]] AssetLoadResult unpack(const std::vector<std::uint8_t>& bytes);

private:
    std::vector<AssetRecord> assets_;
};

} // namespace exgine

#include "exgine/asset.hpp"

#include <algorithm>
#include <limits>
#include <unordered_map>

namespace exgine {
namespace {

constexpr std::uint64_t fnv_offset = 1469598103934665603ULL;
constexpr std::uint64_t fnv_prime = 1099511628211ULL;

std::uint64_t hash_bytes(std::string_view uri, const std::vector<std::uint8_t>& data) noexcept {
    std::uint64_t h = fnv_offset;
    for (const unsigned char c : uri) { h ^= c; h *= fnv_prime; }
    h ^= 0xffU; h *= fnv_prime;
    for (const auto c : data) { h ^= c; h *= fnv_prime; }
    return h == invalid_asset ? 1 : h;
}

void put_u32(std::vector<std::uint8_t>& out, std::uint32_t v) {
    for (int i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xffU));
}
void put_u64(std::vector<std::uint8_t>& out, std::uint64_t v) {
    for (int i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>((v >> (i * 8)) & 0xffU));
}
void put_bytes(std::vector<std::uint8_t>& out, const std::vector<std::uint8_t>& value) { out.insert(out.end(), value.begin(), value.end()); }
void put_string(std::vector<std::uint8_t>& out, std::string_view value) {
    put_u32(out, static_cast<std::uint32_t>(value.size()));
    out.insert(out.end(), value.begin(), value.end());
}

bool read_u32(const std::vector<std::uint8_t>& b, std::size_t& p, std::uint32_t& v) {
    if (p + 4 > b.size()) return false;
    v = 0; for (int i = 0; i < 4; ++i) v |= static_cast<std::uint32_t>(b[p++]) << (i * 8); return true;
}
bool read_u64(const std::vector<std::uint8_t>& b, std::size_t& p, std::uint64_t& v) {
    if (p + 8 > b.size()) return false;
    v = 0; for (int i = 0; i < 8; ++i) v |= static_cast<std::uint64_t>(b[p++]) << (i * 8); return true;
}
bool read_string(const std::vector<std::uint8_t>& b, std::size_t& p, std::string& out) {
    std::uint32_t n = 0; if (!read_u32(b, p, n) || p + n > b.size()) return false;
    out.assign(reinterpret_cast<const char*>(b.data() + p), n); p += n; return true;
}

bool dfs_cycle(AssetId id, const std::unordered_map<AssetId, std::size_t>& index,
               const std::vector<AssetRecord>& assets, std::vector<std::uint8_t>& marks,
               std::vector<AssetId>& order) {
    const auto it = index.find(id); if (it == index.end()) return false;
    const std::size_t i = it->second;
    if (marks[i] == 2) return true;
    if (marks[i] == 1) return false;
    marks[i] = 1;
    for (const AssetId dep : assets[i].dependencies) if (!dfs_cycle(dep, index, assets, marks, order)) return false;
    marks[i] = 2; order.push_back(id); return true;
}

} // namespace

bool AssetRecord::valid() const noexcept {
    return id != invalid_asset && !uri.empty() && uri.size() <= std::numeric_limits<std::uint32_t>::max() &&
           data.size() <= std::numeric_limits<std::uint32_t>::max() &&
           dependencies.size() <= std::numeric_limits<std::uint32_t>::max();
}

AssetId make_asset_id(std::string_view uri, const std::vector<std::uint8_t>& data) noexcept { return hash_bytes(uri, data); }

AssetPackageResult pack_assets(const std::vector<AssetRecord>& assets) {
    AssetPackageResult result;
    if (assets.size() > std::numeric_limits<std::uint32_t>::max()) { result.error = "too many assets"; return result; }
    std::unordered_map<AssetId, bool> ids;
    for (const auto& asset : assets) {
        if (!asset.valid()) { result.error = "invalid asset record"; return result; }
        if (!ids.emplace(asset.id, true).second) { result.error = "duplicate asset id"; return result; }
    }
    result.bytes.reserve(16);
    put_u32(result.bytes, AssetPackageHeader::magic);
    put_u32(result.bytes, AssetPackageHeader::version);
    put_u32(result.bytes, static_cast<std::uint32_t>(assets.size()));
    put_u32(result.bytes, 0);
    for (const auto& asset : assets) {
        result.bytes.push_back(static_cast<std::uint8_t>(asset.type));
        put_u64(result.bytes, asset.id);
        put_string(result.bytes, asset.uri);
        put_u32(result.bytes, static_cast<std::uint32_t>(asset.data.size()));
        put_u32(result.bytes, static_cast<std::uint32_t>(asset.dependencies.size()));
        put_bytes(result.bytes, asset.data);
        for (const AssetId dep : asset.dependencies) put_u64(result.bytes, dep);
    }
    result.success = true;
    return result;
}

AssetLoadResult unpack_assets(const std::vector<std::uint8_t>& bytes, std::vector<AssetRecord>& assets) {
    AssetLoadResult result;
    std::size_t p = 0; std::uint32_t magic = 0, version = 0, count = 0, reserved = 0;
    if (!read_u32(bytes, p, magic) || !read_u32(bytes, p, version) || !read_u32(bytes, p, count) || !read_u32(bytes, p, reserved)) {
        result.error = "truncated package header"; return result;
    }
    if (magic != AssetPackageHeader::magic || version != AssetPackageHeader::version || reserved != 0) {
        result.error = "unsupported package header"; return result;
    }
    std::vector<AssetRecord> decoded; decoded.reserve(count);
    std::unordered_map<AssetId, bool> ids;
    for (std::uint32_t i = 0; i < count; ++i) {
        if (p >= bytes.size()) { result.error = "truncated asset type"; return result; }
        AssetRecord asset; asset.type = static_cast<AssetType>(bytes[p++]);
        std::uint64_t id = 0; std::uint32_t size = 0, dep_count = 0;
        if (!read_u64(bytes, p, id) || !read_string(bytes, p, asset.uri) || !read_u32(bytes, p, size) || !read_u32(bytes, p, dep_count)) {
            result.error = "truncated asset record"; return result;
        }
        asset.id = id;
        if (p + size > bytes.size()) { result.error = "truncated asset payload"; return result; }
        asset.data.assign(bytes.begin() + static_cast<std::ptrdiff_t>(p), bytes.begin() + static_cast<std::ptrdiff_t>(p + size)); p += size;
        if (p + static_cast<std::size_t>(dep_count) * 8 > bytes.size()) { result.error = "truncated dependencies"; return result; }
        asset.dependencies.resize(dep_count);
        for (auto& dep : asset.dependencies) if (!read_u64(bytes, p, dep)) { result.error = "truncated dependency"; return result; }
        if (!asset.valid() || !ids.emplace(asset.id, true).second) { result.error = "invalid or duplicate asset"; return result; }
        decoded.push_back(std::move(asset));
    }
    if (p != bytes.size()) { result.error = "trailing package bytes"; return result; }
    assets = std::move(decoded); result.success = true; return result;
}

bool AssetDatabase::add(AssetRecord asset) {
    if (!asset.valid()) return false;
    if (find(asset.id) != nullptr || find_uri(asset.uri) != nullptr) return false;
    assets_.push_back(std::move(asset)); return true;
}

bool AssetDatabase::remove(AssetId id) noexcept {
    const auto it = std::find_if(assets_.begin(), assets_.end(), [id](const auto& a) { return a.id == id; });
    if (it == assets_.end()) return false;
    assets_.erase(it); return true;
}

void AssetDatabase::clear() noexcept { assets_.clear(); }

const AssetRecord* AssetDatabase::find(AssetId id) const noexcept {
    const auto it = std::find_if(assets_.begin(), assets_.end(), [id](const auto& a) { return a.id == id; });
    return it == assets_.end() ? nullptr : &*it;
}
const AssetRecord* AssetDatabase::find_uri(std::string_view uri) const noexcept {
    const auto it = std::find_if(assets_.begin(), assets_.end(), [uri](const auto& a) { return a.uri == uri; });
    return it == assets_.end() ? nullptr : &*it;
}

bool AssetDatabase::dependencies_resolved(AssetId id) const noexcept {
    const auto* asset = find(id); if (!asset) return false;
    for (const AssetId dep : asset->dependencies) if (find(dep) == nullptr || dep == id) return false;
    return true;
}

std::vector<AssetId> AssetDatabase::load_order() const {
    std::unordered_map<AssetId, std::size_t> index;
    for (std::size_t i = 0; i < assets_.size(); ++i) index.emplace(assets_[i].id, i);
    std::vector<std::uint8_t> marks(assets_.size(), 0); std::vector<AssetId> order;
    for (const auto& asset : assets_) if (!dfs_cycle(asset.id, index, assets_, marks, order)) return {};
    return order;
}

AssetPackageResult AssetDatabase::pack() const { return pack_assets(assets_); }

AssetLoadResult AssetDatabase::unpack(const std::vector<std::uint8_t>& bytes) {
    std::vector<AssetRecord> decoded; auto result = unpack_assets(bytes, decoded); if (!result.success) return result;
    std::unordered_map<AssetId, bool> ids; for (const auto& asset : decoded) ids.emplace(asset.id, true);
    for (const auto& asset : decoded) for (const AssetId dep : asset.dependencies) if (!ids.contains(dep)) { result.success = false; result.error = "package has missing dependency"; return result; }
    assets_ = std::move(decoded); return result;
}

} // namespace exgine

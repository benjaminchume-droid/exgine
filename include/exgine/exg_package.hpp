#pragma once
#include "exgine/geometry.hpp"
#include "exgine/gles_tracks.hpp"
#include <condition_variable>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <span>
#include <string>
#include <thread>
#include <vector>

namespace exgine {

enum class ExgCodec : std::uint8_t { Raw=0, Rle=1, Lz4=2, Lz4HighCompression=3 };
struct ExgPackageBlock { std::uint64_t offset=0; std::uint32_t compressed_size=0; std::uint32_t decoded_size=0; ExgCodec codec=ExgCodec::Raw; std::uint32_t checksum=0; };
struct ExgPackage { std::vector<std::byte> bytes; std::vector<ExgPackageBlock> blocks; [[nodiscard]] bool valid() const noexcept{return !bytes.empty()&&!blocks.empty();} };
struct ExgPackageBuildOptions { std::size_t block_size=256u*1024u; bool high_compression=true; };
std::uint32_t exg_crc32(std::span<const std::byte>) noexcept;
bool build_exg_package(std::span<const std::byte>,const ExgPackageBuildOptions&,ExgPackage&,std::string&) noexcept;
bool open_exg_package(std::span<const std::byte>,ExgPackage&,std::string&) noexcept;
bool decode_exg_block(std::span<const std::byte>,const ExgPackageBlock&,std::vector<std::byte>&,std::string&) noexcept;

class ExgPackageStream {
public:
    using DecodeCallback=std::function<bool(std::uint64_t,std::span<const std::byte>,std::string&)>;
    explicit ExgPackageStream(ExgPackage); ~ExgPackageStream(); ExgPackageStream(const ExgPackageStream&)=delete; ExgPackageStream& operator=(const ExgPackageStream&)=delete;
    bool request(std::uint64_t,DecodeCallback); std::size_t pump(std::size_t max_jobs=8); [[nodiscard]] std::size_t pending()const noexcept;
private:
    struct Job{std::uint64_t block_id=0;DecodeCallback callback;}; struct Completed{std::uint64_t block_id=0;DecodeCallback callback;std::vector<std::byte> data;std::string error;};
    ExgPackage package_; mutable std::mutex mutex_; std::condition_variable condition_; std::queue<Job> jobs_; std::queue<Completed> completed_; std::thread worker_; bool stop_=false; void run();
};

class ExgGpuResidencyStreamer {
public:
    explicit ExgGpuResidencyStreamer(GpuAssetStreamController&gpu)noexcept:gpu_(gpu){}
    bool submit_mesh_block(ExgPackageStream&,std::uint64_t asset_id,std::uint32_t generation,std::uint64_t block_id,std::uint64_t frame_id,std::function<bool(std::span<const std::byte>,Mesh&,std::string&)> decode_mesh);
    std::size_t pump(ExgPackageStream&,std::uint64_t frame_id,std::size_t max_jobs=8);
private:GpuAssetStreamController&gpu_;
};
}
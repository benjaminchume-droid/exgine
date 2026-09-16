#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <limits>
#include <optional>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

namespace exgine::scale {

struct Vec3 { float x=0, y=0, z=0; };
inline float distance_sq(Vec3 a, Vec3 b) noexcept { const float x=a.x-b.x,y=a.y-b.y,z=a.z-b.z; return x*x+y*y+z*z; }

enum class Residency : std::uint8_t { Unloaded, Loading, Resident };
enum class DetailLevel : std::uint8_t { Full, Reduced, Proxy, Persistent };

struct WorldCellId { std::int64_t x=0,y=0,z=0; friend bool operator==(const WorldCellId&,const WorldCellId&) = default; };
struct WorldCellHash { std::size_t operator()(WorldCellId v) const noexcept { std::uint64_t x=static_cast<std::uint64_t>(v.x)*0x9e3779b97f4a7c15ULL; std::uint64_t y=static_cast<std::uint64_t>(v.y)+0x9e3779b97f4a7c15ULL; std::uint64_t z=static_cast<std::uint64_t>(v.z)+0x517cc1b727220a95ULL; x^=y+(x<<6)+(x>>2); x^=z+(x<<6)+(x>>2); return static_cast<std::size_t>(x^(x>>32)); } };

struct WorldCell { WorldCellId id{}; Vec3 center{}; std::uint32_t lod=0; Residency residency=Residency::Unloaded; std::uint64_t generation=0; std::size_t resident_bytes=0; };

struct WorldStreamingConfig {
    float cell_size=256.0f;
    float full_radius=512.0f;
    float reduced_radius=1024.0f;
    float proxy_radius=2048.0f;
    float unload_radius=2560.0f;
    std::size_t byte_budget=512ULL*1024ULL*1024ULL;
    std::size_t max_loaded_cells=512;
    std::size_t max_loads_per_tick=8;
    std::size_t max_unloads_per_tick=16;
};

class WorldPartition {
public:
    explicit WorldPartition(WorldStreamingConfig cfg={});
    void update(Vec3 focus);
    [[nodiscard]] WorldCellId cell_for(Vec3 p) const noexcept;
    [[nodiscard]] const WorldStreamingConfig& config() const noexcept { return cfg_; }
    [[nodiscard]] const std::unordered_map<WorldCellId,WorldCell,WorldCellHash>& cells() const noexcept { return cells_; }
    [[nodiscard]] std::size_t resident_bytes() const noexcept { return resident_bytes_; }
    [[nodiscard]] std::size_t loaded_count() const noexcept { return cells_.size(); }
    void clear() noexcept;
private:
    WorldStreamingConfig cfg_{};
    std::unordered_map<WorldCellId,WorldCell,WorldCellHash> cells_;
    std::size_t resident_bytes_=0;
    std::uint64_t generation_=0;
    static DetailLevel detail(float d,const WorldStreamingConfig&) noexcept;
    void load(WorldCell&,DetailLevel);
    void unload(WorldCellId);
};

struct HlodNode { std::uint64_t id=0; WorldCellId cell{}; float radius=0; std::uint32_t level=0; std::size_t bytes=0; bool visible=true; };
class HlodSystem {
public:
    void clear() noexcept { nodes_.clear(); }
    void add(HlodNode node) { nodes_[node.id]=node; }
    void update(Vec3 camera,float max_distance=4096.0f);
    [[nodiscard]] const std::unordered_map<std::uint64_t,HlodNode>& nodes() const noexcept { return nodes_; }
private: std::unordered_map<std::uint64_t,HlodNode> nodes_;
};

struct StreamRequest { std::uint64_t id=0; std::size_t bytes=0; float priority=0; std::function<void()> load; std::function<void()> unload; };
class AsyncStreamingScheduler {
public:
    explicit AsyncStreamingScheduler(std::size_t budget=512ULL*1024ULL*1024ULL):budget_(budget){}
    void request(StreamRequest request);
    void tick(std::size_t max_loads=8,std::size_t max_unloads=16);
    void set_budget(std::size_t b) noexcept { budget_=b; }
    [[nodiscard]] std::size_t resident_bytes() const noexcept { return resident_bytes_; }
    [[nodiscard]] std::size_t pending() const noexcept { return pending_.size(); }
private:
    std::size_t budget_; std::size_t resident_bytes_=0;
    std::unordered_map<std::uint64_t,StreamRequest> pending_,resident_;
    void evict_until(std::size_t required);
};

struct SimObject { std::uint64_t id=0; Vec3 position{}; float relevance=0; DetailLevel detail=DetailLevel::Persistent; bool active=true; };
class SimulationLodSystem {
public:
    void add(SimObject o) { objects_[o.id]=o; }
    void update(Vec3 focus);
    [[nodiscard]] const std::unordered_map<std::uint64_t,SimObject>& objects() const noexcept { return objects_; }
private: std::unordered_map<std::uint64_t,SimObject> objects_;
};

struct Job { std::uint64_t id=0; std::function<void()> fn; float priority=0; };
class JobSystem {
public:
    void submit(Job job) { queue_.push_back(std::move(job)); }
    std::size_t run(std::size_t max_jobs=std::numeric_limits<std::size_t>::max());
    [[nodiscard]] std::size_t pending() const noexcept { return queue_.size(); }
private: std::vector<Job> queue_;
};

struct NavNode { std::uint32_t id=0; Vec3 position{}; std::vector<std::uint32_t> neighbours; bool walkable=true; };
class NavigationGraph {
public:
    void clear() noexcept { nodes_.clear(); }
    void add(NavNode node) { nodes_[node.id]=std::move(node); }
    [[nodiscard]] std::vector<std::uint32_t> find_path(std::uint32_t start,std::uint32_t goal) const;
private: std::unordered_map<std::uint32_t,NavNode> nodes_;
};

struct Agent { std::uint64_t id=0; std::uint32_t nav_node=0; std::uint32_t target_node=0; float speed=2.0f; std::vector<std::uint32_t> path; std::size_t path_index=0; };
class CrowdNavigation {
public:
    explicit CrowdNavigation(const NavigationGraph& graph):graph_(&graph){}
    void add(Agent a) { agents_[a.id]=std::move(a); }
    void update();
    [[nodiscard]] const std::unordered_map<std::uint64_t,Agent>& agents() const noexcept { return agents_; }
private: const NavigationGraph* graph_; std::unordered_map<std::uint64_t,Agent> agents_;
};

struct VehicleState { std::uint64_t id=0; Vec3 position{}; float speed=0; float steering=0; float throttle=0; float brake=0; float wheelbase=2.6f; };
class VehicleSimulation {
public:
    void add(VehicleState v) { vehicles_[v.id]=v; }
    void tick(float dt);
    [[nodiscard]] const std::unordered_map<std::uint64_t,VehicleState>& vehicles() const noexcept { return vehicles_; }
private: std::unordered_map<std::uint64_t,VehicleState> vehicles_;
};

struct TrafficAgent { std::uint64_t id=0; std::uint64_t vehicle=0; std::vector<Vec3> route; std::size_t route_index=0; float desired_speed=12; };
class TrafficSimulation {
public:
    explicit TrafficSimulation(VehicleSimulation& v):vehicles_(&v){}
    void add(TrafficAgent a) { traffic_[a.id]=std::move(a); }
    void tick(float dt);
private: VehicleSimulation* vehicles_; std::unordered_map<std::uint64_t,TrafficAgent> traffic_;
};

struct AnimationState { std::uint64_t character=0; float speed=0; float phase=0; float weight=1; };
class AnimationLodSystem {
public: void add(AnimationState s){states_[s.character]=s;} void update(const std::unordered_map<std::uint64_t,SimObject>& objects); const auto& states() const noexcept{return states_;}
private: std::unordered_map<std::uint64_t,AnimationState> states_;
};

struct PersistentRecord { std::uint64_t id=0; std::string key; std::vector<std::byte> data; };
class PersistentWorld {
public:
    void put(PersistentRecord r){records_[r.id]=std::move(r);} 
    [[nodiscard]] const PersistentRecord* get(std::uint64_t id) const noexcept { auto i=records_.find(id); return i==records_.end()?nullptr:&i->second; }
    [[nodiscard]] std::vector<PersistentRecord> snapshot() const;
    void restore(const std::vector<PersistentRecord>& records);
private: std::unordered_map<std::uint64_t,PersistentRecord> records_;
};

enum class RenderBackend : std::uint8_t { OpenGLES31, Vulkan, DirectX12 };
struct RenderCapabilities { RenderBackend backend=RenderBackend::OpenGLES31; bool gpu_culling=false; bool bindless=false; bool async_compute=false; bool virtual_geometry=false; bool virtual_textures=false; bool ray_queries=false; bool hdr=false; };
class RenderCapabilityProfile {
public: static RenderCapabilities desktop_vulkan() noexcept; static RenderCapabilities desktop_dx12() noexcept; static RenderCapabilities mobile_gles() noexcept;
};

struct GeometryCluster { std::uint64_t id=0; std::vector<std::uint32_t> indices; float error=0; };
class VirtualGeometry {
public: void add(GeometryCluster c){clusters_[c.id]=std::move(c);} void select(float pixel_error); const auto& visible() const noexcept{return visible_;}
private: std::unordered_map<std::uint64_t,GeometryCluster> clusters_; std::vector<std::uint64_t> visible_;
};

struct TexturePage { std::uint64_t texture=0; std::uint32_t x=0,y=0,mip=0; std::uint64_t physical=0; bool resident=false; };
class VirtualTextureCache {
public: explicit VirtualTextureCache(std::size_t pages=4096):capacity_(pages){}
    void request(TexturePage page); void evict(std::size_t count); [[nodiscard]] std::size_t resident_pages() const noexcept{return pages_.size();}
private: std::size_t capacity_; std::unordered_map<std::uint64_t,TexturePage> pages_; std::vector<std::uint64_t> lru_;
};

struct IrradianceProbe { Vec3 position{}; float intensity=1; };
class GlobalIllumination {
public: void add_probe(IrradianceProbe p){probes_.push_back(p);} [[nodiscard]] float sample(Vec3 p) const noexcept;
private: std::vector<IrradianceProbe> probes_;
};

struct ReflectionQuery { Vec3 origin{}; Vec3 direction{0,0,1}; float max_distance=100; };
struct ReflectionResult { bool hit=false; float distance=0; };
class ReflectionSystem { public: ReflectionResult trace(ReflectionQuery q) const noexcept; };

class VolumetricAtmosphere { public: float transmittance(float density,float distance) const noexcept; };

struct ScaleRuntimeStats { std::size_t cells=0,bytes=0,pending_streams=0,sim_objects=0,jobs=0,vehicles=0; };
class ScaleRuntime {
public:
    explicit ScaleRuntime(WorldStreamingConfig cfg={}):world_(cfg),streaming_(cfg.byte_budget){}
    void update(Vec3 focus,float dt);
    [[nodiscard]] ScaleRuntimeStats stats() const noexcept;
    WorldPartition& world() noexcept{return world_;} AsyncStreamingScheduler& streaming() noexcept{return streaming_;} SimulationLodSystem& simulation() noexcept{return simulation_;} JobSystem& jobs() noexcept{return jobs_;} VehicleSimulation& vehicles() noexcept{return vehicles_;} PersistentWorld& persistence() noexcept{return persistence_;}
private:
    WorldPartition world_; AsyncStreamingScheduler streaming_; SimulationLodSystem simulation_; JobSystem jobs_; VehicleSimulation vehicles_; PersistentWorld persistence_;
};

} // namespace exgine::scale

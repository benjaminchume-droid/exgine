#include "exgine/scale_runtime.hpp"
#include <algorithm>
#include <cmath>
#include <queue>
#include <unordered_map>

namespace exgine::scale {

WorldPartition::WorldPartition(WorldStreamingConfig cfg):cfg_(cfg){
    if(cfg_.cell_size<=0) cfg_.cell_size=256;
    if(cfg_.full_radius<cfg_.cell_size) cfg_.full_radius=cfg_.cell_size;
    if(cfg_.reduced_radius<cfg_.full_radius) cfg_.reduced_radius=cfg_.full_radius*2;
    if(cfg_.proxy_radius<cfg_.reduced_radius) cfg_.proxy_radius=cfg_.reduced_radius*2;
    if(cfg_.unload_radius<cfg_.proxy_radius) cfg_.unload_radius=cfg_.proxy_radius*1.25f;
    if(cfg_.max_loaded_cells==0) cfg_.max_loaded_cells=1;
    if(cfg_.max_loads_per_tick==0) cfg_.max_loads_per_tick=1;
    if(cfg_.max_unloads_per_tick==0) cfg_.max_unloads_per_tick=1;
}
WorldCellId WorldPartition::cell_for(Vec3 p) const noexcept {
    return {static_cast<std::int64_t>(std::floor(p.x/cfg_.cell_size)),static_cast<std::int64_t>(std::floor(p.y/cfg_.cell_size)),static_cast<std::int64_t>(std::floor(p.z/cfg_.cell_size))};
}
DetailLevel WorldPartition::detail(float d,const WorldStreamingConfig& c) noexcept {
    if(d<=c.full_radius) return DetailLevel::Full;
    if(d<=c.reduced_radius) return DetailLevel::Reduced;
    if(d<=c.proxy_radius) return DetailLevel::Proxy;
    return DetailLevel::Persistent;
}
void WorldPartition::load(WorldCell& c,DetailLevel l){
    c.residency=Residency::Resident; c.lod=static_cast<std::uint32_t>(l); c.generation=++generation_;
    const std::size_t base=1024*1024; const std::size_t bytes=(l==DetailLevel::Full?base*8:l==DetailLevel::Reduced?base*3:l==DetailLevel::Proxy?base:base/4);
    resident_bytes_+=bytes; c.resident_bytes=bytes;
}
void WorldPartition::unload(WorldCellId id){auto i=cells_.find(id); if(i!=cells_.end()){resident_bytes_-=std::min(resident_bytes_,i->second.resident_bytes);cells_.erase(i);}}
void WorldPartition::update(Vec3 focus){
    const auto fc=cell_for(focus); const int r=static_cast<int>(std::ceil(cfg_.proxy_radius/cfg_.cell_size));
    std::vector<std::pair<float,WorldCellId>> wanted;
    wanted.reserve(static_cast<std::size_t>((2*r+1)*(2*r+1)*(2*r+1)));
    for(int z=-r;z<=r;++z) for(int y=-r;y<=r;++y) for(int x=-r;x<=r;++x){
        WorldCellId id{fc.x+x,fc.y+y,fc.z+z}; Vec3 center{(id.x+0.5f)*cfg_.cell_size,(id.y+0.5f)*cfg_.cell_size,(id.z+0.5f)*cfg_.cell_size};
        const float d=std::sqrt(distance_sq(center,focus)); if(d<=cfg_.unload_radius) wanted.emplace_back(d,id);
    }
    std::sort(wanted.begin(),wanted.end());
    std::size_t loads=0;
    for(auto [d,id]:wanted){
        auto it=cells_.find(id); if(it==cells_.end()){
            if(loads++>=cfg_.max_loads_per_tick || cells_.size()>=cfg_.max_loaded_cells) continue;
            WorldCell c; c.id=id; c.center={(id.x+0.5f)*cfg_.cell_size,(id.y+0.5f)*cfg_.cell_size,(id.z+0.5f)*cfg_.cell_size}; load(c,detail(d,cfg_));
            cells_.emplace(id,std::move(c));
        } else {
            const auto dl=static_cast<std::uint32_t>(detail(d,cfg_));
            if(it->second.lod!=dl){resident_bytes_-=std::min(resident_bytes_,it->second.resident_bytes);it->second.resident_bytes=0;load(it->second,static_cast<DetailLevel>(dl));}
        }
    }
    std::vector<std::pair<float,WorldCellId>> evict;
    for(const auto& [id,c]:cells_){const float d=std::sqrt(distance_sq(c.center,focus)); if(d>cfg_.unload_radius) evict.emplace_back(d,id);}
    std::sort(evict.rbegin(),evict.rend()); for(std::size_t i=0;i<std::min(cfg_.max_unloads_per_tick,evict.size());++i) unload(evict[i].second);
    while(resident_bytes_>cfg_.byte_budget && !cells_.empty()){
        auto far=std::max_element(cells_.begin(),cells_.end(),[&](const auto&a,const auto&b){return distance_sq(a.second.center,focus)<distance_sq(b.second.center,focus);}); unload(far->first);
    }
}
void WorldPartition::clear() noexcept {cells_.clear();resident_bytes_=0;generation_=0;}

void HlodSystem::update(Vec3 camera,float max_distance){for(auto& [id,n]:nodes_) n.visible=distance_sq(n.cell.x==0?camera:Vec3{static_cast<float>(n.cell.x),static_cast<float>(n.cell.y),static_cast<float>(n.cell.z)},camera)<=max_distance*max_distance;}

void AsyncStreamingScheduler::request(StreamRequest r){if(r.id==0)return; pending_[r.id]=std::move(r);}
void AsyncStreamingScheduler::evict_until(std::size_t required){while(resident_bytes_+required>budget_&&!resident_.empty()){auto it=resident_.begin();if(it->second.unload)it->second.unload();resident_bytes_-=std::min(resident_bytes_,it->second.bytes);resident_.erase(it);}}
void AsyncStreamingScheduler::tick(std::size_t max_loads,std::size_t max_unloads){(void)max_unloads;std::vector<std::reference_wrapper<StreamRequest>> ordered;for(auto& [id,r]:pending_)ordered.push_back(r);std::sort(ordered.begin(),ordered.end(),[](const auto&a,const auto&b){return a.get().priority>b.get().priority;});std::size_t n=0;for(auto& rr:ordered){if(n++>=max_loads)break;auto it=pending_.find(rr.get().id);if(it==pending_.end())continue;evict_until(it->second.bytes);if(it->second.bytes>budget_)continue;if(it->second.load)it->second.load();resident_bytes_+=it->second.bytes;resident_[it->first]=std::move(it->second);pending_.erase(it);}}

void SimulationLodSystem::update(Vec3 focus){for(auto& [id,o]:objects_){const float d=std::sqrt(distance_sq(o.position,focus));o.detail=d<100?DetailLevel::Full:d<500?DetailLevel::Reduced:d<1500?DetailLevel::Proxy:DetailLevel::Persistent;o.active=o.detail!=DetailLevel::Persistent||o.relevance>0.5f;}}
std::size_t JobSystem::run(std::size_t max_jobs){std::sort(queue_.begin(),queue_.end(),[](const Job&a,const Job&b){return a.priority>b.priority;});const std::size_t n=std::min(max_jobs,queue_.size());for(std::size_t i=0;i<n;++i)if(queue_[i].fn)queue_[i].fn();queue_.erase(queue_.begin(),queue_.begin()+static_cast<std::ptrdiff_t>(n));return n;}

std::vector<std::uint32_t> NavigationGraph::find_path(std::uint32_t start,std::uint32_t goal) const {
    if(nodes_.find(start)==nodes_.end()||nodes_.find(goal)==nodes_.end())return{};std::queue<std::uint32_t> q;std::unordered_map<std::uint32_t,std::uint32_t> prev;std::unordered_set<std::uint32_t> seen;q.push(start);seen.insert(start);
    while(!q.empty()){auto n=q.front();q.pop();if(n==goal)break;auto it=nodes_.find(n);if(it==nodes_.end())continue;for(auto next:it->second.neighbours)if(seen.insert(next).second){prev[next]=n;q.push(next);}}
    if(!seen.count(goal))return{};std::vector<std::uint32_t> path;for(auto n=goal;;n=prev[n]){path.push_back(n);if(n==start)break;}std::reverse(path.begin(),path.end());return path;
}
void CrowdNavigation::update(){for(auto& [id,a]:agents_){if(a.path.empty()||a.path_index>=a.path.size())a.path=graph_->find_path(a.nav_node,a.target_node),a.path_index=0;if(a.path_index<a.path.size()&&a.path[a.path_index]==a.nav_node)++a.path_index;if(a.path_index<a.path.size())a.nav_node=a.path[a.path_index];}}

void VehicleSimulation::tick(float dt){for(auto& [id,v]:vehicles_){const float engine=18.0f*v.throttle;const float braking=25.0f*v.brake;v.speed=std::max(0.0f,v.speed+(engine-braking-0.015f*v.speed*v.speed)*dt);v.position.x+=std::cos(v.steering)*v.speed*dt;v.position.z+=std::sin(v.steering)*v.speed*dt;}}
void TrafficSimulation::tick(float dt){for(auto& [id,t]:traffic_){auto it=vehicles_->vehicles().find(t.vehicle);if(it==vehicles_->vehicles().end()||t.route.empty())continue;auto& v=const_cast<VehicleState&>(it->second);v.throttle=v.speed<t.desired_speed?1.0f:0.0f;v.brake=v.speed>t.desired_speed+2?1.0f:0.0f;vehicles_->tick(dt);}}
void AnimationLodSystem::update(const std::unordered_map<std::uint64_t,SimObject>& objects){for(auto& [id,s]:states_){auto it=objects.find(id);if(it==objects.end())continue;const auto d=it->second.detail;s.weight=d==DetailLevel::Full?1.0f:d==DetailLevel::Reduced?0.75f:d==DetailLevel::Proxy?0.35f:0.0f;s.phase+=s.speed*0.016f;}}
std::vector<PersistentRecord> PersistentWorld::snapshot() const{std::vector<PersistentRecord> out;out.reserve(records_.size());for(const auto& [id,r]:records_)out.push_back(r);return out;}
void PersistentWorld::restore(const std::vector<PersistentRecord>& records){records_.clear();for(auto r:records)records_[r.id]=std::move(r);}

RenderCapabilities RenderCapabilityProfile::desktop_vulkan() noexcept{return {RenderBackend::Vulkan,true,true,true,true,true,true,true};}
RenderCapabilities RenderCapabilityProfile::desktop_dx12() noexcept{return {RenderBackend::DirectX12,true,true,true,true,true,true,true};}
RenderCapabilities RenderCapabilityProfile::mobile_gles() noexcept{return {RenderBackend::OpenGLES31,false,false,false,false,false,false,false};}
void VirtualGeometry::select(float pixel_error){visible_.clear();for(const auto& [id,c]:clusters_)if(c.error<=pixel_error||pixel_error<=0)visible_.push_back(id);std::sort(visible_.begin(),visible_.end());}
void VirtualTextureCache::request(TexturePage page){const std::uint64_t key=page.texture^(static_cast<std::uint64_t>(page.x)<<16)^(static_cast<std::uint64_t>(page.y)<<32)^(static_cast<std::uint64_t>(page.mip)<<48);page.physical=key;page.resident=true;if(pages_.find(key)==pages_.end()&&pages_.size()>=capacity_)evict(1);pages_[key]=page;lru_.erase(std::remove(lru_.begin(),lru_.end(),key),lru_.end());lru_.push_back(key);}
void VirtualTextureCache::evict(std::size_t count){while(count--&&!lru_.empty()){auto k=lru_.front();lru_.erase(lru_.begin());pages_.erase(k);}}
float GlobalIllumination::sample(Vec3 p) const noexcept{if(probes_.empty())return 0;float sum=0,w=0;for(const auto& q:probes_){const float d2=distance_sq(p,q.position)+1.0f;const float a=1.0f/d2;sum+=q.intensity*a;w+=a;}return sum/w;}
ReflectionResult ReflectionSystem::trace(ReflectionQuery q) const noexcept{if(q.max_distance<=0)return{};return {true,std::min(q.max_distance,100.0f)};}
float VolumetricAtmosphere::transmittance(float density,float distance) const noexcept{return std::exp(-std::max(0.0f,density)*std::max(0.0f,distance));}

void ScaleRuntime::update(Vec3 focus,float dt){world_.update(focus);simulation_.update(focus);vehicles_.tick(dt);jobs_.run();streaming_.tick();}
ScaleRuntimeStats ScaleRuntime::stats() const noexcept{return {world_.loaded_count(),world_.resident_bytes(),streaming_.pending(),simulation_.objects().size(),jobs_.pending(),vehicles_.vehicles().size()};}

} // namespace exgine::scale

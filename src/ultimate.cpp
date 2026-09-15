#include "exgine/ultimate.hpp"
#include <algorithm>
#include <queue>

namespace exgine {

bool RenderGraph::add_resource(RenderGraphResource resource){
    if(resource.name.empty()||resources_.count(resource.name)) return false;
    resource.width=std::max(1u,resource.width); resource.height=std::max(1u,resource.height);
    resources_.emplace(resource.name,std::move(resource)); return true;
}
bool RenderGraph::add_pass(RenderGraphPass pass){
    if(pass.name.empty()) return false;
    if(std::any_of(passes_.begin(),passes_.end(),[&](const RenderGraphPass& p){return p.name==pass.name;})) return false;
    for(const auto& n:pass.reads) if(!resources_.count(n)) return false;
    for(const auto& n:pass.writes) if(!resources_.count(n)) return false;
    passes_.push_back(std::move(pass)); return true;
}
std::vector<std::string> RenderGraph::compile() const{
    std::vector<std::string> out; if(!valid()) return out;
    const std::size_t n=passes_.size(); std::vector<std::vector<std::size_t>> edges(n); std::vector<std::size_t> indegree(n,0);
    for(std::size_t i=0;i<n;++i) for(std::size_t j=i+1;j<n;++j){
        bool dep=false;
        for(const auto& w:passes_[i].writes){
            if(std::find(passes_[j].reads.begin(),passes_[j].reads.end(),w)!=passes_[j].reads.end() || std::find(passes_[j].writes.begin(),passes_[j].writes.end(),w)!=passes_[j].writes.end()){dep=true;break;}
        }
        if(dep){edges[i].push_back(j);++indegree[j];}
    }
    std::queue<std::size_t> q; for(std::size_t i=0;i<n;++i) if(indegree[i]==0) q.push(i);
    while(!q.empty()){auto i=q.front();q.pop();out.push_back(passes_[i].name);for(auto j:edges[i])if(--indegree[j]==0)q.push(j);}
    if(out.size()!=n) out.clear(); return out;
}
bool RenderGraph::valid() const noexcept{
    for(const auto& p:passes_) { for(const auto& r:p.reads) if(!resources_.count(r)) return false; for(const auto& w:p.writes) if(!resources_.count(w)) return false; }
    return true;
}
void RenderGraph::clear() noexcept { resources_.clear(); passes_.clear(); }

bool GameFlowController::transition(GameFlowState next) noexcept{
    if(next==state_) return true;
    const bool allowed =
      (state_==GameFlowState::Boot && (next==GameFlowState::Loading||next==GameFlowState::Error)) ||
      (state_==GameFlowState::Loading && (next==GameFlowState::Running||next==GameFlowState::Error)) ||
      (state_==GameFlowState::Running && (next==GameFlowState::Paused||next==GameFlowState::Saving||next==GameFlowState::Quitting||next==GameFlowState::Error)) ||
      (state_==GameFlowState::Paused && (next==GameFlowState::Running||next==GameFlowState::Saving||next==GameFlowState::Quitting)) ||
      (state_==GameFlowState::Saving && (next==GameFlowState::Running||next==GameFlowState::Error)) ||
      (state_==GameFlowState::Quitting && next==GameFlowState::Stopped) ||
      (state_==GameFlowState::Error && next==GameFlowState::Stopped);
    if(!allowed) return false; state_=next; ++transitions_; return true;
}

void RuntimeProfiler::begin_frame() noexcept { samples_.clear(); frame_ms_=0; }
void RuntimeProfiler::record(std::string_view name,double milliseconds) noexcept {
    if(name.empty()||milliseconds<0) return;
    for(auto& s:samples_) if(s.name==name){s.milliseconds+=milliseconds;++s.calls;frame_ms_+=milliseconds;return;}
    samples_.push_back({std::string(name),milliseconds,1}); frame_ms_+=milliseconds;
}

bool RollbackBuffer::push(SimulationSnapshot snapshot){
    if(snapshot.bytes.empty()) return false;
    auto it=std::lower_bound(snapshots_.begin(),snapshots_.end(),snapshot.tick,[](const SimulationSnapshot& s,std::uint64_t tick){return s.tick<tick;});
    if(it!=snapshots_.end()&&it->tick==snapshot.tick) *it=std::move(snapshot); else snapshots_.insert(it,std::move(snapshot));
    while(snapshots_.size()>capacity_) snapshots_.erase(snapshots_.begin()); return true;
}
const SimulationSnapshot* RollbackBuffer::find(std::uint64_t tick) const noexcept { auto it=std::lower_bound(snapshots_.begin(),snapshots_.end(),tick,[](const SimulationSnapshot&s,std::uint64_t t){return s.tick<t;}); return it!=snapshots_.end()&&it->tick==tick?&*it:nullptr; }
bool RollbackBuffer::discard_before(std::uint64_t tick) noexcept { const auto before=snapshots_.size(); snapshots_.erase(std::remove_if(snapshots_.begin(),snapshots_.end(),[&](const SimulationSnapshot&s){return s.tick<tick;}),snapshots_.end()); return snapshots_.size()!=before; }

void PresentationSystem::update_environment(const EnvironmentState& environment) noexcept {
    const float w=std::clamp(environment.weather_intensity,0.0f,1.0f);
    switch(environment.weather){
        case WeatherType::Storm: state_.weather.cloud_cover=1;state_.weather.cloud_density=1;state_.weather.precipitation=w;state_.weather.wind_speed=20*w;state_.weather.fog_density=.15f*w;break;
        case WeatherType::Rain: state_.weather.cloud_cover=.8f;state_.weather.cloud_density=.7f;state_.weather.precipitation=w;state_.weather.wind_speed=8*w;state_.weather.fog_density=.08f*w;break;
        case WeatherType::Snow: state_.weather.cloud_cover=.85f;state_.weather.cloud_density=.75f;state_.weather.precipitation=w;state_.weather.wind_speed=6*w;state_.weather.fog_density=.06f*w;state_.weather.snow_cover=w;break;
        case WeatherType::Fog: state_.weather.cloud_cover=.3f;state_.weather.cloud_density=.2f;state_.weather.precipitation=0;state_.weather.fog_density=.3f*w;break;
        case WeatherType::Cloudy: state_.weather.cloud_cover=.7f;state_.weather.cloud_density=.5f;state_.weather.precipitation=0;break;
        case WeatherType::Wind: state_.weather.cloud_cover=.2f;state_.weather.cloud_density=.15f;state_.weather.wind_speed=15*w;break;
        default: state_.weather={}; break;
    }
    const float sun=std::clamp(environment.sun_angle*0.5f+0.5f,0.0f,1.0f);
    state_.lighting.sun_intensity=1.0f+4.0f*sun;
    state_.atmosphere.sun_direction={0.0f,-std::max(0.05f,sun),1.0f};
    state_.atmosphere.cloud_coverage=state_.weather.cloud_cover;
    state_.atmosphere.cloud_density=state_.weather.cloud_density;
}

ProductionReadinessReport evaluate_production_readiness(bool project,bool runtime,bool rendering,bool physics,bool content,bool saving,bool packaging) noexcept { return {project,runtime,rendering,physics,content,saving,packaging}; }

} // namespace exgine

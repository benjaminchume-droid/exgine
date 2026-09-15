#include "exgine/frontier.hpp"
#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {

bool RenderFeatureGraph::add(RenderFeatureNode node) {
    if (node.name.empty() || nodes_.find(node.name) != nodes_.end()) return false;
    for (const auto& dep : node.depends_on) if (dep == node.name) return false;
    nodes_.emplace(node.name,std::move(node));
    return true;
}

bool RenderFeatureGraph::set_enabled(std::string_view name,bool enabled) noexcept {
    auto it=nodes_.find(std::string(name));
    if (it==nodes_.end()) return false;
    it->second.enabled=enabled;
    return true;
}

RenderExecutionPlan RenderFeatureGraph::compile() const {
    RenderExecutionPlan plan;
    std::unordered_map<std::string,std::uint8_t> mark;
    std::function<bool(const std::string&)> visit = [&](const std::string& name) {
        const auto it=nodes_.find(name);
        if (it==nodes_.end()) { plan.error="missing render dependency: "+name; return false; }
        if (!it->second.enabled) return true;
        const auto state=mark[name];
        if (state==1) { plan.error="render dependency cycle"; return false; }
        if (state==2) return true;
        mark[name]=1;
        for (const auto& dep:it->second.depends_on) if(!visit(dep)) return false;
        mark[name]=2;
        plan.order.push_back(name);
        return true;
    };
    for (const auto& [name,node]:nodes_) if(node.enabled && !visit(name)) return plan;
    plan.valid=true;
    return plan;
}

void TemporalReconstruction::resize(std::uint32_t width,std::uint32_t height) noexcept {
    if (history_.width!=width || history_.height!=height) { history_.width=width; history_.height=height; reset(); }
}

TemporalJitter TemporalReconstruction::next_jitter() noexcept {
    static constexpr float inv=1.0f/16777216.0f;
    const std::uint32_t i=++history_.jitter.index;
    auto radical_inverse=[](std::uint32_t n)->float { float r=0.0f,f=0.5f; while(n){r+=(n&1U)?f:0.0f;n>>=1U;f*=0.5f;} return r; };
    history_.jitter.x=radical_inverse(i)-0.5f;
    std::uint32_t n=i; float r=0.0f,f=1.0f/3.0f; while(n){r+=(n%3U==1U?f:n%3U==2U?2.0f*f:0.0f);n/=3U;f/=3.0f;}
    history_.jitter.y=r-0.5f;
    (void)inv;
    return history_.jitter;
}

void TemporalReconstruction::begin_frame(std::uint64_t frame_id,float motion_magnitude,float disocclusion) noexcept {
    const float motion=std::clamp(motion_magnitude,0.0f,1.0f);
    const float disco=std::clamp(disocclusion,0.0f,1.0f);
    history_.blend=std::clamp(0.94f-motion*0.55f-disco*0.75f,0.05f,0.95f);
    if(history_.frame_id!=0 && frame_id!=history_.frame_id+1) history_.valid=false;
    history_.frame_id=frame_id;
    history_.valid=(history_.width>0 && history_.height>0);
    next_jitter();
}

bool GameFlowController::can_transition(GameFlowState next) const noexcept {
    const auto s=snapshot_.state;
    if (s==GameFlowState::Shutdown) return next==GameFlowState::Shutdown;
    if (next==GameFlowState::Error) return true;
    switch(s){
        case GameFlowState::Boot: return next==GameFlowState::Loading||next==GameFlowState::Shutdown;
        case GameFlowState::Loading: return next==GameFlowState::MainMenu||next==GameFlowState::Playing||next==GameFlowState::Error;
        case GameFlowState::MainMenu: return next==GameFlowState::Loading||next==GameFlowState::Playing||next==GameFlowState::Shutdown;
        case GameFlowState::Playing: return next==GameFlowState::Paused||next==GameFlowState::Saving||next==GameFlowState::MainMenu||next==GameFlowState::Shutdown;
        case GameFlowState::Paused: return next==GameFlowState::Playing||next==GameFlowState::Saving||next==GameFlowState::MainMenu||next==GameFlowState::Shutdown;
        case GameFlowState::Saving: return next==GameFlowState::Playing||next==GameFlowState::Paused||next==GameFlowState::Error;
        case GameFlowState::Error: return next==GameFlowState::Boot||next==GameFlowState::Shutdown;
        case GameFlowState::Shutdown: return true;
    }
    return false;
}

bool GameFlowController::transition(GameFlowState next,std::string error) {
    if(!can_transition(next)) return false;
    snapshot_.previous=snapshot_.state;
    snapshot_.state=next;
    snapshot_.error=std::move(error);
    ++snapshot_.transitions;
    return true;
}

void EngineProfiler::begin(std::string_view name) noexcept { if(!name.empty()) ++active_[std::string(name)]; }
void EngineProfiler::end(std::string_view name,double milliseconds) noexcept {
    if(name.empty()||milliseconds<0)return;
    auto key=std::string(name); auto it=active_.find(key); if(it!=active_.end()){if(it->second>1)--it->second;else active_.erase(it);}
    auto& s=samples_[key]; s.name=key; s.milliseconds+=milliseconds; ++s.calls;
}
void EngineProfiler::add_counter(std::string_view name,std::uint64_t amount) noexcept { if(!name.empty()) counters_[std::string(name)]+=amount; }
const ProfileSample* EngineProfiler::sample(std::string_view name) const noexcept { auto it=samples_.find(std::string(name)); return it==samples_.end()?nullptr:&it->second; }
std::uint64_t EngineProfiler::counter(std::string_view name) const noexcept { auto it=counters_.find(std::string(name)); return it==counters_.end()?0:it->second; }
std::vector<ProfileSample> EngineProfiler::samples() const { std::vector<ProfileSample> out; out.reserve(samples_.size()); for(const auto& [_,s]:samples_) out.push_back(s); std::sort(out.begin(),out.end(),[](const auto&a,const auto&b){return a.name<b.name;}); return out; }
void EngineProfiler::reset() noexcept { samples_.clear(); counters_.clear(); active_.clear(); }

bool RollbackBuffer::push(std::uint32_t tick,std::vector<std::uint8_t> state) {
    if(state.empty()) return false;
    auto it=std::lower_bound(frames_.begin(),frames_.end(),tick,[](const RollbackFrame& f,std::uint32_t t){return f.tick<t;});
    if(it!=frames_.end()&&it->tick==tick) it->state=std::move(state); else frames_.insert(it,RollbackFrame{tick,std::move(state)});
    if(frames_.size()>capacity_) frames_.erase(frames_.begin(),frames_.begin()+static_cast<std::ptrdiff_t>(frames_.size()-capacity_));
    return true;
}
const RollbackFrame* RollbackBuffer::exact(std::uint32_t tick) const noexcept { auto it=std::lower_bound(frames_.begin(),frames_.end(),tick,[](const RollbackFrame& f,std::uint32_t t){return f.tick<t;}); return it!=frames_.end()&&it->tick==tick?&*it:nullptr; }
const RollbackFrame* RollbackBuffer::latest_at_or_before(std::uint32_t tick) const noexcept { if(frames_.empty())return nullptr; auto it=std::upper_bound(frames_.begin(),frames_.end(),tick,[](std::uint32_t t,const RollbackFrame& f){return t<f.tick;}); if(it==frames_.begin())return nullptr; --it; return &*it; }
bool RollbackBuffer::discard_after(std::uint32_t tick) noexcept { const auto old=frames_.size(); frames_.erase(std::upper_bound(frames_.begin(),frames_.end(),tick,[](std::uint32_t t,const RollbackFrame& f){return t<f.tick;}),frames_.end()); return frames_.size()!=old; }

void WorldPresentation::set_sun_elevation(float radians) noexcept {
    const float day=std::sin(radians);
    state_.exposure=std::clamp(0.35f+day*1.1f,-2.0f,2.0f);
}
void WorldPresentation::set_weather_fog(float density) noexcept { state_.contrast=std::clamp(1.0f-std::clamp(density,0.0f,1.0f)*0.18f,0.6f,1.0f); }
void WorldPresentation::set_auto_exposure(float luminance,double dt) noexcept {
    if(dt<0)return;
    const float target=std::clamp(0.85f-std::log2(std::max(1.0e-4f,luminance)), -6.0f,6.0f);
    const float alpha=1.0f-std::exp(-static_cast<float>(dt)*2.5f);
    state_.exposure+= (target-state_.exposure)*alpha;
}

ProductionAcceptanceResult evaluate_production_acceptance(const ProductionAcceptanceInput& input) {
    ProductionAcceptanceResult r;
    const bool checks[]={input.project_open,input.scene_loaded,input.player_spawned,input.render_frame_valid,input.physics_running,input.streaming_running,input.save_roundtrip,input.android_build_configured};
    static constexpr const char* names[]={"project_open","scene_loaded","player_spawned","render_frame_valid","physics_running","streaming_running","save_roundtrip","android_build_configured"};
    for(std::size_t i=0;i<sizeof(checks)/sizeof(checks[0]);++i) if(!checks[i]) r.missing.emplace_back(names[i]);
    r.ready=r.missing.empty();
    return r;
}

} // namespace exgine

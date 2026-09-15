#include "exgine/frontier.hpp"
#include <cassert>
#include <cmath>
using namespace exgine;

int main(){
    RenderFeatureGraph graph;
    assert(graph.add({RenderFeature::Shadow,"shadow",{}, {"shadow_map"},{}}));
    assert(graph.add({RenderFeature::GBuffer,"gbuffer",{"shadow_map"},{"scene_color"},{"shadow"}}));
    assert(graph.add({RenderFeature::Lighting,"lighting",{"scene_color"},{"lit"},{"gbuffer"}}));
    auto plan=graph.compile(); assert(plan.valid&&plan.order.size()==3&&plan.order[0]=="shadow"&&plan.order[2]=="lighting");
    assert(graph.set_enabled("gbuffer",false)); plan=graph.compile(); assert(plan.valid&&plan.order.size()==2);
    RenderFeatureGraph cycle; assert(cycle.add({RenderFeature::Depth,"a",{}, {}, {"b"}})); assert(cycle.add({RenderFeature::Lighting,"b",{}, {}, {"a"}})); assert(!cycle.compile().valid);

    TemporalReconstruction temporal; temporal.resize(1920,1080); auto j0=temporal.next_jitter(); temporal.begin_frame(1,.1f,.0f); auto h=temporal.history(); assert(h.valid&&h.width==1920&&h.height==1080&&std::isfinite(j0.x)); temporal.begin_frame(3,0,0); assert(!temporal.history().valid);

    GameFlowController flow; assert(flow.state()==GameFlowState::Boot); assert(flow.transition(GameFlowState::Loading)); assert(flow.transition(GameFlowState::MainMenu)); assert(flow.transition(GameFlowState::Playing)); assert(flow.transition(GameFlowState::Paused)); assert(flow.transition(GameFlowState::Playing)); assert(!flow.transition(GameFlowState::Boot)); assert(flow.snapshot().transitions==5);

    EngineProfiler profiler; profiler.begin("render"); profiler.end("render",4.5); profiler.add_counter("draws",37); assert(profiler.sample("render")&&profiler.sample("render")->calls==1&&profiler.counter("draws")==37); assert(profiler.samples().size()==1);

    RollbackBuffer rollback(3); assert(rollback.push(1,{1})&&rollback.push(2,{2})&&rollback.push(3,{3})&&rollback.push(4,{4})); assert(rollback.size()==3&&!rollback.exact(1)); assert(rollback.latest_at_or_before(3)->state[0]==3); assert(rollback.discard_after(2)&&rollback.size()==2);

    WorldPresentation presentation; presentation.set_sun_elevation(1.0f); const float daylight=presentation.state().exposure; presentation.set_weather_fog(1.0f); assert(presentation.state().contrast<1.0f); presentation.set_auto_exposure(.5f,.1); assert(std::isfinite(presentation.state().exposure)&&std::isfinite(daylight));

    ProductionAcceptanceInput incomplete{true,true,true,true,true,true,false,true}; auto result=evaluate_production_acceptance(incomplete); assert(!result.ready&&result.missing.size()==1&&result.missing[0]=="save_roundtrip");
    incomplete.save_roundtrip=true; result=evaluate_production_acceptance(incomplete); assert(result.ready&&result.missing.empty());
    return 0;
}

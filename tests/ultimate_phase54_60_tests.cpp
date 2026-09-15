#include "exgine/ultimate.hpp"
#include <cassert>

using namespace exgine;

int main(){
    RenderGraph graph;
    assert(graph.add_resource({"depth",RenderResourceKind::Depth,1280,720,true}));
    assert(graph.add_resource({"hdr",RenderResourceKind::Target,1280,720,true}));
    assert(graph.add_pass({"depth",{}, {"depth"}}));
    assert(graph.add_pass({"opaque",{"depth"},{"hdr"}}));
    assert(graph.add_pass({"post",{"hdr"},{"hdr"}}));
    const auto order=graph.compile();
    assert(order.size()==3&&order[0]=="depth"&&order[1]=="opaque");

    TemporalAccumulator temporal;
    TemporalCameraState camera; camera.frame=10;
    temporal.begin(camera); assert(!temporal.history().valid); temporal.commit(); assert(temporal.history().valid);
    camera.frame=11; temporal.begin(camera); temporal.commit(); assert(temporal.history().previous.frame==11); temporal.reset(); assert(!temporal.history().valid);

    GameFlowController flow;
    assert(flow.transition(GameFlowState::Loading));
    assert(flow.transition(GameFlowState::Running));
    assert(flow.transition(GameFlowState::Paused));
    assert(flow.transition(GameFlowState::Running));
    assert(!flow.transition(GameFlowState::Boot));

    RuntimeProfiler profiler;
    profiler.begin_frame(); profiler.record("render",4.0); profiler.record("render",2.0); profiler.record("physics",3.0);
    assert(profiler.samples().size()==2&&profiler.frame_ms()==9.0);
    assert(profiler.samples()[0].calls==2);

    RollbackBuffer rollback(2);
    assert(rollback.push({1,{1}})); assert(rollback.push({2,{2}})); assert(rollback.push({3,{3}}));
    assert(rollback.find(1)==nullptr&&rollback.find(2)&&rollback.find(3));
    assert(rollback.discard_before(3)); assert(rollback.find(2)==nullptr);

    EnvironmentSystem env;
    env.set_weather(WeatherType::Storm,.75f);
    PresentationSystem presentation;
    presentation.update_environment(env.state());
    assert(presentation.state().weather.precipitation>.7f);
    assert(presentation.state().weather.cloud_cover>.9f);

    const auto report=evaluate_production_readiness(true,true,true,true,true,true,true);
    assert(report.ready());
    const auto not_ready=evaluate_production_readiness(true,false,true,true,true,true,true);
    assert(!not_ready.ready());
    return 0;
}

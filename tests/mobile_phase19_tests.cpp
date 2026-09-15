#include "exgine/mobile.hpp"

#include <cassert>
#include <cmath>

using namespace exgine;

namespace {
void test_input_queue() {
    MobileInputQueue queue;
    MobileInputEvent down;
    down.type = MobileInputType::TouchDown;
    down.touch.pointer_id = 7;
    down.touch.x = 10.0f;
    down.touch.y = 20.0f;
    down.touch.pressure = 1.0f;

    assert(queue.push(down));
    assert(queue.touch_active(7));
    assert(queue.active_touch_count() == 1);

    auto event = queue.poll();
    assert(event.has_value());
    assert(event->touch.pointer_id == 7);
    assert(queue.stats().queued == 0);
    assert(queue.stats().valid());

    MobileInputEvent invalid = down;
    invalid.touch.x = NAN;
    assert(!queue.push(invalid));

    MobileInputEvent up = down;
    up.type = MobileInputType::TouchUp;
    assert(queue.push(up));
    assert(!queue.touch_active(7));
    assert(queue.active_touch_count() == 0);
}

void test_input_overflow() {
    MobileInputQueue queue;
    MobileInputEvent key;
    key.type = MobileInputType::KeyDown;
    key.key_code = 4;

    for (std::size_t i = 0; i < MobileInputQueue::capacity + 3; ++i) {
        assert(queue.push(key));
    }
    assert(queue.stats().queued == MobileInputQueue::capacity);
    assert(queue.stats().dropped == 3);
}

void test_pacer() {
    MobileFramePacer pacer(1);
    assert(pacer.target_fps() == 15);
    pacer.set_target_fps(144);
    assert(pacer.target_fps() == 120);
    pacer.set_target_fps(60);

    auto first = pacer.tick(10.0);
    assert(first.state == MobileFrameState::FirstFrame);
    assert(first.delta_seconds == 0.0);

    auto second = pacer.tick(10.02);
    assert(second.state == MobileFrameState::Running);
    assert(second.delta_seconds > 0.0);
    assert(second.delta_seconds < 0.021);
    assert(second.valid());

    pacer.pause();
    auto paused = pacer.tick(10.2);
    assert(paused.state == MobileFrameState::Paused);
    assert(paused.delta_seconds == 0.0);

    pacer.resume();
    auto resumed = pacer.tick(10.3);
    assert(resumed.state == MobileFrameState::FirstFrame);
}

void test_runtime_bridge() {
    MobileRuntimeBridge mobile;
    assert(mobile.lifecycle() == MobileLifecycleState::Created);
    assert(!mobile.running());
    assert(!mobile.renderable());

    mobile.on_start();
    mobile.on_resume();
    assert(mobile.running());
    assert(!mobile.renderable());

    mobile.on_surface_available();
    assert(mobile.renderable());

    mobile.set_target_fps(90);
    auto first = mobile.begin_frame(1.0);
    assert(first.state == MobileFrameState::FirstFrame);
    auto second = mobile.begin_frame(1.01);
    assert(second.state == MobileFrameState::Running);
    assert(second.delta_seconds > 0.0);

    mobile.on_pause();
    assert(!mobile.running());
    assert(!mobile.renderable());

    mobile.on_resume();
    mobile.on_surface_lost();
    assert(!mobile.renderable());

    mobile.on_destroy();
    assert(mobile.lifecycle() == MobileLifecycleState::Destroyed);
    assert(!mobile.running());
}
}

int main() {
    test_input_queue();
    test_input_overflow();
    test_pacer();
    test_runtime_bridge();
    return 0;
}

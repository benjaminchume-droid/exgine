#include "exgine/android.hpp"
#include "exgine/mobile.hpp"
#include "exgine/render.hpp"

#include <android/input.h>
#include <android_native_app_glue.h>

#include <cstdint>
#include <ctime>

namespace {

struct AppState {
    exgine::AndroidEglPresenter presenter;
    exgine::MobileRuntimeBridge mobile;
    std::uint64_t frame_id = 1;
};

std::uint64_t monotonic_time_ns() {
    timespec value{};
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) {
        return 0;
    }
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ull +
           static_cast<std::uint64_t>(value.tv_nsec);
}

exgine::MobileInputEvent make_touch_event(exgine::MobileInputType type, const AInputEvent* input,
                                          std::size_t pointer_index) {
    exgine::MobileInputEvent event;
    event.type = type;
    event.timestamp_ns = monotonic_time_ns();
    event.touch.pointer_id = AMotionEvent_getPointerId(input, pointer_index);
    event.touch.x = AMotionEvent_getX(input, pointer_index);
    event.touch.y = AMotionEvent_getY(input, pointer_index);
    event.touch.pressure = AMotionEvent_getPressure(input, pointer_index);
    return event;
}

int32_t handle_input(android_app* app, AInputEvent* input) {
    auto* state = static_cast<AppState*>(app->userData);
    if (state == nullptr || input == nullptr) {
        return 0;
    }

    if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_KEY) {
        const int32_t action = AKeyEvent_getAction(input);
        if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP) {
            return 0;
        }
        exgine::MobileInputEvent event;
        event.type = action == AKEY_EVENT_ACTION_DOWN ? exgine::MobileInputType::KeyDown
                                                      : exgine::MobileInputType::KeyUp;
        event.timestamp_ns = monotonic_time_ns();
        event.key_code = AKeyEvent_getKeyCode(input);
        event.meta_state = static_cast<std::uint32_t>(AKeyEvent_getMetaState(input));
        return state->mobile.push_input(event) ? 1 : 0;
    }

    if (AInputEvent_getType(input) != AINPUT_EVENT_TYPE_MOTION ||
        (AMotionEvent_getSource(input) & AINPUT_SOURCE_CLASS_POINTER) == 0) {
        return 0;
    }

    const int32_t action = AMotionEvent_getAction(input);
    const int32_t action_type = action & AMOTION_EVENT_ACTION_MASK;
    const std::size_t action_index = static_cast<std::size_t>(
        (action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const std::size_t pointer_count = AMotionEvent_getPointerCount(input);

    if (action_type == AMOTION_EVENT_ACTION_DOWN || action_type == AMOTION_EVENT_ACTION_POINTER_DOWN) {
        if (action_index >= pointer_count) {
            return 0;
        }
        return state->mobile.push_input(make_touch_event(exgine::MobileInputType::TouchDown, input, action_index)) ? 1 : 0;
    }
    if (action_type == AMOTION_EVENT_ACTION_UP || action_type == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (action_index >= pointer_count) {
            return 0;
        }
        return state->mobile.push_input(make_touch_event(exgine::MobileInputType::TouchUp, input, action_index)) ? 1 : 0;
    }
    if (action_type == AMOTION_EVENT_ACTION_CANCEL) {
        bool accepted = false;
        for (std::size_t i = 0; i < pointer_count; ++i) {
            accepted = state->mobile.push_input(make_touch_event(exgine::MobileInputType::TouchCancel, input, i)) || accepted;
        }
        return accepted ? 1 : 0;
    }
    if (action_type == AMOTION_EVENT_ACTION_MOVE) {
        bool accepted = false;
        for (std::size_t i = 0; i < pointer_count; ++i) {
            accepted = state->mobile.push_input(make_touch_event(exgine::MobileInputType::TouchMove, input, i)) || accepted;
        }
        return accepted ? 1 : 0;
    }

    return 0;
}

void handle_cmd(android_app* app, int32_t cmd) {
    auto* state = static_cast<AppState*>(app->userData);
    if (state == nullptr) {
        return;
    }

    switch (cmd) {
    case APP_CMD_START:
        state->mobile.on_start();
        break;
    case APP_CMD_RESUME:
        state->mobile.on_resume();
        break;
    case APP_CMD_PAUSE:
        state->mobile.on_pause();
        break;
    case APP_CMD_STOP:
        state->mobile.on_stop();
        break;
    case APP_CMD_INIT_WINDOW:
        if (app->window != nullptr && state->presenter.attach(app->window)) {
            state->mobile.on_surface_available();
        }
        break;
    case APP_CMD_TERM_WINDOW:
        state->mobile.on_surface_lost();
        state->presenter.detach();
        break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONTENT_RECT_CHANGED:
        state->presenter.resize();
        break;
    default:
        break;
    }
}

exgine::RenderFrame make_clear_frame(const AppState& state) {
    exgine::RenderFrame frame;
    frame.frame_id = state.frame_id;
    frame.config.backend = exgine::RenderBackend::OpenGLES;
    frame.config.width = 1;
    frame.config.height = 1;
    frame.camera = exgine::Camera{};
    frame.view = exgine::Mat4::identity();
    frame.projection = exgine::Mat4::identity();
    frame.view_projection = exgine::Mat4::identity();
    frame.lighting = exgine::RenderLightingSettings{};
    return frame;
}

} // namespace

void android_main(android_app* app) {
    app_dummy();

    AppState state;
    app->userData = &state;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    while (true) {
        int ident = 0;
        int events = 0;
        android_poll_source* source = nullptr;

        while ((ident = ALooper_pollOnce(state.mobile.renderable() ? 0 : -1,
                                         nullptr, &events,
                                         reinterpret_cast<void**>(&source))) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                state.mobile.on_destroy();
                state.mobile.on_surface_lost();
                state.presenter.detach();
                return;
            }
        }

        if (!state.mobile.renderable() || !state.presenter.ready()) {
            continue;
        }

        const double now = static_cast<double>(monotonic_time_ns()) / 1000000000.0;
        const exgine::MobileFrameTiming timing = state.mobile.begin_frame(now);
        if (timing.state == exgine::MobileFrameState::Paused) {
            continue;
        }

        const exgine::RenderFrame frame = make_clear_frame(state);
        if (state.presenter.present(frame)) {
            ++state.frame_id;
        }

        // Game/application systems read the same MobileInputQueue; this sample simply drains it.
        while (state.mobile.poll_input().has_value()) {
        }
    }
}
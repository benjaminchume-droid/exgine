#include "exgine/android_audio.hpp"
#include "exgine/android.hpp"
#include "exgine/mobile.hpp"
#include "exgine/showcase.hpp"

#include <android/asset_manager.h>
#include <android/input.h>
#include <android/log.h>
#include <android_native_app_glue.h>

#include <cstdint>
#include <ctime>
#include <string>

namespace {
constexpr const char* kTag = "EXGINE";

std::uint64_t monotonic_time_ns() {
    timespec value{};
    if (clock_gettime(CLOCK_MONOTONIC, &value) != 0) return 0;
    return static_cast<std::uint64_t>(value.tv_sec) * 1000000000ull + static_cast<std::uint64_t>(value.tv_nsec);
}

bool load_asset(AAssetManager* manager, std::string_view path, std::string& out) {
    if (!manager) return false;
    AAsset* asset = AAssetManager_open(manager, std::string(path).c_str(), AASSET_MODE_BUFFER);
    if (!asset) return false;
    const auto size = static_cast<std::size_t>(AAsset_getLength64(asset));
    out.resize(size);
    const int read = AAsset_read(asset, out.data(), size);
    AAsset_close(asset);
    return read >= 0 && static_cast<std::size_t>(read) == size;
}

struct AppState {
    exgine::AndroidEglPresenter presenter;
    exgine::MobileRuntimeBridge mobile;
    exgine::AndroidAudioBackend audio;
    exgine::ShowcaseGame game;
    std::uint64_t frame_id = 1;
    double previous_time = 0.0;
    bool started = false;
};

exgine::MobileInputEvent make_touch_event(exgine::MobileInputType type, const AInputEvent* input, std::size_t index) {
    exgine::MobileInputEvent event;
    event.type = type;
    event.timestamp_ns = monotonic_time_ns();
    event.touch.pointer_id = AMotionEvent_getPointerId(input, index);
    event.touch.x = AMotionEvent_getX(input, index);
    event.touch.y = AMotionEvent_getY(input, index);
    event.touch.pressure = AMotionEvent_getPressure(input, index);
    return event;
}

int32_t handle_input(android_app* app, AInputEvent* input) {
    auto* state = static_cast<AppState*>(app->userData);
    if (!state || !input) return 0;
    if (AInputEvent_getType(input) == AINPUT_EVENT_TYPE_KEY) {
        const int action = AKeyEvent_getAction(input);
        if (action != AKEY_EVENT_ACTION_DOWN && action != AKEY_EVENT_ACTION_UP) return 0;
        exgine::MobileInputEvent event;
        event.type = action == AKEY_EVENT_ACTION_DOWN ? exgine::MobileInputType::KeyDown : exgine::MobileInputType::KeyUp;
        event.timestamp_ns = monotonic_time_ns();
        event.key_code = AKeyEvent_getKeyCode(input);
        event.meta_state = static_cast<std::uint32_t>(AKeyEvent_getMetaState(input));
        return state->mobile.push_input(event) ? 1 : 0;
    }
    if (AInputEvent_getType(input) != AINPUT_EVENT_TYPE_MOTION || (AMotionEvent_getSource(input) & AINPUT_SOURCE_CLASS_POINTER) == 0) return 0;
    const int32_t action = AMotionEvent_getAction(input);
    const int32_t type = action & AMOTION_EVENT_ACTION_MASK;
    const std::size_t index = static_cast<std::size_t>((action & AMOTION_EVENT_ACTION_POINTER_INDEX_MASK) >> AMOTION_EVENT_ACTION_POINTER_INDEX_SHIFT);
    const std::size_t count = AMotionEvent_getPointerCount(input);
    if (type == AMOTION_EVENT_ACTION_DOWN || type == AMOTION_EVENT_ACTION_POINTER_DOWN || type == AMOTION_EVENT_ACTION_UP || type == AMOTION_EVENT_ACTION_POINTER_UP) {
        if (index >= count) return 0;
        return state->mobile.push_input(make_touch_event(type == AMOTION_EVENT_ACTION_UP || type == AMOTION_EVENT_ACTION_POINTER_UP ? exgine::MobileInputType::TouchUp : exgine::MobileInputType::TouchDown, input, index)) ? 1 : 0;
    }
    if (type == AMOTION_EVENT_ACTION_MOVE || type == AMOTION_EVENT_ACTION_CANCEL) {
        bool accepted = false;
        const auto event_type = type == AMOTION_EVENT_ACTION_MOVE ? exgine::MobileInputType::TouchMove : exgine::MobileInputType::TouchCancel;
        for (std::size_t i = 0; i < count; ++i) accepted = state->mobile.push_input(make_touch_event(event_type, input, i)) || accepted;
        return accepted ? 1 : 0;
    }
    return 0;
}

void handle_cmd(android_app* app, int32_t cmd) {
    auto* state = static_cast<AppState*>(app->userData);
    if (!state) return;
    switch (cmd) {
    case APP_CMD_START: state->mobile.on_start(); break;
    case APP_CMD_RESUME: state->mobile.on_resume(); break;
    case APP_CMD_PAUSE: state->mobile.on_pause(); break;
    case APP_CMD_STOP: state->mobile.on_stop(); state->audio.stop(); break;
    case APP_CMD_INIT_WINDOW:
        if (app->window && state->presenter.attach(app->window)) state->mobile.on_surface_available();
        break;
    case APP_CMD_TERM_WINDOW:
        state->mobile.on_surface_lost();
        state->presenter.detach();
        break;
    case APP_CMD_WINDOW_RESIZED:
    case APP_CMD_CONTENT_RECT_CHANGED:
        (void)state->presenter.resize();
        break;
    default: break;
    }
}

} // namespace

void android_main(android_app* app) {
    app_dummy();

    AppState state;
    AAssetManager* assets = app->activity ? app->activity->assetManager : nullptr;
    state.game = exgine::ShowcaseGame([assets](std::string_view path, std::string& out) { return load_asset(assets, path, out); });
    std::string manifest;
    if (!load_asset(assets, "showcase/project.exg", manifest) || !state.game.open(manifest) || !state.audio.start()) {
        __android_log_print(ANDROID_LOG_ERROR, kTag, "showcase startup failed");
        return;
    }
    app->userData = &state;
    app->onAppCmd = handle_cmd;
    app->onInputEvent = handle_input;

    while (true) {
        int ident = 0;
        int events = 0;
        android_poll_source* source = nullptr;
        while ((ident = ALooper_pollOnce(state.mobile.renderable() ? 0 : -1, nullptr, &events, reinterpret_cast<void**>(&source))) >= 0) {
            if (source) source->process(app, source);
            if (app->destroyRequested) {
                state.mobile.on_destroy();
                state.mobile.on_surface_lost();
                state.audio.stop();
                state.presenter.detach();
                return;
            }
        }
        if (!state.mobile.renderable() || !state.presenter.ready()) continue;
        const double now = static_cast<double>(monotonic_time_ns()) / 1000000000.0;
        const double dt = state.previous_time > 0.0 ? std::min(0.05, std::max(0.0, now - state.previous_time)) : (1.0 / 60.0);
        state.previous_time = now;
        const exgine::MobileFrameTiming timing = state.mobile.begin_frame(now);
        if (timing.state == exgine::MobileFrameState::Paused) continue;
        if (!state.started) state.started = state.game.start();
        if (state.started && state.game.update(dt)) {
            const auto mix = state.game.audio().mix(static_cast<float>(dt));
            if (!mix.empty()) {
                const auto processed = exgine::SpatialAudioProcessor::process(state.game.audio().listener(), {mix.front().clip,mix.front().position,mix.front().gain,mix.front().pitch,mix.front().max_distance,mix.front().looping}, .1f);
                state.audio.set_mix(processed);
            }
            if (state.game.present(state.presenter)) ++state.frame_id;
        }
    }
}

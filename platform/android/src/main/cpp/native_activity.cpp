#include "exgine/android.hpp"
#include "exgine/render.hpp"

#include <android_native_app_glue.h>

#include <cstdint>

namespace {

struct AppState {
    exgine::AndroidEglPresenter presenter;
    std::uint64_t frame_id = 1;
};

void handle_cmd(android_app* app, int32_t cmd) {
    auto* state = static_cast<AppState*>(app->userData);
    if (state == nullptr) {
        return;
    }

    switch (cmd) {
    case APP_CMD_INIT_WINDOW:
        if (app->window != nullptr) {
            state->presenter.attach(app->window);
        }
        break;
    case APP_CMD_TERM_WINDOW:
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

    while (true) {
        int ident = 0;
        int events = 0;
        android_poll_source* source = nullptr;

        while ((ident = ALooper_pollOnce(app->destroyRequested ? 0 : 1,
                                         nullptr, &events,
                                         reinterpret_cast<void**>(&source))) >= 0) {
            if (source != nullptr) {
                source->process(app, source);
            }
            if (app->destroyRequested != 0) {
                state.presenter.detach();
                return;
            }
        }

        if (state.presenter.ready()) {
            const exgine::RenderFrame frame = make_clear_frame(state);
            state.presenter.present(frame);
            ++state.frame_id;
        }
    }
}

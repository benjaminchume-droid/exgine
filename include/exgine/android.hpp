#pragma once

#include "exgine/gpu.hpp"

#include <cstdint>
#include <string>

#if defined(__ANDROID__)
struct ANativeWindow;
#endif

namespace exgine {

#if defined(__ANDROID__)
using AndroidNativeWindow = ::ANativeWindow;
#else
using AndroidNativeWindow = void;
#endif

struct AndroidEglConfig {
    int red_bits = 8;
    int green_bits = 8;
    int blue_bits = 8;
    int alpha_bits = 8;
    int depth_bits = 24;
    int stencil_bits = 8;
    int gl_major = 3;
    int gl_minor = 1;
    int swap_interval = 1;

    [[nodiscard]] bool valid() const noexcept;
};

struct AndroidPresentationStats {
    bool attached = false;
    int width = 0;
    int height = 0;
    std::uint64_t presents = 0;

    [[nodiscard]] bool valid() const noexcept;
};

class AndroidEglPresenter {
public:
    explicit AndroidEglPresenter(AndroidEglConfig config = {}) noexcept;
    ~AndroidEglPresenter();

    AndroidEglPresenter(const AndroidEglPresenter&) = delete;
    AndroidEglPresenter& operator=(const AndroidEglPresenter&) = delete;

    [[nodiscard]] const AndroidEglConfig& config() const noexcept { return config_; }
    [[nodiscard]] const AndroidPresentationStats& stats() const noexcept { return stats_; }
    [[nodiscard]] bool ready() const noexcept;
    [[nodiscard]] const std::string& last_error() const noexcept { return last_error_; }

    bool attach(AndroidNativeWindow* window);
    bool resize();
    [[nodiscard]] bool present(const RenderFrame& frame);
    void detach() noexcept;

private:
    AndroidEglConfig config_{};
    AndroidPresentationStats stats_{};
    std::string last_error_;

#if defined(__ANDROID__)
    void* display_ = nullptr;
    void* surface_ = nullptr;
    void* context_ = nullptr;
    AndroidNativeWindow* window_ = nullptr;
    void* native_ = nullptr;
#endif
};

} // namespace exgine

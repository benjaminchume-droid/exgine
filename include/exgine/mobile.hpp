#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>

namespace exgine {

enum class MobileLifecycleState : std::uint8_t {
    Created = 0,
    Started = 1,
    Resumed = 2,
    Paused = 3,
    Stopped = 4,
    Destroyed = 5,
};

struct MobileTouchPoint {
    std::int32_t pointer_id = -1;
    float x = 0.0f;
    float y = 0.0f;
    float pressure = 0.0f;
};

enum class MobileInputType : std::uint8_t {
    TouchDown = 0,
    TouchMove = 1,
    TouchUp = 2,
    TouchCancel = 3,
    KeyDown = 4,
    KeyUp = 5,
};

struct MobileInputEvent {
    MobileInputType type = MobileInputType::TouchMove;
    std::uint64_t timestamp_ns = 0;
    MobileTouchPoint touch{};
    std::int32_t key_code = 0;
    std::uint32_t meta_state = 0;

    [[nodiscard]] bool valid() const noexcept;
};

struct MobileInputStats {
    std::size_t queued = 0;
    std::size_t dropped = 0;
    std::size_t active_touches = 0;

    [[nodiscard]] bool valid() const noexcept;
};

class MobileInputQueue {
public:
    static constexpr std::size_t capacity = 256;
    static constexpr std::size_t max_touches = 16;

    bool push(const MobileInputEvent& event) noexcept;
    [[nodiscard]] std::optional<MobileInputEvent> poll() noexcept;
    void clear() noexcept;

    [[nodiscard]] const MobileInputStats& stats() const noexcept { return stats_; }
    [[nodiscard]] bool touch_active(std::int32_t pointer_id) const noexcept;
    [[nodiscard]] std::size_t active_touch_count() const noexcept { return stats_.active_touches; }

private:
    std::array<MobileInputEvent, capacity> queue_{};
    std::size_t head_ = 0;
    std::size_t count_ = 0;
    std::array<std::int32_t, max_touches> active_pointers_{};
    std::size_t active_touch_count_ = 0;
    MobileInputStats stats_{};

    void update_touch_state(const MobileInputEvent& event) noexcept;
};

enum class MobileFrameState : std::uint8_t {
    FirstFrame = 0,
    Running = 1,
    Paused = 2,
};

struct MobileFrameTiming {
    double delta_seconds = 0.0;
    double target_seconds = 1.0 / 60.0;
    std::uint64_t frame_index = 0;
    MobileFrameState state = MobileFrameState::FirstFrame;

    [[nodiscard]] bool valid() const noexcept;
};

class MobileFramePacer {
public:
    explicit MobileFramePacer(std::uint32_t target_fps = 60) noexcept;

    [[nodiscard]] std::uint32_t target_fps() const noexcept { return target_fps_; }
    void set_target_fps(std::uint32_t fps) noexcept;
    void reset() noexcept;
    void pause() noexcept;
    void resume() noexcept;

    [[nodiscard]] MobileFrameTiming tick(double timestamp_seconds) noexcept;

private:
    std::uint32_t target_fps_ = 60;
    double last_timestamp_ = 0.0;
    bool has_timestamp_ = false;
    bool paused_ = false;
    std::uint64_t frame_index_ = 0;
};

class MobileRuntimeBridge {
public:
    MobileRuntimeBridge() = default;

    [[nodiscard]] MobileLifecycleState lifecycle() const noexcept { return lifecycle_; }
    [[nodiscard]] bool running() const noexcept;
    [[nodiscard]] bool renderable() const noexcept;

    void on_start() noexcept;
    void on_resume() noexcept;
    void on_pause() noexcept;
    void on_stop() noexcept;
    void on_destroy() noexcept;
    void on_surface_available() noexcept;
    void on_surface_lost() noexcept;

    bool push_input(const MobileInputEvent& event) noexcept { return input_.push(event); }
    [[nodiscard]] std::optional<MobileInputEvent> poll_input() noexcept { return input_.poll(); }
    [[nodiscard]] const MobileInputQueue& input() const noexcept { return input_; }

    void set_target_fps(std::uint32_t fps) noexcept { pacer_.set_target_fps(fps); }
    [[nodiscard]] MobileFrameTiming begin_frame(double timestamp_seconds) noexcept;
    void reset() noexcept;

private:
    MobileLifecycleState lifecycle_ = MobileLifecycleState::Created;
    bool surface_available_ = false;
    MobileInputQueue input_{};
    MobileFramePacer pacer_{};
};

} // namespace exgine

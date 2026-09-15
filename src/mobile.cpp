#include "exgine/mobile.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {

namespace {
constexpr double max_delta_seconds = 0.25;
constexpr std::uint32_t min_fps = 15;
constexpr std::uint32_t max_fps = 120;
}

bool MobileInputEvent::valid() const noexcept {
    switch (type) {
    case MobileInputType::TouchDown:
    case MobileInputType::TouchMove:
    case MobileInputType::TouchUp:
    case MobileInputType::TouchCancel:
        return touch.pointer_id >= 0 && std::isfinite(touch.x) && std::isfinite(touch.y) &&
               std::isfinite(touch.pressure) && touch.pressure >= 0.0f;
    case MobileInputType::KeyDown:
    case MobileInputType::KeyUp:
        return key_code >= 0;
    }
    return false;
}

bool MobileInputStats::valid() const noexcept {
    return queued <= MobileInputQueue::capacity && active_touches <= MobileInputQueue::max_touches;
}

bool MobileInputQueue::push(const MobileInputEvent& event) noexcept {
    if (!event.valid()) {
        return false;
    }

    if (count_ == capacity) {
        head_ = (head_ + 1) % capacity;
        --count_;
        ++stats_.dropped;
    }

    const std::size_t tail = (head_ + count_) % capacity;
    queue_[tail] = event;
    ++count_;
    stats_.queued = count_;
    update_touch_state(event);
    return true;
}

std::optional<MobileInputEvent> MobileInputQueue::poll() noexcept {
    if (count_ == 0) {
        return std::nullopt;
    }
    const MobileInputEvent event = queue_[head_];
    head_ = (head_ + 1) % capacity;
    --count_;
    stats_.queued = count_;
    return event;
}

void MobileInputQueue::clear() noexcept {
    head_ = 0;
    count_ = 0;
    active_touch_count_ = 0;
    active_pointers_.fill(-1);
    stats_.queued = 0;
    stats_.active_touches = 0;
}

bool MobileInputQueue::touch_active(const std::int32_t pointer_id) const noexcept {
    const auto begin = active_pointers_.begin();
    const auto end = begin + static_cast<std::ptrdiff_t>(active_touch_count_);
    return std::find(begin, end, pointer_id) != end;
}

void MobileInputQueue::update_touch_state(const MobileInputEvent& event) noexcept {
    const bool is_touch = event.type == MobileInputType::TouchDown || event.type == MobileInputType::TouchMove ||
                          event.type == MobileInputType::TouchUp || event.type == MobileInputType::TouchCancel;
    if (!is_touch) {
        return;
    }

    const std::int32_t pointer_id = event.touch.pointer_id;
    const auto begin = active_pointers_.begin();
    const auto end = begin + static_cast<std::ptrdiff_t>(active_touch_count_);
    auto it = std::find(begin, end, pointer_id);

    if (event.type == MobileInputType::TouchDown) {
        if (it == end && active_touch_count_ < max_touches) {
            active_pointers_[active_touch_count_] = pointer_id;
            ++active_touch_count_;
        }
    } else if (event.type == MobileInputType::TouchUp || event.type == MobileInputType::TouchCancel) {
        if (it != end) {
            const std::size_t index = static_cast<std::size_t>(std::distance(begin, it));
            active_pointers_[index] = active_pointers_[active_touch_count_ - 1];
            active_pointers_[active_touch_count_ - 1] = -1;
            --active_touch_count_;
        }
    }
    stats_.active_touches = active_touch_count_;
}

bool MobileFrameTiming::valid() const noexcept {
    return std::isfinite(delta_seconds) && delta_seconds >= 0.0 &&
           std::isfinite(target_seconds) && target_seconds > 0.0;
}

MobileFramePacer::MobileFramePacer(std::uint32_t target_fps) noexcept {
    set_target_fps(target_fps);
}

void MobileFramePacer::set_target_fps(std::uint32_t fps) noexcept {
    target_fps_ = std::clamp(fps, min_fps, max_fps);
}

void MobileFramePacer::reset() noexcept {
    last_timestamp_ = 0.0;
    has_timestamp_ = false;
    paused_ = false;
    frame_index_ = 0;
}

void MobileFramePacer::pause() noexcept {
    paused_ = true;
    has_timestamp_ = false;
}

void MobileFramePacer::resume() noexcept {
    paused_ = false;
    has_timestamp_ = false;
}

MobileFrameTiming MobileFramePacer::tick(double timestamp_seconds) noexcept {
    MobileFrameTiming timing;
    timing.target_seconds = 1.0 / static_cast<double>(target_fps_);
    timing.frame_index = frame_index_;
    timing.state = paused_ ? MobileFrameState::Paused
                           : (!has_timestamp_ ? MobileFrameState::FirstFrame : MobileFrameState::Running);

    if (!std::isfinite(timestamp_seconds) || paused_) {
        return timing;
    }

    if (!has_timestamp_) {
        last_timestamp_ = timestamp_seconds;
        has_timestamp_ = true;
        return timing;
    }

    const double raw_delta = timestamp_seconds - last_timestamp_;
    last_timestamp_ = timestamp_seconds;
    if (std::isfinite(raw_delta)) {
        timing.delta_seconds = std::clamp(raw_delta, 0.0, max_delta_seconds);
    }
    timing.frame_index = ++frame_index_;
    timing.state = MobileFrameState::Running;
    return timing;
}

bool MobileRuntimeBridge::running() const noexcept {
    return lifecycle_ == MobileLifecycleState::Resumed;
}

bool MobileRuntimeBridge::renderable() const noexcept {
    return running() && surface_available_;
}

void MobileRuntimeBridge::on_start() noexcept {
    if (lifecycle_ != MobileLifecycleState::Destroyed) {
        lifecycle_ = MobileLifecycleState::Started;
    }
}

void MobileRuntimeBridge::on_resume() noexcept {
    if (lifecycle_ != MobileLifecycleState::Destroyed) {
        lifecycle_ = MobileLifecycleState::Resumed;
        pacer_.resume();
    }
}

void MobileRuntimeBridge::on_pause() noexcept {
    if (lifecycle_ == MobileLifecycleState::Resumed) {
        lifecycle_ = MobileLifecycleState::Paused;
        pacer_.pause();
    }
}

void MobileRuntimeBridge::on_stop() noexcept {
    if (lifecycle_ != MobileLifecycleState::Destroyed) {
        lifecycle_ = MobileLifecycleState::Stopped;
        pacer_.pause();
    }
}

void MobileRuntimeBridge::on_destroy() noexcept {
    lifecycle_ = MobileLifecycleState::Destroyed;
    surface_available_ = false;
    input_.clear();
    pacer_.pause();
}

void MobileRuntimeBridge::on_surface_available() noexcept {
    if (lifecycle_ != MobileLifecycleState::Destroyed) {
        surface_available_ = true;
        if (running()) {
            pacer_.resume();
        }
    }
}

void MobileRuntimeBridge::on_surface_lost() noexcept {
    surface_available_ = false;
    pacer_.pause();
}

MobileFrameTiming MobileRuntimeBridge::begin_frame(double timestamp_seconds) noexcept {
    if (!renderable()) {
        MobileFrameTiming timing = pacer_.tick(timestamp_seconds);
        timing.state = MobileFrameState::Paused;
        timing.delta_seconds = 0.0;
        return timing;
    }
    return pacer_.tick(timestamp_seconds);
}

void MobileRuntimeBridge::reset() noexcept {
    lifecycle_ = MobileLifecycleState::Created;
    surface_available_ = false;
    input_.clear();
    pacer_.reset();
}

} // namespace exgine

#pragma once

#include "exgine/nextgen.hpp"
#include <cstdint>

namespace exgine {

class AndroidAudioBackend {
public:
    AndroidAudioBackend() = default;
    ~AndroidAudioBackend();
    AndroidAudioBackend(const AndroidAudioBackend&) = delete;
    AndroidAudioBackend& operator=(const AndroidAudioBackend&) = delete;

    [[nodiscard]] bool start(std::uint32_t sample_rate = 48000, std::uint32_t channels = 2) noexcept;
    void stop() noexcept;
    void set_mix(AudioMixResult mix) noexcept;
    [[nodiscard]] bool running() const noexcept { return running_; }

private:
#if defined(__ANDROID__)
    struct AAudioStream* stream_ = nullptr;
#endif
    float gain_ = 0.0f;
    float pan_ = 0.0f;
    float phase_ = 0.0f;
    bool running_ = false;
};

} // namespace exgine

#pragma once
#include "exgine/exsound.hpp"
#include "exgine/nextgen.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#if defined(__ANDROID__)
#include <aaudio/AAudio.h>
#endif
namespace exgine {
class AndroidAudioBackend {
public:
    AndroidAudioBackend()=default; ~AndroidAudioBackend();
    AndroidAudioBackend(const AndroidAudioBackend&)=delete; AndroidAudioBackend& operator=(const AndroidAudioBackend&)=delete;
    [[nodiscard]] bool start(std::uint32_t sample_rate=48000,std::uint32_t channels=2) noexcept;
    void stop() noexcept; void set_mix(AudioMixResult mix) noexcept;
    [[nodiscard]] bool submit(const SoundSample&) noexcept; void clear() noexcept;
    [[nodiscard]] bool running() const noexcept{return running_;}
private:
    static constexpr std::size_t ring_samples=262144;
    void* stream_=nullptr; std::array<float,ring_samples> ring_{};
    std::atomic<std::size_t> read_{0},write_{0}; std::uint32_t sample_rate_=48000,channels_=2;
    std::atomic<float> gain_{1},pan_{0}; bool running_=false;
#if defined(__ANDROID__)
    static aaudio_data_callback_result_t audio_callback(AAudioStream*,void*,void*,int32_t) noexcept;
#endif
};
} // namespace exgine

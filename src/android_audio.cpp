#include "exgine/android_audio.hpp"

#include <algorithm>
#include <atomic>
#include <cmath>

#if defined(__ANDROID__)
#include <aaudio/AAudio.h>
#endif

namespace exgine {
namespace {
#if defined(__ANDROID__)
std::atomic<float> g_gain{0.0f};
std::atomic<float> g_pan{0.0f};
std::atomic<float> g_phase{0.0f};

aaudio_data_callback_result_t audio_callback(AAudioStream*, void*, void* audio_data, int32_t num_frames) {
    auto* samples = static_cast<float*>(audio_data);
    const float gain = g_gain.load(std::memory_order_relaxed);
    const float pan = std::clamp(g_pan.load(std::memory_order_relaxed), -1.0f, 1.0f);
    float phase = g_phase.load(std::memory_order_relaxed);
    constexpr float sample_rate = 48000.0f;
    constexpr float frequency = 196.0f;
    constexpr float two_pi = 6.2831853071795864769f;
    const float left = gain * (0.5f - 0.25f * pan);
    const float right = gain * (0.5f + 0.25f * pan);
    for (int32_t i = 0; i < num_frames; ++i) {
        const float tone = std::sin(phase) * 0.035f;
        const float rain = std::sin(phase * 7.31f) * 0.008f;
        samples[i * 2] = (tone + rain) * left;
        samples[i * 2 + 1] = (tone - rain) * right;
        phase += two_pi * frequency / sample_rate;
        if (phase >= two_pi) phase -= two_pi;
    }
    g_phase.store(phase, std::memory_order_relaxed);
    return AAUDIO_CALLBACK_RESULT_CONTINUE;
}
#endif
} // namespace

AndroidAudioBackend::~AndroidAudioBackend() { stop(); }

bool AndroidAudioBackend::start(std::uint32_t sample_rate, std::uint32_t channels) noexcept {
#if defined(__ANDROID__)
    if (running_) return true;
    if (sample_rate == 0 || channels != 2) return false;
    AAudioStreamBuilder* builder = nullptr;
    if (AAudio_createStreamBuilder(&builder) != AAUDIO_OK || !builder) return false;
    AAudioStreamBuilder_setDirection(builder, AAUDIO_DIRECTION_OUTPUT);
    AAudioStreamBuilder_setSampleRate(builder, static_cast<int32_t>(sample_rate));
    AAudioStreamBuilder_setChannelCount(builder, static_cast<int32_t>(channels));
    AAudioStreamBuilder_setFormat(builder, AAUDIO_FORMAT_PCM_FLOAT);
    AAudioStreamBuilder_setPerformanceMode(builder, AAUDIO_PERFORMANCE_MODE_LOW_LATENCY);
    AAudioStreamBuilder_setSharingMode(builder, AAUDIO_SHARING_MODE_SHARED);
    AAudioStreamBuilder_setDataCallback(builder, audio_callback, this);
    AAudioStream* stream = nullptr;
    const auto result = AAudioStreamBuilder_openStream(builder, &stream);
    AAudioStreamBuilder_delete(builder);
    if (result != AAUDIO_OK || !stream) return false;
    if (AAudioStream_requestStart(stream) != AAUDIO_OK) {
        AAudioStream_close(stream);
        return false;
    }
    stream_ = stream;
    running_ = true;
    return true;
#else
    (void)sample_rate;
    (void)channels;
    running_ = false;
    return false;
#endif
}

void AndroidAudioBackend::stop() noexcept {
#if defined(__ANDROID__)
    if (stream_) {
        (void)AAudioStream_requestStop(stream_);
        (void)AAudioStream_close(stream_);
        stream_ = nullptr;
    }
#endif
    running_ = false;
    gain_ = 0.0f;
}

void AndroidAudioBackend::set_mix(AudioMixResult mix) noexcept {
    gain_ = std::max(0.0f, mix.gain);
    pan_ = std::clamp(mix.pan, -1.0f, 1.0f);
#if defined(__ANDROID__)
    g_gain.store(gain_, std::memory_order_relaxed);
    g_pan.store(pan_, std::memory_order_relaxed);
#endif
}

} // namespace exgine

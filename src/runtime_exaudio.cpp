#include "exgine/runtime.hpp"
#include "exgine/android_audio.hpp"
#include <utility>
namespace exgine {
Runtime::Runtime():audio_(std::make_unique<AndroidAudioBackend>()){}
Runtime::~Runtime()=default;
Runtime::Runtime(Runtime&&)=default;
Runtime& Runtime::operator=(Runtime&&)=default;
AndroidAudioBackend& Runtime::audio_backend() noexcept { return *audio_; }
SoundSample Runtime::generate_sound(const SoundEventParams& p) const { return sound_.synthesize_event(p); }
SoundSample Runtime::generate_sound(const SoundRecipe& r) const { return sound_.synthesize_recipe(r); }
SoundSample Runtime::generate_speech(const SpeechRequest& r) const { return sound_.synthesize_speech(r); }
bool Runtime::play_sound(const SoundSample& s) noexcept { return audio_ && audio_->submit(s); }
bool Runtime::start_audio(std::uint32_t sample_rate) noexcept { if(audio_ && audio_->start(sample_rate,2)){sound_=ExSound(sample_rate);return true;}return false; }
void Runtime::stop_audio() noexcept { if(audio_) audio_->stop(); }
void Runtime::set_audio_mix(AudioMixResult m) noexcept { if(audio_) audio_->set_mix(m); }
} // namespace exgine

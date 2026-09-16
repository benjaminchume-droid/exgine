#include "exgine/runtime.hpp"
namespace exgine {
SoundSample Runtime::generate_sound(const SoundEventParams& p) const { return sound_.synthesize_event(p); }
SoundSample Runtime::generate_sound(const SoundRecipe& r) const { return sound_.synthesize_recipe(r); }
SoundSample Runtime::generate_speech(const SpeechRequest& r) const { return sound_.synthesize_speech(r); }
bool Runtime::play_sound(const SoundSample& s) noexcept { return audio_.submit(s); }
bool Runtime::start_audio(std::uint32_t sample_rate) noexcept { if(audio_.start(sample_rate,2)){sound_=ExSound(sample_rate);return true;}return false; }
void Runtime::stop_audio() noexcept { audio_.stop(); }
void Runtime::set_audio_mix(AudioMixResult m) noexcept { audio_.set_mix(m); }
} // namespace exgine

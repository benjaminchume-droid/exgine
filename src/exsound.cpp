#include "exgine/exsound.hpp"
#include <cmath>
#include <limits>

namespace exgine {
namespace {
constexpr float pi = 3.14159265358979323846f;
float clamp01(float x) { return std::clamp(x, 0.0f, 1.0f); }
float hash_noise(std::uint32_t& state) noexcept {
    state ^= state << 13; state ^= state >> 17; state ^= state << 5;
    return (static_cast<float>(state & 0x00ffffffu) / 8388607.5f) - 1.0f;
}
float osc(SoundWave w, float phase, std::uint32_t& state) noexcept {
    const float p = phase - std::floor(phase);
    switch (w) {
        case SoundWave::Sine: return std::sin(2.0f * pi * p);
        case SoundWave::Triangle: return 4.0f * std::fabs(p - 0.5f) - 1.0f;
        case SoundWave::Saw: return 2.0f * p - 1.0f;
        case SoundWave::Square: return p < 0.5f ? 1.0f : -1.0f;
        case SoundWave::Impulse: return p < 0.01f ? 1.0f : 0.0f;
        case SoundWave::Noise: return hash_noise(state);
    }
    return 0.0f;
}
float envelope(float t, float duration, const SoundEnvelope& e) noexcept {
    const float a = std::max(0.0001f, e.attack);
    const float d = std::max(0.0001f, e.decay);
    const float r = std::max(0.0001f, e.release);
    if (t < a) return t / a;
    if (t < a + d) return 1.0f - (1.0f - clamp01(e.sustain)) * ((t - a) / d);
    const float release_start = std::max(a + d, duration - r);
    if (t >= release_start) return clamp01(e.sustain) * std::max(0.0f, (duration - t) / r);
    return clamp01(e.sustain);
}
float material_freq(SoundMaterial m) noexcept {
    switch (m) { case SoundMaterial::Metal: return 1600; case SoundMaterial::Glass: return 2600; case SoundMaterial::Stone: return 700; case SoundMaterial::Wood: return 420; case SoundMaterial::Cloth: return 180; case SoundMaterial::Grass: return 250; case SoundMaterial::Water: return 900; case SoundMaterial::Flesh: return 300; default: return 600; }
}
}

bool ProceduralSound::valid() const noexcept {
    return std::isfinite(frequency) && std::isfinite(frequency_end) && frequency >= 0 && frequency_end >= 0 &&
           std::isfinite(duration) && duration > 0 && std::isfinite(gain) && std::isfinite(pan) &&
           std::isfinite(noise) && std::isfinite(resonance) && std::isfinite(distortion) && std::isfinite(reverb);
}
bool SoundSample::valid() const noexcept { return sample_rate > 0 && channels > 0 && !pcm.empty() && pcm.size() % channels == 0; }

SoundSample ExSound::synthesize(const ProceduralSound& s) const {
    SoundSample out; out.sample_rate = sample_rate_; out.channels = 2;
    if (!s.valid()) return out;
    const auto frames = static_cast<std::size_t>(std::ceil(s.duration * sample_rate_));
    out.pcm.resize(frames * 2);
    std::uint32_t state = s.seed ? s.seed : 1u;
    float phase = 0.0f, last = 0.0f;
    const float pan = std::clamp(s.pan, -1.0f, 1.0f);
    const float left = std::sqrt(0.5f * (1.0f - pan));
    const float right = std::sqrt(0.5f * (1.0f + pan));
    for (std::size_t i = 0; i < frames; ++i) {
        const float t = static_cast<float>(i) / static_cast<float>(sample_rate_);
        const float u = s.duration > 0 ? clamp01(t / s.duration) : 0;
        const float f = s.frequency + (s.frequency_end - s.frequency) * u;
        phase += f / static_cast<float>(sample_rate_);
        const float base = osc(s.wave, phase, state);
        const float n = hash_noise(state);
        float x = base * (1.0f - clamp01(s.noise)) + n * clamp01(s.noise);
        if (s.resonance > 0) { const float a = std::exp(-s.resonance * 12.0f / sample_rate_); last = last * a + x * (1.0f - a); x = last; }
        if (s.distortion > 0) { const float d = std::max(0.0f, s.distortion); x = std::tanh(x * (1.0f + 12.0f * d)); }
        x *= envelope(t, s.duration, s.envelope) * std::max(0.0f, s.gain);
        const float room = clamp01(s.reverb);
        const float echo = i > static_cast<std::size_t>(sample_rate_ * 0.035f) ? out.pcm[(i - static_cast<std::size_t>(sample_rate_ * 0.035f)) * 2] : 0.0f;
        x += echo * room * 0.18f;
        out.pcm[i * 2] = std::clamp(x * left, -1.0f, 1.0f);
        out.pcm[i * 2 + 1] = std::clamp(x * right, -1.0f, 1.0f);
    }
    return out;
}

ProceduralSound ExSound::event(const SoundEventParams& p) const {
    const float i = clamp01(p.intensity);
    ProceduralSound s; s.seed = p.seed; s.duration = 0.18f + i * 1.15f; s.gain = 0.25f + i * 0.6f;
    switch (p.event) {
        case SoundEvent::Footstep: s.wave = SoundWave::Noise; s.frequency = 120; s.frequency_end = 55; s.duration = 0.10f + i * 0.08f; s.noise = .72f; s.resonance = .25f; break;
        case SoundEvent::Impact: case SoundEvent::Collision: s.wave = SoundWave::Impulse; s.frequency = material_freq(p.material); s.frequency_end = s.frequency * .35f; s.noise = .35f; s.resonance = .8f; s.duration = .12f + i * .4f; break;
        case SoundEvent::Explosion: s.wave = SoundWave::Noise; s.frequency = 90; s.frequency_end = 28; s.noise = .9f; s.resonance = .35f; s.distortion = .35f; s.duration = .45f + i * 1.5f; break;
        case SoundEvent::Weapon: s.wave = SoundWave::Noise; s.frequency = 180; s.frequency_end = 65; s.noise = .75f; s.distortion = .22f; s.duration = .12f + i * .25f; break;
        case SoundEvent::Vehicle: s.wave = SoundWave::Saw; s.frequency = 35 + p.speed * 2.0f; s.frequency_end = s.frequency * 1.15f; s.noise = .12f; s.distortion = .08f; s.duration = .08f; break;
        case SoundEvent::UI: s.wave = SoundWave::Sine; s.frequency = 700; s.frequency_end = 1000; s.duration = .09f; s.gain = .22f; break;
        case SoundEvent::Ambient: s.wave = SoundWave::Noise; s.frequency = 100; s.frequency_end = 80; s.noise = 1.0f; s.duration = 1.0f; s.gain = .08f; break;
    }
    const float attenuation = 1.0f / (1.0f + std::max(0.0f, p.distance) * 0.04f);
    s.gain *= attenuation; s.reverb = clamp01(p.room); return s;
}
SoundSample ExSound::synthesize_event(const SoundEventParams& p) const { return synthesize(event(p)); }

ProceduralSound footstep(SoundMaterial m, float force, std::uint32_t seed) { return ExSound{}.event({SoundEvent::Footstep,m,clamp01(force),0,1,0,0,seed}); }
ProceduralSound impact(SoundMaterial m, float force, std::uint32_t seed) { return ExSound{}.event({SoundEvent::Impact,m,clamp01(force),0,1,0,0,seed}); }
ProceduralSound explosion(float intensity, std::uint32_t seed) { return ExSound{}.event({SoundEvent::Explosion,SoundMaterial::Generic,clamp01(intensity),0,1,0,0,seed}); }
ProceduralSound ui_click(std::uint32_t seed) { return ExSound{}.event({SoundEvent::UI,SoundMaterial::Generic,1,0,1,0,0,seed}); }
ProceduralSound vehicle_engine(float rpm, float load, std::uint32_t seed) { return ExSound{}.event({SoundEvent::Vehicle,SoundMaterial::Metal,clamp01(load),rpm,1,0,0,seed}); }

} // namespace exgine

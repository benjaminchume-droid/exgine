#pragma once
#include <algorithm>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

// ExSound is Exgine's procedural audio layer. It describes sounds as
// parameters/events and synthesizes PCM without requiring prerecorded assets.
enum class SoundWave : std::uint8_t { Sine, Triangle, Saw, Square, Noise, Impulse };
enum class SoundMaterial : std::uint8_t { Generic, Wood, Stone, Metal, Glass, Cloth, Grass, Water, Flesh };
enum class SoundEvent : std::uint8_t { UI, Footstep, Impact, Collision, Explosion, Weapon, Vehicle, Ambient };

struct SoundEnvelope {
    float attack = 0.005f;
    float decay = 0.08f;
    float sustain = 0.65f;
    float release = 0.12f;
};

struct ProceduralSound {
    std::string name;
    SoundWave wave = SoundWave::Noise;
    float frequency = 220.0f;
    float frequency_end = 110.0f;
    float duration = 0.25f;
    float gain = 0.5f;
    float pan = 0.0f;
    float noise = 0.0f;
    float resonance = 0.0f;
    float distortion = 0.0f;
    float reverb = 0.0f;
    SoundEnvelope envelope{};
    std::uint32_t seed = 1;

    [[nodiscard]] bool valid() const noexcept;
};

struct SoundSample {
    std::uint32_t sample_rate = 48000;
    std::uint32_t channels = 2;
    std::vector<float> pcm;

    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] std::size_t frames() const noexcept { return channels ? pcm.size() / channels : 0; }
};

struct SoundEventParams {
    SoundEvent event = SoundEvent::Impact;
    SoundMaterial material = SoundMaterial::Generic;
    float intensity = 1.0f;
    float speed = 0.0f;
    float mass = 1.0f;
    float distance = 0.0f;
    float room = 0.0f;
    std::uint32_t seed = 1;
};

class ExSound {
public:
    explicit ExSound(std::uint32_t sample_rate = 48000) noexcept : sample_rate_(sample_rate ? sample_rate : 48000) {}

    [[nodiscard]] SoundSample synthesize(const ProceduralSound&) const;
    [[nodiscard]] ProceduralSound event(const SoundEventParams&) const;
    [[nodiscard]] SoundSample synthesize_event(const SoundEventParams&) const;

    [[nodiscard]] std::uint32_t sample_rate() const noexcept { return sample_rate_; }

private:
    std::uint32_t sample_rate_;
};

// Convenience constructors intended for gameplay code and generated content.
[[nodiscard]] ProceduralSound footstep(SoundMaterial, float force = 1.0f, std::uint32_t seed = 1);
[[nodiscard]] ProceduralSound impact(SoundMaterial, float force = 1.0f, std::uint32_t seed = 1);
[[nodiscard]] ProceduralSound explosion(float intensity = 1.0f, std::uint32_t seed = 1);
[[nodiscard]] ProceduralSound ui_click(std::uint32_t seed = 1);
[[nodiscard]] ProceduralSound vehicle_engine(float rpm, float load = 0.7f, std::uint32_t seed = 1);

} // namespace exgine

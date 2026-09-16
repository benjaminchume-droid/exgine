#pragma once
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <array>

namespace exgine {

enum class SoundNodeType : std::uint8_t { Constant, Oscillator, Noise, Impulse, Envelope, Gain, Add, Multiply, Mix, Filter, Resonator, Delay, Reverb, Distortion, Pan, Wavetable, FM, Formant, Granular, Sequencer };
enum class SoundWave : std::uint8_t { Sine, Triangle, Saw, Square, Noise, Impulse, FM, Pluck, Formant };
enum class SoundFilter : std::uint8_t { LowPass, HighPass, BandPass };
enum class SoundMaterial : std::uint8_t { Generic, Wood, Stone, Metal, Glass, Cloth, Grass, Water, Flesh, Ceramic, Rubber, Plastic, Earth, Sand, Ice, Concrete };
enum class SoundEvent : std::uint8_t { UI, Footstep, Impact, Collision, Explosion, Weapon, Vehicle, Ambient, Door, Pickup, Jump, Land, Melee, Projectile, Weather, Creature, Machine, Destruction };
enum class VoiceGender : std::uint8_t { Neutral, Feminine, Masculine };
enum class VoiceQuality : std::uint8_t { Synthetic, Soft, Natural, Whisper, Robotic, Harsh };

struct SoundEnvelope { float attack=.005f, decay=.08f, sustain=.65f, release=.12f; };
struct SoundLayer { SoundWave wave=SoundWave::Noise; float frequency=220, frequency_end=110, gain=.5f, detune=0, noise=0, resonance=0, distortion=0; SoundEnvelope envelope{}; };

struct SoundNode {
    std::uint32_t id=0;
    SoundNodeType type=SoundNodeType::Constant;
    std::vector<std::uint32_t> inputs;
    std::array<float,8> p{};
    std::uint32_t seed=1;
};

struct SoundGraph {
    std::string name;
    std::vector<SoundNode> nodes;
    std::uint32_t output=0;
    float duration=.25f;
    float gain=1.0f;
    float pan=0.0f;
    std::uint32_t seed=1;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] SoundNode* node(std::uint32_t id) noexcept;
    [[nodiscard]] const SoundNode* node(std::uint32_t id) const noexcept;
    std::uint32_t add(SoundNode node);
};

using SoundRecipe = SoundGraph;
struct ProceduralSound {
    std::string name;
    SoundWave wave=SoundWave::Noise;
    float frequency=220, frequency_end=110, duration=.25f, gain=.5f, pan=0, noise=0, resonance=0, distortion=0, reverb=0;
    SoundEnvelope envelope{};
    std::uint32_t seed=1;
    std::vector<SoundLayer> layers;
    [[nodiscard]] bool valid() const noexcept;
    [[nodiscard]] SoundGraph graph() const;
};
struct SoundSample { std::uint32_t sample_rate=48000, channels=2; std::vector<float> pcm; [[nodiscard]] bool valid() const noexcept; [[nodiscard]] std::size_t frames() const noexcept{return channels?pcm.size()/channels:0;} };
struct SoundEventParams { SoundEvent event=SoundEvent::Impact; SoundMaterial material=SoundMaterial::Generic; float intensity=1,speed=0,mass=1,distance=0,room=0,variation=0; std::uint32_t seed=1; };
struct VoiceProfile { VoiceGender gender=VoiceGender::Neutral; VoiceQuality quality=VoiceQuality::Natural; float pitch=1,speed=1,breath=.08f,formant_shift=1,emphasis=0; std::uint32_t seed=1; };
struct SpeechRequest { std::string text; VoiceProfile voice{}; float gain=.65f,pan=0,room=0; };

class ExSound {
public:
    explicit ExSound(std::uint32_t sample_rate=48000) noexcept:sample_rate_(sample_rate?sample_rate:48000){}
    [[nodiscard]] SoundSample render(const SoundGraph&) const;
    [[nodiscard]] SoundSample synthesize(const ProceduralSound&) const;
    [[nodiscard]] SoundGraph event(const SoundEventParams&) const;
    [[nodiscard]] SoundSample synthesize_event(const SoundEventParams&) const;
    [[nodiscard]] SoundSample synthesize_recipe(const SoundRecipe&) const;
    [[nodiscard]] SoundSample synthesize_speech(const SpeechRequest&) const;
    [[nodiscard]] std::uint32_t sample_rate() const noexcept{return sample_rate_;}
private: std::uint32_t sample_rate_;
};

[[nodiscard]] SoundGraph make_sound_graph(std::string_view name, std::uint32_t seed=1);
[[nodiscard]] SoundGraph sound_catalog(std::uint32_t id);
[[nodiscard]] SoundRecipe catalog(std::uint32_t id);
[[nodiscard]] ProceduralSound footstep(SoundMaterial,float=1,std::uint32_t=1);
[[nodiscard]] ProceduralSound impact(SoundMaterial,float=1,std::uint32_t=1);
[[nodiscard]] ProceduralSound explosion(float=1,std::uint32_t=1);
[[nodiscard]] ProceduralSound ui_click(std::uint32_t=1);
[[nodiscard]] ProceduralSound vehicle_engine(float,float=.7f,std::uint32_t=1);
[[nodiscard]] SpeechRequest speech(std::string_view,const VoiceProfile& voice={});

} // namespace exgine

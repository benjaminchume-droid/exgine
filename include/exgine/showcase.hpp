#pragma once

#include "exgine/android.hpp"
#include "exgine/asset_runtime.hpp"
#include "exgine/playable.hpp"
#include "exgine/nextgen.hpp"

#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

struct ShowcaseMetrics {
    std::uint64_t frames = 0;
    std::uint64_t rendered_frames = 0;
    std::uint64_t physics_steps = 0;
    std::uint64_t stream_results = 0;
    std::uint64_t weather_particles = 0;
    std::uint64_t audio_sources = 0;
    std::uint64_t ui_widgets = 0;
    std::uint64_t draw_calls = 0;
    std::uint64_t visible_draws = 0;
    double total_frame_ms = 0.0;
    double max_frame_ms = 0.0;
    [[nodiscard]] double average_frame_ms() const noexcept { return frames ? total_frame_ms / static_cast<double>(frames) : 0.0; }
    [[nodiscard]] bool valid() const noexcept { return frames > 0 && rendered_frames > 0 && average_frame_ms() >= 0.0 && max_frame_ms() >= average_frame_ms(); }
};

class ShowcaseGame {
public:
    using FileLoader = ProjectSourceLoader;

    explicit ShowcaseGame(FileLoader loader = {});

    [[nodiscard]] bool open(std::string_view project_manifest);
    [[nodiscard]] bool start() noexcept;
    [[nodiscard]] bool update(double dt) noexcept;
    [[nodiscard]] bool build_frame(RenderFrame& frame, RenderResult& result) noexcept;
    [[nodiscard]] bool present(AndroidEglPresenter& presenter) noexcept;
    [[nodiscard]] std::vector<std::uint8_t> save();
    [[nodiscard]] bool restore(const std::vector<std::uint8_t>& bytes) noexcept;
    [[nodiscard]] const ShowcaseMetrics& metrics() const noexcept { return metrics_; }
    [[nodiscard]] const PlayableGame& game() const noexcept { return game_; }
    [[nodiscard]] PlayableGame& game() noexcept { return game_; }
    [[nodiscard]] const WeatherVisualFrame& weather() const noexcept { return weather_.frame(); }
    [[nodiscard]] const UiWorld& ui() const noexcept { return ui_; }
    [[nodiscard]] const AudioWorld& audio() const noexcept { return audio_; }
    [[nodiscard]] bool ready() const noexcept { return ready_; }

private:
    FileLoader loader_{};
    PlayableGame game_{};
    AssetRuntime assets_{};
    ImageDecoderRegistry images_{};
    ParticleWorld particles_{};
    WeatherVisualController weather_{};
    AudioFrameRuntime audio_runtime_{};
    UiInteractionRouter ui_router_{};
    AudioWorld audio_{};
    UiWorld ui_{};
    std::uint64_t rain_emitter_ = 0;
    EntityId rain_entity_ = invalid_entity;
    EntityId prop_entity_ = invalid_entity;
    PhysicsBodyId physics_body_ = invalid_physics_body;
    std::uint64_t crosshair_widget_ = 0;
    std::uint64_t health_widget_ = 0;
    ShowcaseMetrics metrics_{};
    bool ready_ = false;
    bool configured_ = false;

    [[nodiscard]] bool configure_world();
    [[nodiscard]] bool load_authored_asset();
    [[nodiscard]] bool create_physics_probe();
    [[nodiscard]] bool configure_ui();
    void update_weather_mesh() noexcept;
    void update_physics_state(float dt) noexcept;
};

} // namespace exgine

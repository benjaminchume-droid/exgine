#pragma once

#include <cmath>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace exgine {

enum class WeatherType : std::uint8_t { Clear, Cloudy, Rain, Storm, Snow, Fog, Wind, Custom };

enum class DayPhase : std::uint8_t { Night, Dawn, Morning, Afternoon, Dusk, Evening };

struct SeasonDefinition {
    std::string name;
    std::uint32_t start_day = 0;
    std::uint32_t length_days = 90;
    float temperature_bias = 0.0f;
    float vegetation_factor = 1.0f;
    float daylight_factor = 1.0f;
    [[nodiscard]] bool valid() const noexcept {
        return !name.empty() && length_days > 0 && std::isfinite(temperature_bias) &&
               std::isfinite(vegetation_factor) && vegetation_factor >= 0.0f &&
               std::isfinite(daylight_factor) && daylight_factor >= 0.0f;
    }
};

struct CalendarConfig {
    double seconds_per_day = 1200.0;
    std::uint32_t days_per_year = 360;
    std::uint32_t days_per_week = 7;
    std::uint32_t first_day_of_year = 0;
    float dawn_start = 5.0f;
    float dawn_end = 7.0f;
    float dusk_start = 18.0f;
    float dusk_end = 20.0f;
    std::vector<SeasonDefinition> seasons;
    [[nodiscard]] bool valid() const noexcept;
};

struct EnvironmentState {
    double elapsed_seconds = 0.0;
    std::uint64_t absolute_day = 0;
    float day_fraction = 0.0f;
    float hour = 0.0f;
    std::uint32_t day_of_year = 0;
    std::uint32_t day_of_week = 0;
    std::size_t season_index = 0;
    DayPhase day_phase = DayPhase::Night;
    WeatherType weather = WeatherType::Clear;
    float weather_intensity = 0.0f;
    float sun_elevation = -1.0f;
    float sun_azimuth = 0.0f;
    [[nodiscard]] bool valid() const noexcept {
        return std::isfinite(elapsed_seconds) && std::isfinite(day_fraction) && day_fraction >= 0.0f &&
               day_fraction < 1.0f && std::isfinite(hour) && hour >= 0.0f && hour < 24.0f &&
               std::isfinite(sun_elevation) && std::isfinite(sun_azimuth) &&
               std::isfinite(weather_intensity) && weather_intensity >= 0.0f && weather_intensity <= 1.0f;
    }
};

class EnvironmentSystem {
public:
    explicit EnvironmentSystem(CalendarConfig config = {});
    [[nodiscard]] const CalendarConfig& config() const noexcept { return config_; }
    [[nodiscard]] const EnvironmentState& state() const noexcept { return state_; }
    void set_config(CalendarConfig config);
    void reset(double elapsed_seconds = 0.0) noexcept;
    void update(double real_seconds) noexcept;
    void set_elapsed_seconds(double seconds) noexcept;
    void set_day(std::uint64_t day) noexcept;
    void set_hour(float hour) noexcept;
    void set_weather(WeatherType weather, float intensity = 0.0f) noexcept;
    [[nodiscard]] std::string_view current_season_name() const noexcept;
    [[nodiscard]] const SeasonDefinition* current_season() const noexcept;
private:
    CalendarConfig config_{};
    EnvironmentState state_{};
    void recompute() noexcept;
};

} // namespace exgine

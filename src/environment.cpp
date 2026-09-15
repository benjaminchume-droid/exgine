#include "exgine/environment.hpp"

#include <algorithm>
#include <cmath>
#include <limits>

namespace exgine {
namespace {
constexpr double kHoursPerDay = 24.0;
float wrap_hour(float h) noexcept {
    if (!std::isfinite(h)) return 0.0f;
    h = std::fmod(h, 24.0f);
    return h < 0.0f ? h + 24.0f : h;
}
const CalendarConfig& default_config() {
    static const CalendarConfig c = [] {
        CalendarConfig x;
        x.seasons = {
            {"Spring", 0, 90, 0.05f, 1.10f, 1.00f},
            {"Summer", 90, 90, 0.30f, 1.00f, 1.05f},
            {"Autumn", 180, 90, -0.05f, 0.75f, 0.98f},
            {"Winter", 270, 90, -0.30f, 0.35f, 0.90f}
        };
        return x;
    }();
    return c;
}
}

bool CalendarConfig::valid() const noexcept {
    if (!std::isfinite(seconds_per_day) || seconds_per_day <= 0.0 ||
        days_per_year == 0 || days_per_week == 0 ||
        !std::isfinite(dawn_start) || !std::isfinite(dawn_end) ||
        !std::isfinite(dusk_start) || !std::isfinite(dusk_end)) return false;
    if (dawn_start < 0.0f || dawn_start >= 24.0f || dawn_end < 0.0f || dawn_end >= 24.0f ||
        dusk_start < 0.0f || dusk_start >= 24.0f || dusk_end < 0.0f || dusk_end >= 24.0f) return false;
    for (const auto& s : seasons) if (!s.valid() || s.start_day >= days_per_year) return false;
    return true;
}

EnvironmentSystem::EnvironmentSystem(CalendarConfig config) {
    if (!config.valid()) config = default_config();
    config_ = std::move(config);
    reset();
}

void EnvironmentSystem::set_config(CalendarConfig config) {
    config_ = config.valid() ? std::move(config) : default_config();
    recompute();
}

void EnvironmentSystem::reset(double elapsed_seconds) noexcept {
    state_ = {};
    state_.elapsed_seconds = std::isfinite(elapsed_seconds) ? std::max(0.0, elapsed_seconds) : 0.0;
    recompute();
}

void EnvironmentSystem::update(double real_seconds) noexcept {
    if (!std::isfinite(real_seconds) || real_seconds <= 0.0) return;
    state_.elapsed_seconds += real_seconds;
    if (!std::isfinite(state_.elapsed_seconds)) state_.elapsed_seconds = 0.0;
    recompute();
}

void EnvironmentSystem::set_elapsed_seconds(double seconds) noexcept {
    state_.elapsed_seconds = std::isfinite(seconds) ? std::max(0.0, seconds) : 0.0;
    recompute();
}

void EnvironmentSystem::set_day(std::uint64_t day) noexcept {
    const double day_length = std::max(1e-9, config_.seconds_per_day);
    const double fraction = state_.day_fraction;
    state_.elapsed_seconds = static_cast<double>(day) * day_length + fraction * day_length;
    recompute();
}

void EnvironmentSystem::set_hour(float hour) noexcept {
    const double day_length = std::max(1e-9, config_.seconds_per_day);
    const double hour_fraction = static_cast<double>(wrap_hour(hour)) / kHoursPerDay;
    const double day = std::floor(state_.elapsed_seconds / day_length);
    state_.elapsed_seconds = std::max(0.0, day * day_length + hour_fraction * day_length);
    recompute();
}

void EnvironmentSystem::set_weather(WeatherType weather, float intensity) noexcept {
    state_.weather = weather;
    state_.weather_intensity = std::clamp(std::isfinite(intensity) ? intensity : 0.0f, 0.0f, 1.0f);
}

std::string_view EnvironmentSystem::current_season_name() const noexcept {
    const auto* s = current_season();
    return s ? std::string_view{s->name} : std::string_view{};
}

const SeasonDefinition* EnvironmentSystem::current_season() const noexcept {
    if (config_.seasons.empty()) return nullptr;
    return &config_.seasons[state_.season_index % config_.seasons.size()];
}

void EnvironmentSystem::recompute() noexcept {
    const double day_length = std::max(1e-9, config_.seconds_per_day);
    const double total_days = state_.elapsed_seconds / day_length;
    const auto day = static_cast<std::uint64_t>(std::floor(std::max(0.0, total_days)));
    const float fraction = static_cast<float>(std::clamp(total_days - static_cast<double>(day), 0.0, std::nextafter(1.0, 0.0)));
    state_.absolute_day = day;
    state_.day_fraction = fraction;
    state_.hour = fraction * 24.0f;
    state_.day_of_year = config_.days_per_year ? static_cast<std::uint32_t>(day % config_.days_per_year) : 0;
    state_.day_of_week = config_.days_per_week ? static_cast<std::uint32_t>((day + config_.first_day_of_year) % config_.days_per_week) : 0;

    state_.season_index = 0;
    if (!config_.seasons.empty()) {
        std::size_t best = 0;
        std::uint32_t best_start = 0;
        for (std::size_t i = 0; i < config_.seasons.size(); ++i) {
            const auto& s = config_.seasons[i];
            if (s.start_day <= state_.day_of_year && s.start_day >= best_start) { best = i; best_start = s.start_day; }
        }
        if (state_.day_of_year < best_start) {
            for (std::size_t i = 0; i < config_.seasons.size(); ++i)
                if (config_.seasons[i].start_day >= best_start) best = i;
        }
        state_.season_index = best;
    }

    const float h = state_.hour;
    if (h < config_.dawn_start || h >= config_.dusk_end) state_.day_phase = (h >= config_.dusk_end) ? DayPhase::Night : DayPhase::Night;
    else if (h < config_.dawn_end) state_.day_phase = DayPhase::Dawn;
    else if (h < 12.0f) state_.day_phase = DayPhase::Morning;
    else if (h < config_.dusk_start) state_.day_phase = DayPhase::Afternoon;
    else if (h < config_.dusk_end) state_.day_phase = DayPhase::Dusk;
    else state_.day_phase = DayPhase::Evening;

    const float phase = (h / 24.0f) * 6.28318530718f;
    state_.sun_elevation = std::sin(phase - 1.57079632679f);
    state_.sun_azimuth = std::fmod(phase + 3.14159265359f, 6.28318530718f);
    if (state_.sun_azimuth < 0.0f) state_.sun_azimuth += 6.28318530718f;
}

} // namespace exgine

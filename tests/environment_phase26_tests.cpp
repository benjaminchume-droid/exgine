#include "exgine/environment.hpp"

#include <cassert>
#include <cmath>

using namespace exgine;

int main() {
    CalendarConfig c;
    c.seconds_per_day = 100.0;
    c.days_per_year = 20;
    c.seasons = {{"Wet",0,10,0.2f,1.2f,1.0f},{"Dry",10,10,-0.2f,0.5f,0.9f}};
    assert(c.valid());
    EnvironmentSystem env(c);
    assert(env.state().valid());
    env.set_hour(6.0f);
    assert(env.state().day_phase == DayPhase::Dawn);
    env.update(50.0);
    assert(env.state().absolute_day == 0);
    assert(std::fabs(env.state().hour - 12.0f) < 0.01f);
    env.update(60.0);
    assert(env.state().absolute_day == 1);
    assert(env.current_season_name() == "Wet");
    env.set_day(10);
    assert(env.current_season_name() == "Dry");
    env.set_weather(WeatherType::Rain, 2.0f);
    assert(env.state().weather == WeatherType::Rain && env.state().weather_intensity == 1.0f);
    return 0;
}

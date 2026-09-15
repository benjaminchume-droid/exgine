# Phase 26 — Universal Time, Day/Night, Seasons and Weather State

Phase 26 adds an engine-level environmental clock that is independent of any one game.

## Time

`EnvironmentSystem` tracks continuous elapsed simulation time, absolute day, day-of-year, day-of-week, hour and normalized day fraction. A configurable seconds-per-day value lets a project choose its own simulation scale.

## Day/night

The environment derives Dawn, Morning, Afternoon, Dusk, Evening and Night from configurable clock boundaries and exposes sun elevation/azimuth for renderers, lighting systems and gameplay systems.

## Seasons

Projects provide named season definitions with start day, duration and reusable environment coefficients such as temperature, vegetation and daylight factors. There are no hardcoded Spring/Summer/Autumn/Winter rules in gameplay code; the defaults are merely a convenient calendar configuration.

## Weather

The contract includes a generic weather type and intensity channel. Weather simulation, precipitation particles, clouds and audio remain consumers of this state and can be implemented by separate systems without changing the clock API.

## Checkpoint

The environment suite verifies validation, continuous time progression, day transitions, season selection, day-phase changes and weather-intensity clamping.

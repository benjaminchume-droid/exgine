# Phase 13 — Mobile Performance & Runtime Quality

## Status
Core checkpoint implemented.

Phase 13 establishes an explicit performance contract for mobile-first EXGINE deployments. It does not hide device assumptions inside terrain, buildings, vehicles, physics, or gameplay. Performance policy is data-driven and can be selected by the host application.

## Performance architecture
```text
Device limits
      |
      v
PerformanceProfile ---> Renderer / World Streaming / Gameplay host
      ^                         |
      |                         v
PerformanceController <--- PerformanceTelemetry
      |
      +--> frame budgets
      +--> quality tier
      +--> dynamic render scale
      +--> simulation throttle signal
```

### Quality tiers
- Ultra — maximum mobile-class quality target.
- High — high visual quality with bounded dynamic lighting/shadows.
- Medium — balanced default for sustained gameplay.
- Low — aggressive geometry, vegetation, lighting and texture budgets.
- BatterySaver — reduced visual and frame-rate budget for sustained thermal/battery operation.

### Runtime controls
`PerformanceProfile` explicitly owns render scale, target FPS, dynamic-light limits, shadow settings, world streaming budgets, vegetation density and memory budgets.

`DevicePerformanceLimits` prevents a requested profile from exceeding host-provided capabilities.

`FrameBudget` gives explicit CPU/GPU/streaming/physics budgets derived from target frame rate.

`PerformanceTelemetry` is the input boundary for measured frame and subsystem costs.

`PerformanceController` uses hysteresis: sustained overload causes a quality step down; sustained healthy performance allows gradual recovery. Critical thermal state immediately requests reduced quality and simulation throttling.

`make_mobile_profile()` provides deterministic starting profiles; it is a policy helper, not a hard-coded world-generation rule.

## Mobile-first principles
- Bounded memory and streaming residency.
- Bounded per-frame quality adaptation.
- No unbounded automatic quality escalation.
- No dependence on a specific GPU vendor.
- No platform API embedded in core simulation.
- No hidden changes to world/building/vehicle dimensions.
- Existing deterministic world streaming remains authoritative for chunk identity and procedural content.
- Physics remains fixed-step and independent from visual quality.

## Production boundary
This checkpoint is the core performance policy and telemetry contract. It intentionally does not pretend that a generic C++ core can measure every Android GPU/thermal signal without a platform backend. Android, Vulkan/OpenGL ES device telemetry, GPU timestamp queries, memory allocators, job systems, render command buffers, asset compression and platform frame pacing are integration layers for the platform/rendering phases.

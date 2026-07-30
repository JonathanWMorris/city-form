# 0002: Integer Time and Metric Coordinates

## Status

Accepted

## Context

Simulation time must advance deterministically and independently of rendering
frame time. City geometry must support headless algorithms and large maps
without embedding Unreal presentation units in authoritative data.

Floating frame deltas make repeatability and long-horizon advancement harder.
Using Unreal three-dimensional centimeter coordinates everywhere would simplify
early rendering conversion but couple flat graph and parcel algorithms to the
presentation layer.

## Decision

Authoritative simulation time uses signed 64-bit integer ticks. One v0.1 tick
represents one simulated minute.

Authoritative planar geometry uses finite double-precision X/Y coordinates in
meters. `CityForm` converts positions to Unreal centimeters for presentation.
Future elevation and road vertical profiles will use explicit metric data
rather than implicit viewport transforms.

Physical vehicle properties use explicitly documented SI units.

## Consequences

- Game speed controls how many ticks are requested, not the meaning of a tick.
- Systems may run on different integer cadences while preserving ordering.
- Headless geometry and routing do not need an Unreal World.
- Presentation requires a small, testable meter-to-centimeter conversion.
- Continuous microscopic motion will require a finer internal integration step
  while still reporting into the authoritative simulation timeline.
- Cross-platform floating-point determinism remains a measured constraint, not
  an unsupported promise.

## Supersedes

None.

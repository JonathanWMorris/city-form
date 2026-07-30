# 0009: Accelerated Civil and Transit Service Time

## Status

Accepted

## Context

City Form needs physical durations precise enough for traffic and routing while
also letting players observe multiple daily cycles during a normal session.
Using wall-clock time as simulation time would make city growth impractically
slow. Redefining physical seconds to accelerate the calendar would instead make
vehicle speeds, transit connections, and route costs ambiguous.

Transit adds a second boundary at midnight. An overnight run may belong to the
service day on which it began even after the civil clock advances to the next
date. Day and night presentation, weekday service, long-term city progression,
and wall-clock playback therefore cannot be one overloaded clock.

## Decision

`FSimulationInstant` remains the monotonic authoritative timeline, measured in
signed 64-bit integer milliseconds since city creation. Physical movement,
routing, traffic, transit operations, and scheduled events use that timeline.

Civil time is a deterministic projection of simulation time. The initial
gameplay defaults are:

- One 24-hour civil day passes in 45 wall-clock minutes at sustained 1x speed.
- Playback supports Pause, 1x, 2x, and 4x.
- The initial visual daylight interval is 05:00 through 21:00.
- A week has seven civil days so weekday and weekend behavior remains coherent.
- A year has 84 civil days, presented as four seasons of three weeks each.
- Daylight-saving time, real-world time zones, latitude-based sunlight, and
  conventional months are deferred.

Playback rate controls how quickly wall time requests authoritative simulation
advancement. It does not change the meaning of a simulation millisecond. If the
simulation cannot sustain a requested rate, elapsed wall time slows relative to
the target rather than skipping authoritative fixed steps or scheduled events.

Transit schedules use a service date plus an elapsed offset from the start of
that service date. The offset may exceed 24 hours, so a Monday run at `25:15`
occurs at Tuesday 01:15 civil time while remaining part of Monday service.
There is no hard-coded 30-hour limit.

Lighting derives from civil time and never determines whether authoritative
systems advance. Night visibility is a presentation and accessibility concern,
not a reason to create a second simulation truth.

## Consequences

- At 1x, the default clock advances at 32 simulated seconds per wall-clock
  second. One daylight interval lasts 30 wall-clock minutes and the remaining
  night interval lasts 15 minutes.
- Pause and playback multipliers belong at the boundary that drives simulation
  advancement, not inside movement, routing, or economic formulas.
- Deterministic comparisons must use the same simulation-time commands and
  events. Different wall-clock playback rates must not change the result merely
  by changing frame count.
- Transit can model overnight schedules, timepoints, connections, and actual
  arrival times without treating civil midnight as the end of service.
- Gameplay balance may use configurable subsystem durations, but it must not
  silently redefine physical movement units.
- Concrete calendar and playback APIs remain deferred until a gameplay system
  consumes them.

## Supersedes

None. This decision extends
[ADR 0007: Multi-rate millisecond simulation time](0007-multirate-millisecond-time.md).

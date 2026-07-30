# 0007: Multi-Rate Millisecond Simulation Time

## Status

Accepted

## Context

The original clock assigned one simulated minute to every integer tick. That is
adequate for households and construction, but too coarse for routing and
traffic. A typical urban road traversal takes seconds, so rounding every
segment to whole minutes would distort route choice toward paths with fewer
segments.

A fine time representation does not require every system to update at the same
frequency. Routing is evaluated on demand, traffic can use a fixed step, and
economic systems can run on slower schedules.

## Decision

Authoritative timestamps and durations use signed 64-bit integer milliseconds.
Simulation time remains independent of wall-clock time and rendering frames.

There is no universal high-frequency city update. Each subsystem uses an
explicit fixed cadence or scheduled events while sharing the same monotonic
timeline. The initial microscopic traffic design uses a configurable
1,000-millisecond step.

The player-facing calendar is a configurable projection of simulation time.
Calendar balancing does not redefine physical movement units or routing
durations. The initial projection, playback defaults, and extended transit
service-day convention are defined by
[ADR 0009](0009-accelerated-civil-and-service-time.md).

Authoritative geometry remains in meters, and vehicle performance remains in
SI units. Presentation converts meters to Unreal centimeters and interpolates
between authoritative snapshots.

## Consequences

- Short road traversals retain useful precision without floating-point clock
  accumulation.
- Timestamp and duration types must be distinct to prevent unit mistakes.
- Systems may run at different cadences while preserving deterministic order.
- Advancing time must detect negative durations and integer overflow.
- A millisecond timestamp does not imply a one-millisecond update loop.
- Calendar and playback APIs remain deferred until a gameplay system consumes
  the defaults accepted in ADR 0009.

## Supersedes

[ADR 0002: Integer time and metric coordinates](0002-time-and-coordinate-model.md)

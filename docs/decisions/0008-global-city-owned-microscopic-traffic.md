# 0008: Global, City-Owned Microscopic Traffic

## Status

Accepted

## Context

City Form should eventually support detailed traffic while remaining
deterministic, headlessly testable, and usable on strong conventional consumer
hardware. The earlier layered-fidelity decision assigned microscopic traffic
only to selected trips or regions and retained permanent mesoscopic
propagation elsewhere.

That approach would allow camera position or region loading to influence
simulation fidelity. It would also require synchronization between two
authoritative propagation models. The project instead needs one consistent
citywide traffic truth.

Established simulators such as Eclipse SUMO demonstrate that microscopic
traffic can use continuous vehicle positions with a one-second default
simulation step. SUMO is valuable as a research reference, but making it the
runtime authority would duplicate City Form road, trip, save, and editing
state.

## Decision

City Form owns its runtime traffic simulation. SUMO is not a runtime, build,
test, or packaging dependency; it may be used later as an optional offline
validation and benchmarking tool.

v0.1 uses a global microscopic-lite model. Every active vehicle advances
regardless of camera position, visibility, loaded level, or region. Initial
authoritative state includes its traversal, continuous distance, speed,
acceleration, route progress, and leader relationship.

Traffic starts with a configurable 1,000-millisecond global step and ballistic
integration. Rendering interpolates authoritative snapshots each frame but
never controls vehicle lifetime or progression.

v0.1 does not add lanes, lane changes, signals, detailed junction conflicts,
PID control, rigid-body vehicle physics, or physical collision detection.
Those features must extend the same global authority rather than introduce
camera-dependent fidelity.

One million persistent citizens is a long-term design and benchmark target, not
a v0.1 acceptance count and not a requirement for one million simultaneous
vehicles.

Future stochastic accidents use risk rates scaled by elapsed time or distance,
not one unscaled random roll per update. Dedicated deterministic random streams
create authoritative incident records. Future detected collisions may feed the
same incident system.

## Consequences

- Off-screen and visible vehicles follow identical authoritative rules.
- Rendering may cull meshes without changing simulation outcomes.
- Vehicle state must be compact, data-oriented, and independent of Actors.
- Traffic cadence may change only through configuration and benchmark evidence.
- Large-city work must measure persistent population separately from active
  vehicles.
- Fast-forwarding must accelerate the same global model rather than swap
  fidelity providers.
- Stochastic incident rates remain stable when traffic cadence changes.

## Supersedes

[ADR 0004: Layered traffic fidelity](0004-layered-traffic-fidelity.md)

## References

- [Eclipse SUMO simulation time-step documentation](https://sumo.dlr.de/docs/Simulation/Basic_Definition.html)
- [Eclipse SUMO microscopic traffic model](https://eclipse.dev/sumo/docs/Theory/Traffic_Simulations.html)
- [Eclipse SUMO libsumo integration](https://eclipse.dev/sumo/docs/Libsumo.html)

# Global Microscopic Traffic Model

## Status

This document defines the intended v0.1 microscopic-lite traffic model and its
long-term citywide boundary. Stage 7 will implement and calibrate the initial
model. Exact performance budgets remain benchmark outcomes.

## Goals

City Form traffic should:

- Emerge from persistent people, jobs, and trip demand
- Progress every active vehicle globally, regardless of visibility
- Respond to time of day, congestion, road changes, and vehicle capabilities
- Preserve individual trips without requiring one Actor per vehicle
- Remain deterministic and headlessly testable for controlled scenarios
- Scale toward one million persistent citizens on strong consumer hardware

One million citizens is not a v0.1 acceptance count and does not imply one
million simultaneous vehicles.

## Authoritative Boundary

The City owns Trips, Routes, Vehicles, traffic observations, and all movement
state. Unreal presentation reads snapshots and creates only the visual objects
needed to render the current view.

Camera position, rendering visibility, loaded levels, and streamed regions
never determine whether a vehicle advances or which traffic rules it follows.
Visual culling changes draw cost, not simulation fidelity.

SUMO is not a runtime authority or project dependency. It may be used later as
an optional offline reference for comparable networks, demand, flow, queues,
and travel times.

## Time Model

Traffic uses the shared signed 64-bit millisecond timeline. The initial global
traffic step is configurable and defaults to 1,000 milliseconds.

A fine timestamp does not imply millisecond updates. Routing runs on demand,
traffic runs at its configured step, and slower city systems use independent
cadences or scheduled events.

The first model uses ballistic integration during one step:

```text
new distance = old distance
             + old speed × elapsed time
             + 0.5 × acceleration × elapsed time²
```

Integration must detect traversal boundaries and preserve unconsumed time when
a vehicle moves to its next traversal. If behavioral accuracy is inadequate,
benchmarks compare global steps of 1,000, 500, and 200 milliseconds before the
default changes.

## Strategic Trips and Active Vehicles

A Trip owns movement demand:

- Traveler and purpose
- Origin and destination
- Departure instant
- DriverProfile and VehicleClass
- Planned and completed route traversals
- Completion or failure state

An active passenger-car Trip owns or references one compact Vehicle state:

- Vehicle and Trip identity
- VehicleClass
- Current directional traversal
- Continuous distance along the traversal in meters
- Speed and acceleration in SI units
- Route index and node-transition state
- Leader relationship where applicable

Completing, blocking, or cancelling a Trip updates both records atomically.
Destroying a rendered mesh never changes either record.

## Vehicle Classes and Driver Profiles

`FVehicleClassDefinition` remains validated and read-only. Routing and traffic
share its dimensions, effective spacing, mass, maximum speed, acceleration,
braking, turning, restriction, and capacity fields.

v0.1 gameplay generates passenger cars only. Synthetic tests may use slower,
larger, or restricted definitions. Mass does not create a universal speed
penalty; explicit vehicle performance and road conditions determine movement.

DriverProfiles contain deterministic behavioral differences such as desired
speed, following behavior, navigation use, perceived route cost, and rerouting
thresholds. Defaults require calibration against a recorded scenario.

## v0.1 Movement

Each directional traversal maintains vehicles in deterministic distance order.
The initial car-following rule uses the leader's position and speed, the
follower's desired speed, VehicleClass acceleration and braking limits, and a
minimum effective spacing.

v0.1 has one conceptual traffic stream per directional traversal. It does not
model explicit lanes or lane changes. Node admission serializes conflicting
arrivals through a simple deterministic rule sufficient for the prototype
network.

The initial model does not use PID controllers, Chaos rigid bodies, collision
shapes, or Actors for authoritative movement. Detailed signals, junction
conflicts, parking, and physical collisions are later extensions.

## Routing and Forecasting

Trips request a time-dependent A* route at departure. Every candidate
traversal is evaluated at its predicted entry instant.

The initial free-flow provider uses the lower of road speed and vehicle maximum
speed. Stage 7 adds forecasts derived from:

- Historical observations for a time-of-day bucket
- Current vehicle density and delay
- Vehicles committed to enter the traversal
- The querying VehicleClass

An active Trip may reconsider its remaining route only at a RoadNode. Rerouting
uses deterministic DriverProfile rules, a cooldown, and a minimum predicted
improvement. Completed route history is immutable.

## Rendering

The presentation layer receives timestamped traffic snapshots. It renders
smooth motion by interpolating distance, speed, and acceleration along derived
road splines at the display frame rate.

Interpolation is not a second simulation model. The same sampling rule can
produce a pose for any authoritative vehicle, while normal rendering culling
decides only which meshes are submitted.

## Deterministic Update Order

For each global traffic step:

1. Apply accepted network changes.
2. Generate scheduled Trip demand.
3. Apply due departures and route results.
4. Build deterministic traversal and leader ordering.
5. Calculate vehicle decisions from the prior authoritative snapshot.
6. Integrate all active vehicles.
7. Resolve traversal exits and node admission in stable ID order.
8. Publish observations, metrics, validation, and a new snapshot.

Parallel implementations must preserve these semantics through deterministic
partitioning and reduction.

## Future Traffic Incidents

Stochastic accidents use a hazard rate scaled by elapsed time or distance:

```text
interval probability = 1 - exp(-risk rate × elapsed time)
```

This prevents a traffic-step change from multiplying the long-term incident
rate. Risk may later depend on speed, following gap, road geometry, weather,
vehicle condition, DriverProfile, or congestion.

Incident randomness comes from a dedicated stream derived from the City seed,
stable identities, the incident system, and time context. An authoritative
TrafficIncident records participants, traversal and distance, occurrence time,
severity, capacity effects, and clearance state.

Future physical collision detection may create the same incident record.
Neither stochastic nor detected incidents depend on whether the location is
visible.

## Validation and Metrics

Traffic validation covers:

- Valid Trip, VehicleClass, traversal, and Route references
- Finite non-negative distance and speed
- Acceleration within the applicable VehicleClass limits
- Contiguous route progress
- Deterministic traversal ordering
- No overlap beyond documented numerical tolerance
- Consistent active, blocked, completed, and failed states

Initial metrics include active and completed Trips, travel time, free-flow
delay, directional volume, density, stopped vehicles, route failures, and
traffic-step runtime.

## Performance and Calibration

The v0.1 benchmark uses approximately 1,000 persistent residents. Later
benchmarks must report persistent population and simultaneous active vehicles
separately.

Long-term experiments should include 50,000, 200,000, and one million active
vehicles to expose scaling behavior without claiming that all are required for
normal gameplay. Measurements record hardware, build configuration, traffic
step, wall-clock simulation rate, frame time, memory, and determinism results.

MassEntity, SIMD specialization, GPU compute, and other scaling technology are
adopted only after profiling identifies a concrete bottleneck. They do not
become the authoritative domain model.

## References

- [Eclipse SUMO simulation time steps](https://sumo.dlr.de/docs/Simulation/Basic_Definition.html)
- [Eclipse SUMO traffic simulation models](https://eclipse.dev/sumo/docs/Theory/Traffic_Simulations.html)
- [FHWA Guidebook on Dynamic Traffic Assignment](https://ops.fhwa.dot.gov/publications/fhwahop13015/index.htm)
- [FHWA Traffic Analysis Tools Primer](https://ops.fhwa.dot.gov/trafficanalysistools/tat_vol1/sect6.htm)

# Layered Traffic Model

## Status

This document defines the intended v0.1 mesoscopic traffic model and its
boundary with future microscopic simulation. It establishes reusable concepts
now while leaving calibration constants to the representative Stage 7
benchmark.

## Goals

City Form traffic should:

- Emerge from persistent people, jobs, and trip demand
- Respond to time of day, queues, learned conditions, and network changes
- Produce bounded, varied driver choices rather than lockstep reactions
- Support individual authoritative trips without requiring one visible vehicle
  per trip
- Scale by changing propagation fidelity without replacing trip or routing data
- Remain deterministic and headlessly testable for controlled scenarios

## Coupled Traffic Loop

Dynamic traffic assignment couples route choice with network loading:

```text
Trip demand and departure times
             │
             ▼
Time-dependent route choice ◄──────────────┐
             │                             │
             ▼                             │
Mesoscopic network loading                 │
             │                             │
             ▼                             │
Observed queues and travel times ──────────┘
```

The model separates these responsibilities so each can be tested and later
replaced or refined independently.

## Strategic Trip Layer

The strategic layer owns:

- Trip identity and traveler
- Origin, destination, and departure tick
- DriverProfile and VehicleClass
- Planned and completed route traversals
- Current network location
- Completion or failure state

Trip demand is authoritative regardless of propagation fidelity. Destroying a
visible vehicle or unloading a region does not remove its Trip.

v0.1 generates passenger-car commute trips. Freight, deliveries, service
vehicles, public transit, and vehicle ownership are later demand systems.

## Vehicle Classes

`FVehicleClassDefinition` is validated and read-only after its catalog is
accepted by the simulation.

It reserves these explicitly unit-labeled properties:

| Property | Purpose |
| --- | --- |
| Length, width, and height in meters | Visual scale and physical restrictions |
| Effective queue length in meters | Mesoscopic space consumption |
| Mass in kilograms | Future dynamics, grade, energy, and restrictions |
| Maximum/free-flow speed (m/s) | Routing and free-flow time |
| Acceleration (m/s²) | Future stop and microscopic behavior |
| Comfortable/emergency deceleration | Future following and safety |
| Minimum turning radius in meters | Future turn feasibility |
| Passenger-car-equivalent factor | Mesoscopic capacity consumption |
| Restriction categories | Road and traversal eligibility |
| Performance category | Future powertrain and grade response |

The catalog initially contains one passenger-car class. Synthetic definitions
may be used in tests, but v0.1 gameplay does not generate trucks without a
freight or delivery reason.

Mass does not apply a universal speed penalty. Vehicle speed and acceleration
come from explicit performance data and road conditions. Size affects queue
space and capacity immediately; weight and power become especially relevant on
grades and during acceleration.

## Driver Profiles

A DriverProfile is deterministic data derived from the City seed. It separates
human behavior from VehicleClass capabilities.

The traffic design reserves:

- Whether navigation information is used
- Perceived travel-cost variation
- Minimum predicted improvement required to reroute
- Rerouting cooldown
- Responsiveness to live versus historical information

Exact distributions and defaults are calibration data. They must be recorded
with benchmark results before Stage 7 is accepted.

Drivers do not all receive perfect future knowledge or reroute simultaneously.
This avoids synchronized oscillation and represents bounded, varied route
choice.

## v0.1 Mesoscopic Propagation

Each RoadSegment exposes two directional traversal states. The v0.1
mesoscopic layer maintains, per direction:

- Free-flow traversal time
- Capacity expressed in passenger-car equivalents per tick
- A FIFO point queue
- Trips committed to enter
- Trips currently traversing
- Predicted and observed exit ticks

A Trip enters a traversal when permitted by its Route and available directional
capacity. Its VehicleClass consumes the configured capacity equivalent and
effective queue space. The model schedules an exit using the traversal's
free-flow time and queue delay.

Point queues do not model physical spillback into upstream segments. This is an
explicit v0.1 limitation, not an accidental claim of lane-level realism.

The mesoscopic layer advances individual Trips, but it does not simulate
continuous position, lane choice, car-following, acceleration, braking, or
collision avoidance.

## Historical and Live Forecasting

Every completed traversal publishes an observation containing:

- Directional traversal
- VehicleClass
- Entry and exit ticks
- Observed travel ticks
- Time-of-day bucket

Forecasts combine:

- Smoothed historical observations for the applicable time bucket
- Current directional queue delay
- Trips already committed to the traversal
- The querying VehicleClass's intrinsic speed and restrictions

The forecast must not inspect trips or events that have not yet been generated.
At the start of a new simulation, free-flow time is the historical fallback.

Smoothing method, time-bucket width, sample thresholds, and aging are
configuration data selected during Stage 7 calibration. The forecast interface
and observation schema must not depend on their eventual default values.

## Routing and Rerouting

Trips request a time-dependent A* route at departure. The cost provider
evaluates every candidate traversal at the tick when the route predicts the
Trip would enter it.

An active Trip may reconsider only at a RoadNode. It reroutes only when:

- Its DriverProfile uses applicable navigation information
- Its cooldown has expired
- A valid alternative exists
- Predicted improvement exceeds its configured threshold

The completed prefix of a Route is immutable. Only the remaining path changes.
A failed reroute leaves the valid existing route in place unless that route has
become prohibited or disconnected, in which case the Trip enters an explicit
blocked state.

## Deterministic Update Order

Within each simulation tick, traffic updates follow a documented order:

1. Apply accepted network changes.
2. Generate scheduled Trip demand.
3. Complete traversal exits.
4. Update directional queues and live observations.
5. Evaluate eligible departures and reroutes in stable Trip-ID order.
6. Admit Trips to traversals in stable order.
7. Publish summary metrics and validation results.

Parallelization may later preserve these semantics through deterministic
partitioning and reduction. v0.1 does not trade repeatability for concurrency.

## Future Microscopic Fidelity

Microscopic simulation is an additional propagation provider, not a replacement
for the strategic Trip layer.

It will eventually:

- Create active Vehicle instances for selected Trips or loaded regions
- Use VehicleClass dimensions, mass, acceleration, braking, turning, and
  performance data
- Model continuous position, lanes, following, lane changes, junctions,
  signals, queues with physical length, and parking where required
- Accept existing Trips and remaining Routes at explicit fidelity boundaries
- Report traversal observations through the same forecast boundary
- Return Trips to mesoscopic propagation when they leave detailed regions

Mesoscopic propagation remains useful for distant regions, fast-forwarding,
headless scenarios, and cities too large to simulate microscopically
everywhere.

MassEntity may be evaluated as a microscopic implementation tool only after
profiling identifies a concrete need. It does not become authoritative for
Trip, route, or city state.

## Validation and Metrics

Traffic validation covers:

- Valid Trip, DriverProfile, VehicleClass, and traversal references
- Contiguous and permitted remaining Routes
- Non-negative finite travel times and capacities
- FIFO queue ordering
- Capacity-equivalent accounting
- Immutable completed traversal history
- Valid blocked and completed states

Initial metrics include:

- Trips generated, active, completed, blocked, and failed
- Mean and percentile travel time
- Free-flow versus observed delay
- Directional volume, queue, and utilization
- Reroute attempts and accepted reroutes
- Forecast error by time bucket where samples exist

## Calibration and Acceptance

Stage 7 must define a small reproducible network with competing routes and a
commute peak. Calibration records:

- All vehicle and driver-profile defaults
- Capacity and free-flow assumptions
- Historical smoothing and bucket configuration
- Rerouting thresholds and cooldown distributions
- Hardware, build configuration, runtime, and memory
- Traffic outcomes and forecast error

Defaults are accepted only when the scenario produces stable, explainable
patterns without route-choice oscillation. They remain tunable data rather than
hard-coded routing behavior.

## References

- [FHWA Guidebook on Dynamic Traffic Assignment](https://ops.fhwa.dot.gov/publications/fhwahop13015/index.htm)
- [FHWA Traffic Analysis Tools Primer](https://ops.fhwa.dot.gov/trafficanalysistools/tat_vol1/sect6.htm)
- [FHWA truck size, acceleration, and capacity analysis](https://www.fhwa.dot.gov/policy/otps/truck/wusr/chap08.cfm)

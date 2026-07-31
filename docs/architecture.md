# Architecture

## Status

This document defines the architectural boundaries for City Form. The Stage 1
`CitySimulation` module now implements the dependency boundary, deterministic
primitives, simulation clock, validation, summary, regional road defaults,
road-type and vehicle-class catalogs, the logical road graph, and
time-dependent routing. Later systems may evolve the concrete APIs, but changes
must preserve the responsibilities and dependency direction below.

The concrete Stage 1–2 contract is documented in
[Simulation Foundation](simulation-foundation.md). Accepted architectural
choices and their consequences live in
[Architecture Decision Records](decisions/README.md).

## System Boundary

City Form uses one Unreal project with two distinct responsibilities:

```text
Player commands
      │
      ▼
CityForm ───────────────► CitySimulation
Unreal presentation       Authoritative city state
      ▲                          │
      └──── snapshots/events ────┘
```

The dependency direction is:

```text
CityForm → CitySimulation
```

`CitySimulation` must not depend on cameras, meshes, animation, UI, loaded
levels, or City Form presentation classes.

## Module Responsibilities

### CitySimulation

The authoritative simulation currently owns:

- Simulation time and deterministic random state
- Regional configuration and validated RoadType and VehicleClass catalogs
- Logical road topology, commands, and routing
- Validation and summary metrics

As later milestones land, it will also own:
- Parcels, zoning, buildings, and capacities
- Households, citizens, businesses, jobs, and assignments
- Trips, routes, road usage, and abstract traffic state
- Additional commands, metrics, and simulation events

The deepest simulation code should favor plain, data-oriented C++ and explicit
ownership. Unreal types are acceptable where they improve integration without
coupling the model to rendering or UObject lifetimes.

`CitySimulation` is an Unreal runtime module that depends only on `Core`. Its
authoritative types use ordinary C++ without UObject reflection or ownership.

### CityForm

The Unreal presentation layer owns:

- Player input, camera, and editing tools
- Terrain and ground interaction
- Road, parcel, building, vehicle, and pedestrian visuals
- Frame-rate interpolation of authoritative traffic snapshots
- UI, analytical views, effects, and audio
- Translation from player intent into simulation commands
- Translation from simulation snapshots or events into visible state

Blueprints are appropriate for visual assembly and configuration. Reusable
simulation rules and authoritative state belong in C++.

The active city is owned by `UCityFormSimulationSubsystem`, whose game-instance
lifetime is independent of any loaded map or presentation Actor. Native Unreal
tools submit typed commands through this subsystem and receive detached
presentation snapshots. The subsystem does not expose the mutable simulation or
road graph to Actors or Blueprints.

## Data Flow

Player-facing tools submit explicit commands rather than mutating simulation
containers directly. The simulation validates and applies accepted commands,
advances its systems, and publishes read-only state or events for presentation.

This boundary should support:

- Headless simulation without a world or viewport
- Replayable tests using known commands and seeds
- Visual culling and levels of detail for the same authoritative records
- Future undo, inspection, automation, and modding facilities
- Clear rejection reasons when a command would violate an invariant

Each milestone should introduce only the commands and read models needed by its
working vertical slice.

At the Unreal boundary, X and Y coordinates convert explicitly between
authoritative meters and Unreal centimeters using exactly 100 centimeters per
meter. Authoritative planar positions return to presentation at ground Z. Road
graph snapshots contain stable simulation IDs but copy positions and lengths
into Unreal units; changing or discarding a snapshot cannot alter the city.

## Identity and Lifetime

Persistent records use strong typed IDs such as `FRoadNodeId` and
`FRoadSegmentId`. References between records use IDs, not raw pointers or
Actor references.

IDs must:

- Distinguish different entity categories at compile time where practical
- Have a documented invalid or absent state
- Remain stable while the referenced record exists
- Be validated when crossing system boundaries

Deletion and reuse policies must prevent stale references from silently
targeting unrelated records. Current IDs use monotonic allocation from one,
reserve zero as invalid, and are not reused during v0.1. Storage should be
refined only after profiling identifies a need.

## Time and Determinism

Simulation time uses integer millisecond timestamps and durations independent
of rendering frame time. A fine-grained clock does not require a universal
high-frequency update loop. Routing runs on demand, traffic uses an explicit
fixed step, and slower systems use their own cadences or scheduled events.

Rendering may interpolate or visualize snapshots, but it must not decide
whether authoritative systems advance. Civil time, transit service time, and
wall-clock playback are projections or controls around the authoritative
timeline rather than replacements for the physical units used by routing and
movement. Their accepted defaults are documented in
[Gameplay Time and Development Pacing](gameplay-pacing.md) and
[ADR 0009](decisions/0009-accelerated-civil-and-service-time.md).

If a requested playback rate exceeds available simulation throughput, the
simulation must fall behind that wall-clock target instead of skipping
authoritative fixed steps or scheduled events.

Deterministic behavior requires:

- An explicit simulation seed
- Simulation-owned random streams
- Defined system update ordering
- No dependence on Actor iteration order, wall-clock timing, or loaded regions
- Tests that compare meaningful state and metrics for repeated scenarios

Determinism is promised only for controlled builds and scenarios until broader
cross-platform behavior has been measured.

## Roads, Trips, and Visualization

The logical road graph is authoritative. Splines, meshes, debug lines, lanes,
and visible vehicles are representations derived from it.

An authoritative trip contains valid origin and destination references and a
logical route. Each active v0.1 passenger-car trip has global
microscopic-lite vehicle state, including traversal, continuous distance,
speed, acceleration, and route progress.

A rendered vehicle is an interpolated view. Destroying, culling, or unloading
that representation must not destroy or pause its authoritative vehicle.

Trips use time-dependent A* with vehicle-aware traversal costs.
The traffic milestone will advance all active vehicles through one global
fixed-step model and publish historical and live travel-time observations.
Camera position, visibility, and loaded regions never select simulation
fidelity.

The complete boundary is defined in
[Global Microscopic Traffic Model](traffic-model.md).

## Future Transit Boundary

Public transit remains outside v0.1, but later systems must preserve the same
simulation/presentation separation. Stops, scheduled runs, vehicle blocks,
passenger itineraries, actual operation, and dispatch decisions are
authoritative simulation records. Visible vehicles and passengers are derived
representations.

Buses may use time-dependent road routing and global road traffic. Rail and
subway services require guideway topology, while passenger journey planning
requires a schedule-aware multimodal layer rather than treating every problem
as road A*. Overnight service uses extended service-day offsets without
changing the 24-hour civil clock.

The complete forward-looking constraints are documented in
[Future Transit Foundation](transit-foundation.md).

## Performance Strategy

City Form targets strong conventional consumer hardware rather than
workstation-class or flagship-only systems.

The architecture supports that goal by:

- Keeping persistent simulation entities compact
- Batching system work over explicit data
- Separating authoritative and visual update rates
- Using subsystem-specific cadences and scheduled events
- Reserving Actors for objects that need Unreal interaction or physical
  representation
- Adding MassEntity or other specialized systems only after profiling shows a
  concrete need

Optimization work must include a representative scenario, recorded metrics,
and a regression check where practical. v0.1 will produce the first performance
baseline before fixed hardware budgets are adopted.

One million persistent citizens is a long-term design and benchmark target,
not a v0.1 acceptance count or a promise of one million simultaneous vehicles.

## Persistence Boundary

v0.1 does not implement save/load, but it must avoid choices that make
persistence inseparable from presentation.

Future saves should serialize a versioned data schema rather than raw Actors,
UObjects, pointers, or incidental container layouts. The persistence design must
define:

- Schema and game-version metadata
- Validation before state becomes authoritative
- Explicit migrations between supported versions
- Behavior for unsupported or corrupt saves
- Compatibility and deprecation guarantees

No internal v0.1 structure is a public persistence contract.

## Extensibility

Future advanced editing, traffic management, constraint overrides, research
interfaces, and mods require authoritative data to remain addressable and
editable through explicit boundaries.

The v0.1 design should therefore preserve a path to create, inspect, update, and
delete logical records without requiring direct manipulation of visual Actors.
Constraint overrides should be explicit command options or policies rather than
global invalid-state switches. Public APIs and plugin interfaces remain
deferred until their real use cases are demonstrated.

## Validation

Every simulation subsystem owns invariants appropriate to its data. The current
test suite covers:

- Road endpoints reference valid nodes
- No dangling road-segment references
- Entity references use the correct ID category
- Simulation time, deterministic random output, and strong-ID behavior
- Regional defaults and RoadType and VehicleClass catalogs
- Atomic rejection of invalid road commands
- Repeatable graph construction and headless advancement

Planned systems must extend validation to cover:

- Building, home, and job capacities cannot become negative
- Assignments do not exceed capacity
- Trips have valid origins, destinations, and routes
- The same controlled seed and command sequence produce repeatable results

Validation failures should provide enough context to identify the responsible
record and rule.

## Platform Boundary

Platform-neutral standard C++ and Unreal APIs are preferred. Platform-specific
code must be isolated behind a small interface and justified by a concrete
requirement.

macOS on Apple Silicon is the currently verified development environment.
Windows is an intended target, but support claims require testing by a
maintainer or contributor with suitable hardware.

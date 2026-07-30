# Architecture

## Status

This document defines the intended architectural boundaries for City Form. The
`CitySimulation` module described here has not been implemented yet. Concrete
APIs may evolve as the v0.1 vertical slice is built, but changes must preserve
the responsibilities and dependency direction below.

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

The authoritative simulation owns:

- Simulation time and deterministic random state
- Road topology and routing data
- Parcels, zoning, buildings, and capacities
- Households, citizens, businesses, jobs, and assignments
- Trips, routes, road usage, and abstract traffic state
- Commands that modify the city
- Validation, metrics, and simulation events

The deepest simulation code should favor plain, data-oriented C++ and explicit
ownership. Unreal types are acceptable where they improve integration without
coupling the model to rendering or UObject lifetimes.

### CityForm

The Unreal presentation layer owns:

- Player input, camera, and editing tools
- Terrain and ground interaction
- Road, parcel, building, vehicle, and pedestrian visuals
- UI, analytical views, effects, and audio
- Translation from player intent into simulation commands
- Translation from simulation snapshots or events into visible state

Blueprints are appropriate for visual assembly and configuration. Reusable
simulation rules and authoritative state belong in C++.

## Data Flow

Player-facing tools submit explicit commands rather than mutating simulation
containers directly. The simulation validates and applies accepted commands,
advances its systems, and publishes read-only state or events for presentation.

This boundary should support:

- Headless simulation without a world or viewport
- Replayable tests using known commands and seeds
- Multiple visual levels of detail for the same authoritative records
- Future undo, inspection, automation, and modding facilities
- Clear rejection reasons when a command would violate an invariant

The first implementation should introduce only the commands and read models
needed by the active milestone.

## Identity and Lifetime

Persistent records use strong typed IDs such as `RoadNodeId` and
`RoadSegmentId`. References between records use IDs, not raw pointers or Actor
references.

IDs must:

- Distinguish different entity categories at compile time where practical
- Have a documented invalid or absent state
- Remain stable while the referenced record exists
- Be validated when crossing system boundaries

Deletion and reuse policies must prevent stale references from silently
targeting unrelated records. The initial implementation may choose a simple
monotonic allocation strategy and refine storage only after profiling.

## Time and Determinism

Simulation time is independent of rendering frame time. Rendering may
interpolate or visualize snapshots, but it must not decide whether authoritative
systems advance.

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
logical route. A visible vehicle may represent a selected or nearby trip, but
destroying or unloading that vehicle must not destroy the trip.

Road utilization is derived from abstract demand and routes. v0.1 does not
require every trip to be represented by a physically simulated vehicle.

## Performance Strategy

City Form targets strong conventional consumer hardware rather than
workstation-class or flagship-only systems.

The architecture supports that goal by:

- Keeping persistent simulation entities compact
- Batching system work over explicit data
- Separating authoritative and visual update rates
- Allowing visual and simulation detail to scale independently
- Reserving Actors for objects that need Unreal interaction or physical
  representation
- Adding MassEntity or other specialized systems only after profiling shows a
  concrete need

Optimization work must include a representative scenario, recorded metrics,
and a regression check where practical. v0.1 will produce the first performance
baseline before fixed hardware budgets are adopted.

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

Every simulation subsystem owns invariants appropriate to its data. The initial
test suite should cover:

- Road endpoints reference valid nodes
- No dangling road-segment references
- Entity references use the correct ID category
- Building, home, and job capacities cannot become negative
- Assignments do not exceed capacity
- Trips have valid origins, destinations, and routes
- The same controlled seed and command sequence produce repeatable results
- Simulation advancement does not require a gameplay viewport

Validation failures should provide enough context to identify the responsible
record and rule.

## Platform Boundary

Platform-neutral standard C++ and Unreal APIs are preferred. Platform-specific
code must be isolated behind a small interface and justified by a concrete
requirement.

macOS on Apple Silicon is the currently verified development environment.
Windows is an intended target, but support claims require testing by a
maintainer or contributor with suitable hardware.

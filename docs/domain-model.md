# v0.1 Domain Model

## Status

This document defines the vocabulary and relationships used by the City Form
v0.1 simulation. It describes domain meaning and ownership, not a final storage
layout or save-file schema.

Terms marked as future-facing establish an architectural boundary but are not
necessarily implemented in v0.1.

## Relationship Overview

```text
City
├── Region profile
├── Road-type catalog
├── Road graph
│   ├── Road nodes
│   └── Road segments
│       └── Directional traversals
├── Parcels
│   ├── Zone
│   └── Building
│       ├── Household capacity
│       └── Business capacity
├── Households
│   └── Citizens
├── Businesses
│   └── Jobs
├── Vehicle-class catalog
└── Trips
    ├── Citizen
    ├── Driver profile
    ├── Vehicle class
    ├── Active vehicle
    └── Route
        └── Ordered road traversals
```

## City

The City is the authoritative root of one simulation. The current
implementation owns simulation time, deterministic random state, an
`FRegionProfile` snapshot, `FVehicleClass` and `FRoadType` catalogs, the
`FRoadGraph`, ID allocation, validation, and summary metrics. Later systems add
their authoritative entity storage to the same boundary.

A City exists independently of any Unreal World, map, viewport, or visual
representation. v0.1 keeps one City in memory and does not define persistence.

## Regional Defaults

An `FRegionProfile` is an immutable configuration snapshot owned by one City.
It provides regional defaults without making simulation rules depend on a
global locale or platform setting.

The initial profile is `US-CA`. Future map setup may choose a different profile
before constructing a City; changing an existing City's region is outside the
current contract.

## Roads

### Road Type

An `FRoadType` is a validated, read-only definition in the City's RoadType
catalog. It has a strong `FRoadTypeId`, a stable key, and a default speed
limit. The initial `BasicTwoWayRoad` definition supports travel in both
directions and uses the selected `FRegionProfile` default speed.

RoadSegments reference a RoadType and may carry a per-segment speed-limit
override. RoadType definitions are authoritative simulation data, not visual
road styles.

### Road Node

A RoadNode is a topological connection point in the road graph. It has a stable
`FRoadNodeId` and an authoritative planar position in meters.

A node may be isolated while a road network is being assembled. Connections
exist only through RoadSegments; geometric proximity does not imply a
connection.

### Road Segment

A RoadSegment is one logical connection between two distinct RoadNodes. It has
a stable `FRoadSegmentId`, references valid endpoint and RoadType IDs, and has
positive length. Its speed limit normally comes from its RoadType and may carry
an explicit per-segment override.

The single v0.1 road type supports travel in both directions. A segment is not a
mesh, spline, lane collection, or Actor.

### Road Traversal

A RoadTraversal is a directed use of a RoadSegment from one endpoint to the
other. Two-way segments therefore expose two traversals.

Direction-specific traffic state, predicted travel time, restrictions, queues,
and observations belong to traversals rather than to the undirected segment.

## Land and Development

### Parcel

A Parcel is a bounded piece of developable land associated with road access. It
has a stable `FParcelId`, references the RoadSegment and side it fronts, may
receive a Zone, and may contain at most one Building — this 1:1 relationship
holds regardless of a Parcel's footprint size.

A Parcel's footprint is a variable, whole-cell-aligned width and depth
(`CellsWide`/`CellsDeep`, both at least one) rather than a single fixed size.
v0.1 generation always produces `1x1` parcels; the variable footprint exists
so a future feature can combine contiguous `1x1` parcels into a larger
footprint — for example to represent a bigger building — without changing this
schema. v0.1 parcel generation is intentionally simple. A Parcel is still
authoritative simulation data rather than procedural mesh state. See
[Simulation Foundation](simulation-foundation.md#stage-5-roadside-parcels) for
the concrete generation contract and
[ADR 0011](decisions/0011-segment-aligned-roadside-parcels.md) for the
rationale.

### Zone

A Zone is the allowed development category assigned to a Parcel. v0.1 supports
Residential and Commercial.

Zoning does not itself create capacity. It permits compatible development to
create a Building.

### Building

A Building is a developed structure on one Parcel. It has a stable
`FBuildingId`, one v0.1 use, and capacity appropriate to that use.

Residential buildings provide household or dwelling capacity. Commercial
buildings provide space for Businesses and their Jobs. Placeholder meshes
visualize Buildings but do not own them.

## People and Organizations

### Household

A Household is a persistent group of one or more Citizens that occupies
residential capacity together. It has a stable `FHouseholdId` and references
one valid residential Building while housed.

Household membership and housing assignments must agree in both directions.
Unhoused behavior is outside v0.1; move-in logic may create only households that
can be assigned valid capacity.

### Citizen

A Citizen is a lightweight persistent person record with a stable `FCitizenId`.
Every v0.1 Citizen belongs to exactly one Household and may hold at most one
Job.

A Citizen is not normally an Actor. A nearby pedestrian may later visualize a
Citizen without controlling the Citizen's lifetime or authoritative state.

### Business

A Business is a persistent commercial occupant with a stable `FBusinessId`. It
occupies compatible commercial-building capacity and owns zero or more Jobs.

Detailed finances, production, freight, deliveries, and business failure are
outside v0.1.

### Job

A Job is one position owned by a Business. It has a stable `FJobId` and may be
assigned to at most one Citizen.

Citizen-to-Job and Job-to-Citizen references must agree. Job capacity cannot
exceed the capacity supplied by the Business's Building.

## Travel

### Trip

A Trip is authoritative movement demand with a stable `FTripId`. A v0.1 commute
references:

- The traveling Citizen
- An origin and destination
- A departure instant
- A DriverProfile
- A VehicleClass
- Current progress and completion state
- A planned Route when one is available

Trips continue to exist when no vehicle is rendered. A failed route is an
inspectable simulation outcome, not permission to use an invalid reference.

### Route

A Route is an ordered sequence of directed RoadTraversals connecting a valid
origin to a valid destination for a particular departure time and VehicleClass.
It records total distance and predicted arrival time.

Routes may become stale as conditions change. Replanning replaces the remaining
route at a RoadNode; it does not rewrite traversal history already completed.

### Driver Profile

A DriverProfile contains deterministic behavioral variation such as navigation
use, perceived cost, willingness to reroute, and rerouting cooldown.

Driver behavior is separate from vehicle performance. Two drivers in identical
cars may choose differently, while one driver's route may change when using a
different VehicleClass.

### Vehicle Class

A VehicleClass is a validated, read-only definition shared by routing and
global microscopic traffic. It describes dimensions, capacity consumption,
speed and performance limits, and restriction categories.

v0.1 gameplay generates only passenger-car trips. The generic class boundary is
present so later freight, service, transit, and specialized vehicles do not
require replacement of Trip or Route data.

### Vehicle

An active Vehicle is the global movement state for a motorized Trip. It
references its VehicleClass and Route and stores its current traversal,
continuous distance, speed, acceleration, and route progress.

Every active vehicle advances independently of rendering visibility, camera
position, loaded level, or region. Vehicle ownership and persistent fleets are
outside v0.1.

### Traffic Incident

A TrafficIncident is a future authoritative record for a stochastic accident
or detected collision. It identifies involved vehicles, location, occurrence
instant, severity, capacity effects, and clearance state.

Stochastic incident risk is scaled by elapsed time or distance and uses a
dedicated deterministic random stream. Traffic incidents are outside v0.1, but
the global vehicle model must leave room for them without making rendered
Actors authoritative.

## Identity and Lifetime Rules

- Every persistent entity category uses a distinct strong ID type.
- ID value zero is invalid.
- IDs increase monotonically and are not reused during v0.1.
- References cross storage boundaries through IDs, never raw pointers or Actor
  references.
- A referenced entity must outlive the reference or the owning operation must
  update both sides atomically.
- Visual unloading, Actor destruction, and level changes never delete
  simulation records.

## Core v0.1 Invariants

- RoadSegment endpoints reference valid, distinct RoadNodes.
- Traversals use a valid RoadSegment and one of its two endpoint directions.
- Parcels reference valid road access where required.
- A Parcel's column and row position are non-negative, and its footprint
  spans a positive whole number of cells in width and depth.
- Buildings occupy compatible Parcels and Zones.
- Capacities and occupancy counts are never negative.
- Occupancy and assignments do not exceed capacity.
- Household, Citizen, Business, and Job references are mutually consistent.
- Trips reference valid Citizens, endpoints, DriverProfiles, and
  VehicleClasses.
- Active Vehicles reference valid Trips, VehicleClasses, traversals, and
  contiguous route progress.
- Routes contain contiguous, permitted traversals with valid endpoints.
- Completed traversal history is not modified by rerouting.
- All authoritative coordinates and physical parameters are finite and use
  documented units.

## Deferred Concepts

The following are intentionally not defined as concrete v0.1 records:

- Lanes, traffic signals, parking spaces, and junction conflicts
- Freight orders, cargo, deliveries, and vehicle fleets
- Utilities, services, budgets, land value, and detailed economics
- Demand-driven zone density and building growth over time
- Save-file schemas and migration records
- Lane-changing, detailed junction conflicts, and collision state
- Traffic incidents and emergency-response records

They should extend the vocabulary above rather than transfer authority to
visual Actors or invalidate established identities.

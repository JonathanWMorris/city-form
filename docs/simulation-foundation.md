# Simulation Foundation Specification

## Status

The Stage 1 simulation skeleton and Stage 2 millisecond time, logical road
graph, and time-dependent routing contract are implemented. The Stage 5
deterministic roadside parcel slice is implemented; zoning, buildings, and
capacities remain future Stage 5 work.

## Module Boundary

`CitySimulation` is an Unreal runtime module built by UnrealBuildTool.

```text
CityForm → CitySimulation → Core
```

`CitySimulation.Build.cs` may depend on `Core`. It must not depend on
`CoreUObject`, `Engine`, `CityForm`, rendering, input, navigation, or loaded
levels.

Authoritative types live under the `CityForm::Simulation` namespace and are
ordinary C++ structs and classes. They do not use `UCLASS`, `USTRUCT`,
`GENERATED_BODY`, garbage collection, or UObject ownership.

The first implementation uses Unreal Automation Tests. It does not introduce a
second build system or external testing framework.

## Units and Numeric Conventions

| Quantity | Authoritative unit |
| --- | --- |
| Simulation timestamps | Signed 64-bit milliseconds |
| Simulation durations | Signed 64-bit milliseconds |
| Planar position and distance | Double-precision meters |
| Vehicle dimensions | Meters |
| Vehicle mass | Kilograms |
| Vehicle speed | Meters per second |
| Vehicle acceleration | Meters per second squared |
| Capacity-equivalent factor | Passenger-car equivalents |

Conversions to Unreal centimeters occur in `CityForm`. Simulation code must not
store positions in presentation units merely to avoid a conversion.

All externally supplied floating-point values must be finite. Comparisons that
represent geometric tolerance must name and document the tolerance rather than
relying on exact equality accidentally.

## Strong IDs

The foundation supplies a small strong-ID mechanism used to define concrete
types such as:

- `FRoadNodeId`
- `FRoadSegmentId`
- `FRoadTypeId`
- `FVehicleClassId`
- `FParcelId`
- Later v0.1 entity IDs listed in the
  [domain model](domain-model.md)

Every ID is backed by `uint64`. Zero is invalid. Valid IDs are allocated
monotonically from one and are not reused during v0.1.

Different ID categories must not convert implicitly to one another. IDs support
equality, ordering, hashing, validity checks, and access to their underlying
value for diagnostics.

Overflow is a structured failure. It must not wrap into zero or an existing ID.

## Simulation Configuration and Time

### `FSimulationConfig`

Configuration contains an explicit `uint64 Seed` and an `FRegionProfile`
snapshot. Construction does not read wall-clock time, process state, platform
randomness, or global regional settings.

The default region is `US-CA`. Region data supplies applicable defaults to
catalog construction and can be replaced per city without changing graph code.
Future map setup may select another profile before constructing a City.

### Simulation Time Types

`FSimulationInstant` stores signed 64-bit milliseconds since city creation.
`FSimulationDuration` stores a signed 64-bit millisecond difference.
`FSimulationClock` owns the current instant, and a new City starts at zero.

Timestamp and duration types do not convert implicitly. Checked addition
rejects negative advancement and overflow before changing state.

The millisecond representation does not require systems to update every
millisecond. Each system uses an explicit cadence or scheduled events. A future
calendar maps authoritative time into gameplay dates without changing physical
movement units.

### `FCitySimulation`

`FCitySimulation` is the authoritative facade and state owner. The current
facade exposes:

- Construction from a validated `FSimulationConfig`
- Read access to configuration, time, deterministic random state, the
  VehicleClass and RoadType catalogs, the RoadGraph, and the parcel layout
- `Advance(FSimulationDuration Duration)`
- `AddRoadNode(...)`
- `AddRoadSegment(...)`
- `CreateRoadSegment(...)`, an atomic command that accepts new or existing
  endpoints
- `RegenerateParcels()`, a deterministic, idempotent, full-recompute command
  that derives the entire parcel set from the current RoadGraph. It is
  automatically invoked after a successful `AddRoadSegment` or
  `CreateRoadSegment`, and is also safe to call directly at any time by future
  callers.
- `FindRoute(...)`
- `Validate()`
- `GetSummary()`

Advancing by zero succeeds without changing state. Negative durations fail.
Advancement that would exceed the signed 64-bit millisecond range fails before
changing state.

Systems run in a documented deterministic order. Rendering frames and game
speed decide how much duration to request but never change physical time units.

## Deterministic Randomness

The simulation owns a wrapper around `std::mt19937_64`. No system creates an
unseeded engine or reads randomness from Unreal global state.

The wrapper:

- Produces raw 64-bit output
- Implements its own documented unbiased bounded-integer mapping
- Implements any real-number mapping from explicitly selected random bits
- Does not expose implementation-dependent standard distributions
- Supports deterministic derivation of subsystem or entity streams where
  needed later

Golden tests lock the expected initial sequence for a known seed. Changing the
algorithm or mapping is an explicit architecture and compatibility decision,
even before public saves exist.

Determinism is initially promised for controlled builds and test scenarios. It
is not yet a guarantee across arbitrary compilers, engine versions, or
platforms.

## Results, Validation, and Diagnostics

Expected input and domain failures return explicit typed result records. The
foundation does not enable or rely on C++ exceptions.

Each failure contains:

- A stable error code
- A concise human-readable explanation
- Relevant entity IDs or command context where available

Developer assertions protect impossible internal states and programmer
contract violations. They do not replace validation of player commands or
loaded external data.

`FValidationReport` contains zero or more issues. Each issue records severity,
stable code, entity category and ID when applicable, and an explanation.
`IsValid()` is true when the report contains no error-severity issues.

`FCitySummary` reports seed, current time in milliseconds, VehicleClass count,
RoadType count, RoadNode count, RoadSegment count, and Parcel count. Later
systems add their own counts as they become authoritative. Summary generation
is read-only and deterministically ordered.

## Stage 2 Road Graph

### Regional Defaults and Road Types

Each City owns a read-only RoadType catalog constructed from its RegionProfile.
The initial `BasicTwoWayRoad` has stable RoadType ID one. Under the California
profile its default speed limit is 25 miles per hour, stored as exactly
`11.176` meters per second.

RoadSegments reference a valid RoadType and may store an optional speed-limit
override. An absent override resolves to the RoadType default. Post-creation
editing and additional regional profiles or road types remain deferred.

### Coordinates

`FSimPoint2D` stores finite `double X` and `double Y` values in meters. v0.1 is
flat. Future elevation and vertical alignment will be separate metric data
rather than an implicit Unreal Z coordinate.

### Nodes and Segments

A RoadNode contains its `FRoadNodeId` and `FSimPoint2D`.

A RoadSegment contains its `FRoadSegmentId`, two distinct endpoint IDs, a valid
RoadType ID, an optional speed-limit override, and a cached positive length in
meters. Validation confirms that the cached length matches the Euclidean
endpoint distance within a named tolerance.

One two-way segment exposes two directional RoadTraversals. Directional
traversal identity is the tuple of SegmentId, FromNodeId, and ToNodeId; v0.1
does not require a separately allocated traversal ID.

Stage 2 commands:

- Add one RoadNode at a validated point
- Add one RoadSegment between existing nodes
- Create one RoadSegment with any combination of validated existing endpoints
  and new endpoint positions, without leaving partial nodes on failure

Commands return typed result records containing the created ID on success.
Segment creation rejects:

- Invalid or missing endpoint IDs
- An invalid or missing RoadType ID
- The same endpoint at both ends
- Non-finite or zero-length geometry
- A non-finite or non-positive speed-limit override
- A duplicate connection between the same endpoints in v0.1
- ID exhaustion

Isolated valid nodes are permitted. Automatic snapping, geometric intersection
splitting, deletion, lanes, one-way roads, elevation, turn restrictions, and
road meshes are deferred.

## Time-Dependent A*

### Query and Result

A route query contains:

- Origin and destination RoadNode IDs
- Departure instant
- VehicleClass ID

DriverProfile behavior remains outside Stage 2 routing. Later traffic systems
may adapt provider costs or route-selection policy without changing graph
topology.

A successful route contains:

- Ordered directional RoadTraversals
- Ordered node IDs sufficient to validate continuity
- Total distance in meters
- Predicted arrival instant
- Total predicted travel duration in milliseconds

Invalid endpoints, unsupported VehicleClasses, prohibited traversals, and
disconnected destinations return distinct failures.

### Cost Evaluation

A traversal-cost provider evaluates expected travel duration using:

- Directional traversal
- Predicted entry instant
- VehicleClass
- Available historical and live traffic forecast

Stage 2 supplies a free-flow provider through this time-aware interface.
Congestion-aware providers arrive with the traffic milestone.

Free-flow speed is the lower of the resolved RoadSegment speed and the
VehicleClass maximum speed. Duration is rounded up to a positive whole
millisecond.

Costs must be non-negative integer milliseconds and satisfy FIFO: entering the
same traversal later cannot produce an earlier exit. Providers explicitly
declare the FIFO guarantee, and the router rejects providers that do not.

### Heuristic

The A* heuristic is a lower bound on remaining travel milliseconds:

```text
straight-line distance to destination
──────────────────────────────────────
fastest feasible speed for the vehicle
```

The result is rounded down to whole milliseconds to preserve admissibility.
The speed bound ignores congestion but respects intrinsic VehicleClass limits.
Predicted traffic belongs in traversal costs, not in a heuristic that could
overestimate.

If no stronger admissible bound is available, a zero heuristic is valid and
causes the same A* implementation to behave like Dijkstra.

### Deterministic Ordering

The open set orders candidates by:

1. Lowest predicted total arrival cost
2. Lowest heuristic remainder
3. Lowest RoadNode ID

When equal-cost paths reach the same node, predecessor selection orders by
RoadSegment ID and then predecessor RoadNode ID. Route reconstruction must
therefore produce the same result for the same graph, query, and cost provider.

## Stage 5 Roadside Parcels

### Grid and Sizing Defaults

Parcel sizing lives on `FRegionProfile`, alongside the Basic Two-Way Road
default speed limit: `ParcelCellSizeMeters` (8.0), `ParcelMaxDepthRows` (4),
and `ParcelSetbackMeters` (4.0). These defaults intentionally match real
Cities Skylines' zoning-grid constants. `ParcelSetbackMeters` is an
authoritative simulation value defined independently of CityForm's visual
road-placeholder strip width; the two currently coincide numerically but
neither depends on the other, per the architecture's presentation-independence
rule.

### Segment-Aligned Generation

Parcels tile as fixed-size cells **aligned to each generating RoadSegment's
own heading**, not to a world-space axis-aligned grid. This is a deliberate
fix for a well-known city-builder failure mode: a world-space grid does not
follow diagonal or curved roads, leaving unusable jagged slivers. A
per-segment grid instead hugs the road it fronts regardless of its angle.

For each RoadSegment (in `RoadGraph::GetSegments()` order), for each side of
the segment (Left before Right, a fixed counterclockwise-rotation convention
used purely for deterministic ordering, not a real-world claim), cells tile
contiguously along the segment's direction with no gap between them. A
leftover span shorter than one full cell is simply not populated — no
undersized cell is generated. Depth extends away from the road, offset by the
setback, up to `ParcelMaxDepthRows` rows.

### `FParcel`

Each `FParcel` has a stable `FParcelId`, its generating `FRoadSegmentId` and
`ERoadSide`, a `ColumnIndex`/`RowIndex` position (in whole cells, along the
segment and away from the road respectively), and a `CellsWide`/`CellsDeep`
footprint (both at least one). Stage 5 generation always produces `1x1`
parcels; the footprint fields exist so a future feature can combine
contiguous `1x1` parcels into a larger footprint without a schema migration.
A Parcel may still contain at most one Building regardless of footprint size —
see [Domain Model](domain-model.md).

### Regeneration Semantics

`RegenerateParcels()` is a full replace: it recomputes the entire parcel set
from the current RoadGraph and discards the previous set, rather than
appending incrementally. Parcel ID allocation restarts at one on every call,
which is required for "the same road graph produces the same ordered parcel
set" to hold across repeated calls on an unchanged graph. A future feature
that gives a Building a persistent reference to a `FParcelId` will need its
own strategy for reconciling that reference across a regenerate; this is
recorded as an accepted open question in
[ADR 0011](decisions/0011-segment-aligned-roadside-parcels.md), not solved by
Stage 5.

### Accepted v0.1 Limitation

Because each RoadSegment generates its own independently-aligned grid, two
segments meeting at a shared RoadNode at different angles produce parcel
rectangles that are not mitered or trimmed against each other, and may
visually or geometrically overlap near intersections. This is accepted v0.1
scope under the documented non-goal of avoiding irregular subdivision
optimization; parcel validation does not attempt cross-parcel overlap
detection between parcels of different segments. See
[ADR 0011](decisions/0011-segment-aligned-roadside-parcels.md).

## Test Contract

### Implemented Coverage

Unreal Automation Tests currently cover:

- Empty City construction, validation, and summary
- Zero, positive, negative, and overflowing time advancement
- Known-seed RNG golden output
- Strong-ID invalid state, uniqueness, category safety, and exhaustion behavior
- Region, RoadType, and VehicleClass defaults and catalog validation
- Headless advancement without a World or viewport
- Valid and invalid RoadNode and RoadSegment commands
- Atomic command rejection without ID or state mutation
- Duplicate connections, dangling endpoints, cached lengths, and speed
  overrides
- Deterministic directional traversal ordering
- Repeatable graph construction and summary output
- Direct, multi-segment, and disconnected routes
- Known optimal A* routes
- Equal-cost deterministic tie resolution
- Departure-time-dependent route changes using a test cost provider
- Heuristic lower-bound and zero-heuristic behavior
- Vehicle-dependent feasibility and free-flow cost
- FIFO cost-provider acceptance
- Deterministic, repeatable parcel generation from an identical road graph
- Both-sides parcel generation with a centerline-symmetric setback
- Segment-aligned (not world-axis-aligned) tiling on a diagonal segment
- Leftover frontage shorter than one cell is skipped, not undersized
- Parcel depth capped at the configured maximum row count
- A too-short segment produces zero parcels without an error
- Regeneration replaces the parcel set rather than accumulating duplicates
- Automatic parcel regeneration after a successful road command
- Malformed hand-built parcel records reported for each parcel validation code
- RegionProfile parcel-sizing defaults and their validation

Tests must compare meaningful state and diagnostics rather than memory layout or
unordered container iteration.

## Deliberate Exclusions

Stages 1–2 do not add:

- Generalized command buses or event frameworks
- Save/load or schema migration
- Parallel simulation updates
- Traffic queues or congestion calibration
- Microscopic vehicles or MassEntity
- Road rendering, player interaction, or Unreal World ownership

Those systems must build on this contract rather than weaken its dependency
boundary.

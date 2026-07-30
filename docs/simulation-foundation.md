# Simulation Foundation Specification

## Status

The Stage 1 simulation skeleton in this document is implemented. The Stage 2
logical road graph and time-dependent routing sections remain an implementation
contract, not a claim that those types already exist.

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
| Simulation time | Signed 64-bit ticks |
| One v0.1 tick | One simulated minute |
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
- `FVehicleClassId`
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

Stage 1 configuration contains an explicit `uint64 Seed`. Construction does not
read wall-clock time, process state, or platform randomness.

Later configuration may add validated catalogs and system cadence values without
changing the explicit-seed requirement.

### `FSimulationTime`

`FSimulationTime` stores the current signed 64-bit tick. A new City starts at
tick zero. One tick represents one simulated minute.

Calendar displays derive minutes, hours, and days from ticks. The authoritative
clock does not store floating-point elapsed time.

### `FCitySimulation`

`FCitySimulation` is the authoritative facade and state owner. Stage 1 exposes:

- Construction from a validated `FSimulationConfig`
- Current seed and `FSimulationTime`
- `AdvanceTicks(int64 Count)`
- `Validate()`
- `GetSummary()`

`AdvanceTicks(0)` succeeds without changing state. Negative counts fail.
Advancement that would exceed the signed 64-bit tick range fails before
changing state.

Systems run in a documented deterministic order. Rendering frames and game
speed decide how many ticks to request but never alter what one tick means.

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

`FCitySummary` initially reports seed and current tick. Entity counts are added
as their systems become authoritative. Summary generation is read-only and
deterministically ordered.

## Stage 2 Road Graph

### Coordinates

`FSimPoint2D` stores finite `double X` and `double Y` values in meters. v0.1 is
flat. Future elevation and vertical alignment will be separate metric data
rather than an implicit Unreal Z coordinate.

### Nodes and Segments

A RoadNode contains its `FRoadNodeId` and `FSimPoint2D`.

A RoadSegment contains its `FRoadSegmentId`, two distinct endpoint IDs, and the
vehicle-independent definition needed to determine free-flow traversal
properties. Its length is the Euclidean endpoint distance.

One two-way segment exposes two directional RoadTraversals. Directional
traversal identity is the tuple of SegmentId, FromNodeId, and ToNodeId; v0.1
does not require a separately allocated traversal ID.

Stage 2 commands:

- Add one RoadNode at a validated point
- Add one RoadSegment between existing nodes

Commands return typed result records containing the created ID on success.
Segment creation rejects:

- Invalid or missing endpoint IDs
- The same endpoint at both ends
- Non-finite or zero-length geometry
- A duplicate connection between the same endpoints in v0.1
- ID exhaustion

Isolated valid nodes are permitted. Automatic snapping, geometric intersection
splitting, deletion, lanes, one-way roads, elevation, turn restrictions, and
road meshes are deferred.

## Time-Dependent A*

### Query and Result

A route query contains:

- Origin and destination RoadNode IDs
- Departure tick
- VehicleClass ID
- DriverProfile context when behavioral cost perception is requested

A successful route contains:

- Ordered directional RoadTraversals
- Ordered node IDs sufficient to validate continuity
- Total distance in meters
- Predicted arrival tick
- Total predicted travel ticks

Invalid endpoints, unsupported VehicleClasses, prohibited traversals, and
disconnected destinations return distinct failures.

### Cost Evaluation

A traversal-cost provider evaluates expected travel ticks using:

- Directional traversal
- Predicted entry tick
- VehicleClass
- Available historical and live traffic forecast

Stage 2 supplies a free-flow provider through this final time-aware interface.
Congestion-aware providers arrive with the traffic milestone.

Costs must be non-negative and satisfy FIFO: entering the same traversal later
cannot produce an earlier exit. The initial algorithm may reject a provider
that cannot make this guarantee.

### Heuristic

The A* heuristic is a lower bound on remaining travel ticks:

```text
straight-line distance to destination
──────────────────────────────────────
fastest feasible speed for the vehicle
```

The result is rounded down to whole ticks to preserve admissibility. The speed
bound ignores congestion but respects intrinsic VehicleClass limits. Predicted
traffic belongs in traversal costs, not in a heuristic that could overestimate.

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

## Stage 1–2 Test Contract

Unreal Automation Tests must cover:

- Empty City construction, validation, and summary
- Zero, positive, negative, and overflowing tick advancement
- Known-seed RNG golden output
- Strong-ID invalid state, uniqueness, category safety, and exhaustion behavior
- Headless advancement without a World or viewport
- Valid and invalid RoadNode and RoadSegment commands
- Duplicate connections and dangling endpoint validation
- Direct, multi-segment, and disconnected routes
- Known optimal A* routes
- Equal-cost deterministic tie resolution
- Departure-time-dependent route changes using a test cost provider
- Heuristic lower-bound and zero-heuristic behavior
- Vehicle-dependent feasibility and free-flow cost
- FIFO cost-provider acceptance

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

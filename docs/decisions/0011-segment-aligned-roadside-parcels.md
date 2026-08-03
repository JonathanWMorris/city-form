# 0011: Segment-Aligned Deterministic Roadside Parcels

## Status

Accepted

The original `1x1` generated-footprint decision is amended by
[ADR 0013](0013-road-fronting-default-parcel-footprints.md). Segment alignment,
the internal 8 m grid, setback, deterministic ordering, and accepted
intersection-overlap limitation remain in force.

## Context

Stage 5 zoning and buildings need stable logical land units derived from the
road graph before Zone or Building work can begin (issue #31). No prior
document or ADR specified parcel geometry, sizing, or trigger semantics.

A parcel grid could be world-axis-aligned (a single fixed grid across the
whole map) or aligned to each generating road segment's own heading. Real
city builders that use a world-axis-aligned grid — including Cities: Skylines
and Cities: Skylines II — draw a recurring, well-documented complaint: the
grid does not follow diagonal or curved roads, leaving jagged, unusable
leftover slivers along anything but an axis-aligned street. Community
proposals for alternative zoning systems (variable lot shapes, strip-based
zoning) exist specifically to work around this failure mode.

Parcels could be hardcoded to exactly one grid cell, or could store a
variable footprint from the start. A single Parcel already may contain at
most one Building (see [Domain Model](../domain-model.md)); a variable
footprint lets that one Building be bigger without changing that 1:1
relationship, by letting a Parcel itself span more than one cell — as
opposed to merging several independent Parcel records under one Building,
which this decision does not adopt.

Sizing constants could live per-RoadType, in a dedicated parcel-configuration
struct, or on `FRegionProfile` alongside the existing Basic Two-Way Road
speed default.

## Decision

Parcels tile in fixed cells (`FRegionProfile::ParcelCellSizeMeters`, default
`8.0`) **aligned to each generating RoadSegment's own heading** — computed
with the same direction math `FRoadGraph` already uses to derive segment
length — rather than to a world-space axis-aligned grid. This directly avoids
the researched "wasted space on diagonal roads" failure mode: a per-segment
grid stays flush against the road it fronts regardless of the road's angle.

Both sides of every segment long enough for at least one cell receive parcel
rows (Left before Right, a fixed counterclockwise-rotation convention chosen
purely for deterministic ordering, not a real-world claim), up to
`ParcelMaxDepthRows` (default `4`) deep, offset from the segment centerline by
`ParcelSetbackMeters` (default `4.0`). These three defaults intentionally
match real Cities Skylines' zoning-grid constants (8m cells, 4 rows deep).
`ParcelSetbackMeters` is defined independently of, and never references,
CityForm's visual road-placeholder strip width — the two currently coincide
numerically, but the simulation must not depend on a presentation constant
per the architecture's presentation-independence rule. Cells tile
contiguously along a segment with no gap; a leftover span shorter than one
full cell is simply not populated, never forced into an undersized cell.

`FParcel` stores `CellsWide`/`CellsDeep` (both at least one) in addition to
`RoadSegmentId`, `Side`, `ColumnIndex`, and `RowIndex`, even though Stage 5
generation only ever emits `1x1` parcels. This schema lets a future feature
identify and combine contiguous `1x1` parcels along a segment/side into a
larger footprint — for a bigger building — without a save-schema migration,
while leaving the existing "at most one Building per Parcel" invariant
completely unchanged.

`RegenerateParcels()` is a full, idempotent recompute of the entire parcel
set from current RoadGraph state, not an incremental per-segment append.
Parcel ID allocation resets to a fresh sequence on every call — required for
"the same road graph produces the same ordered parcel set" to hold across
repeated calls on an unchanged graph. It is automatically invoked after a
successful road command and is also safe to call directly at any time by
future callers (buildings, moves, mods).

Sizing defaults live on `FRegionProfile`, following the exact pattern of
`BasicTwoWayRoadDefaultSpeedLimitMetersPerSecond`, rather than a new
dedicated configuration type — keeping one place for all region-tunable
defaults.

This issue is scoped entirely to the headless `CitySimulation` module; the
Unreal-side bridge, snapshot, and visualization are explicitly deferred to
issue #34.

## Consequences

- Parcel geometry is computed and validated per-segment without a global
  search or spatial index.
- A future footprint-merging feature needs no new bookkeeping fields on
  `FParcel`.
- Sizing is centrally tunable per region without touching generation code.
- **Accepted limitation**: because each segment generates its own
  independently-aligned grid, two segments meeting at a shared RoadNode at
  different angles produce parcel rectangles that are not mitered or trimmed
  against each other, and may visually or geometrically overlap near
  intersections. This is accepted v0.1 scope under the documented non-goal of
  avoiding irregular subdivision optimization. `FParcelLayout::ValidateRecords`
  intentionally does not attempt cross-parcel overlap detection between
  parcels of different segments.
- **Open question, not solved here**: because `RegenerateParcels()` resets
  parcel IDs on every call to guarantee determinism and idempotency, a future
  feature that gives a Building a persistent reference to a `FParcelId` would
  have that reference silently invalidated by a naive full-replace
  regeneration. That future feature must define its own strategy — such as
  preserving IDs for geometrically-unchanged parcels, or reconciling Building
  references explicitly across a regenerate. This issue does not assume the
  problem is already solved — resolved by
  [ADR 0012](0012-persistent-parcel-identity-and-zoning-commands.md).
- A related, larger idea explored alongside this decision — letting zoned
  density be driven by demand and location rather than only by a painted
  zone tier — is tracked separately in
  [issue #89](https://github.com/JonathanWMorris/city-form/issues/89) and is
  not part of this decision.

## References

- [Domain Model](../domain-model.md)
- [Simulation Foundation: Stage 5 Roadside Parcels](../simulation-foundation.md#stage-5-roadside-parcels)
- [v0.1 Product Scope](../product-scope.md#roads-and-parcels)

## Supersedes

None.

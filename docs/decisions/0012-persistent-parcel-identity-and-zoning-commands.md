# 0012: Persistent Parcel Identity and Zoning Commands

## Status

Accepted

## Context

Issue #32 needs explicit, inspectable commands for a player to designate
Residential or Commercial zoning on a Parcel. ADR 0011 already flagged the
blocking problem this surfaces: because `RegenerateParcels()` reset every
`FParcelId` to a fresh `1..N` sequence on every call — required at the time
only to guarantee "the same road graph produces the same ordered parcel
set" — any persistent per-parcel state stored directly on `FParcel` would be
silently discarded the next time the parcel set regenerated. `RegenerateParcels()`
is auto-invoked after every successful `AddRoadSegment`/`CreateRoadSegment`,
so under the old scheme a player zoning a neighborhood and then drawing one
unrelated road anywhere else in the city would immediately scramble every
existing `FParcelId`, silently wiping the zoning they had just applied.

`FRoadGraph`'s own "Repeatability" test
(`CityForm.Simulation.RoadGraph.Repeatability`) only claims that identical
command sequences produce identical IDs — it does not claim that any graph
reaching the same end state produces the same IDs regardless of build order.
`FRoadNodeId`/`FRoadSegmentId` are monotonic, allocated once, and never reset
or reused; the old Parcel behavior was the outlier relative to every other ID
in this codebase, not the norm. No road-removal or road-edit command exists
anywhere in `CitySimulation` (confirmed by inspection), so a road segment's
set of candidate parcels is strictly append-only today — this bounds what
reconciliation must handle.

## Decision

`FParcelId` allocation becomes persistent and reconciling, matching how
`FRoadNodeId`/`FRoadSegmentId` already behave. `FParcelLayout` gains a
private, persistent `TStrongIdAllocator<FParcelId> ParcelIdAllocator`
member (no longer a fresh local per call). Each parcel's geometric identity
is expressed as a footprint key — `RoadSegmentId`, `Side`, `ColumnIndex`,
`RowIndex`, `CellsWide`, `CellsDeep` — independent of any allocated ID.
`RegenerateParcels()` now:

1. Computes fresh geometric candidates via a renamed, purely geometric
   `GenerateCandidates(Nodes, Segments, RegionProfile)` (previously
   `GenerateRecords`; the rename signals it no longer allocates identity —
   grep-confirmed it is called only internally, so the rename is risk-free).
2. Matches each candidate against the previous parcel set by footprint key.
   A match reuses that parcel's existing `FParcelId` and carries forward its
   `Zone`. No match allocates a new ID.
3. Gates the whole call on `ParcelIdAllocator.CanAllocate(NewCandidateCount)`
   **before** committing any change — necessary now that the allocator is
   persistent instance state, so a would-be exhaustion failure cannot
   silently burn ID space for candidates already resolved before the failure
   is detected. This preserves `RegenerateParcels()`'s existing documented
   atomic-failure contract.

`EZoneCategory { None, Residential, Commercial }` is added, and `FParcel`
gains a `Zone` field defaulting to `None`, appended as the struct's last
member — safe because every existing construction site uses positional
aggregate initialization with exactly as many elements as the prior field
count, and C++ allows a shorter positional list when trailing members have
default initializers.

`ApplyZone(FParcelId, EZoneCategory)` and `ClearZone(FParcelId)` are added to
`FParcelLayout` (which already owns `Parcels`/`ParcelIndexes`, mirroring how
`FRoadGraph` owns its own mutating commands), with thin `FCitySimulation`
pass-throughs. `ApplyZone` rejects `EZoneCategory::None` and any
unrecognized value (`ESimulationErrorCode::InvalidZoneCategory`) and rejects
an unknown `FParcelId` (`ESimulationErrorCode::InvalidParcel`), both checked
before any mutation. Rezoning is unrestricted — `ApplyZone` overwrites a
prior `Zone` unconditionally, since v0.1 has no capacity or Building yet to
conflict with a rezone. `ClearZone` on an already-unzoned parcel succeeds as
a no-op. A new `EValidationIssueCode::InvalidParcelZone` mirrors the existing
`InvalidParcelSide` check for a stored `Zone` outside its defined values.

`FCitySummary` gains `ResidentialParcelCount`, `CommercialParcelCount`, and
`UnzonedParcelCount`, computed read-only from `Parcels.GetParcels()` at
summary time, matching the existing `ParcelCount` pattern.

This scope stays entirely inside `CitySimulation`; the Unreal-side zoning
interaction and visuals are issue #34's job.

## Consequences

- Parcel IDs and Zone now survive any `RegenerateParcels()` call triggered
  elsewhere in the graph, resolving ADR 0011's open question and giving
  issue #33 (Buildings referencing `FParcelId`) and issue #34 (player zoning
  UX) a stable identity to build on instead of each having to solve this
  themselves.
- `RegenerateParcels()`'s atomic all-or-nothing failure contract is
  preserved via an explicit `CanAllocate` pre-check, now that ID allocation
  is persistent instance state rather than a call-local temporary.
- **Deferred, not solved here**: a future road-removal or road-edit-in-place
  feature will make a previously-seen footprint key able to disappear on a
  later regenerate. This reconciliation only ever matches or allocates; it
  does not define what happens to a parcel — and its Zone, and any future
  Building — whose footprint key stops appearing. That is explicitly left to
  whichever future issue introduces road removal.
- Zoning is deliberately unrestricted (freely overwritable, no
  `ClearZone`-first requirement) because v0.1 has no Building or capacity yet
  to conflict with a rezone; a future Building feature may need its own
  constraint on rezoning an occupied parcel, which this decision does not
  anticipate further.
- Two new `ESimulationErrorCode` values and one new `EValidationIssueCode`
  value are additive extensions of already-established naming patterns; no
  existing generic code was reusable, since every existing `Invalid*` code in
  this codebase is entity-specific.

## References

- [Domain Model: Zone](../domain-model.md#zone)
- [Simulation Foundation: Stage 5 Roadside Parcels](../simulation-foundation.md#stage-5-roadside-parcels)
- [ADR 0011: Segment-Aligned Deterministic Roadside Parcels](0011-segment-aligned-roadside-parcels.md)
- Issue #32

## Supersedes

None. ADR 0011 remains accepted for parcel geometry, grid alignment, and
sizing; this decision only replaces its per-call ID-reset behavior and adds
zoning on top of the resulting stable identity.

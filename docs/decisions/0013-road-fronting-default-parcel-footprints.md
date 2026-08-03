# 0013: Road-Fronting Default Parcel Footprints

## Status

Accepted

## Context

ADR 0011 established an 8 m segment-aligned grid and initially exposed every
`1x1` cell as an independently zonable Parcel. That was useful for proving
deterministic geometry, but it makes the internal alignment grid the public
land unit. An 8 m by 8 m record is too shallow to represent a practical
road-fronting lot and would force buildings to combine multiple Parcel
records, conflicting with the domain invariant of at most one Building per
Parcel.

The default should be plausible enough to support the first residential and
commercial placeholders without pretending to model every jurisdiction. A
16 m frontage is approximately 52.5 ft, close to the common 50 ft detached
residential lot width documented in Los Angeles R1 examples. A 32 m depth
produces a roughly 512 m² (5,511 ft²) lot.

## Decision

Keep the 8 m grid as an internal deterministic alignment unit, but generate
one logical Parcel per complete `2x4` footprint:

- `CellsWide == 2` and `CellsDeep == 4` by default;
- 16 m frontage and 32 m depth with the California profile;
- `RowIndex == 0`, so the full depth is one road-fronting development unit;
- both sides of each eligible segment, Left before Right;
- `ColumnIndex` advances by two cells from Endpoint A; and
- any segment-end remainder shorter than the full two-cell frontage is
  skipped rather than emitted as an undersized Parcel.

`FRegionProfile` exposes `ParcelDefaultWidthCells` and
`ParcelDefaultDepthCells` instead of `ParcelMaxDepthRows`. The existing 4 m
road-edge setback remains unchanged. Stable footprint-key reconciliation
continues to preserve IDs and zoning for unrelated regeneration.

This decision does not add corner trimming, intersection-aware subdivision,
irregular polygons, or a spatial overlap solver. Independently aligned lots
from different segments can still overlap near intersections, as already
accepted by ADR 0011.

## Consequences

- Zoning and development operate on recognizable road-fronting lots rather
  than internal grid cells.
- The default footprint can later vary by region without changing the Parcel
  schema.
- Short segments and incomplete end remnants may produce no Parcel.
- Changing the default footprint changes footprint keys. No save/runtime
  migration is needed because City Form has no compatibility guarantee or
  persisted city format at this pre-alpha stage.
- More realistic corner lots and subdivision remain future work driven by
  demonstrated gameplay needs.

## References

- [ADR 0011: Segment-Aligned Deterministic Roadside Parcels](0011-segment-aligned-roadside-parcels.md)
- [ADR 0012: Persistent Parcel Identity and Zoning Commands](0012-persistent-parcel-identity-and-zoning-commands.md)
- [Domain Model](../domain-model.md#parcel)
- [Simulation Foundation](../simulation-foundation.md#stage-5-roadside-parcels)
- [Los Angeles R1 zoning example](https://planning.lacity.gov/odocument/74731ab1-b675-47fb-b43a-e0749163876d/AA-2018-3529.pdf)

## Supersedes

The `1x1` Stage 5 generation footprint and `ParcelMaxDepthRows` configuration
parts of ADR 0011. All other ADR 0011 decisions remain accepted.

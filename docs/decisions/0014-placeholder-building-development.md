# 0014: Placeholder Building Development

## Status

Accepted

## Context

Stage 5 needs the smallest authoritative bridge between zoning and the homes
and jobs consumed by later population systems. Instant capacity would hide the
role of simulation time, while the long-term proposal, finance, resource, and
construction-capacity model is too broad for the first vertical slice.

Building identity and progress must remain in `CitySimulation`, independent of
meshes, loaded levels, or frame rate. Rezoning semantics also need to be
explicit before Buildings reference Parcels persistently.

## Decision

Add stable `FBuildingId` and `FBuildingTypeId` types plus a validated starter
catalog:

- `DetachedHouse`: Residential, one household capacity;
- `SmallCommercial`: Commercial, eight job capacity.

Successful Residential or Commercial zoning creates one authoritative
Building for the Parcel at the current simulation instant. Development has
three stages:

1. `Planned` for 120,000 simulated milliseconds;
2. `UnderConstruction` for 180,000 simulated milliseconds; and
3. `Complete` thereafter.

The durations and starter capacities live in a validated
`FDevelopmentConfig` within `FSimulationConfig`. Stage transitions derive from
stored timestamps and the authoritative clock. Capacity is zero until
completion, then matches the selected catalog type.

Applying the same Zone is idempotent and preserves Building identity and
progress. Rezoning an unoccupied placeholder creates a new Building ID and
restarts development with the compatible type. Clearing zoning removes the
placeholder. Stage 6 must replace these unconditional removal semantics before
households or businesses can occupy Buildings.

Zoning preflights parcel/category validity, type availability, ID capacity,
and both timestamp additions before mutating the Parcel or Building
collection. Rejected commands therefore leave both unchanged. Large clock
advances may cross multiple stages in one call.

## Consequences

- The simulation now exposes stable, inspectable development records and
  active household/job capacity without visual ownership.
- The simple type catalog creates a clean extension point for later building
  varieties without making the first rule demand-driven or probabilistic.
- The five-minute schedule proves authoritative time progression but is not a
  claim about realistic construction speed or final gameplay balance.
- Placeholder removal is safe only because occupants do not exist yet.
- Unreal visualization can consume read-only Building records in issue #34.

## References

- [Domain Model](../domain-model.md#building)
- [Simulation Foundation](../simulation-foundation.md#stage-5-placeholder-development)
- [Gameplay Time and Development Pacing](../gameplay-pacing.md)
- [ADR 0012: Persistent Parcel Identity and Zoning Commands](0012-persistent-parcel-identity-and-zoning-commands.md)

## Supersedes

None.

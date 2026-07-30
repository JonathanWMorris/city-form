# 0006: Configurable Regional Defaults with a California Baseline

## Status

Accepted

## Context

City Form needs concrete defaults before it supports a region-selection
interface or a library of road standards. Unexplained generic values would be
harder to calibrate and inspect, while hard-coding one jurisdiction throughout
the simulation would make future maps and community-created regions difficult.

The maintainer is most familiar with California. The California Driver's
Handbook identifies 25 miles per hour as the speed limit in business or
residential districts unless otherwise posted, making it a defensible baseline
for the first basic urban road.

## Decision

California is City Form's current real-world reference baseline for applicable
default data. It is represented by the stable region identifier `US-CA`.

Every City receives an `FRegionProfile` snapshot through
`FSimulationConfig`. Catalogs derive their applicable defaults from that
snapshot rather than consulting global state. Future Unreal map setup may
select a different profile before constructing the City.

The California `BasicTwoWayRoad` default speed is 25 miles per hour, stored in
authoritative SI units as exactly `11.176` meters per second. RoadSegments
reference a RoadType and may carry an optional explicit speed-limit override.

Changes to a baseline are reviewed data and documentation changes. They do not
silently follow changing external regulations.

## Consequences

- The first road values have a documented real-world reference.
- Different cities and maps can use different profiles without changing graph
  algorithms or relying on process-wide configuration.
- Simulation code remains unit-consistent while presentation can later display
  localized units.
- More regions and road types require validated data and selection UX, but not
  a replacement of RoadSegment identity or routing.
- California is a gameplay and calibration baseline, not a claim that every
  city follows one legal code.

## References

- [California Driver's Handbook: Laws and Rules of the Road](https://www.dmv.ca.gov/portal/handbook/california-driver-handbook/laws-and-rules-of-the-road-cont1/)

## Supersedes

None.

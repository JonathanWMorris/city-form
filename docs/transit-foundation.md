# Future Transit Foundation

## Status

Public transit is outside v0.1. This document records constraints that current
architecture must preserve for later bus, rail, subway, and connecting-service
simulation. It does not define current APIs or add a transit dependency.

## Goals

Future transit should support:

- Scheduled and headway-based service
- Civil-day and overnight service calendars
- Timepoints, dwell, recovery, layovers, and vehicle blocks
- Observable early, on-time, and late operation
- Headway-based holding and debunching
- Explicit transfer and guaranteed-connection policies
- Persistent passenger itineraries and missed connections
- Buses affected by road traffic and vehicle performance
- Rail and subway operation on dedicated guideways
- Headless, deterministic tests and inspectable metrics

## Time and Service Days

Transit shares the authoritative millisecond timeline. Scheduled service is
expressed as a service date plus an elapsed offset from that date's midnight.
Offsets may exceed 24 hours without a fixed upper bound such as 30 hours.

For example:

```text
Monday service departure: 25:15
Civil date and time:       Tuesday 01:15
Owning service date:       Monday
```

Actual arrivals, departures, holds, and disruptions use absolute simulation
instants. Scheduled and actual times remain separate so lateness and headway
metrics can be explained.

Transit operations should be event-driven where practical. Arrivals,
departures, timepoint releases, connection deadlines, and dispatch decisions
are scheduled events; they do not require every route, stop, or passenger to
run at the traffic cadence.

## Operations

A future domain model will need concepts equivalent to:

- Stop and station
- Route, direction, and stop pattern
- Scheduled trip or run
- Vehicle block containing an ordered sequence of runs
- Service calendar
- Scheduled and actual stop arrival and departure
- Timepoint and control stop
- Layover and recovery allowance
- Passenger itinerary and transfer

Concrete names and storage layouts remain deferred.

An early vehicle may wait when it reaches a timepoint before its scheduled
departure. A late vehicle normally proceeds when boarding and applicable
policies permit. Connection guarantees require explicit maximum holds and
exceptions rather than an implicit rule that every service waits.

Debunching should compare observed headways with target headways and issue
inspectable control actions, initially controlled holding at eligible stops.
Short turns, express operation, stop skipping, and other intervention strategies
are later extensions.

## Vehicles, Networks, and Routing

Buses use the authoritative road network and share road congestion. Existing
time-dependent A* remains appropriate for a bus's road path, including its
VehicleClass dimensions and performance.

Rail and subway vehicles require authoritative guideway topology rather than
being forced onto the road graph. Track blocks, junctions, platform occupancy,
and signaling may later constrain their movement.

Passenger journey planning is a separate problem from vehicle road routing. It
must combine walking access, waiting, scheduled service, transfers, predicted
operation, and final access. A timetable-oriented algorithm such as RAPTOR or
the Connection Scan Algorithm is a better future foundation for that layer,
with A* retained for road and access legs where appropriate.

## Passengers and Scaling

Individual citizens retain authoritative trip intent, itinerary, boarding,
alighting, missed-connection, and outcome records. They are not Actors and do
not require continuous per-frame movement.

At a stop event, boarding and alighting may be processed as deterministic
batches while preserving individual identities and outcomes. Itinerary
requests may be staggered, cached, or share reusable results when inputs are
equivalent. These optimizations must not make passenger existence or service
quality depend on camera visibility.

Visible buses, trains, and passengers are interpolated presentations of
authoritative state. Culling or unloading them cannot pause service or erase a
passenger.

## Future Validation and Metrics

Transit validation should eventually cover:

- Valid stop patterns, service calendars, trips, blocks, and vehicle references
- Monotonic stop sequences and feasible scheduled times
- No boarding beyond applicable capacity
- Valid itinerary legs and transfers
- Stable event ordering at equal instants
- Correct extended service-day conversion across civil midnight
- Repeatable dispatch and passenger outcomes for controlled scenarios

Initial operational metrics should include scheduled and actual arrival,
departure, dwell, headway, holding, passenger wait, denied boarding, missed
connection, load, and travel time.

## v0.1 Boundary

Nothing in this document moves transit into v0.1. Current work should preserve
the simulation/presentation dependency direction, authoritative time precision,
vehicle-aware road routing, scheduled-event capability, and compact persistent
records needed by a later transit milestone.

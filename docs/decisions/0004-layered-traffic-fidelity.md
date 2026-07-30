# 0004: Layered Traffic Fidelity

## Status

Accepted

## Context

City Form should eventually model detailed vehicle behavior while also
supporting large cities, headless simulation, fast-forwarding, and hardware
below workstation class.

Implementing microscopic lane and vehicle behavior across the entire city from
the first milestone would expand road, junction, physics, and performance scope
before trip demand exists. Treating an early aggregate traffic model as
disposable would create duplicated authority and migration work later.

## Decision

Trip demand, route choice, and completion are authoritative strategic systems
independent of traffic fidelity.

v0.1 will use permanent mesoscopic propagation citywide. It advances individual
Trips through directional queues and publishes observed traversal times without
simulating continuous vehicle motion.

Future microscopic propagation may take responsibility for selected Trips or
regions behind an explicit boundary. It consumes the same Trips, Routes,
DriverProfiles, VehicleClasses, and forecasts, and reports through the same
observation interface.

Mesoscopic propagation remains available for distant regions, fast-forwarding,
headless testing, and citywide scaling.

## Consequences

- Early traffic code remains a supported fidelity tier rather than prototype
  clutter.
- Visual vehicles never become the authority for trip demand.
- Fidelity transfer requires explicit state and validation boundaries.
- Microscopic road details can be introduced when their requirements are known.
- Cross-fidelity consistency becomes a future testing responsibility.
- MassEntity is an optional future implementation tool, not a domain model.

## Supersedes

None.

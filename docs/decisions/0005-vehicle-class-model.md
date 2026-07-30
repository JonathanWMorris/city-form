# 0005: Shared Vehicle-Class Definitions

## Status

Accepted

## Context

Passenger cars, trucks, buses, and service vehicles differ in dimensions,
capacity consumption, permitted roads, speed, acceleration, braking, turning,
and grade performance.

Embedding those differences only in visual vehicle classes would prevent
headless routing and mesoscopic traffic from modeling them. Reducing every
difference to one speed multiplier would incorrectly conflate size, weight,
performance, driver behavior, and roadway conditions.

## Decision

Trips reference a validated, read-only VehicleClass definition shared by
routing, mesoscopic propagation, and future microscopic simulation.

Definitions reserve explicit SI-unit fields for dimensions, effective queue
length, mass, speed, acceleration, braking, turning radius, capacity equivalent,
restriction categories, and performance category.

Driver preferences remain in a separate DriverProfile.

v0.1 gameplay generates only passenger-car demand. Generic test definitions may
exercise heavier or restricted classes. Trucks enter gameplay alongside a
meaningful freight or delivery demand system after v0.1.

## Consequences

- Vehicle-aware routing and capacity accounting are possible from the start.
- Future microscopic Vehicles reuse the same class identity and physical data.
- Not every reserved property must affect v0.1 behavior.
- Vehicle defaults require units, validation, documentation, and calibration.
- Freight and vehicle ownership remain separate domain decisions.
- Mass alone does not impose a universal speed penalty; applicable performance
  and road conditions determine behavior.

## Supersedes

None.

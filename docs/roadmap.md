# Roadmap

This roadmap orders work toward the v0.1 technical alpha. It deliberately avoids
calendar estimates until the project has enough contribution history to make
them meaningful.

Each stage should finish with a working, reviewable result. Later stages may be
refined as evidence emerges, but v0.1 scope changes belong in
[product-scope.md](product-scope.md).

## Stage 0: Project Foundation

Status: Complete

Deliver:

- A clean Unreal Engine 5.8 C++ project
- Unreal-aware ignore rules and Git LFS configuration
- Project values, v0.1 scope, architecture, roadmap, and contribution guidance

Exit criteria:

- A new contributor can understand the project status, intended first release,
  and architectural boundary from the repository.
- Generated Unreal files do not appear as source-control changes.

## Stage 1: CitySimulation Skeleton

Status: Complete

Deliver:

- A separate `CitySimulation` runtime module
- A minimal city state, simulation clock, and deterministic seed
- Strong typed ID foundations
- A validated passenger-car VehicleClass foundation
- Headless automation-test support
- A validation entry point and summary metrics

Exit criteria:

- The module builds without depending on `CityForm` presentation code.
- A test creates a city, advances time, validates it, and produces repeatable
  summary output for the same seed.

## Stage 2: Logical Road Graph

Status: Complete

Completed:

- Road nodes and segments with strong IDs
- Regional defaults and a validated RoadType catalog
- Commands to add the smallest useful road topology
- Directional traversals and graph validation
- Tests for valid and invalid graph construction, deterministic traversal
  ordering, and headless repeatability
- Millisecond timestamps and durations with subsystem-specific cadences
- Vehicle-aware, time-dependent A* using free-flow costs
- Route tests for direct, multi-segment, disconnected, known-optimal, and
  equal-cost cases

Exit criteria:

- Tests create and route through a small graph without a loaded level.
- Dangling endpoints and invalid references are rejected or reported clearly.
- Known optimal and equal-cost routes are resolved repeatably.

## Stage 3: Prototype Environment

Status: Complete

Completed:

- A project-owned two-kilometer flat test map with documented metric bounds
- A smooth ground-focused city-builder camera
- Keyboard, mouse, edge-scroll, and trackpad-scroll navigation
- Minimal project-owned input and game-mode configuration
- Verified macOS editor and game builds, PIE startup, presentation tests, and
  headless simulation independence

Exit criteria:

- The project launches into the prototype map.
- A user can navigate the buildable area without character controls.
- Simulation state remains independent of the map and camera.

The prototype validates the coordinate and buildable-area contract in
[Map Foundation](map-foundation.md). The current bindings are documented in
[City-Builder Camera](camera-controls.md). Procedural terrain, tile acquisition,
regional gateways, and production-scale streaming remain future work.

## Stage 4: Interactive Roads

Status: In progress

The bridge and first connected-road interaction are implemented. The Road tool
uses a code-built palette, a two-click preview, endpoint-only screen-space
snapping, and one atomic simulation command. Persistent derived road visuals
remain before this stage is complete.

Deliver:

- A game-instance-owned bridge between Unreal and the authoritative simulation
- Explicit meter-to-centimeter conversion and detached road graph snapshots
- Ground raycasting from the player cursor
- Two-click road-segment creation and a live preview
- Endpoint snapping sufficient to connect segments into a routable graph
- Translation from player input into simulation road commands
- Simple derived road visuals

Exit criteria:

- Roads drawn in the viewport create valid logical nodes and segments.
- Rebuilding or removing the visual representation does not modify logical road
  state.

## Stage 5: Parcels, Zoning, and Development

Deliver:

- Simple parcels beside eligible roads
- Residential and commercial zoning
- Placeholder residential and commercial buildings
- Home, household, business, and job capacities

Exit criteria:

- Eligible zoning produces development predictably.
- All generated references validate and capacities cannot become negative.

## Stage 6: Population and Employment

Deliver:

- Lightweight households and residents
- Move-in behavior constrained by home capacity
- Businesses and jobs constrained by commercial capacity
- Employment assignment with inspectable summary metrics

Exit criteria:

- A representative city supports approximately 1,000 persistent residents.
- Assignments are valid, repeatable where promised, and do not exceed capacity.

## Stage 7: Trips and Congestion

Deliver:

- Home-to-work and work-to-home trip generation
- Passenger-car VehicleClasses and bounded DriverProfiles
- Historical and live traversal-time forecasting
- Time-dependent A* routing and bounded node-based rerouting
- Global microscopic-lite vehicles with continuous traversal progress
- Fixed-step ballistic movement, leader following, and simplified node
  admission
- A readable utilization or congestion view

Exit criteria:

- Changing the road layout or job distribution changes route usage.
- Learned and live conditions can change predicted routes by departure time.
- Every active vehicle progresses independently of visibility or loaded level.
- Seeded driver behavior avoids synchronized route-choice oscillation.
- Headless tests validate trips, queues, forecasts, and routes.

## Stage 8: v0.1 Visualization and Stabilization

Deliver:

- A limited number of placeholder vehicles derived from abstract trips
- Minimal UI needed to use and inspect the complete loop
- A reproducible v0.1 scenario and recorded performance baseline
- Updated setup, architecture, and contributor documentation

Exit criteria:

- Every item in the
  [v0.1 definition of done](product-scope.md#definition-of-done) passes.
- The verified macOS build completes the full roads-to-traffic loop.
- Known limitations and performance measurements are published.

## Cross-Cutting Requirements

Every stage must:

- Preserve the `CityForm → CitySimulation` dependency direction
- Add validation and automated tests alongside authoritative systems
- Keep changes small enough to review and explain
- Avoid new plugins and dependencies without documented justification
- Profile before introducing specialized scaling technology
- Update documentation when a public behavior or architectural decision changes

## Beyond v0.1

Potential later milestones include save/load with migrations, advanced object
editing, construction-constraint overrides, lane and junction management,
freight and truck demand, detailed signals and vehicle control, stochastic
traffic incidents, public transit, utilities and services, a deeper economy,
modding and research interfaces, million-citizen benchmark scenarios, improved
visuals, and packaged releases.

Their order is intentionally not fixed until v0.1 provides evidence about the
simulation, player experience, performance, and contributor priorities.

Accepted forward-looking constraints for calendar pacing, development, and
transit are recorded in
[Gameplay Time and Development Pacing](gameplay-pacing.md) and
[Future Transit Foundation](transit-foundation.md). These documents preserve
requirements without moving either system into the v0.1 milestone sequence.

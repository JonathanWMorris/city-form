# v0.1 Product Scope

## Release Definition

City Form v0.1 is a source-first technical alpha. It proves that a small city
can progress through one coherent loop:

**Draw roads → zone parcels → buildings appear → households move in →
businesses provide jobs → residents commute → traffic emerges**

It is not intended to be a feature-complete game, a polished public release, or
a stable modding platform. It is the smallest useful foundation on which those
things can be built.

## Intended Experience

The player starts with a flat, empty prototype map and an unlimited sandbox.
They create a simple road network, zone nearby parcels, observe placeholder
development, and run the simulation long enough for homes, jobs, commuting, and
road usage to become visible.

There is no budget, progression tree, win condition, or failure condition. The
purpose is to build, observe, inspect, and validate the core feedback loop.

## Included in v0.1

### Prototype Environment and Interaction

- One flat project-owned test map
- A ground-focused city-builder camera
- Basic controls for selecting road and zoning tools
- Minimal status information needed to observe the simulation

### Roads and Parcels

- One basic road type
- Two-point road-segment creation
- A logical graph of road nodes and segments, separate from rendered geometry
- Simple rendered or debug geometry for roads
- Simple parcels generated beside eligible roads

Intersections only need enough behavior to form a valid routable graph. Lanes,
signals, sidewalks, road elevation, detailed construction constraints, and
production-quality meshes are deferred.

### Zoning and Development

- Residential and commercial zoning
- Automatic placeholder development on eligible parcels
- Residential buildings with household capacity
- Commercial buildings with job capacity
- Cube or similarly simple placeholder visuals

### Population, Employment, and Trips

- Approximately 1,000 lightweight persistent residents
- Households assigned to available homes
- Businesses and jobs associated with commercial buildings
- Employed residents assigned to valid jobs
- Abstract home-to-work and work-to-home trips
- Routes computed through the logical road graph

Residents and authoritative trips exist independently of visible pedestrians or
vehicles.

### Traffic Feedback

- Road usage derived from routed trips
- A readable congestion or utilization visualization
- A small number of placeholder visible vehicles only after abstract trips are
  functioning

Visible vehicles illustrate simulation state; they are not the authoritative
source of traffic demand.

### Simulation Quality

- A deterministic seed where practical
- A simulation clock that can advance without the editor viewport
- Validation for references, capacities, routes, and other core invariants
- A repeatable headless scenario suitable for automated testing and profiling

## Definition of Done

v0.1 is complete when a contributor can build the project from a clean checkout
on the verified macOS toolchain and demonstrate all of the following:

1. Create a connected road network on the prototype map.
2. Apply residential and commercial zoning to generated parcels.
3. Observe homes and businesses appear with valid capacities.
4. Populate the city with roughly 1,000 persistent residents.
5. Assign eligible residents to valid jobs without exceeding capacity.
6. Generate and route commuting trips through the logical road graph.
7. Observe road utilization change in response to city layout and trips.
8. Run the equivalent simulation scenario without a loaded gameplay viewport.
9. Repeat a defined scenario with the same seed and obtain the same validated
   simulation result where deterministic behavior is promised.
10. Complete the scenario without invalid references or negative capacities.

Performance measurements from this scenario must be recorded before the
milestone is declared complete. v0.1 establishes a baseline; it does not claim
a final city-size or hardware target.

## Explicit Non-Goals

The following are outside v0.1:

- Budgets, taxation, loans, unlocks, or detailed economics
- Persistent save files or save-version compatibility
- Public transit, utilities, education, crime, weather, or politics
- Detailed pedestrian simulation or animation
- Realistic vehicle physics
- Detailed road engineering, traffic signals, or lane management
- Multiplayer, GIS import, or a public modding API
- Production-quality buildings, roads, terrain, or procedural skyscrapers
- A giant map or production-scale city
- MassEntity integration without profiling evidence
- Packaged end-user releases

## Forward-Looking Capabilities

Popular city-building mods demonstrate needs that City Form should ultimately
address in the base experience:

- Fine-grained traffic, lane, junction, and priority management
- Optional construction-constraint overrides
- Direct selection, movement, rotation, and adjustment of placed objects

v0.1 implements only basic road creation and zoning. Its logical data,
identities, and command boundaries must not make these later capabilities
unreasonably difficult.

## Persistence and Compatibility

v0.1 cities live in memory. The simulation must still use stable typed
identities and keep persistence concerns out of Actors and rendered state.

A future persistence milestone will define a versioned save schema, migration
rules, compatibility guarantees, and failure behavior. Until that document
exists, internal simulation structures are not a public save format and may
change freely.

## Platform and Distribution

v0.1 is developed, built, and verified on macOS with Apple Silicon. Code should
use portable standard C++ and Unreal APIs so Windows contributors can build and
validate it, but Windows support must not be claimed until it has been tested.

The milestone is source-first. Packaged builds are deferred until the loop and
its performance characteristics are stable.

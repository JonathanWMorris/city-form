# City Form

City Form is an open-source city-building and urban simulation project built
with Unreal Engine. It aims to make the systems that shape a city coherent,
inspectable, scalable, and enjoyable to control.

> [!IMPORTANT]
> City Form has completed its headless Stage 2 road graph and routing
> foundation. `CitySimulation` supports validated regional road defaults,
> logical topology, millisecond time, and vehicle-aware time-dependent A*, but
> traffic progression and city content do not exist yet. Stage 3 is complete
> with a project-owned flat prototype map and verified C++ city-builder camera.
> Stage 4 is complete with a game-instance-owned Unreal-to-simulation bridge,
> a bottom two-layer road menu, connected two-click placement, endpoint
> snapping, live preview, and persistent placeholder roads rebuilt from
> detached logical-graph snapshots. Stage 5 is underway with deterministic,
> segment-aligned 16 m × 32 m road-fronting parcel generation and
> residential/commercial zoning commands.

## Current Focus

Development is in
[Stage 5: Parcels, Zoning, and Development](https://github.com/JonathanWMorris/city-form/issues/30).
The parcel-footprint refinement is tracked in
[issue #92](https://github.com/JonathanWMorris/city-form/issues/92). The next
implementation packet after that foundation is
[placeholder buildings, development rules, and capacities](https://github.com/JonathanWMorris/city-form/issues/33).
GitHub issues are the task-level todo list; this repository documentation
records accepted behavior, architecture, and milestone outcomes.

## Vision

The first meaningful gameplay loop is intentionally focused:

**Draw roads → zone parcels → buildings appear → households move in →
businesses provide jobs → residents commute → traffic emerges**

The immediate goal is not feature parity with an established city builder. A
small, understandable simulation is more valuable than a broad collection of
disconnected systems.

## Project Values

- **Player and creator agency:** powerful editing, transparent rules, and
  extensibility without requiring mods to make the base game usable.
- **Long-term scalability:** architecture that can grow from a small prototype
  to a large city without tying simulation truth to visual detail.
- **Attainable performance:** strong performance on conventional consumer
  gaming hardware, not only workstations or flagship machines.
- **Community first:** public decisions, approachable documentation, reviewable
  changes, contributor credit, and minimal unnecessary dependencies.

Read the complete decision rules in [Project Values](docs/project-values.md).

## v0.1 Technical Alpha

The first release target is an unlimited, in-memory sandbox containing a flat
prototype map, a city-builder camera, one road type, parcels, residential and
commercial zoning, placeholder buildings, approximately 1,000 lightweight
residents, employment, commuting, routing, and congestion visualization.

Budgets, save files, public transit, utilities, detailed economics, polished
graphics, and advanced editing or traffic-management tools are outside v0.1.
Their future requirements still influence the foundational design.

Traffic will advance authoritative passenger-car vehicles through one global,
microscopic-lite model. Time-dependent A* will combine learned time-of-day
conditions with live traffic while rendering remains a separate interpolated
view of the same citywide truth.

See [v0.1 Product Scope](docs/product-scope.md) for the complete boundary and
acceptance criteria.

## Architecture

City Form uses one Unreal project with a strict intended dependency direction:

```text
CityForm (Unreal presentation and player tools)
    └── depends on CitySimulation (authoritative city state)
```

Persistent citizens, households, businesses, and trips will be compact
simulation records rather than Actors. The Unreal layer will visualize that
state and send commands back to the simulation. Simulation progress must not
depend on a loaded level or editor viewport.

See [Architecture](docs/architecture.md) for the module boundaries, data flow,
determinism rules, performance strategy, and future persistence boundary.

## Documentation

| Document | Purpose |
| --- | --- |
| [Project Values](docs/project-values.md) | Principles used to resolve design tradeoffs |
| [v0.1 Product Scope](docs/product-scope.md) | Included features, non-goals, and definition of done |
| [Domain Model](docs/domain-model.md) | Shared vocabulary, ownership, and entity relationships |
| [Architecture](docs/architecture.md) | Simulation boundaries and technical constraints |
| [Map Foundation](docs/map-foundation.md) | Prototype coordinates and future extensible-world contract |
| [City-Builder Camera](docs/camera-controls.md) | Mouse, keyboard, and trackpad navigation controls |
| [Road Placement Tool](docs/road-tools.md) | Current road-building controls, contracts, and limitations |
| [Simulation Foundation](docs/simulation-foundation.md) | Current APIs and planned routing contract |
| [Traffic Model](docs/traffic-model.md) | Global microscopic traffic, prediction, and future incidents |
| [Gameplay Pacing](docs/gameplay-pacing.md) | Accelerated calendar and ideal construction targets |
| [Future Transit Foundation](docs/transit-foundation.md) | Requirements for realistic multimodal transit |
| [Decision Records](docs/decisions/README.md) | Context and consequences of accepted architecture choices |
| [Roadmap](docs/roadmap.md) | Ordered milestones and exit criteria |
| [Unreal MCP Workflow](docs/unreal-mcp.md) | Optional editor automation setup and troubleshooting |
| [Contributing](CONTRIBUTING.md) | Setup, workflow, testing, and contribution expectations |
| [Code of Conduct](CODE_OF_CONDUCT.md) | Community standards and confidential reporting |

Repository Markdown is the source of truth. GitHub issues can be used for
proposals before accepted decisions are reflected here.

## Current Repository

```text
CityForm/
├── Build/Mac/Resources/   # Unreal-generated macOS project resources
├── Config/                # Initial Unreal project configuration
├── Content/Input/         # Project-owned Enhanced Input assets
├── Content/Maps/          # Project-owned Unreal levels
├── Source/CityForm/       # Unreal presentation and gameplay module
├── Source/CitySimulation/ # Headless authoritative simulation module
├── Source/*.Target.cs     # Game and editor build targets
└── CityForm.uproject      # Unreal Engine 5.8 project descriptor
```

Generated directories such as `Binaries`, `DerivedDataCache`, `Intermediate`,
and `Saved` are excluded from version control. Unreal binary assets and common
large source-asset formats use Git LFS.

## Getting Started

City Form is source-first pre-alpha software. There is no packaged release or
compatibility guarantee yet; contributors currently build the project from
source with Unreal Engine 5.8.

### Requirements

- [Git LFS](https://git-lfs.com/)
- Unreal Engine 5.8, installed separately under Epic Games' terms
- A C++ development toolchain supported by Unreal Engine on your platform

The project is currently developed and verified on an Apple Silicon Mac.
Windows portability is a design requirement, but Windows has not yet been
verified against this baseline.

### Clone and open the project

```sh
git lfs install
git clone https://github.com/JonathanWMorris/city-form.git
cd city-form
git lfs pull
```

Open `CityForm/CityForm.uproject` in Unreal Engine 5.8. If prompted, allow
Unreal to generate platform-specific project files and build the `CityForm`
module.

## Contributing

Contributions from programmers, designers, researchers, artists, testers, and
technical writers are welcome. Please read [CONTRIBUTING.md](CONTRIBUTING.md)
and the [Code of Conduct](CODE_OF_CONDUCT.md) before beginning substantial
work. Setup and architecture questions belong in
[GitHub Discussions Q&A](https://github.com/JonathanWMorris/city-form/discussions/categories/q-a),
while early ideas can be explored in
[GitHub Discussions Ideas](https://github.com/JonathanWMorris/city-form/discussions/categories/ideas).

## License

This repository is licensed under the [MIT License](LICENSE). Unreal Engine is
a separately licensed dependency and is not distributed under the MIT License;
contributors must obtain it directly from Epic Games.

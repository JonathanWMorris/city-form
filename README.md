# City Form

City Form is an open-source city-building and urban simulation project built
with Unreal Engine. Its goal is to make the systems that shape a city coherent,
inspectable, and scalable while presenting them through a modern 3D game.

> [!IMPORTANT]
> City Form is at the blank-project stage. The Unreal project opens and runs,
> but no gameplay systems, custom levels, simulation modules, or custom assets
> have been implemented yet.

## Vision

The first meaningful gameplay loop is intentionally focused:

**Draw roads → zone parcels → buildings appear → households move in →
businesses provide jobs → residents commute → traffic emerges**

Longer term, City Form is intended to support editable transportation networks,
procedural parcels and zoning, persistent households and businesses, traffic
and public transportation, land value and employment systems, utilities and
city services, large cities with scalable simulation detail, and modding or
research-oriented interfaces.

The project is inspired by games such as *Cities: Skylines*, but the immediate
goal is not feature parity. A small, understandable simulation is more valuable
than a broad collection of disconnected systems.

## Architecture

City Form will use one Unreal project with a clear boundary between simulation
truth and its visual presentation:

```text
CityForm (Unreal presentation and player tools)
    └── depends on CitySimulation (authoritative city state)
```

The planned `CitySimulation` module will contain data-oriented C++ systems for
roads, zoning, buildings, households, businesses, trips, the economy, and
simulation time. It should be deterministic where practical and able to run
without a loaded level or editor viewport.

The `CityForm` Unreal layer will handle cameras, input, terrain, meshes,
procedural visuals, visible vehicles and pedestrians, effects, UI, and editing
tools. It will read simulation snapshots or events and send player commands
back to the simulation.

A persistent citizen or authoritative trip should not normally be an Unreal
Actor. Nearby pedestrians and vehicles may temporarily visualize those records.
MassEntity may be evaluated later if profiling demonstrates a need, but it is
not part of the initial architecture.

## First Playable Milestone

The first milestone is a narrow vertical slice:

- A flat prototype map and city-builder camera
- One basic road type drawn between two points
- A logical road graph independent of rendered geometry
- Simple parcels beside roads
- Residential and commercial zoning
- Placeholder buildings with home or job capacity
- Roughly 1,000 lightweight simulated residents
- Employment assignment and home-to-work trips
- Routing through the road graph
- A visualization of road usage or congestion
- A small number of visible placeholder vehicles after abstract trips work

Detailed graphics, public transit, utilities, pedestrians, realistic vehicle
physics, multiplayer, GIS imports, and a detailed economy are deliberately
outside this milestone.

## Current Repository

The repository currently contains:

```text
CityForm/
├── Build/Mac/Resources/   # Unreal-generated macOS project resources
├── Config/                # Initial Unreal project configuration
├── Source/CityForm/       # Blank primary C++ game module
├── Source/*.Target.cs     # Game and editor build targets
└── CityForm.uproject      # Unreal Engine 5.8 project descriptor
```

Generated directories such as `Binaries`, `DerivedDataCache`, `Intermediate`,
and `Saved` are intentionally excluded from version control. Unreal binary
assets and common large source-asset formats are configured for Git LFS.

## Getting Started

### Requirements

- [Git LFS](https://git-lfs.com/)
- Unreal Engine 5.8, installed separately under Epic Games' terms
- A C++ development toolchain supported by Unreal Engine on your platform

The project is currently developed and verified on an Apple Silicon Mac.
Windows is an intended development and release target, but has not yet been
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

## Development Principles

- Keep authoritative simulation state independent of rendering and loaded
  levels.
- Prefer compact persistent records over one heavyweight Actor per simulated
  entity.
- Keep systems inspectable, testable, and deterministic where practical.
- Allow simulation and rendering to update at different rates.
- Use C++ for simulation and reusable systems; use Blueprints where visual
  assembly and rapid iteration are beneficial.
- Keep changes small and avoid speculative frameworks.
- Do not add dependencies, plugins, or platform-specific code without a clear
  need and portability boundary.
- Preserve compatibility with macOS on Apple Silicon and Windows.

Contributions are welcome as the project takes shape. Before proposing a large
feature, open an issue to align it with the current milestone and architectural
direction.

## License

This repository is licensed under the [MIT License](LICENSE). Unreal Engine is
a separately licensed dependency and is not distributed under the MIT License;
contributors must obtain it directly from Epic Games.

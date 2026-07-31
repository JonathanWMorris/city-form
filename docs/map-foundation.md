# Map Foundation

## Purpose

This document defines the spatial contract shared by City Form's prototype map
and future map systems. It separates authoritative city geometry from Unreal
presentation and records the intended path toward community-authored and
procedurally extensible worlds.

The v0.1 implementation remains deliberately small. It does not implement
terrain generation, land purchasing, streaming, outside demand, or persistence.

## Prototype Map Contract

The project-owned `/Game/Maps/Prototype` level is a flat two-kilometer square
test environment centered on the world origin.

| Property | Contract |
| --- | --- |
| Visual ground | `-1,000 m` through `+1,000 m` on X and Y |
| Buildable area | `-900 m` through `+900 m` on X and Y |
| Edge buffer | `100 m` on every side |
| Ground elevation | `0 m` |
| Unreal conversion | `1 simulation meter = 100 Unreal centimeters` |
| Axis mapping | Simulation X/Y maps directly to Unreal X/Y |
| Origin | Simulation `(0, 0)` maps to Unreal `(0, 0, 0)` |

The edge buffer keeps roads, parcels, and placeholder visuals from immediately
overhanging the visible ground. Later editing tools must validate against the
buildable bounds rather than inferring permission from a successful ground
raycast.

The level owns presentation only: collision ground, lighting, sky, and player
spawn configuration. It must not own authoritative roads, parcels, citizens,
trips, time, regional configuration, or procedural-generation state.

v0.1 authoritative geometry remains planar. Future elevations and vertical
road alignments use explicit metric data; they must not be inferred from Actor
transforms or an implicit Unreal Z convention.

## Spatial Layers

Future maps distinguish three boundaries that may have different extents:

1. **Generated terrain** is land available for preview or later acquisition.
2. **Jurisdiction** is the contiguous set of gameplay tiles owned by the
   player.
3. **Detailed city simulation** covers the entire jurisdiction globally and
   never changes fidelity because of the camera or loaded visual regions.

Gameplay purchase tiles are logical ownership units. They are not Unreal
streaming cells, rendering chunks, simulation partitions, or zoning parcels.
Those implementation units may use different sizes and may change after
profiling without changing land ownership.

The prototype's two-kilometer surface validates coordinates and interaction;
it does not select the eventual gameplay tile size.

## Procedurally Extensible Worlds

The long-term procedural world is extensible rather than literally infinite.
A new city generates a sizable initial region around its starting jurisdiction.
At least one complete ring of unowned land should be available for preview.
The precise initial radius and gameplay tile size are tuning decisions that
require a later performance and playability prototype.

Additional contiguous tiles generate on demand as the frontier expands.
Generation must be a deterministic function of:

- A versioned world seed
- Stable integer tile coordinates
- A versioned generator definition
- Explicit map or regional configuration

Generating tiles in a different order must not change their unmodified result.
Once revealed, a tile's generated base remains stable. Save data will eventually
record generated versions and player changes separately so generator updates do
not silently rewrite an existing city.

Land acquisition expands through tiles adjacent to the existing jurisdiction.
Disconnected ownership and satellite jurisdictions are deferred until their
service, economy, and routing consequences are designed explicitly.

## Regional Corridors and Outside Connections

An outside connection is a logical relationship, not a permanent map-edge
Actor. A stable regional corridor represents a highway, railway, shipping
route, or air connection leading to a persistent abstract destination such as
a named neighboring city or wider region.

Each corridor has an identity independent of its temporary gateway at the
generated frontier. When expansion reveals land along that corridor:

- Deterministic generation continues the corridor through the new tiles.
- Previously generated geometry and player changes remain intact.
- The detailed entry or exit gateway moves outward to the new frontier.
- Trips continue referring to the same corridor and external destination.

External cities are modeled in aggregate. They may supply and consume people,
freight, and services without being fully rendered or microsimulated. Expansion
therefore increases local playable land without requiring an endless detailed
simulation outside the player's city.

## Authored Maps and Engine Technology

Community-authored finite maps and seeded procedural worlds should share the
same coordinate, jurisdiction, buildability, and outside-connection concepts.
An authored map may provide a finite extent and handcrafted terrain instead of
a generator while preserving the same simulation boundary.

City Form does not yet commit to Unreal Landscape, World Partition, origin
rebasing, or a particular streaming implementation. Those choices follow a
large-world prototype and profiling on supported macOS and Windows hardware.
The v0.1 prototype level intentionally does not use World Partition.

## Invariants

- All authoritative coordinates and elevations are finite and metric.
- Camera position and visual loading never determine simulation progress.
- Tile, corridor, and external-destination identities remain stable.
- Map generation is reproducible for a declared seed and generator version.
- Level Actors do not become the source of authoritative city state.
- Presentation technology can change without replacing logical map identity.

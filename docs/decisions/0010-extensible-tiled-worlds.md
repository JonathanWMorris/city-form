# 0010: Extensible Tiled Worlds and Regional Gateways

## Status

Accepted

## Context

City Form needs a small finite map for v0.1 without closing off larger,
community-authored, or procedurally generated worlds. A permanently fixed map
edge makes later expansion restrictive, while a literally infinite detailed
simulation conflicts with attainable performance and inspectable city state.

Outside connections also need stable meaning if playable land can expand past
the location that was previously the generated frontier.

## Decision

Future procedural worlds generate a sizable deterministic initial region and
extend through contiguous gameplay tiles on demand. Generated terrain, player
jurisdiction, detailed simulation extent, and Unreal streaming cells are
separate concepts.

Detailed city simulation remains global throughout the player's jurisdiction
and never depends on camera visibility. Distant cities remain persistent but
aggregate external destinations.

Outside transport uses stable regional-corridor and external-destination
identities. A corridor's detailed gateway may move outward when new tiles are
generated, while its identity, existing geometry, and trip relationships
remain stable.

Authored finite maps and procedural worlds share the same metric coordinate,
jurisdiction, buildability, and external-connection concepts. Engine terrain
and streaming technology remain deferred until a profiled large-world
prototype demonstrates the requirements.

## Consequences

- Seeded worlds can grow beyond their initial generated region without
  promising unlimited detailed simulation.
- Land ownership can remain coherent through contiguous expansion.
- Outside demand survives frontier movement without being tied to transient
  boundary Actors.
- Deterministic tile generation requires stable coordinates and explicit
  generator versioning.
- Save data must eventually preserve generated versions and player edits.
- Procedural generation, land purchasing, and regional demand are future work;
  v0.1 implements only the finite flat prototype.
- Exact tile sizes, initial generation radius, and streaming technology require
  measurement and playtesting rather than an early architecture commitment.

## Supersedes

None.

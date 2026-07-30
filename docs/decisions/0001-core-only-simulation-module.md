# 0001: Core-Only CitySimulation Module

## Status

Accepted

## Context

The authoritative city simulation must remain independent of rendering, Actors,
loaded levels, and viewport state. A separately built standard C++ library
would strengthen that separation but would also introduce another build,
testing, and platform workflow before a standalone runner is required.

A regular Unreal gameplay module would be easy to integrate but could allow
Engine and UObject dependencies to spread into persistent simulation state.

## Decision

`CitySimulation` will be an Unreal runtime module built by UnrealBuildTool. It
may depend on `Core`, but not `CoreUObject`, `Engine`, or `CityForm`.

Authoritative types will be ordinary C++ structs and classes without UObject
reflection, garbage collection, Actor ownership, or loaded-World assumptions.
The deepest algorithms will use plain data and explicit ownership so extraction
remains possible if a standalone research runner becomes a real requirement.

Unreal Automation Tests will provide the initial headless test workflow.

## Consequences

- City Form keeps one supported build system and platform configuration.
- Unreal presentation code depends on the simulation, never the reverse.
- Simulation performance comes from data layout and update design rather than
  UObject or Actor participation.
- Contributors must justify any additional module dependency.
- A truly standalone executable remains deferred and would require a later ADR.

## Supersedes

None.

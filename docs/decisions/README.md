# Architecture Decision Records

Architecture Decision Records (ADRs) preserve the context and consequences of
decisions that materially shape City Form.

Accepted ADRs describe the current direction. A later decision does not rewrite
an accepted record; it adds a new ADR that supersedes the old one. Minor
implementation details and easily reversible choices do not require an ADR.

## Records

- [0001: Core-only CitySimulation module](0001-core-only-simulation-module.md)
- [0002: Integer time and metric coordinates](0002-time-and-coordinate-model.md)
- [0003: Time-dependent A* routing](0003-time-dependent-a-star-routing.md)
- [0004: Layered traffic fidelity](0004-layered-traffic-fidelity.md)
- [0005: Shared vehicle-class definitions](0005-vehicle-class-model.md)

## Template

Copy the following structure into the next numbered Markdown file:

```markdown
# NNNN: Decision title

## Status

Proposed | Accepted | Superseded by ADR NNNN

## Context

What problem, constraints, and alternatives led to this decision?

## Decision

What direction has the project chosen?

## Consequences

What becomes easier, harder, required, or deliberately deferred?

## Supersedes

None, or links to earlier ADRs.
```

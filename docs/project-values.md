# Project Values

These values are the durable decision rules for City Form. Features and
implementation details will change, but contributors should be able to use
these principles to resolve tradeoffs consistently.

## Player and Creator Agency

City Form should give people meaningful control over the city they are
building. Rules should be understandable, important outcomes should be
explainable, and automation should save effort without taking ownership away
from the player.

This means:

- Advanced editing and traffic-management capabilities should ultimately be
  part of the base game rather than necessities supplied only by mods.
- Construction constraints should be clear and should eventually support
  deliberate overrides where the simulation can remain valid.
- Automated development and simulation outcomes should expose the reasons
  behind their decisions.
- Logical city data should remain editable through well-defined commands.
- Future modding, custom content, research, and automation interfaces must
  remain possible, even though they are not v0.1 deliverables.

This does not mean every system must be configurable immediately or that the
simulation should accept invalid state.

## Long-Term Scalability

The architecture should work for a small prototype without preventing a much
larger city later. Simulation truth, visual detail, and interaction detail must
be able to scale independently.

This means:

- Persistent entities use compact data rather than one Actor per citizen,
  household, business, or trip.
- Loaded levels and visible regions do not determine whether the underlying
  city exists or advances.
- Systems operate on explicit data and stable identities, with validation at
  their boundaries.
- Simulation and rendering can run at different rates and levels of detail.
- New systems should integrate through clear ownership and data flow rather
  than global access to mutable state.

Scalability is not permission to build speculative frameworks. Each abstraction
must solve a current problem while preserving a credible path forward.

## Attainable Performance

City Form should run well on strong conventional consumer hardware. It should
not require a workstation, the most expensive current GPU, or excessive memory
to produce a meaningful city.

This means:

- Performance decisions are based on profiling and representative scenarios.
- Simulation scale and rendering quality have explicit, independently
  adjustable controls where practical.
- Data layouts and update schedules should suit the work being performed.
- Expensive visual representations are reserved for objects that need them.
- Plugins and experimental engine features require evidence that their benefit
  outweighs their portability, maintenance, and performance costs.

The project will not invent fixed hardware or frame-time promises before a
representative vertical slice exists. v0.1 will establish reproducible
benchmarks that later releases can turn into formal budgets.

## Community First

City Form should be understandable and welcoming to people beyond its original
maintainer. Project knowledge belongs in the repository, and important
decisions should be visible to the community.

This means:

- Documentation changes alongside the systems it describes.
- Proposals and tradeoffs are discussed publicly whenever practical.
- Contributions are kept small enough to review and test with confidence.
- Different disciplines and experience levels have useful ways to contribute.
- Contributors receive clear feedback and visible credit.
- Dependencies, proprietary services, and platform-specific code are added only
  when their value and boundaries are clear.

Community first does not mean accepting every feature request. Scope,
architecture, maintainability, and respectful collaboration remain necessary.

## Applying the Values

When two approaches are both technically plausible, prefer the one that:

1. Gives players clearer control and explanations.
2. Preserves authoritative, editable simulation data.
3. Scales through evidence-based design rather than hardware requirements.
4. Is easier for contributors to understand, test, and improve.

If a major decision conflicts with one of these values, document the tradeoff
before implementation.

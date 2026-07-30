# Gameplay Time and Development Pacing

## Status

This document records accepted future gameplay defaults. The authoritative
millisecond clock exists today; civil-calendar projection, playback controls,
development timing, and construction capacity are not yet implemented.

## Calendar and Playback Defaults

The initial pacing target is:

| Setting | Default |
| --- | --- |
| Civil day at 1x | 45 wall-clock minutes |
| Playback rates | Pause, 1x, 2x, 4x |
| Daylight interval | 05:00–21:00 |
| Week | Seven days |
| Season | Three weeks |
| Year | Four seasons, or 84 days |

The calendar should be presented as year, season, week, and weekday. For
example:

```text
Year 3 · Summer · Week 2 · Thursday
```

This preserves recognizable weekday transit service without requiring
365 day-night cycles per game year. Conventional month names, real-world dates,
daylight-saving transitions, and latitude-based daylight are later decisions.

The values are configurable balancing defaults. Simulation time remains
integer milliseconds, and physical movement continues to use seconds, meters,
and SI vehicle characteristics. When the requested playback rate exceeds the
machine's sustainable simulation throughput, City Form should run slower than
the target rather than discard authoritative work.

## Development Lifecycle

Development is not one opaque construction timer. Its conceptual lifecycle is:

```text
Demand and proposal
    → financing
    → permitting and optional environmental review
    → wait for construction capacity
    → visible site construction
    → inspection, commissioning, and occupancy
```

The construction duration below begins when visible site work starts and ends
when the building is ready for occupancy. Proposal evaluation, financing,
permitting, environmental review, and waiting for an available crew are
separate, inspectable delays.

## Ideal Construction Durations

The default reference assumes idealized high-productivity conditions:

- An approved, financed design
- A cleared and buildable site
- Adequate labor, equipment, and construction capacity
- Reliable prefabrication and material supply
- Unconstrained site access
- Favorable weather
- Successful inspection and commissioning

Under those conditions, the initial balancing targets at 1x are:

| Development | Simulation duration | Wall-clock duration at 1x |
| --- | ---: | ---: |
| Low-density house | 16 hours | 30 minutes |
| Small commercial building | 24 hours | 45 minutes |
| Apartment or office mid-rise | 40–48 hours | 75–90 minutes |
| Mall or major complex | 64 hours | 2 hours |
| High-rise | 96–120 hours | 3–3 hours 45 minutes |
| Exceptional skyscraper | 6 days | 4 hours 30 minutes |

These are gameplay-scaled minimum durations, not claims that real buildings
reach occupancy in hours or days. Visible construction has a seven-day total
gameplay cap, including modeled delay. A project that cannot commit enough
capacity and inputs to meet that bound waits before groundbreaking. A future
unique landmark may exceed the cap only through a separate, explicit design
decision.

Playback affects construction consistently with the rest of the simulation.
For example, a 16-hour house takes 15 wall-clock minutes at 2x and 7.5 minutes
at 4x when the requested rate is sustained.

## Actual Progress and Explanations

Actual completion forecasts derive from the ideal reference plus explicit
constraints. Candidate influences include:

- Crew and equipment availability
- Material or prefabricated-module availability
- Delivery access and traffic delay
- Site preparation and foundation complexity
- Weather
- Design customization
- Inspection or commissioning failures
- Contention from other active projects

The model should report causes instead of presenting unexplained random delay.
A future inspection view could show:

```text
Ideal completion: Tuesday 14:00
Current forecast: Wednesday 03:30

Material delivery     +6h
Crew availability     +4h
Site congestion       +3.5h
```

Construction methods and regional profiles may change ideal work or the
availability of inputs. The simulation should model conventional,
prefabricated, and modular methods rather than assign one fixed speed to a
country.

The ideal reference is directionally informed by highly industrialized
construction rather than presented as a national average. Official Chinese
examples distinguish rapid structural assembly from complete delivery:

- [Yiyang's prefabricated housing report](https://www.yiyang.gov.cn/yiyang/2/3/73/content_2018216.html)
  describes structural assembly in days and finished delivery in about two
  months.
- [Shenzhen's modular high-rise report](https://gzw.sz.gov.cn/ztzl/gzgqztzl/szssgzgqshzrzl/fwms/content/post_10554210.html)
  describes 292 days to structural topping-out and about one year from start to
  delivery for five 28-story towers.

Citywide construction capacity must be bounded. Zoning many parcels cannot
create unlimited crews, equipment, or simultaneous project throughput. An
early implementation may use deterministic capacity and queues; later systems
may derive them from construction businesses, labor, equipment, and supply
chains.

Building visuals may expose construction stages before completion. Home and
job capacity becomes authoritative only when the applicable space is ready for
occupancy.

## Implementation Boundary

This document does not add a public API or require construction systems in
v0.1. Concrete configuration, work, phase, forecast, and capacity types should
be introduced with the milestone that first consumes them.

Future validation should cover:

- Non-negative remaining work and delay durations
- Stable ordering when projects compete for capacity
- No occupancy before applicable capacity is complete
- Repeatable forecasts for controlled inputs
- Consistent results at different wall-clock playback rates

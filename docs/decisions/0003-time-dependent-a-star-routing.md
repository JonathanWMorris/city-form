# 0003: Time-Dependent A* Routing

## Status

Accepted

## Context

City Form routes trips through a spatial road graph. Route choice must
eventually respond to departure time, predicted queues, recurring congestion,
vehicle capabilities, and road restrictions.

Static distance-only routing cannot express those requirements. Dijkstra
supports arbitrary non-negative costs but does not use the graph's spatial
coordinates to guide a point-to-point search.

## Decision

The initial point-to-point router will use time-dependent A*.

Route queries include departure tick and VehicleClass. Traversal costs are
evaluated at the tick when the candidate route predicts entry. Stage 2 uses
free-flow travel time through the final time-aware interface; traffic-aware
cost providers follow later.

The heuristic is straight-line distance divided by the fastest feasible speed
for the VehicleClass, rounded down to ticks. Congestion predictions remain in
traversal costs so the heuristic stays admissible. A zero heuristic is the
fallback when no stronger admissible bound is available.

Cost providers must be non-negative and satisfy FIFO. Stable entity IDs define
all equal-cost ordering.

## Consequences

- The router can remain unchanged when historical and live traffic costs
  arrive.
- Vehicle restrictions and performance may affect feasibility and travel time.
- The spatial heuristic reduces unnecessary exploration while retaining
  optimality under the documented assumptions.
- Non-FIFO models require a different algorithm and a new decision.
- Deterministic tie rules may select one of several equally optimal paths;
  they do not imply that every driver perceives costs identically.

## Supersedes

None.

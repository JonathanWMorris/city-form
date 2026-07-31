# Road Placement Tool

## Current Interaction

Stage 4 begins with one deliberately small road-building tool. In PIE:

1. Select **Road** in the upper-left tool palette.
2. Primary-click the prototype ground to choose the first endpoint.
3. Move the pointer to inspect the live debug preview.
4. Primary-click again to create one logical `BasicTwoWayRoad` segment.

The preview is green when the candidate is valid and red when it is invalid.
An endpoint close to an existing node in screen space snaps to that node. The
snap radius is 12 pixels, so the interaction remains stable as camera zoom
changes. Equal-distance candidates resolve to the lower node ID for repeatable
behavior.

Right click, trackpad secondary click, or `Escape` cancels a pending first
endpoint. Repeating the action while idle exits the Road tool. Clicking the
palette never places a road underneath the UI.

## Simulation Boundary

The first click changes only transient presentation state. The second click
sends one atomic `CreateRoadSegment` command through
`UCityFormSimulationSubsystem`. Each endpoint is either an existing logical
node ID or a new position converted explicitly from Unreal centimeters to
simulation meters.

The command validates both endpoints, road type, speed override, positive
length, duplicate topology, and ID capacity before modifying the graph. A
failed command therefore cannot leave an orphan node behind. The simulation
returns typed errors for the palette and log to present without making Unreal
Actors authoritative.

## Prototype Constraints

- Placement must hit the prototype ground near `Z = 0` and remain inside the
  documented `-900 m` through `+900 m` buildable bounds.
- A road must be at least one meter long.
- Only endpoint snapping is implemented. Mid-segment intersections, splitting,
  arbitrary-angle constraints, grades, lanes, demolition, and undo are future
  editing work.
- The preview uses transient debug drawing. Successful segments are logical but
  do not yet have persistent derived geometry; that is the next Stage 4 task.
- Residential and Commercial buttons reserve the intended tool palette shape
  but stay disabled until their milestones exist.

## Verification

Automation covers the map contract, placement state, minimum length,
screen-space snapping, deterministic tie-breaking, bridge conversion, typed
errors, and atomic connected-graph construction. Hands-on verification should
also confirm primary and secondary trackpad clicks, mouse clicks, preview
colors, cancellation, and camera navigation in PIE.

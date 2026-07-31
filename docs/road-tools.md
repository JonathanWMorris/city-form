# Road Placement Tool

## Current Interaction

Stage 4 provides one deliberately small road-building tool. In PIE:

1. Select **Roads** in the bottom category bar.
2. Select **Basic Two-Way Road** in the contextual tray above it.
3. Primary-click the prototype ground to choose the first endpoint.
4. Move the pointer to inspect the live debug preview.
5. Primary-click again to create one logical `BasicTwoWayRoad` segment.

The preview is green when the candidate is valid and red when it is invalid.
An endpoint close to an existing node in screen space snaps to that node. The
snap radius is 12 pixels, so the interaction remains stable as camera zoom
changes. Equal-distance candidates resolve to the lower node ID for repeatable
behavior.

Right click, trackpad secondary click, or `Escape` steps back through three
states: cancel a pending first endpoint, deactivate Basic Two-Way Road while
leaving Roads open, and then close the Roads tray. Clicking or hovering the
dock never places a road or triggers bottom-edge camera scrolling.

The lower menu layer contains categories and currently exposes only Roads. The
upper layer contains concrete tools for the open category. Unimplemented
categories are not shown. The road symbol is composed from source-defined UMG
shapes rather than editor-only icons or binary artwork.

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

After a successful command, the bridge publishes a presentation-only graph
change notification. One world actor consumes a fresh detached snapshot and
rebuilds an instanced cube mesh: one gray strip per logical segment, with a
parallel stable road-ID mapping for inspection. Rebuilding or destroying this
presentation cannot modify the graph.

## Prototype Constraints

- Placement must hit the prototype ground near `Z = 0` and remain inside the
  documented `-900 m` through `+900 m` buildable bounds.
- A road must be at least one meter long.
- Only endpoint snapping is implemented. Mid-segment intersections, splitting,
  arbitrary-angle constraints, grades, lanes, demolition, and undo are future
  editing work.
- The preview uses transient debug drawing. Successful segments remain visible
  as simple 8-meter-wide, 0.2-meter-thick placeholder strips.
- Production meshes, markings, intersections, curves, selection, deletion, and
  zoning categories remain future work.

## Verification

Automation covers the map contract, placement state, minimum length,
screen-space snapping, deterministic tie-breaking, bridge conversion, graph
change notifications, visual transforms, stable visual IDs, typed errors, and
atomic connected-graph construction. Hands-on verification should also confirm
the two-layer dock, contained status wrapping, persistent road strips, primary
and secondary trackpad clicks, mouse clicks, preview colors, cancellation, and
camera navigation in PIE.

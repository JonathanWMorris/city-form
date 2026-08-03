# Zoning and Development Tools

## Current Interaction

Stage 5 adds a **Zoning** category beside **Roads** in the bottom-center dock.
Opening it displays four actions in the contextual tray:

- **Residential** applies Residential zoning to one parcel per primary click.
- **Commercial** applies Commercial zoning to one parcel per primary click.
- **Clear Zone** returns one parcel to unassigned and removes its unoccupied
  placeholder Building.
- **+5 Minutes** advances authoritative simulation time by exactly 300,000 ms.

The selected zoning tool remains active for repeated individual clicks. Drag
painting and batch commands are intentionally deferred. Right click, trackpad
secondary click, or `Escape` deselects the active tool and then closes the
Zoning tray. Pointer interaction over the dock never zones a parcel or triggers
bottom-edge camera scrolling.

## Picking and Overlay

Opening Zoning shows every current Parcel as an oriented rectangular outline:
gray for unzoned, green for Residential, and blue for Commercial. The overlay
is hidden when Zoning closes; Building placeholders remain visible.

Primary-click picking raycasts to prototype ground and tests the hit point
against detached parcel rectangles using each parcel's center, heading, width,
and depth. Where the accepted intersection-overlap limitation places multiple
parcels under one point, the lowest stable `FParcelId` wins deterministically.
Clicks outside a highlighted parcel return status feedback and do not mutate
the city.

## Simulation and Presentation Boundary

`UCityFormSimulationSubsystem` submits `ApplyZone`, `ClearZone`, and `Advance`
to its game-instance-owned `FCitySimulation`. Successful commands publish a
development-change notification; rejected commands publish none. Road creation
also publishes this notification because it generates authoritative Parcels.

The bridge exposes a detached development snapshot containing presentation
copies of Parcel geometry/zoning and Building identity/type/stage/capacity.
The development visualization actor rebuilds fixed groups of instanced cube
meshes from that snapshot:

- Planned Buildings are shallow gray foundations.
- Under-construction Buildings are half-height orange placeholders.
- Complete Detached Houses are green cubes.
- Complete Small Commercial buildings are taller blue cubes.

Rebuilding, hiding, or destroying these visuals cannot change simulation
records or time. The +5 Minutes action uses normal authoritative advancement;
it exists only to demonstrate the five-simulated-minute placeholder lifecycle
before playback controls are implemented.

## Prototype Constraints

- Zoning applies to one existing Parcel at a time.
- There is no drag painting, undo, demolition, selection inspector, production
  material, animated construction, or detailed building mesh.
- Clearing and rezoning remain unconditional only because Stage 6 occupants do
  not exist yet.
- Parcel boundaries inherit the accepted untrimmed intersection-overlap
  limitation from the authoritative generator.

## Verification

Automation covers detached snapshots, successful and rejected notifications,
axis-aligned and rotated picking, deterministic overlap resolution, boundary
transforms, every placeholder stage, type-specific complete visuals, and
authoritative five-minute advancement. Hands-on verification should also cover
mouse and trackpad clicks, category switching, overlay visibility, status
feedback, multiple viewport sizes, camera controls, and repeated Play sessions.

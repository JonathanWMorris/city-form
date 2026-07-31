# City-Builder Camera

## Purpose

City Form uses a project-owned C++ camera pawn instead of Unreal's character or
free-flying default pawns. It keeps navigation focused on the map's ground
plane and provides stable mouse, keyboard, and trackpad controls for future
editing tools.

The camera is presentation state. Its position, zoom, visibility, and loaded
level never determine which parts of the authoritative city simulation run.

## Controls

| Input | Action |
| --- | --- |
| `W`, `A`, `S`, `D` | Pan relative to the camera heading |
| Pointer at a viewport edge | Pan toward that edge |
| Mouse wheel | Zoom in or out |
| Two-finger vertical trackpad scroll | Zoom in or out |
| Hold middle mouse and drag horizontally | Rotate |
| Hold middle mouse and drag vertically | Tilt |
| `Q`, `E` | Rotate left or right |
| `R`, `F` | Tilt toward the horizon or toward top-down |
| `Home`, `End` | Alternative tilt controls |
| `Z`, `X` | Zoom in or out without a wheel |

Right-click and trackpad secondary click are deliberately unassigned. Future
editing tools will use secondary click to cancel the current operation.

The initial trackpad contract is two-finger scrolling for zoom plus ordinary
pointer edge scrolling. Native pinch, rotation, and two-finger pan gestures are
deferred because they do not yet provide a sufficiently well-verified,
cross-platform Unreal input path. Keyboard controls provide rotation and tilt
when a middle mouse button is unavailable.

## Behavior and Bounds

- Edge scrolling is enabled by default within 24 pixels of the viewport edge.
- Diagonal pan input is normalized so it is not faster than axial movement.
- Pan speed increases with camera distance to cover the map efficiently.
- The ground focus is hard-clamped to the documented `-900 m` through `+900 m`
  buildable bounds on both planar axes.
- Zoom is clamped from `25 m` through `1,500 m`, starting at `750 m`.
- Tilt is clamped from 25 degrees below the horizon through 80 degrees
  top-down, starting at 55 degrees.
- Pan, zoom, rotation, and tilt converge with frame-rate-independent
  exponential smoothing.
- Camera collision is disabled so presentation geometry cannot unexpectedly
  push the camera away from its intended focus.

The defaults are prototype tuning values, not permanent gameplay balance.

## Implementation Boundary

`ACityFormCameraPawn` owns the ground focus, spring arm, camera, tuning, and
Enhanced Input handlers. `ACityFormPlayerController` owns cursor and viewport
input-mode policy. `ACityFormGameMode` selects both classes.

Input actions and the mapping context are project assets under `/Game/Input`.
The mapping table is assembled in C++ from those semantic actions so the
current control contract remains source-reviewable. Future configurable
bindings can replace that assembly without moving authoritative simulation
state into the Unreal presentation layer.

UI and tools may suppress all camera input temporarily. They may also disable
edge scrolling independently when pointer interaction near a screen boundary
must take priority.

## Current Limitations

- There is no in-game rebinding or sensitivity interface.
- Gamepads, touchscreens, native pinch/rotate gestures, drag-to-pan, and
  cursor-centered zoom are not implemented.
- Physical trackpad direction and feel must remain part of macOS playtesting;
  operating-system natural-scroll settings can affect perceived direction.
- Road editing and right-click cancellation are Stage 4 work.

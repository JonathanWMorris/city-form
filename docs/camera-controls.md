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
| Left click or primary trackpad click | Use the selected editing tool |
| Right click or trackpad secondary click | Cancel the pending road endpoint, then exit the Road tool |
| `Escape` | Cancel the pending road endpoint, then exit the Road tool |

Primary clicks over the tool palette operate the UI rather than the map. The
same rule suppresses edge scrolling while the pointer is over the palette.

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

## Stage 3 Verification Baseline

Stage 3 was verified on July 31, 2026, on an Apple Silicon Mac running the
macOS 27 beta, Unreal Engine 5.8, and Xcode 26.1.1 with the Metal Toolchain.

- The editor and game targets both built successfully in Development.
- `/Game/Maps/Prototype` opened as the configured editor and game map.
- PIE spawned and possessed `ACityFormCameraPawn` with
  `ACityFormPlayerController` at the ground-level origin; no character pawn or
  character movement component was used.
- Camera navigation was reviewed in PIE during PR #80. The missing `R`/`F`
  tilt alternatives found during hands-on review were corrected before merge.
- All 23 `CityForm.*` automation tests passed: four presentation camera tests
  and 19 simulation tests.
- A separate `UnrealEditor-Cmd -NullRHI` process found and passed all 19
  `CityForm.Simulation` tests with exit code zero and no gameplay viewport.

This baseline completes the prototype-environment milestone. Future camera
tuning remains presentation work and must not change simulation outcomes.

## Current Limitations

- There is no in-game rebinding or sensitivity interface.
- Gamepads, touchscreens, native pinch/rotate gestures, drag-to-pan, and
  cursor-centered zoom are not implemented.
- Trackpad scroll feel can vary with operating-system natural-scroll settings
  and should be rechecked when configurable input settings are introduced.
- The Stage 4 Road tool supports two-click creation and cancellation. Its
  persistent derived visuals and advanced editing remain incomplete.

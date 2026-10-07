# Continuous Input Comparison

This preparation tool runs until Escape or window close. It is not the SDL game
and does not approve P06 or T2. Both executables share the same scene and import
the frozen GLFW Release camera/ECS/simulation dependencies. Pointer input goes
through the actual `Simulation::ApplyPointerInput` and `Camera::Scroll` functions
(0.05 degrees per motion unit). Movement is a collision-free diagnostic room,
not a replacement for gameplay, physics or world verification.

From the repository root, build SDL with:

```powershell
./test/experimental/sdl3/build.ps1 -Configuration Release -InputComparison -BuildDirectory out/m3-t2/probe/input-comparison-sdl-release-002
```

The option is off by default, so the ordinary SDL API probe remains independent
of saved GLFW libraries. Build the adjacent `../glfw-baseline` Release project
for the actual GLFW platform comparison.

From the repository root, `./test/experimental/input-comparison/run.ps1` runs
GLFW then SDL, each with a fresh evidence directory. `-Backend SDL3` selects only
SDL; `-SystemScale 0` is an explicitly unscaled diagnostic candidate, not a change
to the adopted migration policy. Default SDL scale 1 is a comparison candidate,
not a claim of equal pointer acceleration.

- F1 cycles Lock / Normal / Hidden. F2 resets view and FOV. F3 resets input.
- Escape or the window close button ends the current backend.
- Title reports active keys, mouse buttons, focus, cursor and camera angles.
- The thirteen indicators, left to right, represent Escape, Shift, CapsLock,
  Control, W, S, D, A, Space, E, Q, left and right mouse buttons.
- `frames.csv` records snapshots, resulting camera pose and control actions
  (mask 1: F1, 2: F2, 4: F3). `pointer-events.csv` preserves event order
  and floating-point deltas. `runtime.txt` records actual library/GL facts.
- `first-frame.bmp` is actual GL readback, checked for nonblank content.
- The launcher binds each run to an executable SHA-256, not a generic backend name.

The SDL loop drains events, samples fresh SDL physical state, resets on pause or
cursor changes, resamples after an active reset, then captures keys. It preserves
short press latches independently of held state. No system-wide synthetic input
is sent. F1/F2/F3 are diagnostic-only Windows thread keyboard controls; they are
not part of the production project's eleven-key mapping.

Compare short/held/released keys, wheel and slow/fast pointer movements using the
same mouse/settings; hold W while switching out, release outside and return;
minimize/restore, change cursor modes and close. Hardware layouts and feel still
require explicit human results. Automated `verify.ps1` renders twelve frames in
seven configurations; it cannot approve hardware input or sustained gameplay.

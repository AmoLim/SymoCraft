# SDL S5 Delivery Diagnostics

`verify-sdl-delivery.ps1` runs installed Release packages in fresh, retained
fixtures. It never changes the supplied packages or any frozen archive. Build
`inspect-sdl-delivery.cpp` against the project's existing Release `yaml-cpp`
archive first and supply its executable as `-ProtocolInspector`.

```powershell
& 'F:\GameDevelop\OpenGLProject\Symocraft\test\integration\verify-sdl-delivery.ps1' `
  -GamePackage 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\s5\install\candidate-release-001' `
  -RunnerPackage 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\s5\install\candidate-release-001' `
  -ProtocolInspector 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\s5\inspect-sdl-delivery.exe' `
  -OutputDirectory 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\s5\delivery-release-001'
```

Run only with GUI execution authorization and do not run competing window
fixtures simultaneously. Helpers start hidden. The runner test verifies its own
PID, exact window title and class before setting existing controls. It starts
the existing quick profile and sends `WM_CLOSE` only to its own runner after an
actual child HWND, readiness and structured live warmup/sampling status are
observed. The final summary must separately prove actual collected frames:
`frames.csv` is exported only after shutdown, never treated as a live signal.
The runner then uses
its existing stop token and child-window close path. Normal runner exit 0,
actual child exit 4, `cancelled=true`, `forced_termination=false`, retained
incomplete summary, terminal status, no later rounds and a released session lock
are required. A timeout or cleanup kill is a failure, never a successful cancel.
The runner, its game copy and result parent use actual Unicode/spaces paths.
Three nonempty exported CSV files and a positive summary frame count are
required, but this does not assert complete CSV/summary consistency acceptance.
`final-frame.png` is optional observation only: a legitimately incomplete
cancelled game does not execute the completed-capture screenshot branch.

Cases also cover finite-frame normal exit, working directory `System32`, an
actual Unicode/spaces package path, and named shader/texture/configuration
failures with clean shutdown. Missing app-local CRT is only an observation:
the copy omits local CRT DLLs, but Windows may find installed CRT elsewhere.
Clean-machine missing-CRT behavior is explicitly unavailable on this developer
machine. No system CRT files are deleted or hidden.

All logs, copies, JSON identities and failed attempts are retained. The summary
separates passed, failed and unavailable cases. `verified_cases_passed` can be
true while `all_requested_checks_passed` remains false for the unavailable CRT
case. Failure of any verifiable case or a changed source package throws.

These bounded diagnostics do not satisfy the S5 15-minute manual game run,
Y9000P hardware verification, Q06 matching GLFW/SDL captures, or Q07 approved
performance budgets. Cancellation stops the quick profile early; it is not an
SDL acceptance performance sample.

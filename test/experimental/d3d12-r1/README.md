# Isolated D3D12 R1 Experiments

This test-only experiment evaluates T3's candidate CPU contract and real D3D12
texture drawing, resource update/deletion, and presentation-resource rebuilding.
It is not a production Renderer implementation, gameplay acceptance, or an R1
freeze. T2 acceptance, local RTX 5070 Ti evidence, and the public contract handoff
remain prerequisites for freezing R1 and entering R2. OpenGL and production/root build files are not
modified by this experiment.

The current functional/compatibility scope is the local RTX 5070 Ti only.
Deferred Y9000P/GTX 1650 whole-system performance remains a gate before the next
gameplay phase. Integrated-GPU-specific code and generated packages were removed
at the user's request; the original desktop R1 evidence remains unchanged.

## Build

Windows x64, MSVC C++20, a Windows SDK with D3D12/DXGI 1.6, and DXC are required.
DXC compiles `VSMain`/`PSMain` from `shaders/probe.hlsl` as shader model 6.0 into
`shaders/probe-vs.cso` and `shaders/probe-ps.cso`. Missing DXC is a hard failure;
FXC and older shader profiles are not fallbacks.

From the repository root:

```powershell
./test/experimental/d3d12-r1/build.ps1 -BuildPlatform -Configuration Debug `
  -ProductionBuild out/m3-t3/platform-debug `
  -BuildDirectory out/m3-t3/r1-debug-001 `
  -DxcPath 'C:/Program Files (x86)/Windows Kits/10/bin/10.0.22621.0/x64/dxc.exe'
```

`-BuildPlatform` configures an independent root cache with testing/benchmark
disabled and builds only `symocraft_platform` and its actual dependencies. The
root configuration must enable the game to expose the platform target, but the
game executable and production renderer are not built. Platform construction
is confined to `out/m3-t3`, never T2's cache. Omit `-BuildPlatform` to import an
already built matching `-ProductionBuild` without modifying it.

The experiment imports the real platform, foundation, and static SDL3 archives;
it does not compile a second `window.cpp`, substitute SDL, or require GLAD. Only
the private `NativeBridge` borrows HWND, with the window outliving the renderer.
`production-imports.json` records archive/source/cache/DXC identity, and stale
platform and foundation sources are rejected. The compiler and CLion CMake/Ninja paths are
overrideable with the same options as the existing platform probes.
After a successful build, `build-identities.json` binds the executable to the
source, shader, and import identities; verification rejects later changes.
It also records readable MSVC, Windows SDK, DXC, CMake, and Ninja identities.

The GPU fixture's CTest entry runs the original 70-check candidate registry test.
The independent [CPU contract project](contract/README.md) additionally builds a
declaration-only public consumer, 82 production handoff checks, and actual CPU
cache/compiler/link boundary checks without SDL, DXC, or GPU libraries:

```powershell
./test/experimental/d3d12-r1/contract/build.ps1 -Configuration Debug `
  -BuildDirectory out/m3-t3/r1-cpu-handoff-debug-new
```

## Explicit Hardware Verification

This opens an SDL3 native window. Run only with the user's authorization for
the GUI launch. Hardware adapters only: no WARP or silent backend fallback.
Debug Layer is required by default; missing validation support is a failure,
not an implied successful run.

```powershell
./test/experimental/d3d12-r1/verify-probes.ps1 `
  -Executable ./out/m3-t3/r1-debug-001/SymoCraftD3D12R1Probe.exe `
  -OutputDirectory out/m3-t3/evidence/desktop-debug-001 -RunTimeoutProbe
```

The runner refuses to overwrite evidence, checks imported identities, captures
stdout/stderr and the real exit code, and terminates a process after the bounded
timeout (60 seconds by default). The normal case must report a real D3D12 device,
Debug Layer enabled with zero errors/warnings, successful E01/E02/E03, completed
shutdown, and nonempty PNG output. `result.json`, screenshots, `machine.json`,
and `verification.json` preserve the operation and identity evidence. Inventory
GPU names are not treated as proof of the adapter selected by D3D12. The default
high-performance hardware selection records the actual device LUID, checked
against DXGI, plus name, PCI identity, driver, feature level and shader model.
The current fixture rejects non-RTX-5070-Ti results instead of counting another
adapter. It adds no inventory/explicit adapter-selection CLI. Actual swapchain
dimensions, format, sample count/quality, buffers, effect/flags and windowed mode
are queried after creation/rebuild; actual Present arguments are recorded.

`-RunTimeoutProbe` adds a separately labelled `--inject-timeout` case expected to
exit 1 with the original injected-wait diagnostic. This validates bounded failure
handling, not an actual driver/device failure. The zero-extent suspend input is
explicitly injected, while native minimization/restoration are real window
operations; Windows can retain nonzero client dimensions while minimized.
Screenshots need human visual review; a machine pass alone never authorizes the
R1 freeze.

## Portable Desktop Validation Package

After a successful Release build, create a fresh fixture-only package:

```powershell
./test/experimental/d3d12-r1/package.ps1 `
  -BuildDirectory out/m3-t3/r1-release-001 `
  -OutputDirectory out/m3-t3/package/d3d12-r1-release-001
```

The package contains the executable, shader binaries, app-local Release CRT,
SDL license, the verification runner and immutable origin records. It does not require SDK/DXC
installation on the runtime machine. D3D12 hardware and Windows Graphics Tools
Debug Layer remain mandatory. Run the command in its `README.txt` to create fresh
package-local `evidence` output. `-PackageManifest` validates packaged bytes,
without accessing the builder's original archive/source paths; those paths remain
origin records only. The runner explicitly uses package-local shaders.

Use fresh build, package, and evidence directory names for subsequent runs;
existing output is never overwritten. The original `d3d12-r1-release-001` package
and its desktop evidence are retained as immutable historical output, not
relabelled as another device or a production Renderer acceptance result.

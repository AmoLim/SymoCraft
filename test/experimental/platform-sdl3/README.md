# SDL3 Production Platform Probes

This standalone Windows x64/MSVC project exercises the current production
`symocraft_platform` library. It imports matching platform, foundation,
SDL3-static and GLAD archives from an actual production build; it does not
recompile `window.cpp`, build another SDL runtime or introduce a second window
implementation. The earlier `../sdl3` project remains an independent SDL API
preparation probe, not this production regression suite.

## Mode Responsibilities

| Mode | What is exercised | Ownership and scope |
| --- | --- | --- |
| OpenGL | Production GL 4.6 Core context, driver-reported 4x MSAA, GLAD loading, VSync intervals 1 and 0, nonblank pixel readback and real `GraphicsBridge::Present` | Window owns its GL context and system window. The probe draws a clear color, not a game world; gameplay, textures and GPU timing require the separate production-game checks. |
| Native | Ordinary window, real borrowed Win32 HWND, no current GL context, rejection of GL-only bridge calls, WM_CLOSE through production wait/poll | The HWND is valid only until Window destruction. No D3D12 device, swapchain or rendering is created. |
| Vulkan | Production extensions and SDL loader entry, real Vulkan 1.2 instance and Win32 surface, device enumeration and reverse cleanup | The probe owns surface and instance. It destroys surface, then instance, before Window and SDL video. No Vulkan device, swapchain or rendering is created. |

Production SDK types remain in the narrow private bridge headers. This test is
explicitly allowed to inspect SDL/Win32 state and SDK bridge objects. The Vulkan
test uses the declared installed SDK `Include` directory; all entry points come
from the production SDL loader, never `vulkan-1.lib`. GL-only builds need no
external Vulkan SDK headers.

## Reproduction

Build the current production configuration first, using the main project build
workflow. Debug and Release archives must match the probe configuration and
come from this repository. Stale platform source/header timestamps are rejected
at probe configuration. `production-imports.json` records imported archive
hashes, production cache hash and current Window/input-state source hashes; verification
rejects changed archives or Window/input-state source. The input verifier also
checks the probe sources and production cache identity.

The current reproduction path is **S3**, not the preserved S2 import. Build the
current production `main-debug-001` / `main-release-001` first, then configure
matching `probe-debug-001` / `probe-release-001` with `-InputValidation`. Reusing
an S2 archive after current Window/input-state changes is rejected as stale; do
not weaken that check or rewrite an old identity file to make it fit. Existing
S2 results below remain historical evidence.

The example uses the current S3 build directories and fresh evidence names.
Change the evidence suffix if a directory already exists; earlier evidence is
never overwritten. After a source/configuration change, rebuild production and
reconfigure/rebuild the matching probe before verification.

```powershell
$Root = 'F:/GameDevelop/OpenGLProject/Symocraft'
$CMake = "$Root/out/m3-t2/tools/cmake-3.22.6/portable/cmake-3.22.6-windows-x86_64/bin/cmake.exe"
$CTest = "$Root/out/m3-t2/tools/cmake-3.22.6/portable/cmake-3.22.6-windows-x86_64/bin/ctest.exe"
$SdkHeaders = 'C:/VulkanSDK/1.4.363.0/Include'
$EvidenceSuffix = 'recheck-001'

& "$Root/test/experimental/platform-sdl3/build.ps1" -Configuration Debug `
    -ProductionBuild "$Root/out/m3-t2/s3/main-debug-001" `
    -BuildDirectory "$Root/out/m3-t2/s3/probe-debug-004" `
    -Vulkan -InputValidation -HeadersRoot $SdkHeaders -CMakePath $CMake

& "$Root/test/experimental/platform-sdl3/build.ps1" -Configuration Release `
    -ProductionBuild "$Root/out/m3-t2/s3/main-release-001" `
    -BuildDirectory "$Root/out/m3-t2/s3/probe-release-004" `
    -Vulkan -InputValidation -HeadersRoot $SdkHeaders -CMakePath $CMake

& "$Root/test/experimental/platform-sdl3/verify-probes.ps1" `
    -Executable "$Root/out/m3-t2/s3/probe-debug-004/SymoCraftPlatformSdl3Probe.exe" `
    -OutputDirectory "$Root/out/m3-t2/s3/runtime-probe-debug-$EvidenceSuffix" -Vulkan

& "$Root/test/experimental/platform-sdl3/verify-probes.ps1" `
    -Executable "$Root/out/m3-t2/s3/probe-release-004/SymoCraftPlatformSdl3Probe.exe" `
    -OutputDirectory "$Root/out/m3-t2/s3/runtime-probe-release-$EvidenceSuffix" -Vulkan
```

The helper defaults to MSVC 14.38 and eight build jobs. To build GL-only, omit
`-Vulkan` and `-HeadersRoot` and use a separate probe cache; also omit `-Vulkan`
from verification. That configuration replaces the two Vulkan runtime cases
and Vulkan surface fixture with a Vulkan-disabled diagnostic case, for 15 cases.

GUI verification needs desktop execution permission and is never registered in
the pure CTest suite. Each child process is launched hidden, has a 30-second
timeout and retains its process handle before waiting. `summary.json` includes
actual non-null exit codes, timeout status, executable/fixture identities,
production imports and each JSON report. Production logger text is preserved
on stderr while stdout is one JSON document. An expected negative case must
return 1 with the required diagnostic and cleanup evidence; it is not a
successful rendering result.

## Vulkan Enabled Case Matrix

Eight cases run without the failure fixture:

| Case | Required result |
| --- | --- |
| `gl` | Actual GL 4.6 Core, 4 samples/1 sample buffer, VSync 1/0, eight nonblank presentations and repeated cleanup. Debug requires its debug-context flag. |
| `native` | Valid production HWND, no GL context, mode-query rejection and WM_CLOSE retained by wait/poll. |
| `benchmark-window` | Actual 1920x1080, SDL borderless/fixed/nonexclusive/non-topmost flags, native client bounds without chrome, NonRudeHWND and wait/close. |
| `lifecycle` | Three Init/Create/Destroy/Free cycles, repeated calls and preservation of another SDL video owner's reference. |
| `initialization-failure` | Real SDL initialization failure with an invalid video driver, not a fixture success. |
| `opengl-library-failure` | Real window creation failure with a nonexistent OpenGL library path. |
| `vulkan` | Real extensions, instance, surface, devices and surface/instance/window/video cleanup; no GL context. |
| `vulkan-library-failure` | Real window creation failure with a nonexistent Vulkan loader library path. |

Nine additional cases are explicitly **restricted failure injection**, not
claims that a real GPU driver produced these failures:

| Case | Injected boundary and checked cleanup |
| --- | --- |
| `restricted-create-window` | SDL window allocation returns null; SDL video is released. |
| `restricted-create-context` | GL context allocation returns null after a real window exists; the window and video are released. |
| `restricted-make-current` | Current-context operation reports failure after real context allocation; context, window and video are released. |
| `restricted-set-vsync` | Swap-interval operation reports failure instead of silently continuing. |
| `restricted-present` | Swap operation reports failure, ending normal submission. |
| `restricted-query-extension` | A required GL enumeration procedure is unavailable with an owned diagnostic; this is not legitimate unsupported capability. |
| `restricted-get-procedure` | A required GLAD procedure is unavailable through the real production bridge; GLAD loading fails. |
| `restricted-destroy-context-after-release` | Real context release occurs first, then a secondary failure is reported; window destruction and SDL video release must still execute. This does not prove recovery from a real driver retaining a failed context. |
| `restricted-surface` | Surface creation reports failure after a real Vulkan instance exists; the instance, window and video are released. |

The test-only fixture DLL uses `SDL3_DYNAMIC_API` and the probe-only exported
`SDL_DYNAPI_entry` to obtain original functions from the **same static SDL**.
It replaces only selected negative-case entries and records real cleanup calls.
There are no production fault switches, vendor modifications or fixture
installation rules. Every injected case must record exactly one injection,
matching created/destroyed window and context counts, and an inactive video
subsystem after failure. The surface case also requires actual instance cleanup.

## S3 Input and Window Validation

The S3 additions are explicit opt-in targets under `-InputValidation`; they do
not add GUI tests to the main project's pure CTest run. The runtime targets
import the same actual production archives as the mode probe. Only
`SymoCraftPlatformInputStateTests` compiles the actual private `input_state.h`
reducer directly, without initializing SDL or compiling another Window.
`SymoCraftPlatformInputOptionsTests` separately checks the wide-character
argument parser without SDL or a GPU, including Unicode output paths.

| Target / helper | Scope and required evidence |
| --- | --- |
| `SymoCraftPlatformInputStateTests` / `platform.input_state` | Current 263-check private reducer regression: eleven scancodes, short-press latches/repeats, ordered motion/floating wheel, held/Reset, focus/minimize/restore and authoritative fact reconciliation, close and Benchmark isolation. Synthetic data only; not a runtime or hardware pass. |
| `SymoCraftPlatformInputOptionsTests` / `platform.input_options` | CPU-only argument parsing: seven normal cases, required fresh-directory argument for `hardware-manual`, wide Unicode paths, reordered arguments, duplicate/missing values and unknown-option rejection. No window or hardware acceptance. |
| `SymoCraftPlatformInputTests` / `verify-input.ps1 -Repeat 3` | Seven real-production runtime cases per iteration (21 child runs), with non-null exit codes, cleanup and fresh summaries. SDL queue fixtures and owned Win32 messages are not physical hardware. |
| `SymoCraftPlatformAllocationTests` / `verify-allocation.ps1` | Four one-shot allocation failures in the actual imported production library; preserves the prior nonempty snapshot, rejects continued Capture after an event-buffer failure and checks context/window/video cleanup. No production fault switch. |
| `verify-application.ps1` | Seven owned-window operations against the installed game, with a structured protocol inspector. Three-second diagnostics only, not Q06 sampling or a performance budget. |
| `hardware-manual` / `run-manual-input.ps1` | Actual production Window, physical keyboard/mouse, default enabled SDL Raw Input policy and recorded observations. No automatic human acceptance. |

Run the pure reducer and the two runtime helpers for both configurations:

```powershell
foreach ($Configuration in @('Debug', 'Release')) {
    $Name = $Configuration.ToLowerInvariant()
    $Probe = "$Root/out/m3-t2/s3/probe-$Name-004"
    & $CTest --test-dir $Probe -C $Configuration --output-on-failure --no-tests=error
    if ($LASTEXITCODE -ne 0) { throw 'Production input-state CTest failed.' }

    & "$Root/test/experimental/platform-sdl3/verify-input.ps1" `
        -Executable "$Probe/SymoCraftPlatformInputTests.exe" `
        -OutputDirectory "$Root/out/m3-t2/s3/runtime-input-$Name-$EvidenceSuffix" -Repeat 3

    & "$Root/test/experimental/platform-sdl3/verify-allocation.ps1" `
        -Executable "$Probe/SymoCraftPlatformAllocationTests.exe" `
        -OutputDirectory "$Root/out/m3-t2/s3/runtime-allocation-$Name-$EvidenceSuffix"
}
```

The seven input cases are:

| Case | Boundary exercised |
| --- | --- |
| `queue` | Same-drain project-key taps, foreign-window filtering, repeats, ordered pointer events, Reset and physical-state resampling. |
| `wait-input` | Owned queued first input retained by real production Wait/Poll and consumed once, with stale pre-wait SDL errors cleared. |
| `native-keyboard` | Owned Windows key messages translated by SDL for the current layout, including the retained SDL held state and Reset. This case explicitly disables Raw Input to exercise that translated fixture path. |
| `focus` | Actual minimize/restore and owned focus operations with authoritative Window facts; unsuccessful OS focus requests remain partial rather than passed. |
| `cursor-resize` | Production Normal/Hidden/Lock changes and actual logical/pixel resize queries. Not the full DPI or cross-screen matrix. |
| `benchmark` | Player keys/buttons/motion/wheel rejected; queued own Escape cancellation retained across relevant focus ordering and consumed once. |
| `wait-close` | Owned WM_CLOSE as the first waiting event, through real production Window wait/close. |

`partial` or missing cleanup/exit evidence is rejected by the helper, not treated
as pass. Synthetic button fixtures must match the resampled SDL physical state;
they cannot prove physically held mouse buttons. Automated translated keyboard
results cannot establish the default Raw Input hardware path or other layouts.

The four allocation cases are `window-object`, `input-reserve`,
`event-growth-poll` and `event-growth-wait`. The test executable arms one actual
C++ allocation failure, catches the named production diagnostic and requires
exactly one injection. Pointer-vector growth is calibrated from the current
MSVC runtime: the current 14.38 build grows reserve 64 to capacity **96**, not a
hard-coded 128. Verification checks the reported calibrated capacity and actual
snapshot/failure/cleanup evidence; it does not pass because a requested fixed
allocation size happened never to occur. Fixture-created resources and external
video-owner preservation are reported separately from the failed operation.

### Installed Game Diagnostics

The current generated inspector is
[`out/m3-t2/s3/inspect-protocol.exe`](../../../out/m3-t2/s3/inspect-protocol.exe),
with [inspection source](../../../out/m3-t2/s3/inspect-protocol.cpp). It reads the
result through structured protocol parsing, not substring assertions. Supply
the verified inspector and current installed game; the helper records both exe
hashes. Use an absolute output path (this helper does not apply the other
helpers' repository-relative output resolution).

```powershell
foreach ($Name in @('debug', 'release')) {
    & "$Root/test/experimental/platform-sdl3/verify-application.ps1" `
        -Executable "$Root/out/m3-t2/s3/install/main-$Name-001/SymoCraft.exe" `
        -ProtocolInspector "$Root/out/m3-t2/s3/inspect-protocol.exe" `
        -OutputDirectory "$Root/out/m3-t2/s3/runtime-application-$Name-$EvidenceSuffix"
}
```

| Application case | Required observation |
| --- | --- |
| `paused-close` / `minimized-close` | Owned focus transfer / actual minimization followed by owned WM_CLOSE and normal game shutdown, not a forced-kill success. |
| `allow-unfocused` | Actual unfocused frames retained, completed run valid, exit 0. |
| `strict-unfocused` | Actual unfocused frames, completed run invalid with the focus reason, exit 4. |
| `benchmark-minimized` / `benchmark-resized` | Actual owned minimize / resize, incomplete run invalid with `framebuffer-changed-or-minimized`, exit 4. |
| `benchmark-close` | Owned WM_CLOSE, incomplete run invalid with duration-not-completed, exit 4 and normal cleanup. |

The Benchmark cases use static workload, zero warmup and three seconds of
sampling at actual 1920x1080 with VSync disabled. They do not prove walk/edit
sampling, Q06/Q07 acceptance or physical Escape/play input. A successful
`paused-close` / `minimized-close` shows the actual OS operation and game exit;
there is no dynamic trace proving entry into the application's Wait branch or
restoration `dt=0`. The independent `wait-input` / `wait-close` cases exercise
real Window wait directly. Do not substitute these scopes for one another.

### Physical Hardware Observations

Use the actual `SymoCraftPlatformInputTests.exe --case hardware-manual` through
the launcher, which supplies a fresh existing `--output-directory`, verifies
current imports/source identities and removes inherited fixture/Raw-disable
environment settings:

```powershell
& "$Root/test/experimental/platform-sdl3/run-manual-input.ps1" `
    -Executable "$Root/out/m3-t2/s3/probe-release-004/SymoCraftPlatformInputTests.exe" `
    -OutputDirectory "$Root/out/m3-t2/s3/hardware-manual-desktop-$EvidenceSuffix"
```

Click the color window and use real controls. The **test executable only** adds
F5 (Reset, then fresh Capture), F6 (Normal -> Hidden -> Lock -> Normal) and F7
(immediate second Capture). These are not game shortcuts. Test Escape last; it
is logged before exit. The session has a ten-minute safety limit and records
ordered snapshots, window logical/pixel facts, focus/mode/layout observations,
session/exe identities and exit metadata. It injects no SDL/Win32/global input
and does not disable the production default Raw Input policy.

`human_acceptance_passed=false` is intentional even after a normal exit. Fill
the separate [S3 manual checklist](../../../docs/milestones/m3-t2/s3-manual-verification.md)
for the desktop and Y9000P, with actual observations and package/exe identity.
The tool does not establish gameplay resume `dt=0`, complete DPI/taskbar
usability or fifteen-minute gameplay. The already approved earlier continuous
tool feel and Q04/P06 decisions are not reopened; S3 production hardware checks
remain distinct. Double-machine fifteen-minute play and Q06/Q07 belong to S5.

### S3 Result Recording

Current reproduction identities are `s3/main-debug-001`,
`s3/main-release-001`, `s3/probe-debug-003` and `s3/probe-release-003`.
On 2026-10-08, main CTest passed 62 enabled Debug tests (the original allocation
test remains disabled) and 63 Release tests. The actual private reducer passed
263 checks per configuration. Final production input runs
`runtime-input-debug-003` and `runtime-input-release-003` each passed 21/21;
earlier foreground-denied partial runs remain preserved, not rewritten.
Allocation `runtime-allocation-*-002` each passed 4/4, including observed
nonempty snapshot preservation; three-mode `runtime-probe-*-001` each passed
17/17 against this stage's actual production archives. Both actual games
completed 120 frames and normal cleanup. Release application `004` passed 6/7;
the fixed benchmark window rejected the forced resize and the case remains
unavailable/partial, not passed or a proven product regression. Querying native
pixels requires a local DPI-aware probe-thread scope; its prior DPI context is
restored without changing game policy. Final manual tool startup, JSONL output
and owned close are smoke-tested separately, with no physical input acceptance.
The portable development package is `s3/install/candidate-release-002`; its
`run-hardware-check.ps1` verifies packaged file hashes without requiring the
original build archives on Y9000P. Hardware, default Raw Input, alternate
layouts, DPI/multimonitor/taskbar and actual benchmark size-change validation
remain open in the manual checklist. Idle wait may wake on OS events, so this
run does not claim forced-timeout proof. Neither S3 overall nor T2 is accepted.
Keep failed attempts and old identity files unchanged; S3 observations do not
retroactively upgrade independent preparation or S2 evidence.

## S2 Verified Results and Preserved Failures

On 2026-10-07, the following production-bound runs passed all 17 cases with no
timeouts or missing exit codes:

| Configuration | Probe build | Production import | Successful evidence |
| --- | --- | --- | --- |
| Debug | `out/m3-t2/s2/probe-debug-002` | `out/m3-t2/s2/main-debug-001` | `out/m3-t2/s2/runtime-probe-debug-003/summary.json` |
| Release | `out/m3-t2/s2/probe-release-001` | `out/m3-t2/s2/main-release-002` | `out/m3-t2/s2/runtime-probe-release-001/summary.json` |

Actual GL was `4.6.0 NVIDIA 616.64` on RTX 5070 Ti with Core profile 1,
4 samples and one sample buffer. Context flags were 2 for Debug and 0 for
Release. Readback RGB was 31/122/69. Benchmark native window and client bounds
were both 1920x1080 with zero client offset. Vulkan used header version 363,
loader API 4211051 (1.4.363), `VK_KHR_surface,VK_KHR_win32_surface` and two
physical devices. Both successful runs' executable, fixture, imported archives,
production cache and Window source hashes were rechecked against their evidence.

Two earlier Debug failures are preserved unchanged:

- `runtime-probe-debug-001`: 6/17. PowerShell 7.6/.NET null environment assignment
  left an empty SDL library path, causing `SDL_CreateWindow: Failed loading` in
  GL/Vulkan cases. The verifier now truly deletes isolation variables with
  `Remove-Item Env:NAME`; restoring an originally absent variable also deletes
  it. Syntax and deletion behavior were checked in PowerShell 7.6.5 and Windows
  PowerShell 5.1.26100.9444.
- `runtime-probe-debug-002`: 16/17. A probe assertion incorrectly rejected the
  `WS_CAPTION` bit. Fixed SDL source retains that bit for native windowed/taskbar
  behavior and removes actual chrome in WM_NCCALCSIZE. The probe now checks
  actual SDL flags and native window/client geometry. Production hints and
  window policy were not changed to satisfy the test.

These probes do not close physical input/layout/acceleration, DPI/multimonitor,
taskbar usability, human gameplay, Y9000P or real driver-failure acceptance.
The authoritative stage status and remaining scope are recorded in
`docs/milestones/m3-t2/README.md` and the formal M3-T2 spec.

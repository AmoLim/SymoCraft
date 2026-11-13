# Developer Scripts

> 已验收候选及保留限制见 [change 验证记录](../docs/openspec/changes/integrate-renderdoc-tracy/verification.md)。构建和运行命令以仓库根目录为准。

Python 3.10+ is required. These entry points use only the standard library, plus
the existing external tools called through `subprocess`. No packages or tools
are installed automatically.

| Entry Point | Scope |
| --- | --- |
| `build.py` | VS 2026 / MSVC x64 configure/build/test/install; Windows only |
| `verify-runtime.py` | bounded game run, exit code, markers and frame count |
| `verify-runtime-faults.py` | invalid OpenGL assets in copied packages only |
| `capture-migration-inputs.py` | SHA-256 verified source snapshot and Git evidence |

```sh
python scripts/build.py --configuration Debug --clion-path "C:/path/to/CLion"
python scripts/build.py --configuration Release --action Install --clion-path "C:/path/to/CLion"
python scripts/build.py --build-directory out/build/custom --action Test
python scripts/verify-runtime.py --executable out/install/windows-release/SymoCraft.exe --output-directory out/runtime/new-run --frames 120 --seed 424242
python scripts/verify-runtime-faults.py --executable out/install/windows-release/SymoCraft.exe --output-directory out/faults/new-run
python scripts/capture-migration-inputs.py --output-directory out/snapshots/new-run
python -m unittest discover -s test/scripts -v
```

Run an entry point with `--help` for its complete parameter list. The old
`-Executable`, `-OutputDirectory`, etc. spellings are also accepted by Python.
The game scenario entry is `--profile-scenario static|walk|edit`; it takes one scenario per process.

## Build Setup And Actions

Use a complete Visual Studio 2026 C++ installation, Windows SDK, CMake 3.22+
and Ninja. MSVC needs its English compiler language pack: the Ninja dependency
checks require `1033/clui.dll`, `VSLANG=1033` and the English include prefix.
After changing compiler, SDK or generator, or repairing language detection, use a new
build directory rather than trusting an old dependency database.

In CLion, select an x64 Visual Studio toolchain named `Symocraft MSVC`, with
CMake and Ninja available, and enable `windows-debug` / `windows-release`.
CLion import is not evidence that CTest ran. The helper can instead locate
VS2026, load the x64 environment and use CLion's bundled CMake/Ninja without
opening the IDE; override local paths through its options or `CLION_HOME`.

The build and native-verification helpers default to the stable VS 2026
installation, MSVC tools `14.51.36231` and Windows SDK `10.0.28000.0`.
They verify the selected toolset/SDK rather than silently falling back to VS2022.
An explicit installation and toolset can still select the retained old compiler.
Use a fresh `--build-directory out/build/msvc51-sdk28000-debug` (or `msvc51-sdk28000-release`)
when switching compilers or SDKs; `build.py` refuses a cache with a different compiler without
changing or deleting it. In CLion, select the VS2026 installation for the named
toolchain and initialize the same toolset/SDK; create fresh CMake profiles/caches.
The scripts do not change shared IDE settings. Existing builds and results remain
bound to their original compiler. SDK/DXC and third-party upgrades are separate.

The 2026-11-02 approved SDK baseline is `10.0.28000.0`; existing `10.0.22621.0`
builds and historical results stay unchanged. For a custom SDK installation,
the helper passes the selected `WindowsSdkDir` to CMake's child-process
`CMAKE_WINDOWS_KITS_10_DIR` so app-local UCRT deployment uses that SDK.
When invoking CMake directly, set the same variable in the developer terminal
if CMake cannot find the selected SDK's UCRT. Do not change system-wide paths.

| `build.py` Option | Meaning |
| --- | --- |
| `--configuration Debug/Release` | Select the matching preset; default Debug |
| `--action Configure` | Configure only |
| `--action Build` | Configure and build enabled default targets |
| `--action Test` | Configure, build and run CTest; default, zero tests is an error |
| `--action Install` | Run configure/build/test successfully, then install |
| `--build-directory PATH` | Repository-relative or absolute build directory; never deletes an old tree |
| `--jobs N` | Build parallelism, 1..64; default 8 |
| `--visual-studio-path`, `--toolset-version` | Select already installed VS/tools; no download |
| `--sdk-version` | Select an installed Windows SDK; default `10.0.28000.0` |
| `--clion-path` or `--cmake-path` / `--ninja-path` | Select existing tools without storing machine paths in the repo |

The helper has no arbitrary CMake-option passthrough. Configure CPU-only or
D3D12 with explicit CMake options in an initialized x64 developer terminal,
using the [root build instructions](../README.md#快速开始).
D3D12 candidates additionally require benchmark OFF, CPU-only OFF and a fresh
child build directory below `out/m3-t3/`; the production POST_BUILD identity
step rejects other roots/combinations. See the
[engineering contract](../docs/architecture/engineering.md#d3d12-候选身份限制).

## Profiling Build

`windows-profiling` inherits the Release game configuration, retains optimization,
and enables `SYMOCRAFT_ENABLE_TRACY` and `SYMOCRAFT_GPU_DEBUG_MARKERS`. Native
D3D12/Vulkan names and command regions use the second option independently of Tracy.
Ordinary Debug/Release presets explicitly keep both options OFF. CPU-only locally
forces both OFF, even if their cache entries were requested ON.

In an x64 VS2026 environment using MSVC `14.51.36231` and SDK `10.0.28000.0`:

```sh
cmake --preset windows-profiling -B out/build/msvc51-sdk28000-profiling -DSYMOCRAFT_BUILD_BENCHMARK=OFF
cmake --build out/build/msvc51-sdk28000-profiling
```

Use the fixed local Tracy v0.14.1 client and a matching v0.14.1 Viewer; source,
commit, original license and archive SHA256 are in [SOURCES](../cmake/licenses/SOURCES.txt).
On this machine, use the user-managed external installations at
`F:\Toolchains\Tracy` (Viewer/capture/csvexport, 0.14.1) and
`F:\Toolchains\RenderDoc` (`qrenderdoc.exe`, 1.47). These take precedence over
the earlier worktree `out/tools` copies. Their relevant binaries were compared
byte-for-byte with the previously verified versions; this changes the external
tool location, not the vendored client or historical capture identities.
OFF needs no Tracy vendor; ON with missing inputs fails configuration explicitly.
The client is compiled once in its own static target, consumed only through the
private profiling facade. No Viewer or RenderDoc executable is installed with the
game. Release compilation keeps `/O2`, adds `/Zi`, and links `/DEBUG /OPT:REF /OPT:ICF`
to produce matching PDBs. Debug retains the Debug CRT and DBG01 across dependencies.

Before running an enabled build, verify `TRACY_NO_EXIT` is not set to a value
starting with `1`. That upstream override is unsupported: no source patch,
global environment rewrite, or forced termination substitutes for normal exit.
Tracy is local/on-demand with system tracing, automatic sampling, callstacks and
crash hooks disabled. Connect the matching Viewer to `127.0.0.1`. Production zones
cover existing app/world/simulation/renderer stages; Present zones measure CPU calls
and waits. Fixed plots identify per-frame counts and bytes. Tool captures change
costs and do not establish the historical CSV budgets or complete memory analysis.

Plots auto-scale: read the numeric axis before treating a low point as zero.
`renderer.draw` counts world draws plus an applicable selection draw;
`renderer.upload` counts Render-local transfers, while `mesh.upload` records
mesh-cache transfers. Zero upload does not mean that retained world meshes were
not drawn. The lightweight debug values are an owned snapshot API, not a new HUD.

Run a bounded workload without an output directory or external tools:

```sh
out/build/msvc51-sdk28000-profiling/bin/SymoCraft.exe --renderer d3d12 --profile-scenario edit --scenario-seconds 60
```

Choose `static`, `walk` or `edit`; duration is an integer from 1 to 1200, default 60.
The default fixed seed is 424242, resolution 1920x1080, MSAA 4x, VSync requested off.
Walk starts at the existing spawn checkpoint; static/edit use four-chunk, with 45
degree initial FOV. Esc/close cancel; loss of the requested size/minimization fails
with exit 3. Player input is isolated in this mode. Old `--benchmark`, `--output`,
warmup/sample and focus-preparation arguments are rejected before native startup.

For Vulkan, first configure `SYMOCRAFT_RENDER_VULKAN=ON` in a separate build directory
and provide the SDK root via `VULKAN_SDK` (this machine: `F:/SDK/VulkanSDK/1.4.363.0`).
Use `--renderer vulkan` on the resulting game. Do not mix backend assets from caches.

In external RenderDoc, set Executable Path to the absolute path of that same
`bin/SymoCraft.exe`, Working Directory to its `bin` directory, and Command-line
Arguments to the scenario invocation above without the executable name. Assets
resolve beside the executable, including generated shaders for the chosen backend.
Launch only this target; do not enable global process injection or child hooking.
Request a frame after the world is ready, then replay it on the actual supported
adapter and inspect world draws/atlas layers, applicable selection and event groups.
Capture each backend/scenario separately. The checked external tool is RenderDoc 1.47.
For a portable Vulkan ZIP installation that has not registered its implicit layer,
set child environment `VK_ADD_IMPLICIT_LAYER_PATH` to the tool directory containing
`renderdoc.json`, then launch from RenderDoc. This is scoped to that launched
process; do not change global loader/registry configuration. The installed loader
must support this variable. The local SDK/loader 1.4.363.0 path was checked; keep
actual tool/driver identity separate from the SPIR-V target (Vulkan 1.2). Resource names identify persistent meshes,
atlas, present/depth/MSAA, upload and command objects; only actual work gets a region.
Vulkan annotation availability is optional and diagnosed separately from validation.
Record RenderDoc version, build/PDB, shader configuration, backend/adapter/driver,
actual extent/MSAA/present/validation and scenario. A capture file alone is not replay
or visual acceptance. The current change's verification records actual results and
unrun checks. The old benchmark product and performance protocol are retired;
current correctness probes do not read their CSV/YAML outputs.

## Paths And Evidence

Runtime and fault output paths are relative to the caller's working
directory. Build and snapshot paths are relative to the repository root. New
evidence directories must not exist; failed evidence is retained, never erased
or reused. Paths with spaces and Unicode are passed as separate process arguments.
Only owned child processes are stopped on timeout, failure or interruption.

The build helper loads Microsoft's `VsDevCmd.bat` through `cmd.exe`, then runs
CMake, Ninja and CTest with an isolated environment. It does not invoke
PowerShell, change the caller's environment, or edit IDE/system settings.
Custom build directories use CTest's `--test-dir`, never a conflicting preset.
CTest uses the same Python interpreter that launched `build.py`.
CPU-only/game scope is controlled by CMake. The retired benchmark option defaults
OFF; ON fails explicitly before CPU-only overrides, including in old caches.

Snapshots require Git and ripgrep (`rg`) on PATH. Git patch output is preserved
as bytes rather than decoded and rewritten. The snapshot is reference-only.

## Retired Performance Product

`SymoCraftBenchmark`, `scripts/benchmark.py`/`.ps1`, old Session/CSV/YAML performance
exports and their dedicated timing/budget analyzers are retired. They are not
compatibility launchers or installed products. Use the bounded game scenarios and
external tools above. Historical commands, raw data, frozen packages and paused
Fix1/experimental sources remain unchanged; they keep their original candidate
and protocol identities and need not run against the current product.

## Compatibility And Migration Scope

The four remaining `.ps1` files are startup-only compatibility launchers. They
forward explicitly supplied parameters to Python and report nonzero exits as
failures. They look for `python.exe` or `py.exe`; set `SYMOCRAFT_PYTHON` to an
explicit Python executable when those commands are unavailable. If Python is
not installed, the launchers fail rather than silently skipping the work.
Copying a launcher alone is no longer sufficient: carry its adjacent `.py` and
`scripts_common.py` too, or invoke the Python entry point directly.

Retained experimental/integration entry points in `test/` use Python. Their adjacent READMEs contain updated commands. Historical
documentation, frozen evidence/source copies and vendor code remain unchanged.
The old benchmark launcher and its dedicated fixture tests are retired. CTest
registers the retained Python helper suites;
native tests and build/dependency/install rules stay in C++ and CMake.

## Experimental And Integration Entrypoints

The first seven migrations are CPU baseline capture/verification, vendor ZIP
identity, exploratory presample analysis, the two incremental-build checks,
and old/new migration runtime verification. The remaining entries cover SDL
capabilities and input, offline CMake auditing, publication diagnostic batches,
exploratory presampling, evidence collection and D3D12 builds/packages/probes.

Most test evidence paths are repository-relative and constrained below `out/`.
CPU snapshot/verification output and presample/publication paths retain their
caller-relative semantics. The three curated collectors accept only fresh,
unlinked directories below `docs/verification/archive/`; existing outputs and
old milestone evidence destinations are rejected. SDL defaults use separate
`t2-preparation/preparation-001` and `t2-input/input-v2-001` directories; choose
a new batch name for another collection. D3D12 requires an explicit destination
and retains caller-relative path semantics. Each raw copy is byte-verified and
registered with its source identity; sources and partial outputs remain on
failure. These collectors never clean source directories or upload evidence.
`out/` remains working storage, not a parallel permanent evidence archive.
Each change keeps one `verification.md`; attachments and raw-data inclusion are
off by default unless explicitly required. These existing collectors are not a
default archival step; their destination constraints remain unchanged.
Retention exceptions and cleanup authorization follow
[the documentation policy](../docs/DOCUMENTATION_MAP.md#每个-change-一份验证记录).
[VERIFICATION](../docs/verification/VERIFICATION.md) holds only key-node approvals
and navigation. Historical commands, frozen evidence and portable-package
`evidence/` working directories are not rewritten by this policy.
Use `--help` on each entry, including the old CamelCase
parameter aliases. Python list arguments use spaces, not PowerShell commas.

Native build orchestration lives in `experimental_build.py`, SDL report checks
in `platform_verification.py`, and owned Win32 GUI operations in
`windows_verification.py`. Platform-specific code is explicitly Windows-only.
The manual-input launchers never assign human acceptance. Partial focus/input
observations do not pass. Missing app-local CRT clean-machine evidence remains
unavailable even when a developer machine supplies a system fallback.
Allocation verification now also requires an observed nonempty snapshot for
the event-growth cases. The old launcher recorded this limitation but could
still report overall success; an unchanged empty snapshot is no longer accepted
as proof. A foreground-focus restriction can therefore leave these cases
unverified even when the native cleanup checks pass.

R1 install rules include the Python runner and its two common helpers, so the
portable package needs Python 3.10+ but not the original source tree. When
shipping `run-package-input.py`, also carry `manual_input.py`,
`verification_common.py`, and `scripts_common.py`, and bind all four into the
package identity manifest. Do not rewrite historical package manifests.

R1 host metadata now uses Python platform facts; uncollected CIM display fields
are recorded explicitly as unavailable. Exploratory presampling uses Win32 and
registry facts in its historical candidate. Its old benchmark helper was retired;
that collector is retained as historical source, not a supported new-product
workflow. Native fixture device/driver/capability observations remain the GPU proof.

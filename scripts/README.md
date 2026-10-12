# Developer Scripts

Python 3.10+ is required. These entry points use only the standard library, plus
the existing external tools called through `subprocess`. No packages or tools
are installed automatically.

| Entry Point | Scope |
| --- | --- |
| `build.py` | MSVC 2022 x64 configure/build/test/install; Windows only |
| `benchmark.py` | static/walk/edit collection, sensor logs, strict validity and statistics |
| `verify-runtime.py` | bounded game run, exit code, markers and frame count |
| `verify-runtime-faults.py` | invalid OpenGL assets in copied packages only |
| `capture-migration-inputs.py` | SHA-256 verified source snapshot and Git evidence |

```sh
python scripts/build.py --configuration Debug --clion-path "C:/path/to/CLion"
python scripts/build.py --configuration Release --action Install --clion-path "C:/path/to/CLion"
python scripts/build.py --build-directory out/build/custom --action Test
python scripts/benchmark.py --executable out/install/windows-release/SymoCraft.exe --output-directory out/benchmark/new-run --scenarios static walk edit --machine-label development
python scripts/verify-runtime.py --executable out/install/windows-release/SymoCraft.exe --output-directory out/runtime/new-run --frames 120 --seed 424242
python scripts/verify-runtime-faults.py --executable out/install/windows-release/SymoCraft.exe --output-directory out/faults/new-run
python scripts/capture-migration-inputs.py --output-directory out/snapshots/new-run
python -m unittest discover -s test/scripts -v
```

Run an entry point with `--help` for its complete parameter list. The old
`-Executable`, `-OutputDirectory`, etc. spellings are also accepted by Python.
Use separate scenario arguments (`--scenarios static walk`), not comma-separated
PowerShell array syntax, when invoking Python directly.

## Build Setup And Actions

Use a complete Visual Studio 2022 C++ installation, Windows SDK, CMake 3.22+
and Ninja. MSVC needs its English compiler language pack: the Ninja dependency
checks require `1033/clui.dll`, `VSLANG=1033` and the English include prefix.
After changing compiler/generator or repairing language detection, use a new
build directory rather than trusting an old dependency database.

In CLion, select an x64 Visual Studio toolchain named `Symocraft MSVC`, with
CMake and Ninja available, and enable `windows-debug` / `windows-release`.
CLion import is not evidence that CTest ran. The helper can instead locate
VS2022, load the x64 environment and use CLion's bundled CMake/Ninja without
opening the IDE; override local paths through its options or `CLION_HOME`.

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
| `--clion-path` or `--cmake-path` / `--ninja-path` | Select existing tools without storing machine paths in the repo |

The helper has no arbitrary CMake-option passthrough. Configure CPU-only or
D3D12 with explicit CMake options in an initialized x64 developer terminal,
using the [root build instructions](../README.md#构建与测试).
D3D12 candidates additionally require benchmark OFF, CPU-only OFF and a fresh
child build directory below `out/m3-t3/`; the production POST_BUILD identity
step rejects other roots/combinations. See the
[engineering contract](../docs/architecture/engineering.md#d3d12-候选身份限制).

## Paths And Evidence

Runtime, fault and benchmark output paths are relative to the caller's working
directory. Build and snapshot paths are relative to the repository root. New
evidence directories must not exist; failed evidence is retained, never erased
or reused. Paths with spaces and Unicode are passed as separate process arguments.
Only owned child processes are stopped on timeout, failure or interruption.

The build helper loads Microsoft's `VsDevCmd.bat` through `cmd.exe`, then runs
CMake, Ninja and CTest with an isolated environment. It does not invoke
PowerShell, change the caller's environment, or edit IDE/system settings.
Custom build directories use CTest's `--test-dir`, never a conflicting preset.
CTest uses the same Python interpreter that launched `build.py`.
CPU-only/game/benchmark target scope is still controlled by the existing CMake
options, not by the scripting language.

Snapshots require Git and ripgrep (`rg`) on PATH. Git patch output is preserved
as bytes rather than decoded and rewritten. The snapshot is reference-only.

## Benchmark Metadata

Collector version 3 keeps run/result JSON fields and nearest-rank p95/p99
statistics. `machine.json` now records `pythonVersion` instead of
`powershellVersion`, and hashes the common Python helper as well as the entry
point. On Windows, CPU topology, memory and battery use Win32 APIs; CPU name and
display driver identity use the registry, not CIM. Battery status records native
flags rather than WMI's `BatteryStatus` enum. OS fields come from Python's
`platform` module. On other systems, unavailable hardware fields are explicitly
marked unavailable rather than invented. Optional `nvidia-smi` remains an
external sensor collector; CPU temperature is not collected.

Portable file/process handling does not make the Windows-only game, MSVC build
or OpenGL-specific fault protocol cross-platform. Headless fixture tests do not
claim GPU or human gameplay acceptance.

## Compatibility And Migration Scope

The five existing `.ps1` files are startup-only compatibility launchers. They
forward explicitly supplied parameters to Python and report nonzero exits as
failures. They look for `python.exe` or `py.exe`; set `SYMOCRAFT_PYTHON` to an
explicit Python executable when those commands are unavailable. If Python is
not installed, the launchers fail rather than silently skipping the work.
Copying a launcher alone is no longer sufficient: carry its adjacent `.py` and
`scripts_common.py` too, or invoke the Python entry point directly.

The 31 current experimental/integration business entry points in `test/` also
use Python. Their adjacent READMEs contain updated commands. Historical
documentation, frozen evidence/source copies and vendor code remain unchanged.
The original `test/benchmark/benchmark_script_tests.ps1` remains solely to test
the five compatibility launchers. CTest registers both Python helper suites;
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
registry facts through the existing benchmark helper, not CIM. Native fixture
device/driver/capability observations remain the required GPU proof.

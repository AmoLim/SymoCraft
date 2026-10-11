SymoCraft T3 R1 D3D12 portable experiment

Windows x64 with a real D3D12-capable GPU is required. This package contains the
Release fixture, shader binaries, app-local MSVC runtime, and immutable origin
records. No developer SDK, DXC compiler, SDL runtime DLL, or game assets are needed.

The Windows Graphics Tools optional feature must supply the D3D12 Debug Layer.
Missing Debug Layer support is blocked evidence, never an implicit pass. The
fixture does not use WARP or silently fall back to another graphics backend.

Extract the entire package to a writable directory on Y9000P. From that directory:

powershell -NoProfile -ExecutionPolicy Bypass -File .\verify-probes.ps1 -Executable .\SymoCraftD3D12R1Probe.exe -OutputDirectory .\evidence\y9000p-release-001 -PackageManifest .\package-manifest.json -RunTimeoutProbe

The fixture opens and resizes a window, minimizes it for at least 10 seconds,
restores it, and then exits. The runner records machine/device identity, result
JSON, lifecycle, screenshots, stdout/stderr, exit codes, and bounded timeouts.
Use a new evidence directory for every run. Do not modify the manifest, executable,
shaders, verification runner, DLLs, licenses, or origin records before verification.

Review all five PNGs visually and return the complete evidence directory. The
timeout case is explicit injection, not an actual driver/device-loss test. Zero
extent is a labelled contract input; native minimization/restoration are real.

Origin manifests retain the builder's absolute paths only as provenance. Portable
verification validates packaged bytes via package-manifest.json and does not need
the original source tree or production archives on this machine.

This is test-only R1 evidence, not gameplay or T3 acceptance. Separate hardware
evidence and T2 acceptance must be reviewed before R1 freeze or entry into R2.

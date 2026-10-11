# Fix1 Independent Publication Collector

`collect-runs.ps1` runs the real `foundation.files` and `performance.export` test executables. It does not build a replacement publication implementation, start a GUI or Process Monitor, change registry settings, retry failed publication, sleep, or delete existing evidence.

Each invocation requires a new output directory and the exact executable paths. Each child process receives its own fresh `case-parent` directory; test-created descendants and failed source/target files stay at their original locations. `stdout.log`, `stderr.log`, `result.json`, `runs.jsonl`, `manifest.json`, and `summary.json` record actual attempts, process exits, timeout, source/executable/library identities, raw JSONL diagnostics, and the retained file scene. Source identity is a digest of the explicitly listed Fix1 source inputs, not a Git commit or a claim that an executable was built from those inputs; build provenance must also be recorded by the build owner.

```powershell
& .\test\experimental\publication-fix1\collect-runs.ps1 `
  -BuildDir 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\fix1\build-debug-001' `
  -Config Debug -Test both `
  -FilesExe 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\fix1\build-debug-001\test\unit\foundation\files_tests.exe' `
  -PerfExe 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\fix1\build-debug-001\test\unit\telemetry\performance_tests.exe' `
  -Output 'F:\GameDevelop\OpenGLProject\Symocraft\out\m3-t2\fix1\untraced-debug-001' `
  -RunCount 20 -NoApiDiagnostics
```

API diagnostics are enabled by default; `-ApiDiagnostics` makes that choice explicit. `-NoApiDiagnostics` sets `SYMOCRAFT_PUBLISH_DIAGNOSTICS=0` only in the child process, even if the parent had it enabled. The collector also supplies run/config/source/executable identity environment values for the test JSONL. It never changes the parent environment. Use the disabled option for the initial no-API-trace control; use a new directory for enabled diagnostic runs. If an external Process Monitor capture is active, record that capture and its configuration separately: this collector cannot certify the absence of external tracing.

`-Test` accepts `foundation.files`, `performance.export`, or `both`. `-RunCount` is limited to 1 through 20 per suite for the bounded F2 investigation batch. `-StopOnUnexpectedFailure` permits an early stop of the affected suite after a non-passing attempt; actual counts remain distinct from planned counts. `-TimeoutSeconds` defaults to 60. A timed-out child is terminated and recorded as incomplete, not as a numeric Win32 publication failure. A capture/launch/identity failure is separately recorded as a collector failure and stops that suite.

Only structured `publication_test` and `publish_api` numeric fields supply error codes. Localized `what()`/stderr text is never used to infer a code. An expected rejection, or `ReplaceFileW` returning 2 followed by a successful first `MoveFileExW`, is not counted as a product failure. Missing structured records or a missing raw code in a Win32 failure is diagnostic incompleteness, including empty stderr. API records are associated only with a preceding test `begin` on the same stream and PID/TID; unassociated events retain that limitation instead of guessing.

Process counts, structured test-operation counts, expected Win32 rejections, expected non-Win32 rejections, and observed API calls have separate denominators. A telemetry operation can make several publication calls. With API diagnostics disabled, `api_calls` is null rather than claiming that no API calls occurred. Suite-success counters are checked against the actually retained events; a diagnostic writer failure is not a green result.

Exit 0 requires all planned processes completed successfully, with complete structured diagnostics and no collector error. Exit 1 keeps all records and reports the separate product, diagnostic, and collector statuses. The collector cannot close M3-I1/M3-I2, prove a root cause, or satisfy V05's 400-process final acceptance. Those require the evidence and subsequent verification specified in `docs/spec/m3-t2-fix1-spec.md`.

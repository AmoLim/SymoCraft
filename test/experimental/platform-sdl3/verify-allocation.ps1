[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [ValidateRange(1, 120)][int]$TimeoutSeconds = 30
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$executablePath = (Get-Item -LiteralPath $Executable).FullName
$importsPath = Join-Path (Split-Path $executablePath) 'production-imports.json'
$imports = Get-Content -LiteralPath $importsPath -Raw | ConvertFrom-Json
$windowSource = Join-Path $root 'game/modules/platform/src/window.cpp'
$inputSource = Join-Path $root 'game/modules/platform/src/input_state.h'
$windowHash = (Get-FileHash -LiteralPath $windowSource -Algorithm SHA256).Hash.ToLowerInvariant()
$inputHash = (Get-FileHash -LiteralPath $inputSource -Algorithm SHA256).Hash.ToLowerInvariant()
if ($windowHash -ne $imports.current_window_source_sha256 -or
    $inputHash -ne $imports.current_input_state_source_sha256) {
    throw 'Production platform sources changed after probe configuration; rebuild production and the probe first.'
}
foreach ($entry in $imports.imports) {
    if ((Get-FileHash -LiteralPath $entry.archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw 'A production archive changed after probe configuration; rebuild the probe before recording evidence.'
    }
}
$cachePath = Join-Path $imports.production_build 'CMakeCache.txt'
if ((Get-FileHash -LiteralPath $cachePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $imports.production_cache_sha256) {
    throw 'The imported production configuration changed; rebuild and reconfigure the probe first.'
}
$output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) {
    $OutputDirectory
} else {
    Join-Path $root $OutputDirectory
}))
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Evidence must stay under workspace out.'
}
if (Test-Path -LiteralPath $output) { throw 'Use a new directory; earlier evidence is never overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$executableHash = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant()
$environmentNames = @('SDL_VIDEO_DRIVER', 'SDL_OPENGL_LIBRARY', 'SDL_VULKAN_LIBRARY', 'SDL3_DYNAMIC_API')
$saved = @{}
foreach ($name in $environmentNames) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
$results = @()
try {
    foreach ($name in $environmentNames) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
    foreach ($case in @('window-object', 'input-reserve', 'event-growth-poll', 'event-growth-wait')) {
        $stdout = Join-Path $output "$case.stdout.log"
        $stderr = Join-Path $output "$case.stderr.log"
        $process = $null
        $exitCode = $null
        $timedOut = $false
        $report = $null
        $errorText = $null
        $passed = $false
        try {
            $process = Start-Process -FilePath $executablePath -ArgumentList @('--case', $case) `
                -WorkingDirectory $output -WindowStyle Hidden -PassThru `
                -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            $null = $process.Handle
            $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
            if ($timedOut) { $process.Kill() }
            $process.WaitForExit()
            $process.Refresh()
            $exitCode = $process.ExitCode
            $errors = [IO.File]::ReadAllText($stderr)
            $report = [IO.File]::ReadAllText($stdout) | ConvertFrom-Json
            $passed = -not $timedOut -and $null -ne $exitCode -and $exitCode -eq 0 -and
                $report.status -eq 'pass' -and $report.failures_injected -eq 1 -and
                $report.window_destroyed -and $report.gl_context_released -and
                $report.external_video_owner_preserved -and $report.sdl_shutdown_completed -and
                -not ($errors -match 'cleanup:|Window secondary cleanup failure:')
            if ($passed -and $case -eq 'window-object') {
                $passed = $report.windows_shown -eq 0 -and $report.windows_destroyed -eq 0 -and
                    $report.expected_diagnostic.Contains('Window::Create: Window/Impl allocation failed')
            } elseif ($passed -and $case -eq 'input-reserve') {
                $passed = $report.windows_shown -eq 1 -and $report.windows_destroyed -eq 1 -and
                    $report.actual_context_observed -and $report.context_gone_before_window_destroy_event -and
                    $report.expected_diagnostic.Contains('Window::Create: input buffers could not be allocated')
            } elseif ($passed) {
                $passed = $report.windows_shown -eq 1 -and $report.windows_destroyed -eq 1 -and
                    $report.actual_context_observed -and $report.context_gone_before_window_destroy_event -and
                    $report.calibrated_growth_capacity -gt 64 -and $report.previous_snapshot_preserved -and
                    $report.capture_rethrows_saved_failure -and
                    $report.expected_diagnostic.Contains('Window input event adaptation failed') -and
                    $report.capture_rethrow_diagnostic.Contains('Window input event adaptation failed')
            }
        } catch {
            $passed = $false
            $errorText = $_.Exception.Message
        } finally {
            if ($process) {
                if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
                $process.Dispose()
            }
        }
        $results += [pscustomobject]@{
            case = $case
            expected_exit_code = 0
            exit_code = $exitCode
            timed_out = $timedOut
            passed = $passed
            harness_error = $errorText
            report = $report
            stdout = $stdout
            stderr = $stderr
        }
    }
} finally {
    foreach ($name in $environmentNames) {
        if ($null -eq $saved[$name]) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
        else { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
}
$identityUnchanged = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant() -eq $executableHash -and
    (Get-FileHash -LiteralPath $windowSource -Algorithm SHA256).Hash.ToLowerInvariant() -eq $windowHash -and
    (Get-FileHash -LiteralPath $inputSource -Algorithm SHA256).Hash.ToLowerInvariant() -eq $inputHash
$nonemptySnapshotProof = @($results | Where-Object {
    $_.case.StartsWith('event-growth-') -and
    ($null -eq $_.report -or
     $_.report.PSObject.Properties.Name -notcontains 'previous_nonempty_snapshot_observed' -or
     -not $_.report.previous_nonempty_snapshot_observed)
}).Count -eq 0
$summary = [ordered]@{
    executable = $executablePath
    executable_sha256 = $executableHash
    probe_source_sha256 = (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'allocation_tests.cpp') -Algorithm SHA256).Hash.ToLowerInvariant()
    verification_script_sha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash.ToLowerInvariant()
    production_imports_sha256 = (Get-FileHash -LiteralPath $importsPath -Algorithm SHA256).Hash.ToLowerInvariant()
    production_imports = $imports
    source_and_executable_identity_unchanged = $identityUnchanged
    nonempty_snapshot_preservation_proof_complete = $nonemptySnapshotProof
    scope = 'Actual production archives; probe-only exact-size one-shot C++ allocation failures and synthetic queued reducer fault route. Expected faults and complete cleanup are self-checked; a passing case exits 0.'
    unverified = 'OS/SDL allocation exhaustion, real hardware or OS foreground focus, actual driver resource-release failures.'
    timeout_seconds = $TimeoutSeconds
    passed = $identityUnchanged -and @($results | Where-Object { -not $_.passed }).Count -eq 0
    cases = $results
}
$summary | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding UTF8
[pscustomobject]$summary | Select-Object passed, cases | ConvertTo-Json -Depth 9
if (-not $summary.passed) { throw "Production SDL3 allocation probes failed; see $output" }

[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [ValidateRange(1, 10)][int]$Repeat = 1
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$executablePath = (Get-Item -LiteralPath $Executable).FullName
$importsPath = Join-Path (Split-Path $executablePath) 'production-imports.json'
$imports = Get-Content -LiteralPath $importsPath -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath (Join-Path $imports.production_build 'CMakeCache.txt') -Algorithm SHA256).Hash.ToLowerInvariant() -ne $imports.production_cache_sha256) {
    throw 'Production configuration changed after probe configuration; rebuild the probe before recording evidence.'
}
foreach ($source in @('input_tests.cpp','input_test_events.h')) {
    if ((Get-Item -LiteralPath (Join-Path $PSScriptRoot $source)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $executablePath).LastWriteTimeUtc) {
        throw 'Input probe source changed after executable creation; rebuild the probe before recording evidence.'
    }
}
foreach ($source in @(
    @{path='game/modules/platform/src/window.cpp'; digest=$imports.current_window_source_sha256},
    @{path='game/modules/platform/src/input_state.h'; digest=$imports.current_input_state_source_sha256}
)) {
    if ((Get-FileHash -LiteralPath (Join-Path $root $source.path) -Algorithm SHA256).Hash.ToLowerInvariant() -ne $source.digest) {
        throw 'Production platform source changed after probe configuration; rebuild production and the probe first.'
    }
}
foreach ($entry in $imports.imports) {
    if ((Get-FileHash -LiteralPath $entry.archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw 'A production import changed after probe configuration; rebuild the probe before recording evidence.'
    }
}
$output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $root $OutputDirectory }))
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Evidence must stay under workspace out.'
}
if (Test-Path -LiteralPath $output) { throw 'Use a fresh directory; earlier evidence is never overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$environmentNames = @('SDL_VIDEO_DRIVER','SDL_OPENGL_LIBRARY','SDL_VULKAN_LIBRARY','SDL3_DYNAMIC_API','SDL_WINDOWS_RAW_KEYBOARD')
$saved = @{}
foreach ($name in $environmentNames) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
$results = @()
try {
    foreach ($name in $environmentNames) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
    for ($iteration = 1; $iteration -le $Repeat; ++$iteration) {
        foreach ($case in @('queue','wait-input','native-keyboard','focus','cursor-resize','benchmark','wait-close')) {
            $name = '{0}-{1:D3}' -f $case,$iteration
            $stdout = Join-Path $output ($name + '.stdout.log')
            $stderr = Join-Path $output ($name + '.stderr.log')
            $process = $null
            try {
                $process = Start-Process -FilePath $executablePath -ArgumentList @('--case',$case) `
                    -WorkingDirectory $output -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
                $null = $process.Handle
                $timedOut = -not $process.WaitForExit(30000)
                if ($timedOut) { $process.Kill() }
                $process.WaitForExit()
                $process.Refresh()
                $exitCode = $process.ExitCode
                $errors = [IO.File]::ReadAllText($stderr)
                $report = $null
                try { $report = [IO.File]::ReadAllText($stdout) | ConvertFrom-Json } catch { }
                $valid = -not $timedOut -and $null -ne $exitCode -and $null -ne $report -and
                    $null -ne $report.PSObject.Properties['window_destroyed'] -and
                    $null -ne $report.PSObject.Properties['sdl_shutdown_completed'] -and
                    $report.window_destroyed -and $report.sdl_shutdown_completed -and -not $errors.Contains('cleanup:')
                $passed = $valid -and $exitCode -eq 0 -and $report.status -eq 'pass' -and $report.checks -gt 0
                $partial = $valid -and $exitCode -eq 2 -and $report.status -eq 'partial'
                $results += [pscustomobject]@{
                    case=$case; iteration=$iteration; exit_code=$exitCode; timed_out=$timedOut
                    passed=$passed; partial=$partial; report=$report
                }
            } finally {
                if ($process) {
                    if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
                    $process.Dispose()
                }
            }
        }
    }
} finally {
    foreach ($name in $environmentNames) {
        if ($null -eq $saved[$name]) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
        else { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
}
$failed = @($results | Where-Object { -not $_.passed -and -not $_.partial }).Count
$partialCount = @($results | Where-Object { $_.partial }).Count
$summary = [ordered]@{
    executable=$executablePath
    executable_sha256=(Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant()
    production_imports=$imports
    scope='Actual imported production Window. SDL_PushEvent validates queue adaptation, owned Win32 messages validate the translated current-layout path; no synthetic test is physical hardware acceptance.'
    unverified='Physical held buttons/motion, default Raw Input path, alternate layouts, acceleration/feel, DPI/multimonitor/taskbar and other machines remain manual gates.'
    status=$(if ($failed -gt 0) { 'fail' } elseif ($partialCount -gt 0) { 'partial' } else { 'pass' })
    passed=($failed -eq 0 -and $partialCount -eq 0)
    partial_cases=$partialCount
    failed_cases=$failed
    cases=$results
}
$summary | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding UTF8
[pscustomobject]$summary | Select-Object status,passed,partial_cases,failed_cases,cases | ConvertTo-Json -Depth 9
if ($failed -gt 0) { throw "Production SDL3 input probes failed; see $output" }
if ($partialCount -gt 0) { throw "Production SDL3 input probes are partial, not passed; see $output" }

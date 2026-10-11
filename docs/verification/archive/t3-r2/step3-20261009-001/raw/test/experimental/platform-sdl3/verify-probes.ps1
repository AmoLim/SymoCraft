[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [switch]$Vulkan
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$executablePath = (Get-Item -LiteralPath $Executable).FullName
$fixture = (Get-Item -LiteralPath (Join-Path (Split-Path $executablePath) 'symocraft_platform_failure_fixture.dll')).FullName
$imports = Get-Content -LiteralPath (Join-Path (Split-Path $executablePath) 'production-imports.json') -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath (Join-Path $root 'game/modules/platform/src/window.cpp') -Algorithm SHA256).Hash.ToLowerInvariant() -ne $imports.current_window_source_sha256) {
    throw 'Production platform source changed after probe configuration; rebuild production and the probe first.'
}
if ((Get-FileHash -LiteralPath (Join-Path $root 'game/modules/platform/src/input_state.h') -Algorithm SHA256).Hash.ToLowerInvariant() -ne $imports.current_input_state_source_sha256) {
    throw 'Production input source changed after probe configuration; rebuild production and the probe first.'
}
if ((Get-FileHash -LiteralPath (Join-Path $imports.production_build 'CMakeCache.txt') -Algorithm SHA256).Hash.ToLowerInvariant() -ne $imports.production_cache_sha256) {
    throw 'Production configuration changed after probe configuration; rebuild the probe first.'
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
if (Test-Path -LiteralPath $output) { throw 'Use a new directory; earlier evidence is never overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$cases = @(
    @{name='gl'; mode='gl'; exit=0; diagnostic=''; env=@{}; extra=@(); fault=$false},
    @{name='native'; mode='native'; exit=0; diagnostic=''; env=@{}; extra=@('--wait-close'); fault=$false},
    @{name='benchmark-window'; mode='gl'; exit=0; diagnostic=''; env=@{}; extra=@('--benchmark','--allow-unfocused','--wait-close'); fault=$false},
    @{name='lifecycle'; mode='lifecycle'; exit=0; diagnostic=''; env=@{}; extra=@(); fault=$false},
    @{name='initialization-failure'; mode='native'; exit=1; diagnostic='SDL_InitSubSystem'; env=@{SDL_VIDEO_DRIVER='symocraft-invalid-driver'}; extra=@(); fault=$false},
    @{name='opengl-library-failure'; mode='gl'; exit=1; diagnostic='SDL_CreateWindow'; env=@{SDL_OPENGL_LIBRARY='symocraft-missing-opengl-library.dll'}; extra=@(); fault=$false}
)
if ($Vulkan) {
    $cases += @{name='vulkan'; mode='vulkan'; exit=0; diagnostic=''; env=@{}; extra=@('--wait-close'); fault=$false}
    $cases += @{name='vulkan-library-failure'; mode='vulkan'; exit=1; diagnostic='SDL_CreateWindow'; env=@{SDL_VULKAN_LIBRARY='symocraft-missing-vulkan-library.dll'}; extra=@(); fault=$false}
} else {
    $cases += @{name='vulkan-disabled'; mode='vulkan'; exit=1; diagnostic='Vulkan mode was not compiled'; env=@{}; extra=@(); fault=$false}
}
foreach ($name in @('create-window','create-context','make-current','set-vsync','present','query-extension','get-procedure','destroy-context-after-release')) {
    $cases += @{name="restricted-$name"; mode='gl'; exit=1; diagnostic=$name; env=@{SDL3_DYNAMIC_API=$fixture}; extra=@('--fault',$name); fault=$true}
}
if ($Vulkan) {
    $cases += @{name='restricted-surface'; mode='vulkan'; exit=1; diagnostic='surface'; env=@{SDL3_DYNAMIC_API=$fixture}; extra=@('--fault','surface'); fault=$true}
}
$environmentNames = @('SDL_VIDEO_DRIVER','SDL_OPENGL_LIBRARY','SDL_VULKAN_LIBRARY','SDL3_DYNAMIC_API')
$saved = @{}
foreach ($name in $environmentNames) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
$results = @()
try {
    foreach ($case in $cases) {
        foreach ($name in $environmentNames) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
        foreach ($name in $case.env.Keys) { [Environment]::SetEnvironmentVariable($name, $case.env[$name], 'Process') }
        $stdout = Join-Path $output ($case.name + '.stdout.log')
        $stderr = Join-Path $output ($case.name + '.stderr.log')
        $process = $null
        try {
            $process = Start-Process -FilePath $executablePath -ArgumentList (@('--mode',$case.mode,'--frames','8') + $case.extra) `
                -WorkingDirectory $output -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            # Cache the handle before waiting so Windows PowerShell retains the real exit code.
            $null = $process.Handle
            $timedOut = -not $process.WaitForExit(30000)
            if ($timedOut) { $process.Kill() }
            $process.WaitForExit()
            $process.Refresh()
            $exitCode = $process.ExitCode
            $errors = [IO.File]::ReadAllText($stderr)
            $report = $null
            try { $report = [IO.File]::ReadAllText($stdout) | ConvertFrom-Json } catch { }
            $passed = -not $timedOut -and $null -ne $exitCode -and $exitCode -eq $case.exit -and $null -ne $report
            if ($passed -and $case.exit -eq 0) {
                $passed = $report.status -eq 'pass' -and $report.window_destroyed -and $report.sdl_shutdown_completed -and
                    $report.destroy_repeated -and $report.free_repeated -and -not $errors.Contains('cleanup:')
                if ($passed -and $case.mode -eq 'gl') {
                    $passed = $report.gl_samples -eq 4 -and $report.gl_sample_buffers -eq 1 -and
                        $report.vsync_on_actual -eq 1 -and $report.vsync_off_actual -eq 0 -and $report.gl_presented_frames -gt 0
                }
                if ($passed -and $case.mode -eq 'vulkan') {
                    $passed = $report.vulkan_instance_created -and $report.vulkan_surface_created -and
                        $report.vulkan_surface_destroyed -and $report.vulkan_instance_destroyed
                }
                if ($passed -and $case.extra -contains '--wait-close') { $passed = $report.wait_close_received }
                if ($passed -and $case.mode -eq 'lifecycle') { $passed = $report.external_sdl_owner_preserved -and $report.init_repeated }
            } elseif ($passed) {
                $passed = $report.status -eq 'fail' -and $errors.Contains($case.diagnostic) -and -not $report.video_active_after_failure
                if ($passed -and $case.fault) {
                    $passed = $report.restricted_fixture_loaded -and $report.fixture_failures_injected -eq 1 -and
                        $report.fixture_windows_created -eq $report.fixture_windows_destroyed -and
                        $report.fixture_contexts_created -eq $report.fixture_contexts_destroyed -and $report.fixture_video_quit_calls -gt 0
                    if ($passed -and $case.name -eq 'restricted-surface') {
                        $passed = $report.vulkan_instance_created -and $report.vulkan_instance_destroyed -and
                            $report.fixture_surface_create_calls -eq 1 -and $report.fixture_surfaces_destroyed -eq 0
                    }
                }
            }
            $results += [pscustomobject]@{
                case=$case.name; expected_exit_code=$case.exit; exit_code=$exitCode; timed_out=$timedOut
                restricted_fault_injection=$case.fault; passed=$passed; report=$report
            }
        } finally {
            if ($process) {
                if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
                $process.Dispose()
            }
        }
    }
} finally {
    foreach ($name in $environmentNames) {
        if ($null -eq $saved[$name]) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
        else { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
}
$summary = [ordered]@{
    executable=$executablePath
    executable_sha256=(Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant()
    production_imports=$imports
    fixture_sha256=(Get-FileHash -LiteralPath $fixture -Algorithm SHA256).Hash.ToLowerInvariant()
    scope='Real same-source production platform libraries and SDL loader; restricted SDL dynamic-API fixture only in explicitly labelled negative cases, not actual driver-failure or physical-input proof.'
    unverified='Physical input/layout/acceleration, DPI/multimonitor/taskbar usability, gameplay, Y9000P, real driver cleanup failures and GPU backends remain separate.'
    passed=@($results | Where-Object { -not $_.passed }).Count -eq 0
    cases=$results
}
$summary | ConvertTo-Json -Depth 9 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding UTF8
[pscustomobject]$summary | Select-Object passed,cases | ConvertTo-Json -Depth 9
if (-not $summary.passed) { throw "Production SDL3 platform probes failed; see $output" }

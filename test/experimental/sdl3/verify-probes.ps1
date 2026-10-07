[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [switch]$Vulkan
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$executablePath = (Get-Item -LiteralPath $Executable).FullName
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw 'Use a new directory; earlier probe evidence is never overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$cases = @(
    @{name = 'gl'; mode = 'gl'; exit = 0; diagnostic = ''; driver = ''; extra = @()},
    @{name = 'native'; mode = 'native'; exit = 0; diagnostic = ''; driver = ''; extra = @()},
    @{name = 'benchmark-window'; mode = 'gl'; exit = 0; diagnostic = ''; driver = ''; extra = @('--benchmark', '--allow-unfocused', '--width', '1920', '--height', '1080', '--test-wait-close')},
    @{name = 'initialization-failure'; mode = 'native'; exit = 1; diagnostic = 'SDL_Init(SDL_INIT_VIDEO)'; driver = 'symocraft-invalid-driver'; extra = @()}
)
if ($Vulkan) {
    $cases += @{name = 'vulkan'; mode = 'vulkan'; exit = 0; diagnostic = ''; driver = ''; extra = @()}
} else {
    $cases += @{name = 'vulkan-disabled'; mode = 'vulkan'; exit = 1; diagnostic = 'Vulkan mode was not compiled'; driver = ''; extra = @()}
}
$results = @()
foreach ($case in $cases) {
    $stdout = Join-Path $output ($case.name + '.stdout.log')
    $stderr = Join-Path $output ($case.name + '.stderr.log')
    $oldDriver = [Environment]::GetEnvironmentVariable('SDL_VIDEO_DRIVER', 'Process')
    $process = $null
    try {
        if ($case.driver) { $env:SDL_VIDEO_DRIVER = $case.driver }
        else { [Environment]::SetEnvironmentVariable('SDL_VIDEO_DRIVER', $null, 'Process') }
        $arguments = @('--mode', $case.mode, '--frames', '8') + $case.extra
        $process = Start-Process -FilePath $executablePath -ArgumentList $arguments `
            -WorkingDirectory $output -WindowStyle Hidden -PassThru `
            -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        $timedOut = -not $process.WaitForExit(30000)
        if ($timedOut) { $process.Kill(); $process.WaitForExit() }
        $process.Refresh()
        $text = [IO.File]::ReadAllText($stdout)
        $errors = [IO.File]::ReadAllText($stderr)
        $report = $null
        try { $report = $text | ConvertFrom-Json } catch { }
        $passed = -not $timedOut -and $process.ExitCode -eq $case.exit -and $null -ne $report
        if ($case.exit -eq 0) {
            $passed = $passed -and $report.status -eq 'pass' -and $report.native_hwnd_valid -and
                $report.window_destroyed -and $report.sdl_shutdown_completed -and -not $errors.Contains('cleanup:')
        } else {
            $passed = $passed -and $report.status -eq 'fail' -and $errors.Contains($case.diagnostic)
        }
        $results += [pscustomobject]@{
            case = $case.name; expected_exit_code = $case.exit; exit_code = $process.ExitCode
            timed_out = $timedOut; passed = $passed; report = $report
        }
    } finally {
        if ($process) {
            if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
            $process.Dispose()
        }
        [Environment]::SetEnvironmentVariable('SDL_VIDEO_DRIVER', $oldDriver, 'Process')
    }
}
$summary = [ordered]@{
    executable = $executablePath
    executable_sha256 = (Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant()
    scope = 'Real independent SDL API/window capability and limited failure probes; not production game, input feel, DPI matrix or backend acceptance.'
    passed = @($results | Where-Object { -not $_.passed }).Count -eq 0
    cases = $results
}
$summary | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding UTF8
[pscustomobject]$summary | Select-Object passed,executable_sha256,cases | ConvertTo-Json -Depth 7
if (-not $summary.passed) { throw "SDL preparation probes failed; see $output" }

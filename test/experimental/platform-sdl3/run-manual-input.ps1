[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$executablePath = (Get-Item -LiteralPath $Executable).FullName
$imports = Get-Content -LiteralPath (Join-Path (Split-Path $executablePath) 'production-imports.json') -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath (Join-Path $imports.production_build 'CMakeCache.txt') -Algorithm SHA256).Hash.ToLowerInvariant() -ne $imports.production_cache_sha256) {
    throw 'Production configuration changed after probe configuration; rebuild the probe before recording evidence.'
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
        throw 'A production import changed after probe configuration; rebuild the probe before recording observations.'
    }
}
foreach ($source in @('input_tests.cpp','input_test_events.h')) {
    if ((Get-Item -LiteralPath (Join-Path $PSScriptRoot $source)).LastWriteTimeUtc -gt (Get-Item -LiteralPath $executablePath).LastWriteTimeUtc) {
        throw 'Input probe source changed after executable creation; rebuild the probe before recording observations.'
    }
}
$output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $root $OutputDirectory }))
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Manual observations must stay under workspace out.'
}
if (Test-Path -LiteralPath $output) { throw 'Use a fresh directory; earlier observations are never overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$identity = [ordered]@{
    started_utc=[DateTime]::UtcNow.ToString('o')
    executable=$executablePath
    executable_sha256=(Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant()
    production_imports=$imports
    input_probe_source_sha256=(Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'input_tests.cpp') -Algorithm SHA256).Hash.ToLowerInvariant()
    scope='Interactive physical hardware observations through actual production Window. Test-only F5 Reset, F6 cursor-mode cycle and F7 second capture; Escape is logged before exit. No automatic manual acceptance.'
    human_acceptance_passed=$false
}
$identity | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'manual-session-identity.json') -Encoding UTF8
$environmentNames = @('SDL_VIDEO_DRIVER','SDL_OPENGL_LIBRARY','SDL_VULKAN_LIBRARY','SDL3_DYNAMIC_API','SDL_WINDOWS_RAW_KEYBOARD')
$saved = @{}
foreach ($name in $environmentNames) { $saved[$name] = [Environment]::GetEnvironmentVariable($name, 'Process') }
try {
    foreach ($name in $environmentNames) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
    Write-Host 'Click the SDL3 color window to begin. F5 Reset; F6 Normal/Hidden/Lock; F7 second Capture. Test Escape last.'
    Write-Host "Console remains visible; detailed observations are saved in $output. This session does not automatically pass H01/H02/H09."
    # Direct invocation deliberately keeps interactive console instructions visible.
    & $executablePath '--case' 'hardware-manual' '--output-directory' $output
    $exitCode = $LASTEXITCODE
    [ordered]@{completed_utc=[DateTime]::UtcNow.ToString('o'); exit_code=$exitCode; human_acceptance_passed=$false} |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'manual-launch-result.json') -Encoding UTF8
    if ($exitCode -ne 0) { throw "Manual input observation tool failed with exit code $exitCode; see $output." }
    Write-Host 'Observation session ended. Review the snapshots and provide the separate human checklist result.'
} finally {
    foreach ($name in $environmentNames) {
        if ($null -eq $saved[$name]) { Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue }
        else { [Environment]::SetEnvironmentVariable($name, $saved[$name], 'Process') }
    }
}

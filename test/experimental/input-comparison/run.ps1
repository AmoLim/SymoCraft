[CmdletBinding()]
param(
    [ValidateSet('Both', 'GLFW', 'SDL3')]
    [string]$Backend = 'Both',
    [ValidateSet('0', '1')]
    [string]$SystemScale = '1',
    [string]$GlfwExecutable,
    [string]$SdlExecutable
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
if (-not $GlfwExecutable) { $GlfwExecutable = Join-Path $root 'out/m3-t2/probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe' }
if (-not $SdlExecutable) { $SdlExecutable = Join-Path $root 'out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe' }
$sessionRoot = Join-Path $root ('out/m3-t2/input-comparison/manual-' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [Guid]::NewGuid().ToString('N').Substring(0,8))
$backends = if ($Backend -eq 'Both') { @('GLFW', 'SDL3') } else { @($Backend) }
New-Item -ItemType Directory -Path $sessionRoot | Out-Null
Write-Host 'F1: Lock -> Normal -> Hidden; F2: reset view; F3: reset input; Escape: close this window.'
Write-Host 'The room uses the frozen game camera, but it is not a full gameplay test.'
Write-Host "Evidence: $sessionRoot"
foreach ($name in $backends) {
    $exe = (Get-Item -LiteralPath $(if ($name -eq 'GLFW') { $GlfwExecutable } else { $SdlExecutable })).FullName
    [ordered]@{ created_utc=[DateTime]::UtcNow.ToString('o'); backend=$name; executable=$exe; executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant(); system_scale_requested=$SystemScale; manual_result='not assessed automatically' } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $sessionRoot ($name + '-identity.json')) -Encoding utf8
    & $exe '--output' (Join-Path $sessionRoot $name) '--system-scale' $SystemScale
    if ($LASTEXITCODE -ne 0) { throw "$name input comparison exited with $LASTEXITCODE." }
    $identity = Get-Content -LiteralPath (Join-Path $sessionRoot ($name + '-identity.json')) -Raw | ConvertFrom-Json
    if ((Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant() -ne $identity.executable_sha256) { throw "$name executable changed during the manual run." }
}

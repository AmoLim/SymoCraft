[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$CMakePath
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
$output = [IO.Path]::GetFullPath($OutputDirectory)
foreach ($path in @($build, $output)) {
    if (!$path.StartsWith((Join-Path $root 'out') + '\', [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Incremental verification requires build/output paths inside this repository out directory.'
    }
}
New-Item -ItemType Directory -Path $output -Force | Out-Null
function Build-Probe([string]$Name) {
    $lines = & $CMakePath --build $build --target SymoCraft --parallel 8 2>&1
    $code = $LASTEXITCODE
    $lines | Set-Content -LiteralPath (Join-Path $output ($Name + '.log')) -Encoding utf8
    if ($code -ne 0) { throw "Incremental build failed: $Name ($code)" }
    return ($lines -join "`n")
}
$baseline = Build-Probe 'baseline'
if ($baseline -match 'Building CXX object') { throw 'The initial build was not settled. Rerun after production changes finish.' }
$results = [Collections.Generic.List[object]]::new()
foreach ($probe in @(
    @{ Name='private-renderer'; File='game/modules/renderer/src/batch.hpp' },
    @{ Name='public-scene'; File='game/modules/scene/include/symocraft/scene/mesh.h' }
)) {
    $file = Get-Item -LiteralPath (Join-Path $root $probe.File)
    $timestamp = $file.LastWriteTimeUtc
    $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
    try {
        $file.LastWriteTimeUtc = [DateTime]::UtcNow
        $log = Build-Probe $probe.Name
        $counts = [ordered]@{}
        foreach ($module in @('world','simulation','renderer','app')) {
            $directory = if ($module -eq 'app') { 'game[/\\]app' } else { "game[/\\]modules[/\\]$module" }
            $counts[$module] = [regex]::Matches($log, "Building CXX object $directory[/\\]").Count
        }
        if ($probe.Name -eq 'private-renderer') {
            if ($counts.world -ne 0 -or $counts.simulation -ne 0 -or $counts.app -ne 0 -or $counts.renderer -eq 0) {
                throw "Private renderer header rebuilt the wrong owners: $($counts | ConvertTo-Json -Compress)"
            }
        } elseif ($counts.world -eq 0 -or $counts.renderer -eq 0 -or $counts.app -eq 0) {
            throw "Public scene header did not rebuild its real consumers: $($counts | ConvertTo-Json -Compress)"
        }
        if ($log -notmatch 'Linking CXX executable .*SymoCraft.exe') { throw 'Changed library did not relink the game.' }
        $results.Add([pscustomobject]@{ Probe=$probe.Name; Header=$probe.File; Sha256=$hash; CompiledOwners=$counts; Relinked=$true })
    } finally {
        if ((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -eq $hash) {
            $file.LastWriteTimeUtc = $timestamp
        } else {
            throw 'A header changed concurrently; its timestamp was not restored. Rebuild and rerun the verification.'
        }
    }
}
$results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding utf8
$results | ConvertTo-Json -Depth 5

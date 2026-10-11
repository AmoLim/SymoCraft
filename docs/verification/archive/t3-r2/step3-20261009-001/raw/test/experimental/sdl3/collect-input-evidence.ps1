[CmdletBinding()]
param(
    [string]$AuditDirectory = 'out/m3-t2/cmake-3.22-offline-audit-005',
    [string]$OutputDirectory = 'docs/milestones/m3-t2/evidence/input-v2'
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
function FullPath([string]$Path) {
    return [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $root $Path }))
}
function Digest([string]$Path) { return (Get-FileHash -LiteralPath (FullPath $Path) -Algorithm SHA256).Hash.ToLowerInvariant() }
function Json([object]$Value, [string]$Name) { $Value | ConvertTo-Json -Depth 16 | Set-Content -LiteralPath (Join-Path $destination $Name) -Encoding utf8 }
$destination = FullPath $OutputDirectory
$allowed = FullPath 'docs/milestones/m3-t2/evidence'
if (-not $destination.StartsWith($allowed + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Use a fresh T2 evidence subdirectory.' }
if (Test-Path -LiteralPath $destination) { throw 'Do not overwrite earlier evidence.' }
$auditRoot = FullPath $AuditDirectory
$audit = Get-Content -LiteralPath (Join-Path $auditRoot 'summary.json') -Raw | ConvertFrom-Json
if (-not $audit.completed -or $audit.cases.Count -ne 4 -or @($audit.cases | Where-Object { -not $_.completed -or $_.changed_inputs.Count }).Count) { throw 'The declared four-case audit is not complete or has changed inputs.' }
$vendor = Get-Content -LiteralPath (FullPath 'out/m3-t2/sdl-official-vendor-input-v2-comparison.json') -Raw | ConvertFrom-Json
if ($vendor.different -or $vendor.missing -or $vendor.extra.Count -or $vendor.identical -ne 2183) { throw 'Official SDL source identity changed.' }
New-Item -ItemType Directory -Path $destination | Out-Null
$copies = @{
    (Join-Path $auditRoot 'summary.json') = 'cmake322-audit.json'
    (Join-Path $auditRoot 'import-audit.json') = 'cmake322-imports.json'
    (FullPath 'out/m3-t2/input-comparison/automated-002/result.json') = 'comparison-rendering.json'
    (FullPath 'out/m3-t2/probe/gl-release-cmake322-runtime-001/summary.json') = 'cmake322-gl-runtime.json'
    (FullPath 'out/m3-t2/runtime/sdl-input-002/result.json') = 'runtime-input.json'
    (FullPath 'out/m3-t2/runtime/sdl-input-002/stdout.log') = 'runtime-input.stdout.log'
    (FullPath 'out/m3-t2/runtime/sdl-input-002/stderr.log') = 'runtime-input.stderr.log'
    (FullPath 'out/m3-t2/probe/input-comparison-sdl-release-002/Testing/Temporary/LastTest.log') = 'candidate-185.log'
}
foreach ($entry in $copies.GetEnumerator()) { Copy-Item -LiteralPath $entry.Key -Destination (Join-Path $destination $entry.Value) }
$zip = 'out/m3-t2/tools/cmake-3.22.6/cmake-3.22.6-windows-x86_64.zip'
$zipHash = Digest $zip
$checksumPath = FullPath 'out/m3-t2/tools/cmake-3.22.6/cmake-3.22.6-SHA-256.txt'
$checksum = @(Get-Content -LiteralPath $checksumPath | Where-Object { $_ -match '^([0-9a-fA-F]{64})\s+cmake-3\.22\.6-windows-x86_64\.zip$' })
if ($checksum.Count -ne 1 -or $checksum[0].Split(' ',[StringSplitOptions]::RemoveEmptyEntries)[0].ToLowerInvariant() -ne $zipHash) { throw 'Portable CMake ZIP checksum differs from the official checksum file.' }
Json ([ordered]@{ version='3.22.6'; source_url='https://github.com/Kitware/CMake/releases/download/v3.22.6/cmake-3.22.6-windows-x86_64.zip'; archive_sha256=$zipHash; official_checksum_file_sha256=Digest $checksumPath; system_installation_modified=$false }) 'cmake322-provenance.json'
Json ([ordered]@{ archive_sha256=$vendor.archive_sha256; files=$vendor.archive_files; identical=$vendor.identical; different=$vendor.different; missing=$vendor.missing; extra=$vendor.extra; raw_comparison='out/m3-t2/sdl-official-vendor-input-v2-comparison.json'; raw_comparison_sha256=Digest 'out/m3-t2/sdl-official-vendor-input-v2-comparison.json' }) 'vendor-recheck.json'

$files = @(& rg --files (Join-Path $root 'test/experimental'))
if ($LASTEXITCODE -ne 0) { throw 'Experimental source inventory failed.' }
$inputs = @($files | Sort-Object | ForEach-Object { [ordered]@{ path=$_.Substring($root.Length+1).Replace('\','/'); sha256=Digest $_ } })
$executables = @('out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe',
    'out/m3-t2/probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe',
    'out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3RuntimeInputTests.exe',
    'out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3InputCandidateTests.exe') |
    ForEach-Object { [ordered]@{ path=$_; sha256=Digest $_ } }
$baseline = Get-Content -LiteralPath (FullPath 'out/m3-t2/baseline-input-001/build-inputs.json') -Raw | ConvertFrom-Json
$production = @($baseline.files | Where-Object { $_.path -eq 'CMakeLists.txt' -or $_.path -eq 'CMakePresets.json' -or $_.path -match '^(game/|cmake/|assets/|vendor/(glfw|glad|KHR|yaml-cpp)/)' })
$changed = @($production | Where-Object { -not (Test-Path -LiteralPath (FullPath $_.path)) -or (Digest $_.path) -ne $_.sha256 })
if ($changed.Count) { throw 'Production inputs differ from the saved GLFW baseline; inspect before claiming preparation-only work.' }
Json ([ordered]@{ created_utc=[DateTime]::UtcNow.ToString('o'); scope='Experimental source observations and current binaries; full SDL bound by official archive recheck. Frozen CPU/GLFW libraries are recorded in comparison-rendering.json.'; sources=$inputs; executables=@($executables); production_inputs_compared=$production.Count; production_changes=@(); production_platform='GLFW'; audit_root=$AuditDirectory; audit_input_manifest_sha256=Digest (Join-Path $auditRoot 'inputs-sha256.json'); continuous_release_build_log='out/m3-t2/input-comparison-sdl-release-002-build.log'; continuous_release_build_log_sha256=Digest 'out/m3-t2/input-comparison-sdl-release-002-build.log'; hardware_validation=$false }) 'input-identities.json'
Write-Host "Curated input preparation evidence: $destination"

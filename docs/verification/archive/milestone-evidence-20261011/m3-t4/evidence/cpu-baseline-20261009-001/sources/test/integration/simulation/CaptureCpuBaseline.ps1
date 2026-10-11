[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$CompareDirectory,
    [ValidateSet('production', 'dependencies', 'assets', 'build', 'fixtures', 'reference_only')]
    [string[]]$CompareScopes = @('production', 'dependencies', 'assets'),
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Refusing to overwrite baseline identity: $output" }

$groups = [ordered]@{
    production = @('game/modules/foundation', 'game/modules/scene', 'game/modules/ecs',
        'game/modules/world', 'game/modules/simulation')
    dependencies = @('vendor/glm', 'vendor/fast_noise_lite', 'vendor/yaml-cpp')
    assets = @('assets/configs/blockFormats.yaml')
    build = @('CMakeLists.txt', 'CMakePresets.json', 'cmake', 'game/CMakeLists.txt', 'test/CMakeLists.txt')
    fixtures = @('test/unit/simulation', 'test/integration/simulation')
    reference_only = @('game/app/src/application.cpp')
}
$entries = @(
    foreach ($group in $groups.Keys) {
        foreach ($relative in $groups[$group]) {
            $item = Get-Item -LiteralPath (Join-Path $root $relative)
            $files = if ($item.PSIsContainer) { Get-ChildItem -LiteralPath $item.FullName -File -Recurse } else { @($item) }
            foreach ($file in $files) {
                $path = [IO.Path]::GetRelativePath($root, $file.FullName).Replace('\', '/')
                [pscustomobject]@{ scope = $group; path = $path; sha256 = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
            }
        }
    }
) | Sort-Object path

if ($CompareDirectory) {
    $previous = Get-Content -LiteralPath (Join-Path $CompareDirectory 'identity.json') -Raw | ConvertFrom-Json
    $stable = $CompareScopes
    $before = @($previous.files | Where-Object scope -In $stable | ForEach-Object { "$($_.path) $($_.sha256)" })
    $after = @($entries | Where-Object scope -In $stable | ForEach-Object { "$($_.path) $($_.sha256)" })
    $differences = @(Compare-Object $before $after)
    if ($differences.Count) { throw "Baseline inputs changed in $($stable -join ', '): $($differences | Out-String)" }
}

New-Item -ItemType Directory -Path $output | Out-Null
foreach ($entry in $entries | Where-Object scope -NE 'dependencies') {
    $source = Join-Path $root $entry.path
    $destination = Join-Path $output "sources/$($entry.path)"
    New-Item -ItemType Directory -Path (Split-Path $destination) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $destination
    if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) {
        throw "Source changed while capturing: $($entry.path)"
    }
}
$head = & git -C $root rev-parse HEAD
if ($LASTEXITCODE -ne 0) { throw 'Cannot identify repository HEAD' }
$status = @(& git -C $root status --short)
if ($LASTEXITCODE -ne 0) { throw 'Cannot record working tree status' }
$identity = [ordered]@{
    schema = 1; captured_utc = [DateTime]::UtcNow.ToString('o'); repository = $root; head = $head
    authority = 'Actual file SHA-256, including dirty working tree; HEAD is context only.'
    dependency_policy = 'Vendor dependencies hashed in place; CPU/build/fixture sources copied byte-for-byte.'
    reference_policy = 'Application is scheduling context only, not linked or frozen by these CPU tests.'
    compared_directory = $CompareDirectory; compared_scopes = @($CompareScopes)
    working_tree = $status; files = @($entries)
}
$identity | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'identity.json') -Encoding utf8
Write-Host "Captured $($entries.Count) files in $output"

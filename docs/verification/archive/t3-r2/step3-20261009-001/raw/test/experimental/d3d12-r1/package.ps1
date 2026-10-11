[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$CMakePath
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$buildRoot = [IO.Path]::GetFullPath((Join-Path $root 'out/m3-t3'))
$packageRoot = [IO.Path]::GetFullPath((Join-Path $buildRoot 'package'))
function Workspace-Path {
    param([string]$Path)
    return [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $root $Path }))
}
function Assert-Identity {
    param([string]$Path, [string]$ExpectedHash, [string]$Description)
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ExpectedHash) {
        throw "$Description changed after the successful build; rebuild before packaging."
    }
}
$build = Workspace-Path $BuildDirectory
$output = Workspace-Path $OutputDirectory
if (-not $build.StartsWith($buildRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package only an isolated out/m3-t3 build.'
}
if (-not $output.StartsWith($packageRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package outputs must be fresh children of out/m3-t3/package.'
}
if ($build.Equals($output, [StringComparison]::OrdinalIgnoreCase) -or
    $build.StartsWith($output + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The package must not contain or overwrite its originating build.'
}
if (Test-Path -LiteralPath $output) { throw 'Use a new package directory; prior packages are never overwritten.' }
$cachePath = Join-Path $build 'CMakeCache.txt'
$cache = Get-Content -LiteralPath $cachePath -Raw
$sourceHome = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)')
$configuration = [regex]::Match($cache, '(?m)^CMAKE_BUILD_TYPE:STRING=([^\r\n]+)')
if (-not $sourceHome.Success -or -not $configuration.Success -or $configuration.Groups[1].Value -ne 'Release' -or
    -not [IO.Path]::GetFullPath($sourceHome.Groups[1].Value).Equals([IO.Path]::GetFullPath($PSScriptRoot), [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Package only the matching isolated Release fixture; Debug CRT is not redistributable.'
}
$identitiesPath = Join-Path $build 'build-identities.json'
$importsPath = Join-Path $build 'production-imports.json'
$identities = Get-Content -LiteralPath $identitiesPath -Raw | ConvertFrom-Json
$imports = Get-Content -LiteralPath $importsPath -Raw | ConvertFrom-Json
if ($imports.configuration -ne 'Release') { throw 'Origin production imports are not Release.' }
$executable = Join-Path $build 'SymoCraftD3D12R1Probe.exe'
Assert-Identity $executable $identities.executable_sha256 'Fixture executable'
Assert-Identity $importsPath $identities.production_imports_sha256 'Production import manifest'
foreach ($entry in $identities.sources) { Assert-Identity $entry.path $entry.sha256 'Fixture source' }
foreach ($entry in $identities.shaders) { Assert-Identity $entry.path $entry.sha256 'Compiled shader' }
Assert-Identity (Join-Path $imports.production_build 'CMakeCache.txt') $imports.production_cache_sha256 'Production configuration'
foreach ($entry in $imports.imports) { Assert-Identity $entry.archive $entry.sha256 'Production archive' }
foreach ($entry in $imports.platform_sources) { Assert-Identity $entry.path $entry.sha256 'Production platform source' }
if ($imports.PSObject.Properties.Name -notcontains 'foundation_sources' -or @($imports.foundation_sources).Count -eq 0) {
    throw 'Production foundation source identities are missing; rebuild before packaging.'
}
foreach ($entry in $imports.foundation_sources) { Assert-Identity $entry.path $entry.sha256 'Production foundation source' }

if (-not $CMakePath) {
    $cmakeEntry = [regex]::Match($cache, '(?m)^CMAKE_COMMAND:INTERNAL=([^\r\n]+)')
    if (-not $cmakeEntry.Success) { throw 'No recorded CMake executable; pass -CMakePath.' }
    $CMakePath = $cmakeEntry.Groups[1].Value
}
$cmake = (Get-Item -LiteralPath $CMakePath -ErrorAction Stop).FullName
New-Item -ItemType Directory -Path $output | Out-Null
& $cmake --install $build --config Release --component R1Fixture --prefix $output
if ($LASTEXITCODE -ne 0) { throw "Fixture installation failed with exit code $LASTEXITCODE; preserve the partial package for diagnosis." }

foreach ($name in @('SymoCraftD3D12R1Probe.exe', 'shaders/probe-vs.cso', 'shaders/probe-ps.cso',
    'build-identities.json', 'production-imports.json', 'verify-probes.ps1', 'licenses/SDL3-LICENSE.txt',
    'msvcp140.dll', 'vcruntime140.dll', 'vcruntime140_1.dll')) {
    if (-not (Test-Path -LiteralPath (Join-Path $output $name) -PathType Leaf)) {
        throw "Required fixture payload is missing: $name. Reconfigure the fixture with its Release install rules."
    }
}
foreach ($file in @(Get-ChildItem -LiteralPath $output -Filter '*.dll' -File)) {
    if ($file.BaseName -match '(?i)(?:140d|ucrtbased|msvcrtd)$') { throw 'Debug runtime libraries must never enter this package.' }
}
Assert-Identity (Join-Path $output 'SymoCraftD3D12R1Probe.exe') $identities.executable_sha256 'Installed fixture executable'
Assert-Identity (Join-Path $output 'build-identities.json') (Get-FileHash -LiteralPath $identitiesPath -Algorithm SHA256).Hash.ToLowerInvariant() 'Installed origin build manifest'
Assert-Identity (Join-Path $output 'production-imports.json') $identities.production_imports_sha256 'Installed origin import manifest'
Assert-Identity (Join-Path $output 'verify-probes.ps1') (Get-FileHash -LiteralPath (Join-Path $PSScriptRoot 'verify-probes.ps1') -Algorithm SHA256).Hash.ToLowerInvariant() 'Installed verification runner'
foreach ($name in @('probe-vs.cso', 'probe-ps.cso')) {
    Assert-Identity (Join-Path $output "shaders/$name") (Get-FileHash -LiteralPath (Join-Path $build "shaders/$name") -Algorithm SHA256).Hash.ToLowerInvariant() 'Installed compiled shader'
}

$readme = @'
SymoCraft T3 R1 discrete-adapter D3D12 portable experiment

Windows x64 with a real D3D12-capable GPU is required. This package contains the
Release fixture, shader binaries, app-local MSVC runtime, and immutable origin
records. No developer SDK, DXC compiler, SDL runtime DLL, or game assets are needed.

The Windows Graphics Tools optional feature must supply the D3D12 Debug Layer.
Missing Debug Layer support is blocked evidence, never an implicit pass. The
fixture does not use WARP or silently fall back to another graphics backend.

Extract the entire package to a writable directory. From that directory:

powershell -NoProfile -ExecutionPolicy Bypass -File .\verify-probes.ps1 -Executable .\SymoCraftD3D12R1Probe.exe -OutputDirectory .\evidence\discrete-release-001 -PackageManifest .\package-manifest.json -RunTimeoutProbe

The fixture opens and resizes a window, minimizes it for at least 10 seconds,
restores it, and then exits. The runner records machine/device identity, result
JSON, lifecycle, screenshots, stdout/stderr, exit codes, and bounded timeouts.
Use a new evidence directory for every run. Do not modify the manifest, executable,
shaders, verification runner, DLLs, licenses, or origin records before verification.

Review all five PNGs visually and return the complete evidence directory. The
timeout case is explicit injection, not an actual driver/device-loss test. Zero
extent is a labelled contract input; native minimization/restoration are real.

Origin manifests retain the builder's absolute paths only as provenance. Portable
verification validates packaged bytes via package-manifest.json and does not need
the original source tree or production archives on this machine.

This is a candidate test-only discrete-adapter R1 package, not Renderer v1 freeze,
gameplay, or T3 acceptance. Wait for T2 acceptance and the current spec R1 handoff
and evidence review before R1 freeze or R2. This package does not waive unexecuted
matrix items or establish any deferred whole-system performance gate as passed.
'@
$readme | Set-Content -LiteralPath (Join-Path $output 'README.txt') -Encoding UTF8
$payload = @()
foreach ($file in @(Get-ChildItem -LiteralPath $output -Recurse -File | Sort-Object FullName)) {
    $relative = $file.FullName.Substring($output.Length + 1).Replace('\', '/')
    $payload += [pscustomobject]@{path=$relative; bytes=$file.Length; sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$manifest = [ordered]@{
    schema_version=1
    configuration='Release'
    created_utc=[DateTimeOffset]::UtcNow.ToString('o')
    origin_build=[ordered]@{
        directory=$build
        build_identities_sha256=(Get-FileHash -LiteralPath $identitiesPath -Algorithm SHA256).Hash.ToLowerInvariant()
        production_imports_sha256=$identities.production_imports_sha256
        executable_sha256=$identities.executable_sha256
    }
    payload=$payload
    required_debug_layer=$true
    software_adapter_allowed=$false
    scope='Candidate isolated discrete-adapter D3D12 R1 fixture; no production game or validation-layer redistribution and no Renderer v1 freeze, gameplay, whole-system performance, or T3 acceptance.'
}
$manifest | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'package-manifest.json') -Encoding UTF8
[pscustomobject]@{
    package_directory=$output
    manifest=(Join-Path $output 'package-manifest.json')
    payload_files=$payload.Count
    manifest_sha256=(Get-FileHash -LiteralPath (Join-Path $output 'package-manifest.json') -Algorithm SHA256).Hash.ToLowerInvariant()
} | ConvertTo-Json

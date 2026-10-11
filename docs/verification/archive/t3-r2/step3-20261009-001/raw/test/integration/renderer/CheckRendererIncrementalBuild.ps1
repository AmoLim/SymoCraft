[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [Parameter(Mandatory)][string]$CMakePath
)

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
$output = [IO.Path]::GetFullPath($OutputDirectory)
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out')) + [IO.Path]::DirectorySeparatorChar
foreach ($path in @($build, $output)) {
    if (!$path.StartsWith($outRoot, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Incremental verification requires build/output paths inside this repository out directory.'
    }
}
if (!(Test-Path -LiteralPath (Join-Path $build 'CMakeCache.txt') -PathType Leaf)) {
    throw 'BuildDirectory must contain an already configured and settled CMake build.'
}
if (Test-Path -LiteralPath $output) {
    throw 'OutputDirectory must be new; existing incremental evidence will not be overwritten.'
}
if (!(Test-Path -LiteralPath $CMakePath -PathType Leaf)) {
    throw 'CMakePath must name the configured CMake executable.'
}
New-Item -ItemType Directory -Path $output | Out-Null

function Build-Probe([string]$Name) {
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $lines = & $CMakePath --build $build --parallel 8 2>&1
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    $lines | Set-Content -LiteralPath (Join-Path $output ($Name + '.log')) -Encoding utf8
    if ($code -ne 0) { throw "Incremental build failed: $Name ($code)" }
    return ($lines -join "`n")
}

function Get-CompiledOwners([string]$Log) {
    $counts = [ordered]@{}
    $patterns = [ordered]@{
        coreRenderer = 'game[/\\]modules[/\\]renderer[/\\].*CMakeFiles[/\\]symocraft_renderer\.dir[/\\]'
        legacyRenderer = 'game[/\\]modules[/\\]renderer[/\\].*CMakeFiles[/\\]symocraft_renderer_legacy_opengl\.dir[/\\]'
        nativeD3D12 = 'game[/\\]modules[/\\]renderer[/\\].*CMakeFiles[/\\]symocraft_renderer_d3d12\.dir[/\\]'
        world = 'game[/\\]modules[/\\]world[/\\]'
        simulation = 'game[/\\]modules[/\\]simulation[/\\]'
        app = 'game[/\\]app[/\\]'
        appRenderData = 'game[/\\]app[/\\].*CMakeFiles[/\\]symocraft_app_render_data\.dir[/\\]'
        appRuntime = 'game[/\\]app[/\\].*CMakeFiles[/\\]SymoCraft\.dir[/\\]'
        appCacheTests = 'CMakeFiles[/\\]app_render_cache_tests\.dir[/\\]'
        appConversionTests = 'CMakeFiles[/\\]app_render_conversion_tests\.dir[/\\]'
        benchmark = 'tools[/\\]benchmark[/\\]'
        publicConsumer = 'CMakeFiles[/\\]renderer_public_api_consumer\.dir[/\\]'
        facadeTests = 'CMakeFiles[/\\]renderer_facade_tests\.dir[/\\]'
    }
    $compilations = [regex]::Matches($Log, 'Building CXX object ([^\r\n]+)', [Text.RegularExpressions.RegexOptions]::IgnoreCase)
    foreach ($owner in $patterns.Keys) {
        $counts[$owner] = 0
        foreach ($compilation in $compilations) {
            if ($compilation.Groups[1].Value -match $patterns[$owner]) {
                $counts[$owner]++
            }
        }
    }
    return $counts
}

function Test-ExactSources([string[]]$Actual, [string[]]$Expected) {
    return ($Actual.Count -eq $Expected.Count -and
        @($Actual | Where-Object { $_ -cnotin $Expected }).Count -eq 0 -and
        @($Expected | Where-Object { $_ -cnotin $Actual }).Count -eq 0)
}

function Test-ProductionTarget([string]$Name, [string]$Type, [string[]]$Sources) {
    $types = @($targetRows | Where-Object { $_.target -ceq $Name -and $_.property -eq 'TYPE' })
    $production = @($sourceRows | Where-Object { $_.target -ceq $Name })
    if ($types.Count -eq 0 -and $production.Count -eq 0) { return $false }
    if ($types.Count -ne 1 -or $types[0].type -cne $Type -or $types[0].raw_value -cne $Type) {
        throw "The production target declaration is missing or inconsistent: $Name"
    }
    $actual = @($production | Where-Object { $_.file -match '\.cpp$' } |
        ForEach-Object { $_.file -replace '\\', '/' })
    if (!(Test-ExactSources $actual $Sources) -or
        @($production | Where-Object { $_.type -cne $Type }).Count -ne 0) {
        throw "The production source ownership changed; review probe expectations for $Name"
    }
    $declared = @($targetRows | Where-Object { $_.target -ceq $Name -and $_.property -eq 'SOURCES' })
    if ($declared.Count -ne 1) { throw "Missing target sources: $Name" }
    $declaredCpp = @($declared[0].raw_value.Split(';') | Where-Object { $_ -match '\.cpp$' } |
        ForEach-Object { [IO.Path]::GetFileName($_) })
    $expectedCpp = @($Sources | ForEach-Object { [IO.Path]::GetFileName($_) })
    if (!(Test-ExactSources $declaredCpp $expectedCpp)) {
        throw "Target sources disagree with production ownership: $Name"
    }
    return $true
}

function Get-CompiledAppSources([string]$Log) {
    foreach ($match in [regex]::Matches($Log, 'Building CXX object ([^\r\n]+)')) {
        $object = $match.Groups[1].Value -replace '\\', '/'
        if ($object -match 'game/app/CMakeFiles/(symocraft_app_render_data|SymoCraft)\.dir/src/([^/]+)\.obj$') {
            $Matches[1] + ':' + $Matches[2]
        }
    }
}

function Assert-AppSources($Counts, [string[]]$Actual, [string[]]$Expected) {
    $dataCount = @($Expected | Where-Object { $_.StartsWith('symocraft_app_render_data:') }).Count
    $runtimeCount = @($Expected | Where-Object { $_.StartsWith('SymoCraft:') }).Count
    if ($Counts.app -ne $Expected.Count -or $Counts.appRenderData -ne $dataCount -or
        $Counts.appRuntime -ne $runtimeCount -or !(Test-ExactSources $Actual $Expected)) {
        throw "Header rebuilt the wrong app sources: $($Actual -join ', '); owners=$($Counts | ConvertTo-Json -Compress)"
    }
}

$probes = [Collections.Generic.List[object]]::new()
$verification = [ordered]@{
    schema = 'symocraft.renderer-incremental-build'
    version = 3
    started_utc = [DateTime]::UtcNow.ToString('o')
    finished_utc = $null
    build_directory = $build
    output_directory = $output
    status = 'running'
    baseline_compilations = $null
    production_callers = $null
    probes = $probes
    error = $null
}
$failure = $null
try {
    $baseline = Build-Probe 'baseline'
    $verification.baseline_compilations = [regex]::Matches($baseline, 'Building (?:C|CXX) object', [Text.RegularExpressions.RegexOptions]::IgnoreCase).Count
    if ($verification.baseline_compilations -ne 0) {
        throw 'The initial build was not settled. Rerun after production changes finish.'
    }
    $targetRows = Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $build 'renderer-targets.tsv')
    $sourceRows = Import-Csv -Delimiter "`t" -LiteralPath (Join-Path $build 'production-sources.tsv')
    $hasAppData = Test-ProductionTarget 'symocraft_app_render_data' 'STATIC_LIBRARY' @(
        'game/app/src/world_render_cache.cpp', 'game/app/src/render_conversion.cpp')
    $hasAppRuntime = Test-ProductionTarget 'SymoCraft' 'EXECUTABLE' @(
        'game/app/src/main.cpp', 'game/app/src/application.cpp', 'game/app/src/game_renderer.cpp')
    if ($hasAppRuntime -and !$hasAppData) { throw 'The game runtime requires its real app render data target.' }
    $hasCacheTests = @($targetRows | Where-Object {
        $_.target -ceq 'app_render_cache_tests' -and $_.property -eq 'TYPE' -and
        $_.type -eq 'EXECUTABLE' -and $_.raw_value -eq 'EXECUTABLE' }).Count -eq 1
    $verification.production_callers = [ordered]@{
        app_render_data = $hasAppData
        game_runtime = $hasAppRuntime
        app_cache_tests = $hasCacheTests
        targets_sha256 = (Get-FileHash -LiteralPath (Join-Path $build 'renderer-targets.tsv') -Algorithm SHA256).Hash
        sources_sha256 = (Get-FileHash -LiteralPath (Join-Path $build 'production-sources.tsv') -Algorithm SHA256).Hash
    }
    $publicAppSources = @()
    $cacheAppSources = @()
    if ($hasAppData) {
        $publicAppSources += 'symocraft_app_render_data:world_render_cache.cpp', 'symocraft_app_render_data:render_conversion.cpp'
        $cacheAppSources += 'symocraft_app_render_data:world_render_cache.cpp'
    }
    if ($hasAppRuntime) {
        $publicAppSources += 'SymoCraft:application.cpp', 'SymoCraft:game_renderer.cpp'
        $cacheAppSources += 'SymoCraft:game_renderer.cpp'
    }
    $probeInputs = @(
        @{ Name = 'private-backend-contract'; File = 'game/modules/renderer/src/backend/render_backend.h' },
        @{ Name = 'public-renderer-types'; File = 'game/modules/renderer/include/symocraft/renderer/types.h' }
    )
    if ($hasAppData) {
        $probeInputs += @{ Name = 'private-app-world-cache'; File = 'game/app/src/world_render_cache.h' }
    }
    $graph = Get-Content -Raw -LiteralPath (Join-Path $build 'build.ninja')
    if ($graph -match 'symocraft_renderer_d3d12\.dir') {
        $probeInputs += @{ Name = 'private-d3d12-pipeline'; File = 'game/modules/renderer/src/backend/d3d12/pipeline.h' }
        $probeInputs += @{ Name = 'private-d3d12-shader'; File = 'assets/shaders/hlsl/draw_constants.hlsli' }
    }
    foreach ($probe in $probeInputs) {
        $file = Get-Item -LiteralPath (Join-Path $root $probe.File)
        $timestamp = $file.LastWriteTimeUtc
        $hash = (Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash
        $result = [ordered]@{
            probe = $probe.Name
            header = $probe.File
            sha256 = $hash
            original_mtime_utc = $timestamp.ToString('o')
            probe_mtime_utc = $null
            restored_mtime = $false
            compiled_owners = $null
            compiled_app_sources = @()
            compiled_shaders = 0
            relinked_public_consumer = $false
            status = 'running'
        }
        $probes.Add($result)
        try {
            if ($timestamp -gt [DateTime]::UtcNow.AddSeconds(1)) {
                throw 'A probe header has a future timestamp; settle the checkout before incremental verification.'
            }
            # Avoid filesystem timestamp granularity and never leave a future mtime.
            while ([DateTime]::UtcNow -lt $timestamp.AddSeconds(2)) {
                Start-Sleep -Milliseconds 250
            }
            $file.LastWriteTimeUtc = [DateTime]::UtcNow
            $file.Refresh()
            if ([Math]::Abs(($file.LastWriteTimeUtc - $timestamp).TotalSeconds) -lt 1) {
                throw 'The probe timestamp did not change enough to invalidate the header.'
            }
            $result.probe_mtime_utc = $file.LastWriteTimeUtc.ToString('o')
            $log = Build-Probe $probe.Name
            $counts = Get-CompiledOwners $log
            $result.compiled_owners = $counts
            $result.compiled_app_sources = @(Get-CompiledAppSources $log)
            $result.compiled_shaders = [regex]::Matches($log, 'Compiling D3D12 (?:blocks|selection)\.(?:vs|ps) Shader Model 6\.0').Count
            if ($probe.Name -eq 'private-d3d12-shader') {
                if ($result.compiled_shaders -ne 4 -or
                    [regex]::Matches($log, 'Building (?:C|CXX) object').Count -ne 0) {
                    throw 'Shared HLSL include must regenerate all four DXIL assets without recompiling C++.'
                }
            } elseif ($probe.Name -eq 'private-app-world-cache') {
                Assert-AppSources $counts $result.compiled_app_sources $cacheAppSources
                $expectedTests = if ($hasCacheTests) { 1 } else { 0 }
                if ($counts.appCacheTests -ne $expectedTests -or $counts.appConversionTests -ne 0 -or
                    $counts.coreRenderer -ne 0 -or $counts.legacyRenderer -ne 0 -or $counts.nativeD3D12 -ne 0 -or
                    $counts.publicConsumer -ne 0 -or $counts.facadeTests -ne 0 -or $counts.world -ne 0 -or
                    $counts.simulation -ne 0 -or $counts.benchmark -ne 0 -or $result.compiled_shaders -ne 0 -or
                    [regex]::Matches($log, 'Building (?:C|CXX) object').Count -ne $cacheAppSources.Count + $expectedTests) {
                    throw "Private app cache header rebuilt the wrong owners: $($counts | ConvertTo-Json -Compress)"
                }
            } elseif ($probe.Name -eq 'private-d3d12-pipeline') {
                if ($counts.nativeD3D12 -eq 0 -or $counts.coreRenderer -ne 0 -or $counts.legacyRenderer -ne 0 -or
                    $counts.publicConsumer -ne 0 -or $counts.facadeTests -ne 0 -or $counts.world -ne 0 -or
                    $counts.simulation -ne 0 -or $counts.app -ne 0 -or $counts.benchmark -ne 0 -or
                    $counts.appCacheTests -ne 0 -or $counts.appConversionTests -ne 0) {
                    throw "Private D3D12 header rebuilt the wrong owners: $($counts | ConvertTo-Json -Compress)"
                }
            } elseif ($probe.Name -eq 'private-backend-contract') {
                if ($counts.coreRenderer -eq 0 -or $counts.legacyRenderer -ne 0 -or $counts.publicConsumer -ne 0 -or
                    $counts.world -ne 0 -or $counts.simulation -ne 0 -or $counts.app -ne 0 -or $counts.benchmark -ne 0 -or
                    $counts.appCacheTests -ne 0 -or $counts.appConversionTests -ne 0) {
                    throw "Private backend header rebuilt the wrong owners: $($counts | ConvertTo-Json -Compress)"
                }
            } else {
                Assert-AppSources $counts $result.compiled_app_sources $publicAppSources
                if ($counts.coreRenderer -eq 0 -or $counts.publicConsumer -eq 0 -or $counts.legacyRenderer -ne 0 -or
                    $counts.world -ne 0 -or $counts.simulation -ne 0 -or $counts.benchmark -ne 0) {
                    throw "Public renderer types rebuilt the wrong owners: $($counts | ConvertTo-Json -Compress)"
                }
            }
            $result.relinked_public_consumer = $log -match 'Linking CXX executable [^\r\n]*renderer_public_api_consumer(?:\.exe)?(?:\s|$)'
            if ($probe.Name -notin 'private-d3d12-shader', 'private-app-world-cache' -and !$result.relinked_public_consumer) {
                throw 'The changed renderer implementation did not relink its real public API consumer.'
            }
            $result.status = 'passed'
        } catch {
            $result.status = 'failed'
            throw
        } finally {
            if ((Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash -eq $hash) {
                $file.LastWriteTimeUtc = $timestamp
                $result.restored_mtime = $true
            } else {
                $result.status = 'failed'
                throw 'A probe header changed concurrently; its timestamp was not restored. Rebuild and rerun the verification.'
            }
        }
    }
    $verification.status = 'passed'
} catch {
    $failure = $_
    $verification.status = 'failed'
    $verification.error = $_.Exception.Message
} finally {
    $verification.finished_utc = [DateTime]::UtcNow.ToString('o')
    $verification | ConvertTo-Json -Depth 7 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding utf8
}
$verification | ConvertTo-Json -Depth 7
if ($null -ne $failure) { throw $failure }

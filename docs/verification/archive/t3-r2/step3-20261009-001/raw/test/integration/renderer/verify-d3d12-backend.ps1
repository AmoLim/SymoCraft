param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [Parameter(Mandatory = $true)][string]$ShaderRoot,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$Atlas = '',
    [string]$ExpectedDeviceName = 'NVIDIA GeForce RTX 5070 Ti',
    [ValidateRange(15, 300)][int]$ProcessTimeoutSeconds = 90
)

$ErrorActionPreference = 'Stop'
$Repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\..'))
$AllowedOutput = [IO.Path]::GetFullPath((Join-Path $Repository 'out\m3-t3'))
$Output = [IO.Path]::GetFullPath($OutputDirectory)
$Build = [IO.Path]::GetFullPath($BuildDirectory)
$ExecutablePath = [IO.Path]::GetFullPath($Executable)
$Shaders = [IO.Path]::GetFullPath($ShaderRoot)
if ([string]::IsNullOrWhiteSpace($Atlas)) {
    $Atlas = Join-Path $Repository 'assets\textures\texture_atlas.png'
}
$AtlasPath = [IO.Path]::GetFullPath($Atlas)

function Assert-Within([string]$Path, [string]$Root) {
    $Prefix = $Root.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $Path.StartsWith($Prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Resolved path is outside the required workspace directory: $Path"
    }
}

Assert-Within $Output $AllowedOutput
Assert-Within $Build (Join-Path $Repository 'out')
Assert-Within $ExecutablePath (Join-Path $Repository 'out')
Assert-Within $Shaders (Join-Path $Repository 'out')
Assert-Within $AtlasPath $Repository
if (Test-Path -LiteralPath $Output) { throw "Output directory must be new: $Output" }
foreach ($RequiredFile in @($ExecutablePath, $AtlasPath, (Join-Path $Build 'CMakeCache.txt'))) {
    if (-not (Test-Path -LiteralPath $RequiredFile -PathType Leaf)) { throw "Required input does not exist: $RequiredFile" }
}
$ShaderNames = @('blocks.vs.dxil', 'blocks.ps.dxil', 'selection.vs.dxil', 'selection.ps.dxil')
foreach ($Name in $ShaderNames) {
    if (-not (Test-Path -LiteralPath (Join-Path $Shaders $Name) -PathType Leaf)) { throw "Compiled shader does not exist: $Name" }
}
[IO.Directory]::CreateDirectory($Output) | Out-Null

function File-Identity([string]$Path) {
    $File = Get-Item -LiteralPath $Path
    return [ordered]@{
        path = $File.FullName
        bytes = $File.Length
        sha256 = (Get-FileHash -LiteralPath $File.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    }
}

function Quote-Argument([string]$Value) {
    if ($Value.Contains('"')) { throw 'A process argument contains a double quote.' }
    if ($Value.EndsWith('\')) { $Value = $Value + '\' }
    return '"' + $Value + '"'
}

$RunId = [Guid]::NewGuid().ToString()
$Summary = [ordered]@{
    schema = 'symocraft.d3d12-backend-verification.v1'
    run_id = $RunId
    started_utc = [DateTime]::UtcNow.ToString('o')
    finished_utc = $null
    repository = $Repository
    build_directory = $Build
    output_directory = $Output
    process_timeout_seconds = $ProcessTimeoutSeconds
    gpu_verification = $true
    world_or_gameplay_verification = $false
    production_package_acceptance = $false
    d3d12_default_approved = $false
    device_lost_exercised = $false
    actual_in_flight_timeout_exercised = $false
    expected_device_name = $ExpectedDeviceName
    source_identity = @()
    input_identity = @()
    cases = @()
    artifacts = @()
    success = $false
}

function Save-Summary {
    $Summary | ConvertTo-Json -Depth 30 | Set-Content -LiteralPath (Join-Path $Output 'verification.json') -Encoding UTF8
}

try {
    $SourceRoots = @(
        (Join-Path $Repository 'game\modules\renderer'),
        (Join-Path $Repository 'game\modules\platform'),
        (Join-Path $Repository 'game\modules\assets'),
        (Join-Path $Repository 'test\integration\renderer')
    )
    $Sources = @()
    foreach ($Root in $SourceRoots) {
        $Sources += Get-ChildItem -LiteralPath $Root -Recurse -File |
            Where-Object { $_.Extension -in @('.h', '.cpp', '.hlsl', '.ps1') -or $_.Name -eq 'CMakeLists.txt' }
    }
    $Sources += Get-Item -LiteralPath (Join-Path $Repository 'CMakeLists.txt')
    $Sources += Get-Item -LiteralPath (Join-Path $Repository 'cmake\ModuleBoundaries.cmake')
    $Sources += Get-Item -LiteralPath (Join-Path $Repository 'cmake\RendererD3D12.cmake')
    $Sources += Get-Item -LiteralPath (Join-Path $Repository 'cmake\PublicHeaderTests.cmake')
    $Sources += Get-Item -LiteralPath (Join-Path $Repository 'test\integration\CheckRendererBoundary.cmake')
    $Sources += Get-Item -LiteralPath (Join-Path $Repository 'test\integration\CMakeLists.txt')
    foreach ($RelativePath in @(
        'game\modules\foundation\include\symocraft\foundation\document.h',
        'game\modules\foundation\src\document.cpp',
        'game\modules\foundation\src\files.cpp',
        'game\modules\scene\include\symocraft\scene\mesh.h'
    )) {
        $Sources += Get-Item -LiteralPath (Join-Path $Repository $RelativePath)
    }
    $ShaderSourceRoot = Join-Path $Repository 'assets\shaders'
    $Sources += Get-ChildItem -LiteralPath $ShaderSourceRoot -Recurse -File | Where-Object { $_.Extension -in @('.hlsl', '.hlsli') }
    foreach ($Source in ($Sources | Sort-Object -Property FullName -Unique)) {
        $Summary.source_identity += File-Identity $Source.FullName
    }
    foreach ($InputFile in @($ExecutablePath, $AtlasPath, (Join-Path $Build 'CMakeCache.txt'))) {
        $Summary.input_identity += File-Identity $InputFile
    }
    foreach ($Name in $ShaderNames) { $Summary.input_identity += File-Identity (Join-Path $Shaders $Name) }
    $BuildProof = Join-Path $Output 'build-identity'
    [IO.Directory]::CreateDirectory($BuildProof) | Out-Null
    foreach ($Name in @('CMakeCache.txt', 'compile_commands.json', 'renderer-targets.tsv', 'production-sources.tsv', 'production-dependencies.tsv')) {
        $SourcePath = Join-Path $Build $Name
        if (Test-Path -LiteralPath $SourcePath -PathType Leaf) {
            Copy-Item -LiteralPath $SourcePath -Destination (Join-Path $BuildProof $Name)
        }
    }
    $Summary.host = [ordered]@{
        machine_name = [Environment]::MachineName
        os_version = [Environment]::OSVersion.VersionString
        process_architecture = if ([Environment]::Is64BitProcess) { 'x64' } else { 'x86' }
        powershell_version = $PSVersionTable.PSVersion.ToString()
    }

    $MissingShaderRoot = Join-Path $Output 'fixtures\missing-shaders'
    $BadShaderRoot = Join-Path $Output 'fixtures\bad-shaders'
    [IO.Directory]::CreateDirectory($MissingShaderRoot) | Out-Null
    [IO.Directory]::CreateDirectory($BadShaderRoot) | Out-Null
    foreach ($Name in $ShaderNames) {
        [IO.File]::WriteAllBytes((Join-Path $BadShaderRoot $Name), [byte[]]@(0x4e, 0x4f, 0x54, 0x5f, 0x44, 0x58, 0x49, 0x4c))
    }
    $Cases = @(
        [ordered]@{ name = 'normal-samples-1'; mode = 'normal'; samples = 1; shaders = $Shaders },
        [ordered]@{ name = 'normal-samples-4'; mode = 'normal'; samples = 4; shaders = $Shaders },
        [ordered]@{ name = 'missing-shader'; mode = 'missing-shader'; samples = 1; shaders = $MissingShaderRoot },
        [ordered]@{ name = 'bad-shader'; mode = 'bad-shader'; samples = 1; shaders = $BadShaderRoot }
    )
    Save-Summary
    foreach ($Case in $Cases) {
        $CaseOutput = Join-Path $Output $Case.name
        $Row = [ordered]@{
            name = $Case.name
            mode = $Case.mode
            samples = $Case.samples
            requested_vsync = $false
            started_utc = [DateTime]::UtcNow.ToString('o')
            finished_utc = $null
            timed_out = $false
            exit_code = $null
            success = $false
            report = Join-Path $CaseOutput 'result.json'
            stdout = Join-Path $Output ($Case.name + '.stdout.log')
            stderr = Join-Path $Output ($Case.name + '.stderr.log')
            failure = $null
        }
        $Process = $null
        try {
            $Arguments = @(
                '--output-dir', (Quote-Argument $CaseOutput),
                '--shader-root', (Quote-Argument $Case.shaders),
                '--recovery-shader-root', (Quote-Argument $Shaders),
                '--atlas', (Quote-Argument $AtlasPath),
                '--run-id', (Quote-Argument $RunId),
                '--expected-device-name', (Quote-Argument $ExpectedDeviceName),
                '--mode', $Case.mode,
                '--samples', [string]$Case.samples,
                '--vsync', 'false'
            )
            $Row.arguments = $Arguments
            $Process = Start-Process -FilePath $ExecutablePath -ArgumentList $Arguments -WorkingDirectory (Split-Path $ExecutablePath) `
                -WindowStyle Hidden -RedirectStandardOutput $Row.stdout -RedirectStandardError $Row.stderr -PassThru
            if (-not $Process.WaitForExit($ProcessTimeoutSeconds * 1000)) {
                $Row.timed_out = $true
                $Process.Kill()
                if (-not $Process.WaitForExit(10000)) { throw 'Timed-out verifier process could not be stopped within 10 seconds.' }
            }
            $Process.Refresh()
            $Row.exit_code = $Process.ExitCode
            if (Test-Path -LiteralPath $Row.report -PathType Leaf) {
                $Result = Get-Content -LiteralPath $Row.report -Raw -Encoding UTF8 | ConvertFrom-Json
                if ($Result.schema -ne 'symocraft.d3d12-backend-smoke.v1' -or $Result.run_id -ne $RunId -or $Result.mode -ne $Case.mode -or
                    $Result.expected_device_name -ne $ExpectedDeviceName) {
                    throw 'Verifier report identity does not match the current process request.'
                }
                if ($Result.json_emitter_probe.all_control_characters.Length -ne 32 -or
                    $Result.json_emitter_probe.quote_backslash -ne '"\' -or
                    $null -ne $Result.json_emitter_probe.null -or
                    $Result.json_emitter_probe.unicode_utf8.Length -ne 2 -or
                    [int]$Result.json_emitter_probe.unicode_utf8[0] -ne 0xe9 -or
                    [int]$Result.json_emitter_probe.unicode_utf8[1] -ne 0x4e2d -or
                    -not $Result.json_emitter_probe.nonfinite_rejected) {
                    throw 'Structured JSON emitter probe did not round-trip through ConvertFrom-Json.'
                }
                $Row.check_count = @($Result.checks).Count
                $Row.failed_checks = @($Result.checks | Where-Object { -not $_.passed })
                $Row.success = -not $Row.timed_out -and $Row.exit_code -eq 0 -and $Result.success -and $Row.failed_checks.Count -eq 0
                if ($Result.failure) { $Row.failure = $Result.failure }
            } else { $Row.failure = 'Verifier process did not publish result.json.' }
        } catch {
            $Row.failure = $_.Exception.Message
        } finally {
            if ($null -ne $Process) {
                try {
                    if (-not $Process.HasExited) {
                        $Process.Kill()
                        if (-not $Process.WaitForExit(10000)) { $Row.failure = 'Process remained running after bounded cleanup.' }
                    }
                } finally { $Process.Dispose() }
            }
            $Row.finished_utc = [DateTime]::UtcNow.ToString('o')
            $Summary.cases += $Row
            Save-Summary
        }
        Write-Output ("{0}: success={1}, exit={2}, timeout={3}" -f $Row.name, $Row.success, $Row.exit_code, $Row.timed_out)
    }
    foreach ($Identity in $Summary.source_identity) {
        if ((Get-FileHash -LiteralPath $Identity.path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Identity.sha256) {
            throw "Source changed during verifier execution: $($Identity.path)"
        }
    }
    foreach ($Identity in $Summary.input_identity) {
        if ((Get-FileHash -LiteralPath $Identity.path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $Identity.sha256) {
            throw "Input changed during verifier execution: $($Identity.path)"
        }
    }
    $Summary.success = $Summary.cases.Count -eq 4 -and @($Summary.cases | Where-Object { -not $_.success }).Count -eq 0
} catch {
    $Summary.failure = $_.Exception.Message
} finally {
    foreach ($File in (Get-ChildItem -LiteralPath $Output -Recurse -File | Where-Object { $_.Name -ne 'verification.json' })) {
        $Summary.artifacts += File-Identity $File.FullName
    }
    $Summary.finished_utc = [DateTime]::UtcNow.ToString('o')
    Save-Summary
}
if (-not $Summary.success) { throw "D3D12 verifier failed. Retained evidence: $Output" }

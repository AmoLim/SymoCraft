[CmdletBinding()]
param(
    [string]$ProductionBuild,
    [switch]$BuildPlatform,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [ValidateSet('Configure', 'Build')][string]$Action = 'Build',
    [string]$BuildDirectory,
    [string]$DxcPath,
    [string]$CLionPath = 'E:/Applications/JetBrains/CLion',
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$VisualStudioPath = 'E:/Applications/Microsoft VS/2022',
    [ValidatePattern('^14\.[0-9]+(\.[0-9]+)?$')][string]$ToolsetVersion = '14.38',
    [ValidateRange(1, 64)][int]$Jobs = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
function Workspace-OutPath {
    param([string]$Path)
    $resolved = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($Path)) { $Path } else { Join-Path $root $Path }))
    if (-not $resolved.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Production imports and experiment builds must stay under workspace out.'
    }
    return $resolved
}
function Resolve-Tool {
    param([string]$ExplicitPath, [string]$BundledRelativePath)
    $path = if ($ExplicitPath) { $ExplicitPath } else { Join-Path $CLionPath $BundledRelativePath }
    return (Get-Item -LiteralPath $path -ErrorAction Stop).FullName
}
function Invoke-Native {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE." }
}
function Read-Version {
    param([string]$Program, [string[]]$Arguments)
    try {
        $lines = @(& $Program @Arguments 2>&1 | ForEach-Object { $_.ToString() })
        return [pscustomobject]@{exit_code=$LASTEXITCODE; output=($lines | Select-Object -First 6) -join "`n"}
    } catch {
        return [pscustomobject]@{exit_code=$null; output=$_.Exception.Message}
    }
}
function Assert-Cache {
    param([string]$Directory, [string]$Source, [string]$Imports = '')
    $cachePath = Join-Path $Directory 'CMakeCache.txt'
    if (-not (Test-Path -LiteralPath $cachePath -PathType Leaf)) { return }
    $cache = Get-Content -LiteralPath $cachePath -Raw
    $sourceHome = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)')
    $configurationEntry = [regex]::Match($cache, '(?m)^CMAKE_BUILD_TYPE:STRING=([^\r\n]+)')
    if (-not $sourceHome.Success -or -not $configurationEntry.Success -or
        -not [IO.Path]::GetFullPath($sourceHome.Groups[1].Value).Equals([IO.Path]::GetFullPath($Source), [StringComparison]::OrdinalIgnoreCase) -or
        $configurationEntry.Groups[1].Value -ne $Configuration) {
        throw 'Existing cache belongs to another source or configuration; choose a new directory.'
    }
    if ($Imports) {
        $entry = [regex]::Match($cache, '(?m)^SYMOCRAFT_R1_PRODUCTION_BUILD:PATH=([^\r\n]+)')
        if (-not $entry.Success -or -not [IO.Path]::GetFullPath($entry.Groups[1].Value).Equals($Imports, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Existing experiment cache uses different production imports; choose a new directory.'
        }
    }
}

if (-not $ProductionBuild) { $ProductionBuild = "out/m3-t3/platform-$($Configuration.ToLowerInvariant())" }
if (-not $BuildDirectory) { $BuildDirectory = "out/m3-t3/r1-$($Configuration.ToLowerInvariant())-001" }
$production = Workspace-OutPath $ProductionBuild
$build = Workspace-OutPath $BuildDirectory
if ($build.Equals($production, [StringComparison]::OrdinalIgnoreCase)) { throw 'Keep experiment and production caches separate.' }
if ($BuildPlatform -and -not $production.StartsWith((Join-Path $outRoot 'm3-t3') + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw '-BuildPlatform only writes isolated out/m3-t3 caches, never T2 caches.'
}
Assert-Cache $production $root
Assert-Cache $build $PSScriptRoot $production
if (-not $BuildPlatform -and -not (Test-Path -LiteralPath (Join-Path $production 'CMakeCache.txt') -PathType Leaf)) {
    throw 'Supply an existing independent -ProductionBuild or use -BuildPlatform.'
}
$cmake = Resolve-Tool $CMakePath 'bin/cmake/win/x64/bin/cmake.exe'
$ninja = Resolve-Tool $NinjaPath 'bin/ninja/win/x64/ninja.exe'
$VisualStudioPath = (Get-Item -LiteralPath $VisualStudioPath -ErrorAction Stop).FullName
$savedEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $savedEnvironment[$_.Name] = $_.Value }
try {
    Import-Module (Join-Path $VisualStudioPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll') -ErrorAction Stop
    Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation `
        -DevCmdArguments "-arch=x64 -host_arch=x64 -vcvars_ver=$ToolsetVersion" | Out-Null
    $env:VSLANG = '1033'
    $compiler = (Get-Command cl.exe -CommandType Application -ErrorAction Stop).Source.Replace('\', '/')
    if (-not $DxcPath) {
        $command = Get-Command dxc.exe -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
        if ($command) { $DxcPath = $command.Source }
        elseif ($env:WindowsSdkDir) {
            $candidates = @(Get-ChildItem -LiteralPath (Join-Path $env:WindowsSdkDir 'bin') -Directory -ErrorAction SilentlyContinue |
                Sort-Object Name -Descending | ForEach-Object { Join-Path $_.FullName 'x64/dxc.exe' } |
                Where-Object { Test-Path -LiteralPath $_ -PathType Leaf })
            if ($candidates.Count -gt 0) { $DxcPath = $candidates[0] }
        }
    }
    if (-not $DxcPath) { throw 'DXC is unavailable. Install/provide DXC and pass -DxcPath; there is no FXC fallback.' }
    $dxc = (Get-Item -LiteralPath $DxcPath -ErrorAction Stop).FullName.Replace('\', '/')
    $common = @('-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_TRY_COMPILE_CONFIGURATION=$Configuration",
        "-DCMAKE_C_COMPILER=$compiler", "-DCMAKE_CXX_COMPILER=$compiler", "-DCMAKE_MAKE_PROGRAM=$($ninja.Replace('\', '/'))")
    if ($BuildPlatform) {
        # Root configuration exposes platform only with BUILD_GAME; target selection excludes the game/renderer build.
        Invoke-Native $cmake (@('-S', $root, '-B', $production) + $common + @('-DBUILD_TESTING=OFF',
            '-DSYMOCRAFT_BUILD_GAME=ON', '-DSYMOCRAFT_BUILD_BENCHMARK=OFF'))
        Invoke-Native $cmake @('--build', $production, '--target', 'symocraft_platform', '--parallel', "$Jobs")
    }
    Invoke-Native $cmake (@('-S', $PSScriptRoot, '-B', $build) + $common + @('-DBUILD_TESTING=ON',
        "-DSYMOCRAFT_R1_PRODUCTION_BUILD=$($production.Replace('\', '/'))", "-DSYMOCRAFT_R1_DXC=$dxc"))
    if ($Action -eq 'Build') {
        Invoke-Native $cmake @('--build', $build, '--parallel', "$Jobs")
        $executable = (Get-Item -LiteralPath (Join-Path $build 'SymoCraftD3D12R1Probe.exe')).FullName
        $sources = @()
        foreach ($file in @(Get-ChildItem -LiteralPath $PSScriptRoot -Recurse -File | Where-Object { $_.Extension -in @('.cpp', '.h', '.hlsl', '.ps1', '.txt', '.cmake') })) {
            $sources += [pscustomobject]@{path=$file.FullName; sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
        }
        $shaders = @()
        foreach ($name in @('probe-vs.cso', 'probe-ps.cso')) {
            $path = (Get-Item -LiteralPath (Join-Path $build "shaders/$name")).FullName
            $shaders += [pscustomobject]@{path=$path; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
        }
        $dxcVersion = Read-Version $dxc @('--version')
        if ($null -eq $dxcVersion.exit_code -or $dxcVersion.exit_code -ne 0) { $dxcVersion = Read-Version $dxc @('-help') }
        $toolchain = [ordered]@{
            compiler_path=$compiler
            compiler_file_version=(Get-Item -LiteralPath $compiler).VersionInfo.FileVersion
            msvc_tools_version=$env:VCToolsVersion
            visual_studio_path=$VisualStudioPath
            visual_studio_version=$env:VisualStudioVersion
            windows_sdk_path=$env:WindowsSdkDir
            windows_sdk_version=$env:WindowsSDKVersion
            windows_sdk_library_version=$env:WindowsSDKLibVersion
            universal_crt_version=$env:UCRTVersion
            dxc_path=$dxc
            dxc_file_version=(Get-Item -LiteralPath $dxc).VersionInfo.FileVersion
            dxc_version=$dxcVersion
            cmake_path=$cmake
            cmake_version=(Read-Version $cmake @('--version'))
            ninja_path=$ninja
            ninja_version=(Read-Version $ninja @('--version'))
        }
        [ordered]@{
            built_utc=[DateTimeOffset]::UtcNow.ToString('o')
            executable=$executable
            executable_sha256=(Get-FileHash -LiteralPath $executable -Algorithm SHA256).Hash.ToLowerInvariant()
            production_imports_sha256=(Get-FileHash -LiteralPath (Join-Path $build 'production-imports.json') -Algorithm SHA256).Hash.ToLowerInvariant()
            toolchain=$toolchain; sources=$sources; shaders=$shaders
        } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $build 'build-identities.json') -Encoding UTF8
    }
} finally {
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (-not $savedEnvironment.ContainsKey($entry.Name)) { Remove-Item -LiteralPath "Env:$($entry.Name)" }
    }
    foreach ($name in $savedEnvironment.Keys) { [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process') }
}

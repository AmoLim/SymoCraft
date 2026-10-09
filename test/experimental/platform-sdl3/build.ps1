[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$ProductionBuild,
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [ValidateSet('Configure', 'Build')][string]$Action = 'Build',
    [string]$BuildDirectory,
    [switch]$Vulkan,
    [switch]$InputValidation,
    [string]$HeadersRoot,
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
        throw 'Production imports and probe builds must stay under workspace out.'
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

$production = Workspace-OutPath $ProductionBuild
if (-not (Test-Path -LiteralPath (Join-Path $production 'CMakeCache.txt') -PathType Leaf)) {
    throw 'Build the current production configuration first; this probe only imports its actual libraries.'
}
if (-not $BuildDirectory) {
    $stage = if ($InputValidation) { 's3' } else { 's2' }
    $BuildDirectory = "out/m3-t2/$stage/probe-$($Configuration.ToLowerInvariant())-001"
}
$build = Workspace-OutPath $BuildDirectory
if ($build.Equals($production, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Keep the probe and production build directories separate.'
}
if ($Vulkan -and [string]::IsNullOrWhiteSpace($HeadersRoot)) { throw 'Pass the installed SDK Include as -HeadersRoot with -Vulkan.' }
if (-not $Vulkan -and $HeadersRoot) { throw '-HeadersRoot requires -Vulkan.' }
$headers = ''
if ($Vulkan) {
    $headers = (Get-Item -LiteralPath $HeadersRoot -ErrorAction Stop).FullName
    if (-not (Test-Path -LiteralPath (Join-Path $headers 'vulkan/vulkan_core.h') -PathType Leaf)) {
        throw 'HeadersRoot must contain vulkan/vulkan_core.h.'
    }
}
$cachePath = Join-Path $build 'CMakeCache.txt'
if (Test-Path -LiteralPath $cachePath) {
    $cache = Get-Content -LiteralPath $cachePath -Raw
    $sourceHome = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)')
    $cachedConfiguration = [regex]::Match($cache, '(?m)^CMAKE_BUILD_TYPE:STRING=([^\r\n]+)')
    $imports = [regex]::Match($cache, '(?m)^SYMOCRAFT_PLATFORM_PRODUCTION_BUILD:PATH=([^\r\n]+)')
    if (-not $sourceHome.Success -or -not $cachedConfiguration.Success -or -not $imports.Success -or
        -not [IO.Path]::GetFullPath($sourceHome.Groups[1].Value).Equals([IO.Path]::GetFullPath($PSScriptRoot), [StringComparison]::OrdinalIgnoreCase) -or
        $cachedConfiguration.Groups[1].Value -ne $Configuration -or
        -not [IO.Path]::GetFullPath($imports.Groups[1].Value).Equals($production, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Existing probe cache belongs to another source, configuration or production import; choose a new directory.'
    }
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
    Invoke-Native $cmake @('-S', $PSScriptRoot, '-B', $build, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_TRY_COMPILE_CONFIGURATION=$Configuration",
        "-DCMAKE_C_COMPILER=$compiler", "-DCMAKE_CXX_COMPILER=$compiler",
        "-DCMAKE_MAKE_PROGRAM=$($ninja.Replace('\', '/'))",
        "-DSYMOCRAFT_PLATFORM_PRODUCTION_BUILD=$($production.Replace('\', '/'))",
        "-DSYMOCRAFT_PLATFORM_PROBE_VULKAN=$(if ($Vulkan) { 'ON' } else { 'OFF' })",
        "-DSYMOCRAFT_PLATFORM_PROBE_INPUT=$(if ($InputValidation) { 'ON' } else { 'OFF' })",
        "-DSYMOCRAFT_VULKAN_HEADERS_ROOT=$($headers.Replace('\', '/'))")
    if ($Action -eq 'Build') { Invoke-Native $cmake @('--build', $build, '--parallel', "$Jobs") }
} finally {
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (-not $savedEnvironment.ContainsKey($entry.Name)) { Remove-Item -LiteralPath "Env:$($entry.Name)" }
    }
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}

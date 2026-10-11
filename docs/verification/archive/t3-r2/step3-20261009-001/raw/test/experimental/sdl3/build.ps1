[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('Configure', 'Build', 'Test')]
    [string]$Action = 'Test',
    [string]$BuildDirectory,
    [switch]$Vulkan,
    [switch]$InputComparison,
    [string]$HeadersRoot,
    [string]$CLionPath = 'E:/Applications/JetBrains/CLion',
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$VisualStudioPath = 'E:/Applications/Microsoft VS/2022',
    [ValidatePattern('^14\.[0-9]+(\.[0-9]+)?$')]
    [string]$ToolsetVersion = '14.38',
    [ValidateRange(1, 64)]
    [int]$Jobs = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or -not [Environment]::Is64BitProcess) {
    throw 'Use 64-bit PowerShell on Windows with MSVC 2022 and the Windows SDK installed.'
}

function Resolve-Tool {
    param([string]$ExplicitPath, [string]$BundledRelativePath)
    $path = if ($ExplicitPath) { $ExplicitPath } else { Join-Path $CLionPath $BundledRelativePath }
    return (Get-Item -LiteralPath $path -ErrorAction Stop).FullName
}

function Invoke-Native {
    param([string]$Program, [string[]]$Arguments)
    Write-Host "Running: $Program $($Arguments -join ' ')"
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE."
    }
}

$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outputRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$mode = $Configuration.ToLowerInvariant() + $(if ($Vulkan) { '-vulkan' } else { '-gl' })
if (-not $BuildDirectory) {
    $BuildDirectory = "out/m3-t2/build/sdl3-$mode"
}
$buildPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
    [IO.Path]::GetFullPath($BuildDirectory)
} else {
    [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
}
if (-not $buildPath.StartsWith($outputRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The preparation build directory must be below the workspace out directory.'
}
$cachePath = Join-Path $buildPath 'CMakeCache.txt'
if ($InputComparison -and $Configuration -ne 'Release') {
    throw 'The continuous comparison imports the saved Release camera; use -Configuration Release.'
}
if (Test-Path -LiteralPath $cachePath -PathType Leaf) {
    $cache = Get-Content -LiteralPath $cachePath -Raw
    $sourceMatch = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)')
    if (-not $sourceMatch.Success -or
        -not [IO.Path]::GetFullPath($sourceMatch.Groups[1].Value).Equals(
            [IO.Path]::GetFullPath($PSScriptRoot), [StringComparison]::OrdinalIgnoreCase)) {
        throw 'The build directory belongs to another project; choose a separate SDL3 preparation directory.'
    }
    $configurationMatch = [regex]::Match($cache, '(?m)^CMAKE_BUILD_TYPE:STRING=([^\r\n]+)')
    $vulkanMatch = [regex]::Match($cache, '(?m)^SYMOCRAFT_SDL3_PROBE_VULKAN:BOOL=([^\r\n]+)')
    $expectedVulkan = if ($Vulkan) { 'ON' } else { 'OFF' }
    if (($configurationMatch.Success -and $configurationMatch.Groups[1].Value -ne $Configuration) -or
        ($vulkanMatch.Success -and $vulkanMatch.Groups[1].Value -ne $expectedVulkan)) {
        throw 'Keep Debug/Release and GL/Vulkan preparation caches in separate build directories.'
    }
}
if ($Vulkan -and [string]::IsNullOrWhiteSpace($HeadersRoot)) {
    throw 'Pass -HeadersRoot explicitly when enabling the Vulkan preparation probe.'
}
if (-not $Vulkan -and $HeadersRoot) {
    throw '-HeadersRoot requires -Vulkan; keep the GL-only preparation configuration independent.'
}
$headersPath = ''
if ($Vulkan) {
    $headersPath = (Get-Item -LiteralPath $HeadersRoot -ErrorAction Stop).FullName
    if (-not (Test-Path -LiteralPath (Join-Path $headersPath 'vulkan/vulkan_core.h') -PathType Leaf)) {
        throw 'HeadersRoot must contain vulkan/vulkan_core.h.'
    }
}
$cmake = Resolve-Tool $CMakePath 'bin/cmake/win/x64/bin/cmake.exe'
$ninja = Resolve-Tool $NinjaPath 'bin/ninja/win/x64/ninja.exe'
$ctest = (Get-Item -LiteralPath (Join-Path (Split-Path $cmake) 'ctest.exe') -ErrorAction Stop).FullName
$VisualStudioPath = (Get-Item -LiteralPath $VisualStudioPath -ErrorAction Stop).FullName
$devShellModule = Join-Path $VisualStudioPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll'
$savedEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $savedEnvironment[$_.Name] = $_.Value }
$locationPushed = $false

try {
    Import-Module $devShellModule -ErrorAction Stop
    Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation `
        -DevCmdArguments "-arch=x64 -host_arch=x64 -vcvars_ver=$ToolsetVersion" | Out-Null
    $env:PATH = "$(Split-Path $cmake);$(Split-Path $ninja);$env:PATH"
    $env:VSLANG = '1033'
    $compiler = (Get-Command cl.exe -CommandType Application -ErrorAction Stop).Source
    Write-Host "Visual Studio: $VisualStudioPath"
    Write-Host "Compiler: $compiler"
    $compilerCachePath = $compiler.Replace('\', '/')
    $ninjaCachePath = $ninja.Replace('\', '/')
    $installCachePath = (Join-Path $outputRoot "m3-t2/install/sdl3-$mode").Replace('\', '/')
    Invoke-Native $cmake @('--version')
    Invoke-Native $ninja @('--version')
    Push-Location $root
    $locationPushed = $true
    $vulkanValue = if ($Vulkan) { 'ON' } else { 'OFF' }
    $arguments = @('-S', $PSScriptRoot, '-B', $buildPath, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_TRY_COMPILE_CONFIGURATION=$Configuration", "-DCMAKE_MAKE_PROGRAM=$ninjaCachePath",
        "-DCMAKE_C_COMPILER=$compilerCachePath", "-DCMAKE_CXX_COMPILER=$compilerCachePath",
        "-DCMAKE_INSTALL_PREFIX=$installCachePath",
        "-DSYMOCRAFT_SDL3_PROBE_VULKAN=$vulkanValue",
        "-DSYMOCRAFT_SDL3_INPUT_COMPARISON=$(if ($InputComparison) { 'ON' } else { 'OFF' })",
        "-DSYMOCRAFT_VULKAN_HEADERS_ROOT=$($headersPath.Replace('\', '/'))")
    Invoke-Native $cmake $arguments
    if ($Action -ne 'Configure') {
        Invoke-Native $cmake @('--build', $buildPath, '--parallel', "$Jobs")
    }
    if ($Action -eq 'Test') {
        Invoke-Native $ctest @('--test-dir', $buildPath, '-C', $Configuration,
            '--output-on-failure', '--no-tests=error')
    }
} finally {
    if ($locationPushed) {
        Pop-Location
    }
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (-not $savedEnvironment.ContainsKey($entry.Name)) {
            Remove-Item -LiteralPath "Env:$($entry.Name)"
        }
    }
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}

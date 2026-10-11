[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')][string]$Configuration = 'Debug',
    [Parameter(Mandatory = $true)][string]$BuildDirectory,
    [string]$CLionPath = 'E:/Applications/JetBrains/CLion',
    [string]$VisualStudioPath = 'E:/Applications/Microsoft VS/2022',
    [ValidatePattern('^14\.[0-9]+(\.[0-9]+)?$')][string]$ToolsetVersion = '14.38',
    [ValidateRange(1, 64)][int]$Jobs = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../../..'))
$outRoot = Join-Path $root 'out/m3-t3'
$build = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($BuildDirectory)) { $BuildDirectory } else { Join-Path $root $BuildDirectory }))
if (-not $build.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The R1 CPU build must stay under workspace out/m3-t3.'
}
$cachePath = Join-Path $build 'CMakeCache.txt'
if (Test-Path -LiteralPath $cachePath) {
    $cache = Get-Content -LiteralPath $cachePath -Raw
    $sourceHome = [regex]::Match($cache, '(?m)^CMAKE_HOME_DIRECTORY:INTERNAL=([^\r\n]+)')
    $type = [regex]::Match($cache, '(?m)^CMAKE_BUILD_TYPE:STRING=([^\r\n]+)')
    if (-not $sourceHome.Success -or -not $type.Success -or
        -not [IO.Path]::GetFullPath($sourceHome.Groups[1].Value).Equals($PSScriptRoot, [StringComparison]::OrdinalIgnoreCase) -or
        $type.Groups[1].Value -ne $Configuration) {
        throw 'Existing cache belongs to another project or configuration; use a new directory.'
    }
}
$cmake = (Get-Item -LiteralPath (Join-Path $CLionPath 'bin/cmake/win/x64/bin/cmake.exe')).FullName
$ninja = (Get-Item -LiteralPath (Join-Path $CLionPath 'bin/ninja/win/x64/ninja.exe')).FullName
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
function Invoke-Native {
    param([string]$Program, [string[]]$Arguments)
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE." }
}
$savedEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $savedEnvironment[$_.Name] = $_.Value }
try {
    Import-Module (Join-Path $VisualStudioPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll') -ErrorAction Stop
    Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation `
        -DevCmdArguments "-arch=x64 -host_arch=x64 -vcvars_ver=$ToolsetVersion" | Out-Null
    $env:VSLANG = '1033'
    $compiler = (Get-Command cl.exe -CommandType Application -ErrorAction Stop).Source
    Invoke-Native $cmake @('-S', $PSScriptRoot, '-B', $build, '-G', 'Ninja',
        "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_TRY_COMPILE_CONFIGURATION=$Configuration",
        "-DCMAKE_C_COMPILER=$compiler", "-DCMAKE_CXX_COMPILER=$compiler", "-DCMAKE_MAKE_PROGRAM=$ninja",
        '-DBUILD_TESTING=ON')
    Invoke-Native $cmake @('--build', $build, '--parallel', "$Jobs")
    Invoke-Native $ctest @('--test-dir', $build, '--output-on-failure')
} finally {
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (-not $savedEnvironment.ContainsKey($entry.Name)) { Remove-Item -LiteralPath "Env:$($entry.Name)" }
    }
    foreach ($name in $savedEnvironment.Keys) { [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process') }
}

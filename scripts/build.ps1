[CmdletBinding()]
param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Debug',
    [ValidateSet('Configure', 'Build', 'Test', 'Install')]
    [string]$Action = 'Test',
    [string]$CLionPath = $env:CLION_HOME,
    [string]$CMakePath,
    [string]$NinjaPath,
    [string]$VisualStudioPath,
    [string]$BuildDirectory,
    [ValidatePattern('^$|^14\.[0-9]+(\.[0-9]+)?$')]
    [string]$ToolsetVersion,
    [ValidateRange(1, 64)]
    [int]$Jobs = 8
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or
    -not [Environment]::Is64BitProcess) {
    throw 'Use 64-bit PowerShell on Windows with MSVC 2022 and the Windows SDK installed.'
}

function Resolve-Tool {
    param([string]$ExplicitPath, [string]$Name, [string]$BundledRelativePath)
    if ($ExplicitPath) {
        return (Get-Item -LiteralPath $ExplicitPath -ErrorAction Stop).FullName
    }
    if ($CLionPath) {
        return (Get-Item -LiteralPath (Join-Path $CLionPath $BundledRelativePath) -ErrorAction Stop).FullName
    }
    $command = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($null -ne $command) {
        return $command.Source
    }
    throw "Cannot find $Name. Supply -CLionPath (or CLION_HOME), or the explicit tool path."
}

function Invoke-Native {
    param([string]$Program, [string[]]$Arguments)
    Write-Host "Running: $Program $($Arguments -join ' ')"
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$Program failed with exit code $LASTEXITCODE."
    }
}

$cmake = Resolve-Tool $CMakePath 'cmake.exe' 'bin/cmake/win/x64/bin/cmake.exe'
$ninja = Resolve-Tool $NinjaPath 'ninja.exe' 'bin/ninja/win/x64/ninja.exe'
$ctest = (Get-Item -LiteralPath (Join-Path (Split-Path $cmake) 'ctest.exe')).FullName
if (-not $VisualStudioPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere -PathType Leaf)) {
        throw 'Cannot find vswhere. Install Visual Studio 2022 C++ tools or supply -VisualStudioPath.'
    }
    $VisualStudioPath = & $vswhere -latest -products '*' -version '[17.0,18.0)' `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($VisualStudioPath)) {
        throw 'No complete Visual Studio 2022 C++ installation found. Install the C++ tools and Windows SDK.'
    }
}
$VisualStudioPath = (Get-Item -LiteralPath $VisualStudioPath).FullName
$devShellModule = Join-Path $VisualStudioPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll'
$root = Split-Path $PSScriptRoot
$preset = 'windows-' + $Configuration.ToLowerInvariant()
$buildDirectoryPath = Join-Path $root "out/build/$preset"
if ($BuildDirectory) {
    $buildDirectoryPath = if ([IO.Path]::IsPathRooted($BuildDirectory)) {
        [IO.Path]::GetFullPath($BuildDirectory)
    } else {
        [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
    }
}
$savedEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $savedEnvironment[$_.Name] = $_.Value }
$locationPushed = $false

try {
    Import-Module $devShellModule -ErrorAction Stop
    $devArguments = '-arch=x64 -host_arch=x64'
    if ($ToolsetVersion) {
        $devArguments += " -vcvars_ver=$ToolsetVersion"
    }
    Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation `
        -DevCmdArguments $devArguments | Out-Null
    $env:PATH = "$(Split-Path $cmake);$(Split-Path $ninja);$env:PATH"
    # Ninja must see the same /showIncludes language during detection and compilation.
    $env:VSLANG = '1033'
    $compiler = (Get-Command cl.exe -CommandType Application -ErrorAction Stop).Source
    Write-Host "Visual Studio: $VisualStudioPath"
    Write-Host "Compiler: $compiler"
    Invoke-Native $cmake @('--version')
    Invoke-Native $ninja @('--version')
    Push-Location $root
    $locationPushed = $true
    Invoke-Native $cmake @('--preset', $preset, '-B', $buildDirectoryPath,
        "-DCMAKE_MAKE_PROGRAM=$ninja", "-DCMAKE_C_COMPILER=$compiler", "-DCMAKE_CXX_COMPILER=$compiler")
    if ($Action -ne 'Configure') {
        if ($BuildDirectory) {
            Invoke-Native $cmake @('--build', $buildDirectoryPath, '--parallel', "$Jobs")
        } else {
            Invoke-Native $cmake @('--build', '--preset', $preset, '--parallel', "$Jobs")
        }
    }
    if ($Action -in @('Test', 'Install')) {
        if ($BuildDirectory) {
            # Mixing a preset binaryDir with --test-dir can leave logs in the preset directory.
            Invoke-Native $ctest @('--test-dir', $buildDirectoryPath, '-C', $Configuration,
                '--output-on-failure', '--no-tests=error')
        } else {
            Invoke-Native $ctest @('--preset', $preset, '--output-on-failure', '--no-tests=error')
        }
    }
    if ($Action -eq 'Install') {
        Invoke-Native $cmake @('--install', $buildDirectoryPath)
    }
} finally {
    if ($locationPushed) {
        Pop-Location
    }
    # Developer-shell changes belong to this invocation, not the caller's shell.
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (-not $savedEnvironment.ContainsKey($entry.Name)) {
            Remove-Item -LiteralPath "Env:$($entry.Name)"
        }
    }
    foreach ($name in $savedEnvironment.Keys) {
        [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process')
    }
}

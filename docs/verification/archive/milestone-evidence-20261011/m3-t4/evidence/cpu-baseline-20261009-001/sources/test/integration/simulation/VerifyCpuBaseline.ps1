[CmdletBinding()]
param(
    [Parameter(Mandatory)][ValidateSet('Debug', 'Release')][string]$Configuration,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [string]$VisualStudioPath,
    [string]$ToolsetVersion = '14.38',
    [string]$RepositoryRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path -LiteralPath $RepositoryRoot).Path
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) { throw "Use a new output directory; refusing to overwrite $output" }
New-Item -ItemType Directory -Path $output | Out-Null
$cmake = (Get-Command cmake.exe -CommandType Application).Source
$ctest = Join-Path (Split-Path $cmake) 'ctest.exe'
$ninja = (Get-Command ninja.exe -CommandType Application).Source
if (-not $VisualStudioPath) {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    $VisualStudioPath = & $vswhere -latest -products '*' -version '[17.0,18.0)' `
        -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($VisualStudioPath)) { throw 'MSVC 2022 installation not found' }
}
Import-Module (Join-Path $VisualStudioPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll')
Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation `
    -DevCmdArguments "-arch=x64 -host_arch=x64 -vcvars_ver=$ToolsetVersion" | Out-Null
$env:VSLANG = '1033'
$compiler = (Get-Command cl.exe -CommandType Application).Source
$steps = [Collections.Generic.List[object]]::new()
$result = [ordered]@{ configuration = $Configuration; compiler = $compiler;
    compiler_version = (Get-Item -LiteralPath $compiler).VersionInfo.ProductVersion;
    windows_sdk = $env:WindowsSDKVersion; success = $false; steps = $steps }

function Invoke-Logged {
    param([string]$Name, [string]$Program, [string[]]$Arguments)
    $log = Join-Path $output "$Name.log"
    & $Program @Arguments 2>&1 | Tee-Object -FilePath $log | Write-Host
    $code = $LASTEXITCODE
    $steps.Add([ordered]@{ name = $Name; program = $Program; arguments = $Arguments; exit_code = $code; log = $log })
    if ($code -ne 0) { throw "$Name failed with exit code $code" }
}

try {
    Invoke-Logged 'cmake-version' $cmake @('--version')
    Invoke-Logged 'ninja-version' $ninja @('--version')
    $build = Join-Path $output 'build'
    $options = @('-G', 'Ninja', "-DCMAKE_BUILD_TYPE=$Configuration", "-DCMAKE_MAKE_PROGRAM=$ninja",
        "-DCMAKE_C_COMPILER=$compiler", "-DCMAKE_CXX_COMPILER=$compiler", '-DSYMOCRAFT_CPU_ONLY=ON')
    Invoke-Logged 'configure' $cmake (@('-S', $root, '-B', $build, '-DBUILD_TESTING=ON') + $options)
    Invoke-Logged 'build' $cmake @('--build', $build, '--parallel', '8')
    $compilations = Get-Content -LiteralPath (Join-Path $build 'compile_commands.json') -Raw | ConvertFrom-Json
    $physicsUnits = @($compilations | Where-Object { $_.file.Replace('\', '/') -like '*/game/modules/simulation/src/physics_system.cpp' })
    if ($physicsUnits.Count -ne 1) { throw 'Production physics.cpp must be compiled exactly once by its owning library' }
    Invoke-Logged 'baseline-repeat' $ctest @('--test-dir', $build, '-R', '^simulation\.legacy_(physics|gameplay)_baseline$',
        '--repeat', 'until-fail:3', '--output-on-failure', '--no-tests=error',
        '--output-junit', (Join-Path $output 'baseline-repeat.xml'))
    foreach ($target in @('legacy_physics_baseline_tests', 'legacy_gameplay_baseline_tests')) {
        Invoke-Logged "$target-commands" $ninja @('-C', $build, '-t', 'commands', $target)
        $commands = Get-Content -LiteralPath (Join-Path $output "$target-commands.log") -Raw
        if ($commands -match '(?i)symocraft_(platform|renderer|glad|startup|app_)|game[/\\]app[/\\]|SDL3|glfw|d3d12|dxgi|vulkan') {
            throw "Graphics/window dependency in actual baseline commands: $target"
        }
        Invoke-Logged "$target-output" (Join-Path $build "test/unit/simulation/$target.exe") @((Join-Path $root 'assets/configs/blockFormats.yaml'))
    }
    $noTests = Join-Path $output 'no-tests'
    Invoke-Logged 'no-tests-configure' $cmake (@('-S', $root, '-B', $noTests, '-DBUILD_TESTING=OFF') + $options)
    Invoke-Logged 'no-tests-build' $cmake @('--build', $noTests, '--target', 'symocraft_simulation', '--parallel', '8')
    $targets = Get-Content -LiteralPath (Join-Path $noTests 'CMakeFiles/TargetDirectories.txt') -Raw
    if ($targets -match '(?i)(_tests|fixture|test/unit|test/integration)') { throw 'Test target leaked into BUILD_TESTING=OFF' }
    $commands = Get-Content -LiteralPath (Join-Path $noTests 'compile_commands.json') -Raw
    if ($commands -match '(?i)legacy_(physics|gameplay)_baseline|cpu_baseline_fixture|[/\\]test[/\\]') {
        throw 'Test compilation leaked into BUILD_TESTING=OFF'
    }
    foreach ($directory in @($build, $noTests)) {
        $targets = Get-Content -LiteralPath (Join-Path $directory 'CMakeFiles/TargetDirectories.txt') -Raw
        if ($targets -match '(?i)symocraft_(platform|glad|renderer_(legacy_opengl|d3d12|vulkan))\.dir|SDL3[^/]*\.dir|vendor/sdl3') {
            throw "Native target configured in CPU build: $directory"
        }
    }
    Invoke-Logged 'ctest' $ctest @('--test-dir', $build, '--output-on-failure', '--no-tests=error',
        '--output-junit', (Join-Path $output 'ctest.xml'))
    $result['boundary_checks'] = @('actual baseline commands have no renderer/app/window/SDK dependencies',
        'production physics_system.cpp compiled exactly once', 'BUILD_TESTING=OFF has no test targets or compilations',
        'CPU configurations have no native targets')
    $result.success = $true
} finally {
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding utf8
}

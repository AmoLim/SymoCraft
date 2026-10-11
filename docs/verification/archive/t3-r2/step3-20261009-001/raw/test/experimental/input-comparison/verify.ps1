[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$OutputDirectory,
    [string]$GlfwExecutable,
    [string]$SdlExecutable
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $root $OutputDirectory }))
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence must stay under workspace out.' }
if (Test-Path -LiteralPath $output) { throw 'Choose a fresh output directory.' }
if (-not $GlfwExecutable) { $GlfwExecutable = Join-Path $root 'out/m3-t2/probe/input-comparison-glfw-release-001/Release/SymoCraftGlfwInputComparison.exe' }
if (-not $SdlExecutable) { $SdlExecutable = Join-Path $root 'out/m3-t2/probe/input-comparison-sdl-release-002/SymoCraftSdl3InputComparison.exe' }
$executables = @{ GLFW = (Get-Item -LiteralPath $GlfwExecutable).FullName; SDL3 = (Get-Item -LiteralPath $SdlExecutable).FullName }
New-Item -ItemType Directory -Path $output | Out-Null
$cases = @(
    @{ Name='glfw-normal'; Backend='GLFW'; Cursor='normal'; Scale='1' },
    @{ Name='sdl-normal'; Backend='SDL3'; Cursor='normal'; Scale='1' },
    @{ Name='glfw-lock'; Backend='GLFW'; Cursor='lock'; Scale='1' },
    @{ Name='sdl-lock-system'; Backend='SDL3'; Cursor='lock'; Scale='1' },
    @{ Name='sdl-lock-unscaled'; Backend='SDL3'; Cursor='lock'; Scale='0' },
    @{ Name='glfw-hidden'; Backend='GLFW'; Cursor='hidden'; Scale='1' },
    @{ Name='sdl-hidden'; Backend='SDL3'; Cursor='hidden'; Scale='1' }
)
$results = @()
foreach ($case in $cases) {
    $exe = $executables[$case.Backend]
    $caseOutput = Join-Path $output $case.Name
    $stdout = Join-Path $output ($case.Name + '.stdout.log')
    $stderr = Join-Path $output ($case.Name + '.stderr.log')
    $arguments = @('--frames', '12', '--cursor', $case.Cursor, '--system-scale', $case.Scale, '--output', ('"{0}"' -f $caseOutput))
    $process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $root -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    try {
        $null = $process.Handle
        if (-not $process.WaitForExit(30000)) { throw "$($case.Name) timed out." }
        $process.Refresh()
        $diagnostics = [IO.File]::ReadAllText($stderr)
        if ($process.ExitCode -ne 0 -or -not [string]::IsNullOrWhiteSpace($diagnostics)) { throw "$($case.Name) failed: $diagnostics" }
        $report = Get-Content -LiteralPath (Join-Path $caseOutput 'session.json') -Raw | ConvertFrom-Json
        if (-not $report.normal_exit -or $report.rendered_frames -ne 12 -or $report.recorded_frames -ne 12) { throw "$($case.Name) did not render the expected bounded frames." }
        $rows = @(Import-Csv -LiteralPath (Join-Path $caseOutput 'frames.csv'))
        if ($rows.Count -ne 12) { throw "$($case.Name) CSV row count differs." }
        $frame = Get-Item -LiteralPath (Join-Path $caseOutput 'first-frame.bmp')
        if ($frame.Length -lt 1024) { throw "$($case.Name) has no usable rendered frame." }
        $results += [ordered]@{ name=$case.Name; passed=$true; executable=$exe; executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant(); session=$report; frame_sha256=(Get-FileHash -LiteralPath $frame.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
    } finally {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
        $process.Dispose()
    }
}
$sources = @(Get-ChildItem -LiteralPath $PSScriptRoot -File | Sort-Object Name | ForEach-Object {
    [ordered]@{ path=$_.FullName.Substring($root.Length+1).Replace('\','/'); sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant() }
})
$dependencies = @()
foreach ($path in @('test/experimental/sdl3/CMakeLists.txt', 'test/experimental/glfw-baseline/CMakeLists.txt', 'test/experimental/sdl3/input_candidate.h',
    'out/m3-t2/build/glfw-release/game/modules/foundation/symocraft_foundation.lib', 'out/m3-t2/build/glfw-release/game/modules/ecs/symocraft_ecs.lib',
    'out/m3-t2/build/glfw-release/game/modules/simulation/symocraft_simulation.lib', 'out/m3-t2/build/glfw-release/game/modules/world/symocraft_world.lib',
    'out/m3-t2/build/glfw-release/symocraft_yaml.lib', 'out/m3-t2/build/glfw-release/game/modules/platform/symocraft_platform.lib',
    'out/m3-t2/build/glfw-release/vendor/glfw/src/glfw3.lib')) {
    $dependencies += [ordered]@{ path=$path; sha256=(Get-FileHash -LiteralPath (Join-Path $root $path) -Algorithm SHA256).Hash.ToLowerInvariant() }
}
$summary = [ordered]@{ created_utc=[DateTime]::UtcNow.ToString('o'); kind='bounded real GL rendering, not human input validation'; source_inputs=$sources; dependency_inputs=$dependencies; passed=$results.Count; cases=$results }
$summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding utf8
Write-Host "Input comparison rendering: $($results.Count)/$($cases.Count) passed."

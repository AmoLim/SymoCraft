[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$BuildDirectory,
    [Parameter(Mandatory)][string]$OutputDirectory,
    [switch]$IncludeOpenGLComparison,
    [ValidateRange(30, 300)][int]$ProcessTimeoutSeconds = 180
)

$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$build = [IO.Path]::GetFullPath($BuildDirectory)
$output = [IO.Path]::GetFullPath($OutputDirectory)
$allowed = [IO.Path]::GetFullPath((Join-Path $root 'out/m3-t3')) + [IO.Path]::DirectorySeparatorChar
foreach ($path in @($build, $output)) {
    if (-not $path.StartsWith($allowed, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'Game verification uses only this repository out/m3-t3 directories.'
    }
}
if (Test-Path -LiteralPath $output) { throw 'OutputDirectory must be new; old runs are never overwritten.' }
$executable = Join-Path $build 'bin/SymoCraft.exe'
$probe = Join-Path $build 'test/integration/renderer_game_report_probe.exe'
$inputs = @($executable, $probe, (Join-Path $build 'CMakeCache.txt'))
$inputs += @(Get-ChildItem -LiteralPath (Join-Path $build 'bin/assets') -Recurse -File | ForEach-Object FullName)
foreach ($path in $inputs) { if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Missing build input: $path" } }
[IO.Directory]::CreateDirectory($output) | Out-Null
$workdir = Join-Path $output 'unrelated-cwd'
[IO.Directory]::CreateDirectory($workdir) | Out-Null
function Identity([string]$path) {
    $item = Get-Item -LiteralPath $path
    [pscustomobject]@{path=$item.FullName; bytes=$item.Length; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
function Quote([string]$argument) {
    if ($argument.Contains('"')) { throw 'Unsupported quote in process argument.' }
    if ($argument.EndsWith('\')) { $argument += '\' }
    return '"' + $argument + '"'
}
$sourceFiles = @(Get-ChildItem -LiteralPath (Join-Path $root 'game'), (Join-Path $root 'cmake'),
    (Join-Path $root 'test/integration/renderer'), (Join-Path $root 'assets/shaders') -Recurse -File |
    Where-Object { $_.Extension -in @('.h','.hpp','.cpp','.hlsl','.hlsli','.cmake','.ps1') -or $_.Name -eq 'CMakeLists.txt' })
$sourceFiles += Get-Item -LiteralPath (Join-Path $root 'CMakeLists.txt')
$sources = @($sourceFiles | Sort-Object FullName -Unique | ForEach-Object { Identity $_.FullName })
$manifest = [ordered]@{
    schema='symocraft.t3.r2-step3-game-verification'; version=1; run_id=[Guid]::NewGuid().ToString()
    started_utc=[DateTime]::UtcNow.ToString('o'); finished_utc=$null
    source_identity=$sources; input_identity=@($inputs | ForEach-Object { Identity $_ })
    process_timeout_seconds=$ProcessTimeoutSeconds; cases=@(); success=$false
    fixed_complete_world_short_workloads=$true; manual_gameplay_acceptance=$false
    fifteen_minute_session=$false; actual_device_lost_exercised=$false; actual_in_flight_timeout_exercised=$false
    installed_package_acceptance=$false; r3_acceptance=$false; opengl_default_changed=$false
}
function Save-Manifest {
    $manifest | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'verification.json') -Encoding utf8
}
function Run-Process([string]$name, [string[]]$arguments, [int]$expectedExit) {
    $stdout = Join-Path $output "$name.stdout.log"
    $stderr = Join-Path $output "$name.stderr.log"
    $process = Start-Process -FilePath $executable -ArgumentList @($arguments | ForEach-Object { Quote $_ }) `
        -WorkingDirectory $workdir -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
    $timedOut = -not $process.WaitForExit($ProcessTimeoutSeconds * 1000)
    if ($timedOut) {
        $process.Kill()
        if (-not $process.WaitForExit(10000)) { throw 'Timed-out game could not be stopped within 10 seconds.' }
    }
    $process.WaitForExit()
    [pscustomobject]@{name=$name; arguments=$arguments; exit_code=$process.ExitCode; timed_out=$timedOut
        expected_exit=$expectedExit; success=(-not $timedOut -and $process.ExitCode -eq $expectedExit)
        stdout=$stdout; stderr=$stderr; observation=$null}
}
try {
    foreach ($preflight in @(
        @{name='d3d12-assets'; args=@('--renderer','d3d12','--check-assets'); exit=0},
        @{name='unavailable-vulkan'; args=@('--renderer','vulkan','--smoke-frames','1'); exit=3},
        @{name='invalid-backend'; args=@('--renderer','unknown'); exit=64}
    )) {
        $case = Run-Process $preflight.name $preflight.args $preflight.exit
        $manifest.cases += $case
        if ($case.name -eq 'unavailable-vulkan') {
            $diagnostic = [IO.File]::ReadAllText($case.stderr)
            $stdout = [IO.File]::ReadAllText($case.stdout)
            $case.success = $case.success -and $diagnostic.Contains('Vulkan') -and $diagnostic.Contains('SelectBackend') -and
                -not $stdout.Contains('Monitor size:')
        }
        Save-Manifest
    }
    $backends = @('d3d12')
    if ($IncludeOpenGLComparison) { $backends += 'opengl' }
    foreach ($backend in $backends) {
        foreach ($workload in @('static','walk','edit')) {
            $name = "$backend-$workload"
            $directory = Join-Path $output $name
            $duration = if ($workload -eq 'edit') { '9' } elseif ($workload -eq 'walk') { '3' } else { '2' }
            $arguments = @('--renderer',$backend,'--benchmark',$workload,'--output',$directory,
                '--seed','424242','--width','1920','--height','1080','--vsync','0',
                '--warmup-seconds','0','--sample-seconds',$duration,'--focus-policy','allow-unfocused')
            if ($backend -eq 'd3d12') { $arguments += @('--msaa','4','--validation','1') }
            $case = Run-Process $name $arguments 0
            $manifest.cases += $case
            if ($case.success) {
                $expected = if ($backend -eq 'd3d12') { 'D3D12' } else { 'OpenGL' }
                $facts = & $probe $directory $expected $workload 2> (Join-Path $output "$name.probe.stderr.log")
                $probeExit = $LASTEXITCODE
                $facts | Set-Content -LiteralPath (Join-Path $output "$name.probe.json") -Encoding utf8
                if ($probeExit -eq 0) {
                    $case.observation = ($facts -join "`n") | ConvertFrom-Json
                    $frames = @(Import-Csv -LiteralPath (Join-Path $directory 'frames.csv'))
                    $case.observation | Add-Member -NotePropertyName frame_rows -NotePropertyValue $frames.Count
                    if ($backend -eq 'd3d12') {
                        $ids = @($frames.renderer_frame_id)
                        if ($ids.Count -ne @($ids | Sort-Object -Unique).Count -or $ids -contains '') { throw 'Renderer frame IDs are not unique and explicit.' }
                        $resourceRows = @($frames | Where-Object { [uint64]$_.resource_upload_bytes -gt 0 })
                        if ($workload -ne 'edit' -and $resourceRows.Count -ne 0) { throw 'Unchanged world geometry was reuploaded.' }
                        if ($workload -eq 'edit' -and $resourceRows.Count -eq 0) { throw 'Real edits did not update cached geometry.' }
                        foreach ($frame in $frames) {
                            if ($frame.gpu_draw_ms -eq '' -and $frame.gpu_missing_reason -eq '') { throw 'Missing GPU duration has no explanation.' }
                            if ($frame.gpu_draw_ms -ne '' -and $frame.gpu_missing_reason -ne '') { throw 'Valid GPU duration is also marked missing.' }
                            if ([uint64]$frame.upload_calls -ne [uint64]$frame.rebuilt_chunks) { throw 'Cache operations differ from actual published mesh rebuilds.' }
                        }
                    }
                    if ($workload -eq 'walk') {
                        $positions = @($frames.x | Sort-Object -Unique)
                        if ($positions.Count -lt 2) { throw 'Walk workload did not move the actual player.' }
                    }
                    $case.success = $true
                } else { $case.success = $false }
            }
            Save-Manifest
            Write-Output "$name success=$($case.success) exit=$($case.exit_code)"
        }
    }
    foreach ($identity in @($manifest.source_identity) + @($manifest.input_identity)) {
        $current = Identity $identity.path
        if ($current.sha256 -ne $identity.sha256 -or $current.bytes -ne $identity.bytes) { throw "Input changed during verification: $($identity.path)" }
    }
    $manifest.success = @($manifest.cases | Where-Object { -not $_.success }).Count -eq 0
} catch {
    if ($case) { $case.success = $false }
    $manifest.failure = $_.Exception.Message
    throw
} finally {
    $manifest.finished_utc = [DateTime]::UtcNow.ToString('o')
    $manifest.artifacts = @(Get-ChildItem -LiteralPath $output -Recurse -File |
        Where-Object { $_.Name -ne 'verification.json' } | ForEach-Object { Identity $_.FullName })
    Save-Manifest
}
if (-not $manifest.success) { throw 'One or more real game verification cases failed; preserved in verification.json.' }

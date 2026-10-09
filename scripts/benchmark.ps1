[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [ValidateSet('static','walk','edit')][string[]]$Scenarios = @('static','walk','edit'),
    [ValidateRange(0,600)][int]$WarmupSeconds = 60,
    [ValidateRange(1,600)][int]$SampleSeconds = 180,
    [ValidateRange(1,10)][int]$Repeats = 3,
    [ValidateRange(320,8192)][int]$Width = 1920,
    [ValidateRange(320,8192)][int]$Height = 1080,
    [ValidateSet(0,1)][int]$VSync = 0,
    [uint32]$Seed = 424242,
    [string]$MachineLabel = 'unrecorded',
    [string]$PowerMode = 'unrecorded',
    [string]$GpuMode = 'unrecorded',
    [string]$ClockSettings = 'unrecorded',
    [switch]$SkipGpuSensors
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$exe = (Get-Item -LiteralPath $Executable).FullName
$provider = $null
$drive = $null
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputDirectory, [ref]$provider, [ref]$drive)
if ($provider.Name -ne 'FileSystem') { throw 'Benchmark output must use the FileSystem provider.' }
if (Test-Path -LiteralPath $output) { throw 'Choose a new output directory; previous runs are not overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$culture = [Globalization.CultureInfo]::InvariantCulture

function Number([string]$Value) { return [double]::Parse($Value, $culture) }
function Stats([double[]]$Values) {
    if ($Values.Count -eq 0) { return [ordered]@{ count = 0 } }
    [Array]::Sort($Values)
    $sum = 0.0
    foreach ($value in $Values) { $sum += $value }
    return [ordered]@{
        count = $Values.Count; mean = $sum / $Values.Count
        p95 = $Values[[int][math]::Ceiling($Values.Count * 0.95) - 1]
        p99 = $Values[[int][math]::Ceiling($Values.Count * 0.99) - 1]
        max = $Values[-1]
    }
}
function Try-Read([scriptblock]$Reader) {
    try { return & $Reader } catch { return "unavailable: $($_.Exception.Message)" }
}

$machine = [ordered]@{
    startedAt = (Get-Date).ToString('o')
    collectorScriptVersion = 2; powershellVersion = $PSVersionTable.PSVersion.ToString()
    label = $MachineLabel; powerMode = $PowerMode; gpuMode = $GpuMode; clockSettings = $ClockSettings
    cpuTemperature = 'missing; no CPU temperature sensor configured'
    operatingSystem = Try-Read { Get-CimInstance Win32_OperatingSystem | Select-Object Caption,Version,BuildNumber }
    cpu = Try-Read { Get-CimInstance Win32_Processor | Select-Object Name,NumberOfCores,NumberOfLogicalProcessors }
    memory = Try-Read { Get-CimInstance Win32_ComputerSystem | Select-Object TotalPhysicalMemory }
    battery = Try-Read { @(Get-CimInstance Win32_Battery | Select-Object BatteryStatus,EstimatedChargeRemaining) }
    activePowerScheme = Try-Read { (& powercfg /getactivescheme) -join "`n" }
    displayControllers = Try-Read { @(Get-CimInstance Win32_VideoController | Select-Object Name,DriverVersion) }
    executable = $exe; executableSha256 = (Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash
    scriptSha256 = (Get-FileHash -LiteralPath $PSCommandPath -Algorithm SHA256).Hash
    seed = $Seed; width = $Width; height = $Height; vsync = $VSync
    warmupSeconds = $WarmupSeconds; sampleSeconds = $SampleSeconds; repeats = $Repeats
    requestedScenarios = $Scenarios
    gpuSensorScope = 'nvidia-smi samples all GPUs; device-wide temperature/memory, not process VRAM'
    sourceNote = 'Keep build identity with this directory. This script does not infer the source commit from an executable.'
}
$machine | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'machine.json') -Encoding UTF8
$smi = if ($SkipGpuSensors) { $null } else { Get-Command nvidia-smi -ErrorAction SilentlyContinue }
$results = [Collections.Generic.List[object]]::new()

foreach ($scenario in $Scenarios) {
    for ($repeat = 1; $repeat -le $Repeats; $repeat++) {
        $run = Join-Path $output "$scenario-$repeat"
        New-Item -ItemType Directory -Path $run | Out-Null
        $stdout = Join-Path $run 'stdout.log'
        $stderr = Join-Path $run 'stderr.log'
        $arguments = @('--benchmark', $scenario, '--output', 'capture', '--seed', "$Seed", '--width', "$Width", '--height', "$Height",
            '--vsync', "$VSync", '--warmup-seconds', "$WarmupSeconds", '--sample-seconds', "$SampleSeconds")
        $monitor = $null
        $process = $null
        $timer = [Diagnostics.Stopwatch]::StartNew()
        $timedOut = $false
        try {
            if ($smi) {
                $monitor = Start-Process -FilePath $smi.Source -ArgumentList @(
                    '--query-gpu=timestamp,uuid,name,driver_version,temperature.gpu,power.draw,clocks.current.graphics,clocks.current.memory,memory.used,memory.total,utilization.gpu',
                    '--format=csv', '--loop-ms=1000') -WindowStyle Hidden -PassThru `
                    -RedirectStandardOutput (Join-Path $run 'gpu-sensors.csv') -RedirectStandardError (Join-Path $run 'gpu-sensors.stderr.log')
            }
            Write-Output "Running $scenario $repeat/$Repeats; $WarmupSeconds s warmup + $SampleSeconds s sampling. Keep the game focused; Esc cancels."
            $process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $run -WindowStyle Hidden -PassThru `
                -RedirectStandardOutput $stdout -RedirectStandardError $stderr
            # Retain the native handle before exit; Windows PowerShell 5.1 otherwise loses ExitCode.
            $null = $process.Handle
            if (-not $process.WaitForExit(($WarmupSeconds + $SampleSeconds + 120) * 1000)) {
                $timedOut = $true
                $process.Kill()
                $process.WaitForExit()
            }
            $process.WaitForExit()
            $timer.Stop()
            $process.Refresh()
            $exitCode = $process.ExitCode
            $text = [IO.File]::ReadAllText($stdout)
            $errorText = [IO.File]::ReadAllText($stderr)
            $failures = [Collections.Generic.List[string]]::new()
            if ($timedOut) { $failures.Add('process-timeout') }
            if ($null -eq $exitCode) { $failures.Add('process-exit-code-unavailable') }
            elseif ($exitCode -ne 0) { $failures.Add("process-exit-code-$exitCode") }
            if (-not $text.Contains('[performance] exported; valid_run=1')) { $failures.Add('valid-export-marker-missing') }
            if (-not $text.Contains('[runtime] shutdown complete; exit_code=0')) { $failures.Add('successful-shutdown-marker-missing') }
            if ($errorText.Contains('OpenGL diagnostic')) { $failures.Add('opengl-diagnostic') }
            $result = [ordered]@{
                scenario = $scenario; repeat = $repeat; arguments = $arguments; processId = $process.Id
                exitCode = $exitCode; timedOut = $timedOut; passed = $false; elapsedSeconds = $timer.Elapsed.TotalSeconds
                gpuSensorStatus = if (-not $monitor) { 'missing' } elseif ($monitor.HasExited) { 'collector-exited; inspect sensor stderr' } else { 'collected; inspect CSV for unsupported fields' }
            }
            $csvPath = Join-Path $run 'capture/frames.csv'
            if (Test-Path -LiteralPath $csvPath) {
                $all = @(Import-Csv -LiteralPath $csvPath)
                $sample = @($all | Where-Object phase -eq 'sample')
                $frameTimes = @($sample | ForEach-Object { Number $_.frame_ms })
                $result.frameMs = Stats $frameTimes
                $result.gpuDrawMs = Stats @($sample | Where-Object gpu_draw_ms -ne '' | ForEach-Object { Number $_.gpu_draw_ms })
                $result.uploadBytes = Stats @($sample | ForEach-Object { Number $_.upload_bytes })
                $result.uploadCpuMs = Stats @($sample | ForEach-Object { Number $_.upload_cpu_ms })
                $result.meshMs = Stats @($sample | ForEach-Object { Number $_.mesh_ms })
                $result.presentMs = Stats @($sample | ForEach-Object { Number $_.present_ms })
                $result.editFrames = @($sample | Where-Object { (Number $_.edits) -gt 0 }).Count
                $result.focusLostFrames = @($all | Where-Object focused -ne '1').Count
                $result.playerX = Stats @($sample | ForEach-Object { Number $_.x })
                $result.playerY = Stats @($sample | ForEach-Object { Number $_.y })
                if ($sample.Count -eq 0) { $failures.Add('no-sample-frames') }
                if ($result.focusLostFrames -gt 0) { $failures.Add("window-not-focused: $($result.focusLostFrames) frames") }
            } else { $failures.Add('frames-csv-missing') }
            $result.failureReasons = @($failures.ToArray())
            $result.passed = $failures.Count -eq 0
            $results.Add($result)
            $result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $run 'result.json') -Encoding UTF8
            ConvertTo-Json -InputObject @($results.ToArray()) -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'results.json') -Encoding UTF8
            if (-not $result.passed) { throw "Benchmark invalid or failed ($($result.failureReasons -join '; ')); inspect $run. Remaining runs were not started." }
        } finally {
            if ($process) {
                if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
                $process.Dispose()
            }
            if ($monitor) {
                if (-not $monitor.HasExited) { $monitor.Kill(); $monitor.WaitForExit() }
                $monitor.Dispose()
            }
        }
    }
}
Write-Output "Finished $($results.Count) runs. Raw data and machine conditions: $output"

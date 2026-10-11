[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$GameExe,
    [Parameter(Mandatory = $true)][string]$ValidatorExe,
    [Parameter(Mandatory = $true)][string]$BenchmarkCoreLibrary,
    [Parameter(Mandatory = $true)][string]$YamlLibrary,
    [Parameter(Mandatory = $true)][string]$Output,
    [string]$AssetsRoot,
    [string]$DeclaredPowerMode = 'unknown',
    [string]$DeclaredClockSettings = 'unknown',
    [ValidateRange(60, 600)][int]$TimeoutSeconds = 180
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$utf8 = New-Object Text.UTF8Encoding($false)
$culture = [Globalization.CultureInfo]::InvariantCulture
function Write-Json([string]$Path, $Value) { [IO.File]::WriteAllText($Path, (($Value | ConvertTo-Json -Depth 20) + "`n"), $script:utf8) }
function Sha([string]$Path) { return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }
function Existing-File([string]$Path) {
    if (-not [IO.Path]::IsPathRooted($Path)) { throw "Use an absolute path: $Path" }
    $full = [IO.Path]::GetFullPath($Path)
    if (-not (Test-Path -LiteralPath $full -PathType Leaf)) { throw "Missing input: $full" }
    return $full
}
function Safe-Cim([string]$Class, [string[]]$Properties) {
    try {
        $records = @(Get-CimInstance -ClassName $Class -ErrorAction Stop | Select-Object -Property $Properties)
        return [pscustomobject]@{
            status = if ($records.Count -gt 0) { 'observed' } else { 'empty' }
            query = 'root/cimv2:' + $Class; utc = [DateTime]::UtcNow.ToString('o')
            records = $records; error = $null
        }
    } catch {
        return [pscustomobject]@{
            status = 'unavailable'; query = 'root/cimv2:' + $Class; utc = [DateTime]::UtcNow.ToString('o')
            records = @(); error = $_.Exception.Message; error_id = $_.FullyQualifiedErrorId
        }
    }
}
function Active-PowerFact([string]$Root) {
    $record = [ordered]@{ status = 'unavailable'; query = 'powercfg /getactivescheme (read-only)'; process = $null; stdout_text = $null; stderr_text = $null; error = $null }
    try {
        $exe = Existing-File (Join-Path ([Environment]::GetFolderPath('System')) 'powercfg.exe')
        $record.process = Invoke-Captured $exe @('/getactivescheme') $Root (Join-Path $Root 'powercfg-stdout.log') (Join-Path $Root 'powercfg-stderr.log') 15
        $record.stdout_text = [string](Get-Content -LiteralPath $record.process.stdout -Raw)
        $record.stderr_text = [string](Get-Content -LiteralPath $record.process.stderr -Raw)
        if ($record.process.launched -and -not $record.process.timeout -and -not $record.process.collector_error -and $record.process.exit_code -eq 0) { $record.status = 'observed' }
        else { $record.error = 'Power-plan query failed; raw output and its actual exit/timeout are retained.' }
    } catch { $record.error = $_.Exception.Message }
    return $record
}
function Assert-NoReparse([string]$Path) {
    $current = $Path
    while ($current) {
        if (Test-Path -LiteralPath $current) {
            if (((Get-Item -LiteralPath $current -Force).Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) { throw "Reparse path is not allowed: $current" }
        }
        $parent = [IO.Directory]::GetParent($current)
        if ($null -eq $parent) { break }
        $current = $parent.FullName
    }
}
function Decimal-Value([string]$Text) {
    $value = 0.0
    if ([string]::IsNullOrWhiteSpace($Text) -or -not [double]::TryParse($Text, [Globalization.NumberStyles]::Float, $script:culture, [ref]$value) -or [double]::IsNaN($value) -or [double]::IsInfinity($value) -or $value -lt 0) {
        throw "Missing or invalid numeric CSV data: $Text"
    }
    return $value
}
function Memory-Statistics([string]$Path) {
    $rows = @(Import-Csv -LiteralPath $Path -Encoding UTF8)
    if ($rows.Count -eq 0) { throw 'Missing memory CSV rows.' }
    foreach ($column in @('elapsed_s', 'working_set_bytes', 'private_bytes')) {
        if ($null -eq $rows[0].PSObject.Properties[$column]) { throw "Missing memory CSV column: $column" }
    }
    $selected = New-Object 'System.Collections.Generic.List[object]'
    $problems = New-Object 'System.Collections.Generic.List[string]'
    $missing = [ordered]@{ elapsed_s = 0; working_set_bytes = 0; private_bytes = 0 }
    $previous = -1.0
    foreach ($row in $rows) {
        try { $elapsed = Decimal-Value $row.elapsed_s }
        catch { ++$missing.elapsed_s; $problems.Add($_.Exception.Message); continue }
        if ($elapsed -le $previous) { $problems.Add('Memory timestamps are not strictly ordered.') }
        $previous = $elapsed
        if ($elapsed -ge 10 -and $elapsed -lt 40) {
            $point = [ordered]@{ elapsed_s = $elapsed; working_set_bytes = $null; private_bytes = $null }
            foreach ($column in @('working_set_bytes', 'private_bytes')) {
                try {
                    $value = Decimal-Value $row.$column
                    if ($value -le 0 -or $value -ne [Math]::Truncate($value)) { throw "Invalid process memory bytes: $column" }
                    $point[$column] = $value
                } catch { ++$missing[$column]; $problems.Add($_.Exception.Message) }
            }
            $selected.Add([pscustomobject]$point)
        }
    }
    $first = $null; $last = $null; $maxGap = $null
    if ($selected.Count -gt 0) {
        $first = $selected[0].elapsed_s; $last = $selected[$selected.Count - 1].elapsed_s; $maxGap = 0.0
        for ($index = 1; $index -lt $selected.Count; ++$index) { $maxGap = [Math]::Max($maxGap, $selected[$index].elapsed_s - $selected[$index - 1].elapsed_s) }
    }
    if ($selected.Count -lt 24) { $problems.Add('Measured memory coverage has fewer than 24 periodic observations.') }
    if ($null -eq $first -or $first -gt 12 -or $last -lt 38) { $problems.Add('Measured memory observations do not cover both ends of the 10-to-40-second window.') }
    if ($null -ne $maxGap -and $maxGap -gt 2.5) { $problems.Add('Measured memory observation gap exceeds 2.5 seconds.') }
    $statistics = [ordered]@{
        window = 'elapsed_s >= 10 and < 40'; requested_window_seconds = 30; samples = $selected.Count
        first_elapsed_s = $first; last_elapsed_s = $last
        observed_span_seconds = if ($null -ne $first) { $last - $first } else { $null }
        max_observed_gap_seconds = $maxGap; invalid_or_missing_cells = $missing
        nominal_1s_window_rows = 30; nominal_row_shortfall = [Math]::Max(0, 30 - $selected.Count)
        coverage_requirements = 'at least 24 points, first <= 12s, last >= 38s, max gap <= 2.5s; collection validity only, not a resource budget'
        method = 'nearest-rank ceil(p*N)-1 over periodic memory CSV observations, not continuous peak detection'
        scope = 'process working set and private committed bytes; private bytes are not VRAM'
        passed = ($problems.Count -eq 0); problems = $problems.ToArray()
    }
    foreach ($column in @('working_set_bytes', 'private_bytes')) {
        if ($selected.Count -eq 0 -or $missing[$column] -ne 0) { $statistics[$column] = $null; continue }
        $values = @($selected | ForEach-Object { $_.$column } | Sort-Object)
        $statistics[$column] = [ordered]@{
            p50_bytes = $values[[int][Math]::Ceiling($values.Count * 0.50) - 1]
            p95_bytes = $values[[int][Math]::Ceiling($values.Count * 0.95) - 1]
            max_observed_bytes = $values[$values.Count - 1]
            first_bytes = $selected[0].$column; last_bytes = $selected[$selected.Count - 1].$column
        }
    }
    return $statistics
}
function Invoke-Captured([string]$Exe, [string[]]$Arguments, [string]$WorkingDirectory, [string]$StdoutPath, [string]$StderrPath, [int]$Limit) {
    foreach ($argument in $Arguments) { if ($argument.Contains('"') -or $argument.EndsWith('\')) { throw 'Unsupported quoted native argument.' } }
    $process = New-Object Diagnostics.Process
    $process.StartInfo = New-Object Diagnostics.ProcessStartInfo
    $process.StartInfo.FileName = $Exe
    $process.StartInfo.Arguments = (@($Arguments | ForEach-Object { '"' + $_ + '"' }) -join ' ')
    $process.StartInfo.WorkingDirectory = $WorkingDirectory
    $process.StartInfo.UseShellExecute = $false; $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true; $process.StartInfo.RedirectStandardError = $true
    $process.StartInfo.StandardOutputEncoding = $script:utf8; $process.StartInfo.StandardErrorEncoding = $script:utf8
    $process.StartInfo.EnvironmentVariables['SYMOCRAFT_PUBLISH_DIAGNOSTICS'] = '0'
    $started = [DateTime]::UtcNow.ToString('o'); $clock = [Diagnostics.Stopwatch]::StartNew()
    $launched = $false; $pidValue = $null; $exitCode = $null; $timeout = $false; $errorText = $null
    $stdout = ''; $stderr = ''; $outTask = $null; $errTask = $null
    try {
        if (-not $process.Start()) { throw 'Process.Start returned false.' }
        $launched = $true; $pidValue = $process.Id
        $outTask = $process.StandardOutput.ReadToEndAsync(); $errTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($Limit * 1000)) {
            $timeout = $true; $process.Kill()
            if (-not $process.WaitForExit(5000)) { throw 'Timed-out child did not exit after termination.' }
        }
        $exitCode = $process.ExitCode
    } catch { $errorText = $_.Exception.Message }
    finally {
        if ($launched) {
            try { if (-not $process.HasExited) { $process.Kill(); if (-not $process.WaitForExit(5000)) { throw 'Child remains running.' } } }
            catch { $errorText = [string]$errorText + '; cleanup: ' + $_.Exception.Message }
        }
        foreach ($capture in @([pscustomobject]@{ task = $outTask; stream = 'stdout' }, [pscustomobject]@{ task = $errTask; stream = 'stderr' })) {
            if ($null -eq $capture.task) { continue }
            try {
                if (-not $capture.task.Wait(5000)) { throw "Capture did not drain: $($capture.stream)" }
                if ($capture.stream -eq 'stdout') { $stdout = $capture.task.Result } else { $stderr = $capture.task.Result }
            } catch { $errorText = [string]$errorText + '; capture: ' + $_.Exception.Message }
        }
        $clock.Stop(); $process.Dispose()
    }
    $stdoutSha = $null; $stderrSha = $null
    try { [IO.File]::WriteAllText($StdoutPath, $stdout, $script:utf8); $stdoutSha = Sha $StdoutPath }
    catch { $errorText = [string]$errorText + '; stdout persistence: ' + $_.Exception.Message }
    try { [IO.File]::WriteAllText($StderrPath, $stderr, $script:utf8); $stderrSha = Sha $StderrPath }
    catch { $errorText = [string]$errorText + '; stderr persistence: ' + $_.Exception.Message }
    return [pscustomobject]@{
        executable = $Exe; arguments = $Arguments; working_directory = $WorkingDirectory
        started_utc = $started; ended_utc = [DateTime]::UtcNow.ToString('o'); elapsed_ms = $clock.ElapsedMilliseconds
        launched = $launched; pid = $pidValue; exit_code = $exitCode; timeout = $timeout; collector_error = $errorText
        stdout = $StdoutPath; stderr = $StderrPath; stdout_sha256 = $stdoutSha; stderr_sha256 = $stderrSha
        child_api_diagnostics = '0'; parent_environment_mutated = $false
    }
}

$game = Existing-File $GameExe; $validator = Existing-File $ValidatorExe
$core = Existing-File $BenchmarkCoreLibrary; $yaml = Existing-File $YamlLibrary
$gameDirectory = Split-Path -Parent $game
if ([string]::IsNullOrWhiteSpace($AssetsRoot)) { $AssetsRoot = Join-Path $gameDirectory 'assets' }
$assets = [IO.Path]::GetFullPath($AssetsRoot)
if (-not $assets.TrimEnd('\', '/').Equals([IO.Path]::GetFullPath((Join-Path $gameDirectory 'assets')).TrimEnd('\', '/'), [StringComparison]::OrdinalIgnoreCase)) { throw 'Only the actual game-directory assets root may be identified.' }
if (-not (Test-Path -LiteralPath $assets -PathType Container)) { throw "Missing assets directory: $assets" }
$out = [IO.Path]::GetFullPath($Output)
Assert-NoReparse $out; Assert-NoReparse $assets
if (Test-Path -LiteralPath $out) { throw "Use a new output directory; evidence cannot be overwritten: $out" }
$assetPrefix = $assets.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
$resources = @(Get-ChildItem -LiteralPath $assets -Recurse -File | Sort-Object FullName | ForEach-Object {
    Assert-NoReparse $_.FullName
    [pscustomobject]@{ path = $_.FullName.Substring($assetPrefix.Length).Replace('\', '/'); bytes = $_.Length; sha256 = Sha $_.FullName }
})
if ($resources.Count -eq 0) { throw 'No game resources were found.' }
$identities = @($game, $validator, $core, $yaml, $PSCommandPath, (Join-Path $PSScriptRoot 'verify-capture.cpp'), (Join-Path $PSScriptRoot 'CMakeLists.txt')) | ForEach-Object {
    [pscustomobject]@{ path = $_; sha256 = Sha $_ }
}
$importsPath = Join-Path (Split-Path -Parent $validator) 'production-imports.json'
if (-not (Test-Path -LiteralPath $importsPath -PathType Leaf)) { throw "Validator import identity is missing: $importsPath" }
$imports = Get-Content -LiteralPath $importsPath -Raw | ConvertFrom-Json
if ($imports.configuration -ne 'Release' -or $imports.benchmark_core.sha256 -ne (Sha $core) -or $imports.yaml.sha256 -ne (Sha $yaml)) { throw 'Validator import identities do not match the declared actual libraries.' }
[void][IO.Directory]::CreateDirectory($out)
$plan = @('static', 'walk', 'edit') | ForEach-Object { $scene = $_; 1..3 | ForEach-Object { [pscustomobject]@{ scene = $scene; repeat = $_; warmup_seconds = 10; sample_seconds = 30 } } }
$manifest = [ordered]@{
    purpose = 'GLFW frozen identity exploratory Q07 presampling; not formal same-source Q06 or A11 runner acceptance'
    started_utc = [DateTime]::UtcNow.ToString('o'); planned = @($plan); planned_count = 9
    focus_policy = 'allow-unfocused'; framebuffer = @(1920, 1080); vsync = 0; seed = 424242
    identities = @($identities); resources_root = $assets; resources = $resources
    validator_imports = $imports; validator_imports_sha256 = Sha $importsPath
    api_diagnostics = $false; status_polling = $false; retries = $false; sleep = $false
    user_declarations = [ordered]@{ power_mode = $DeclaredPowerMode; clock_settings = $DeclaredClockSettings; scope = 'Human declaration, separate from read-only observations; unknown remains unknown.' }
    powershell = $PSVersionTable.PSVersion.ToString(); machine = $null
}
Write-Json (Join-Path $out 'manifest.json') $manifest
$results = New-Object 'System.Collections.Generic.List[object]'
$failure = $null
try {
    $manifest.machine = [ordered]@{
        computer = Safe-Cim 'Win32_ComputerSystem' @('Manufacturer', 'Model', 'TotalPhysicalMemory')
        os = Safe-Cim 'Win32_OperatingSystem' @('Caption', 'Version', 'BuildNumber', 'OSArchitecture')
        cpu = Safe-Cim 'Win32_Processor' @('Name', 'NumberOfCores', 'NumberOfLogicalProcessors', 'MaxClockSpeed', 'CurrentClockSpeed')
        gpu = Safe-Cim 'Win32_VideoController' @('Name', 'DriverVersion', 'DriverDate', 'PNPDeviceID', 'AdapterRAM')
        active_power_plan = Active-PowerFact $out
        battery = Safe-Cim 'Win32_Battery' @('Name', 'BatteryStatus', 'EstimatedChargeRemaining')
        temperatures = 'not collected'; clock_settings_declared = $DeclaredClockSettings; power_mode_declared = $DeclaredPowerMode
        clock_scope = 'single coarse CIM CPU clock snapshot; not locked-clock proof or continuous CPU/GPU sensor sampling'
        note = 'Read-only snapshot; GPU selection is checked using actual GL metadata. AdapterRAM is not authoritative VRAM capacity; no battery does not prove an AC-line status.'
    }
    Write-Json (Join-Path $out 'manifest.json') $manifest
    foreach ($round in $plan) {
        $name = $round.scene + '-' + $round.repeat
        $case = Join-Path $out $name; [void][IO.Directory]::CreateDirectory($case)
        $capture = Join-Path $case 'capture'
        $result = [ordered]@{ scene = $round.scene; repeat = $round.repeat; directory = $case; game = $null; validation = $null; metrics = $null; memory = $null; runtime_logs = $null; passed = $false; error = $null; failure_kind = $null }
        $results.Add([pscustomobject]$result)
        $stage = 'input_identity'
        try {
            foreach ($identity in $identities) { if ((Sha $identity.path) -ne $identity.sha256) { throw "Identity changed before sampling: $($identity.path)" } }
            $arguments = @('--benchmark', $round.scene, '--output', $capture, '--seed', '424242', '--width', '1920', '--height', '1080', '--vsync', '0', '--warmup-seconds', '10', '--sample-seconds', '30', '--focus-policy', 'allow-unfocused')
            $stage = 'game_process_collection'
            $result.game = Invoke-Captured $game $arguments $gameDirectory (Join-Path $case 'stdout.log') (Join-Path $case 'stderr.log') $TimeoutSeconds
            if ($result.game.timeout -or $result.game.collector_error -or $null -eq $result.game.exit_code) { throw 'Game process timed out or could not be collected; inspect its raw logs.' }
            $metricsPath = Join-Path $case 'validated-metrics.csv'
            $stage = 'actual_capture_validation'
            $result.validation = Invoke-Captured $validator @($capture, $round.scene, [string]$round.repeat, [string]$result.game.exit_code, $metricsPath) $case (Join-Path $case 'validated-summary.yaml') (Join-Path $case 'validation-stderr.log') 60
            if ($result.validation.timeout -or $result.validation.collector_error -or $result.validation.exit_code -ne 0) { throw 'Actual capture failed existing runner validation or device/Release/MSAA checks; inspect validation-stderr.log.' }
            $metrics = @(Import-Csv -LiteralPath $metricsPath -Encoding UTF8)
            if ($metrics.Count -ne 1 -or $metrics[0].valid -ne 'true') { throw 'Missing validated capture metrics.' }
            $result.metrics = $metrics[0]
            $stage = 'memory_data_quality'
            $result.memory = Memory-Statistics (Join-Path $capture 'memory.csv')
            if (-not $result.memory.passed) { throw 'Measured-window memory coverage or values are incomplete; inspect the retained coverage diagnostics.' }
            $stage = 'runtime_log_validation'
            $stdoutText = [string](Get-Content -LiteralPath $result.game.stdout -Raw)
            $stderrText = [string](Get-Content -LiteralPath $result.game.stderr -Raw)
            $logLines = [regex]::Split($stdoutText + "`n" + $stderrText, '\r?\n')
            $result.runtime_logs = [ordered]@{
                shutdown_success_marker = $stdoutText.Contains('[runtime] shutdown complete; exit_code=0')
                opengl_diagnostics = @($logLines | Where-Object { $_.Contains('OpenGL diagnostic [') })
                cleanup_failures = @($logLines | Where-Object { $_.Contains('[cleanup]') -or $_.Contains('[runtime] shutdown failed:') })
                stderr_nonempty = -not [string]::IsNullOrWhiteSpace($stderrText)
                note = 'Checks emitted logs only. Release may not enable the GL debug callback; empty stderr does not establish complete driver-error coverage.'
            }
            if (-not $result.runtime_logs.shutdown_success_marker -or $result.runtime_logs.opengl_diagnostics.Count -ne 0 -or $result.runtime_logs.cleanup_failures.Count -ne 0) { throw 'Game shutdown marker is missing or OpenGL/cleanup diagnostics were emitted.' }
            $result.passed = $true
        } catch { $result.error = $_.Exception.Message; $result.failure_kind = $stage; $failure = $result.error }
        $results[$results.Count - 1] = [pscustomobject]$result
        Write-Json (Join-Path $case 'result.json') $result
        [IO.File]::AppendAllText((Join-Path $out 'runs.jsonl'), (($result | ConvertTo-Json -Depth 20 -Compress) + "`n"), $utf8)
        Write-Output "$name passed=$($result.passed)"
        if (-not $result.passed) { break }
    }
    foreach ($identity in $identities) { if ((Sha $identity.path) -ne $identity.sha256) { throw "Input identity changed: $($identity.path)" } }
    foreach ($resource in $resources) { if ((Sha (Join-Path $assets $resource.path)) -ne $resource.sha256) { throw "Resource changed: $($resource.path)" } }
} catch { $failure = $_.Exception.Message }
$summary = [ordered]@{
    purpose = $manifest.purpose; ended_utc = [DateTime]::UtcNow.ToString('o'); manifest = 'manifest.json'; records = 'runs.jsonl'
    planned_count = 9; attempted_count = $results.Count
    actual_game_processes = @($results | Where-Object { $null -ne $_.game -and $_.game.launched }).Count
    passed_count = @($results | Where-Object { $_.passed }).Count; failure = $failure
    rounds = $results.ToArray(); passed = ($null -eq $failure -and $results.Count -eq 9 -and @($results | Where-Object { -not $_.passed }).Count -eq 0)
    sample_window_seconds = 30; memory_scope = 'Periodic sample-window process memory observations, not instantaneous peaks or process VRAM'
    q06_comparison_complete = $false; q07_budget_approved = $false; a11_accepted = $false; fix1_resumed = $false
}
Write-Json (Join-Path $out 'summary.json') $summary
Write-Output "GLFW exploratory presampling: $($summary.passed_count)/9 validated rounds; $out\summary.json"
if (-not $summary.passed) { exit 1 }

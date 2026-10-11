[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BuildDir,
    [Parameter(Mandatory = $true)][ValidateSet('Debug', 'Release')][string]$Config,
    [ValidateSet('foundation.files', 'performance.export', 'both')][string]$Test = 'both',
    [string]$FilesExe,
    [string]$PerfExe,
    [Parameter(Mandatory = $true)][string]$Output,
    [ValidateRange(1, 20)][int]$RunCount = 20,
    [ValidateRange(1, 3600)][int]$TimeoutSeconds = 60,
    [switch]$ApiDiagnostics,
    [switch]$NoApiDiagnostics,
    [switch]$StopOnUnexpectedFailure
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest
$utf8 = New-Object System.Text.UTF8Encoding($false)
$repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$apiEnabled = -not $NoApiDiagnostics.IsPresent
if ($ApiDiagnostics -and $NoApiDiagnostics) { throw 'Choose only one API diagnostics option.' }

function Write-Text([string]$Path, [string]$Value) {
    [IO.File]::WriteAllText($Path, $Value, $script:utf8)
}

function Write-Json([string]$Path, $Value) {
    Write-Text $Path (($Value | ConvertTo-Json -Depth 30) + [Environment]::NewLine)
}

function Get-Property($Value, [string]$Name) {
    if ($null -eq $Value) { return $null }
    $property = $Value.PSObject.Properties[$Name]
    if ($null -eq $property) { return $null }
    return $property.Value
}

function Get-Sha([string]$Path) { return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() }

function Test-NumericCode($Value) {
    if ($null -eq $Value -or $Value -is [string] -or $Value -is [bool]) { return $false }
    try {
        $number = [decimal]$Value
        return ($number -ge 0 -and $number -le [uint32]::MaxValue -and $number -eq [Math]::Truncate($number))
    } catch { return $false }
}

function Get-Relative([string]$Root, [string]$Path) {
    $prefix = $Root.TrimEnd('\', '/') + [IO.Path]::DirectorySeparatorChar
    if (-not $Path.StartsWith($prefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Path is outside its declared parent: $Path"
    }
    return $Path.Substring($prefix.Length).Replace('\', '/')
}

function Assert-NoReparseAncestor([string]$Path) {
    $current = $Path
    while ($current) {
        if (Test-Path -LiteralPath $current) {
            $entry = Get-Item -LiteralPath $current -Force
            if (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0) {
                throw "Reparse output ancestor is not allowed: $current"
            }
        }
        $parent = [IO.Directory]::GetParent($current)
        if ($null -eq $parent) { break }
        $current = $parent.FullName
    }
}

function Get-SourceIdentity {
    $paths = @(
        'game/modules/foundation/src/files.cpp',
        'game/modules/foundation/include/symocraft/foundation/files.h',
        'game/modules/telemetry/src/performance.cpp',
        'game/modules/telemetry/include/symocraft/telemetry/performance.h',
        'test/unit/foundation/files_tests.cpp',
        'test/unit/foundation/CMakeLists.txt',
        'test/unit/telemetry/performance_tests.cpp',
        'test/unit/telemetry/CMakeLists.txt',
        'test/support/publication_diagnostics.h',
        'test/experimental/publication-fix1/collect-runs.ps1'
    )
    $records = @($paths | Sort-Object | ForEach-Object {
        $path = Join-Path $script:repo $_
        if (-not (Test-Path -LiteralPath $path -PathType Leaf)) {
            throw "Required source identity input is missing: $path"
        }
        [pscustomobject]@{ path = $_; sha256 = Get-Sha $path; bytes = (Get-Item -LiteralPath $path).Length }
    })
    $canonical = ($records | ForEach-Object { $_.path + ':' + $_.sha256 }) -join "`n"
    $hasher = [Security.Cryptography.SHA256]::Create()
    try { $digest = [BitConverter]::ToString($hasher.ComputeHash($script:utf8.GetBytes($canonical))).Replace('-', '').ToLowerInvariant() }
    finally { $hasher.Dispose() }
    return [pscustomobject]@{ method = 'sorted-relative-path:sha256, UTF-8, LF, no-final-LF'; sha256 = $digest; files = $records }
}

function Get-Scene([string]$Root) {
    $records = New-Object 'System.Collections.Generic.List[object]'
    try {
        foreach ($entry in Get-ChildItem -LiteralPath $Root -Recurse -Force) {
            $item = [ordered]@{
                path = Get-Relative $Root $entry.FullName
                is_directory = $entry.PSIsContainer
                attributes = [int]$entry.Attributes
                reparse = (($entry.Attributes -band [IO.FileAttributes]::ReparsePoint) -ne 0)
                bytes = $null
                sha256 = $null
                query_error = $null
            }
            if (-not $entry.PSIsContainer -and -not $item.reparse) {
                try { $item.bytes = $entry.Length; $item.sha256 = Get-Sha $entry.FullName }
                catch { $item.query_error = $_.Exception.Message }
            }
            $records.Add([pscustomobject]$item)
        }
    } catch {
        $records.Add([pscustomobject]@{ path = '.'; query_error = $_.Exception.Message })
    }
    return $records.ToArray()
}

function Read-Diagnostics([string]$Stdout, [string]$Stderr, [string]$Suite, [string]$RunId, [int]$PidValue, [bool]$ApiEnabled, [string]$SourceSha, [string]$ExeSha) {
    $testEvents = New-Object 'System.Collections.Generic.List[object]'
    $apiEvents = New-Object 'System.Collections.Generic.List[object]'
    $problems = New-Object 'System.Collections.Generic.List[string]'
    $otherJson = New-Object 'System.Collections.Generic.List[object]'
    foreach ($stream in @([pscustomobject]@{ name = 'stdout'; text = $Stdout }, [pscustomobject]@{ name = 'stderr'; text = $Stderr })) {
        $lineNumber = 0
        $current = @{}
        foreach ($line in [regex]::Split($stream.text, '\r?\n')) {
            ++$lineNumber
            $trimmed = $line.Trim()
            if (-not $trimmed.StartsWith('{')) { continue }
            try { $value = $trimmed | ConvertFrom-Json }
            catch { $problems.Add("Invalid JSON at $($stream.name):$lineNumber"); continue }
            $kind = Get-Property $value 'kind'
            $envelope = [ordered]@{ stream = $stream.name; line = $lineNumber; record = $value }
            if ($kind -eq 'publication_test') {
                foreach ($required in @('suite', 'run_id', 'pid', 'tid', 'event', 'phase', 'expected', 'outcome', 'utc_filetime')) {
                    if ($null -eq (Get-Property $value $required)) { $problems.Add("Missing publication_test.$required at $($stream.name):$lineNumber") }
                }
                if ((Get-Property $value 'suite') -ne $Suite) { $problems.Add('Publication suite identity differs from invocation.') }
                if ((Get-Property $value 'run_id') -ne $RunId) { $problems.Add('Publication run identity differs from invocation.') }
                if ((Get-Property $value 'pid') -ne $PidValue) { $problems.Add('Publication PID differs from launched process.') }
                if ((Get-Property $value 'config') -ne $script:Config) { $problems.Add('Publication configuration differs from invocation.') }
                if ((Get-Property $value 'source_sha') -ne $SourceSha) { $problems.Add('Publication source identity differs from invocation.') }
                if ((Get-Property $value 'exe_sha') -ne $ExeSha) { $problems.Add('Publication executable identity differs from invocation.') }
                $failureClass = Get-Property $value 'failure_class'
                $numeric = Get-Property $value 'numeric_win32'
                if ($failureClass -eq 'win32_system_error' -or (Get-Property $value 'error_category') -eq 'system' -or (Get-Property $value 'event') -eq 'expected_rejection') {
                    if (-not (Test-NumericCode $numeric)) {
                        $problems.Add("Missing/invalid raw numeric Win32 code at $($stream.name):$lineNumber")
                    }
                    if ([string]::IsNullOrEmpty([string](Get-Property $value 'error_category'))) {
                        $problems.Add("Missing error category at $($stream.name):$lineNumber")
                    }
                }
                $key = [string](Get-Property $value 'pid') + ':' + [string](Get-Property $value 'tid')
                if ((Get-Property $value 'event') -eq 'begin') { $current[$key] = $value }
                $testEvents.Add([pscustomobject]$envelope)
                if ((Get-Property $value 'event') -in @('success', 'expected_rejection', 'expected_standard_rejection', 'unexpected_failure')) { $current.Remove($key) }
            } elseif ($kind -eq 'publish_api') {
                foreach ($required in @('api', 'flags', 'win32', 'success', 'pid', 'tid', 'utc_filetime', 'sequence')) {
                    if ($null -eq (Get-Property $value $required)) { $problems.Add("Missing publish_api.$required at $($stream.name):$lineNumber") }
                }
                $numeric = Get-Property $value 'win32'
                if (-not (Test-NumericCode $numeric)) {
                    $problems.Add("Invalid publish_api.win32 at $($stream.name):$lineNumber")
                }
                if ((Get-Property $value 'success') -isnot [bool] -or ((Get-Property $value 'success') -eq $true) -ne ($numeric -eq 0)) {
                    $problems.Add("API success/code fields disagree at $($stream.name):$lineNumber")
                }
                if ((Get-Property $value 'pid') -ne $PidValue) { $problems.Add('API PID differs from launched process.') }
                if ((Get-Property $value 'api') -notin @('ReplaceFileW', 'MoveFileExW')) { $problems.Add('Unexpected publication API name.') }
                $key = [string](Get-Property $value 'pid') + ':' + [string](Get-Property $value 'tid')
                $envelope.context = if ($current.ContainsKey($key)) { $current[$key] } else { $null }
                $envelope.context_association = if ($current.ContainsKey($key)) { 'same-stream preceding begin, same PID/TID' } else { 'unassociated' }
                if (-not $current.ContainsKey($key)) { $problems.Add("API event has no active same-stream test context at $($stream.name):$lineNumber") }
                $apiEvents.Add([pscustomobject]$envelope)
            } else {
                if ($kind -eq 'diagnostic_failure') { $problems.Add("Test reported a diagnostic capture failure at $($stream.name):$lineNumber") }
                $otherJson.Add([pscustomobject]$envelope)
            }
        }
    }
    if ($testEvents.Count -eq 0) { $problems.Add('No structured publication test diagnostics were emitted; empty stderr is retained, not parsed as a code.') }
    $completions = @($testEvents | Where-Object { (Get-Property $_.record 'event') -eq 'suite_success' })
    $failures = @($testEvents | Where-Object { (Get-Property $_.record 'event') -in @('unexpected_failure', 'outer_failure') })
    if ($failures.Count -eq 0 -and $completions.Count -ne 1) { $problems.Add('Successful process must emit exactly one suite_success event.') }
    if ($ApiEnabled -and $apiEvents.Count -eq 0) { $problems.Add('API diagnostics were enabled but no actual API events were emitted.') }
    if (-not $ApiEnabled -and $apiEvents.Count -gt 0) { $problems.Add('API events appeared in an explicitly untraced invocation.') }
    $begins = @($testEvents | Where-Object { (Get-Property $_.record 'event') -eq 'begin' })
    $rejections = @($testEvents | Where-Object { (Get-Property $_.record 'event') -eq 'expected_rejection' })
    $standardRejections = @($testEvents | Where-Object { (Get-Property $_.record 'event') -eq 'expected_standard_rejection' })
    if ($completions.Count -eq 1) {
        foreach ($counter in @(
            [pscustomobject]@{ name = 'operation_events'; actual = $begins.Count },
            [pscustomobject]@{ name = 'expected_rejections'; actual = $rejections.Count },
            [pscustomobject]@{ name = 'expected_standard_rejections'; actual = $standardRejections.Count }
        )) {
            if ((Get-Property $completions[0].record $counter.name) -ne $counter.actual) {
                $problems.Add("Suite completion $($counter.name) disagrees with actual structured records.")
            }
        }
    }
    return [pscustomobject]@{
        status = if ($problems.Count -eq 0) { 'complete' } else { 'incomplete' }
        problems = $problems.ToArray()
        test_events = $testEvents.ToArray()
        api_events = $apiEvents.ToArray()
        other_json = $otherJson.ToArray()
        unexpected_events = $failures.Count
        completed_suites = $completions.Count
        api_calls = if ($ApiEnabled) { $apiEvents.Count } else { $null }
        api_calls_observed = $apiEvents.Count
        api_successes = @($apiEvents | Where-Object { (Get-Property $_.record 'success') -eq $true }).Count
        expected_rejections = $rejections.Count
        expected_standard_rejections = $standardRejections.Count
        test_operation_successes = @($testEvents | Where-Object { (Get-Property $_.record 'event') -eq 'success' }).Count
        operation_events = $begins.Count
    }
}

function Invoke-IndependentRun([string]$Suite, [string]$Exe, [string]$ExeSha, [string]$Root, [string]$RunId, [string]$SourceSha) {
    $caseParent = Join-Path $Root 'case-parent'
    [void][IO.Directory]::CreateDirectory($Root)
    [void][IO.Directory]::CreateDirectory($caseParent)
    if ($caseParent.Contains('"')) { throw 'A native argument cannot contain a quote.' }
    $startedUtc = [DateTime]::UtcNow.ToString('o')
    $stopwatch = [Diagnostics.Stopwatch]::StartNew()
    $process = New-Object Diagnostics.Process
    $process.StartInfo = New-Object Diagnostics.ProcessStartInfo
    $process.StartInfo.FileName = $Exe
    $process.StartInfo.Arguments = '"' + $caseParent + '"'
    $process.StartInfo.WorkingDirectory = $script:build
    $process.StartInfo.UseShellExecute = $false
    $process.StartInfo.CreateNoWindow = $true
    $process.StartInfo.RedirectStandardOutput = $true
    $process.StartInfo.RedirectStandardError = $true
    $process.StartInfo.StandardOutputEncoding = $script:utf8
    $process.StartInfo.StandardErrorEncoding = $script:utf8
    $environment = [ordered]@{
        SYMOCRAFT_FIX1_RUN_ID = $RunId
        SYMOCRAFT_FIX1_CONFIG = $script:Config
        SYMOCRAFT_FIX1_SOURCE_SHA = $SourceSha
        SYMOCRAFT_FIX1_EXE_SHA = $ExeSha
        SYMOCRAFT_PUBLISH_DIAGNOSTICS = if ($script:apiEnabled) { '1' } else { '0' }
    }
    foreach ($key in $environment.Keys) { $process.StartInfo.EnvironmentVariables[$key] = $environment[$key] }
    $stdout = ''; $stderr = ''; $exitCode = $null; $pidValue = 0; $timedOut = $false
    $collectorErrors = New-Object 'System.Collections.Generic.List[string]'
    $outTask = $null; $errTask = $null; $launched = $false
    try {
        if (-not $process.Start()) { throw 'Process.Start returned false.' }
        $launched = $true; $pidValue = $process.Id
        $outTask = $process.StandardOutput.ReadToEndAsync()
        $errTask = $process.StandardError.ReadToEndAsync()
        if (-not $process.WaitForExit($script:TimeoutSeconds * 1000)) {
            $timedOut = $true
            $process.Kill()
            if (-not $process.WaitForExit(5000)) { $collectorErrors.Add('Timed-out child did not exit after termination request.') }
        }
        if ($process.HasExited) { $exitCode = $process.ExitCode }
    } catch { $collectorErrors.Add($_.Exception.Message) }
    finally {
        if ($launched) {
            try {
                if (-not $process.HasExited) {
                    $process.Kill()
                    if (-not $process.WaitForExit(5000)) { $collectorErrors.Add('Child remains running after collector cleanup.') }
                }
            } catch { $collectorErrors.Add('Process cleanup failed: ' + $_.Exception.Message) }
        }
        foreach ($capture in @([pscustomobject]@{ task = $outTask; name = 'stdout' }, [pscustomobject]@{ task = $errTask; name = 'stderr' })) {
            if ($null -eq $capture.task) { continue }
            try {
                if (-not $capture.task.Wait(5000)) { throw "$($capture.name) capture did not complete." }
                if ($capture.name -eq 'stdout') { $stdout = $capture.task.Result } else { $stderr = $capture.task.Result }
            } catch { $collectorErrors.Add($_.Exception.Message) }
        }
        $stopwatch.Stop()
        $process.Dispose()
    }
    $stdoutSha = $null; $stderrSha = $null
    foreach ($capture in @([pscustomobject]@{ name = 'stdout'; text = $stdout }, [pscustomobject]@{ name = 'stderr'; text = $stderr })) {
        try {
            $logPath = Join-Path $Root ($capture.name + '.log')
            Write-Text $logPath $capture.text
            if ($capture.name -eq 'stdout') { $stdoutSha = Get-Sha $logPath } else { $stderrSha = Get-Sha $logPath }
        } catch { $collectorErrors.Add('Raw log persistence failed: ' + $_.Exception.Message) }
    }
    try { $diagnostics = Read-Diagnostics $stdout $stderr $Suite $RunId $pidValue $script:apiEnabled $SourceSha $ExeSha }
    catch {
        $collectorErrors.Add('Diagnostic parsing failed: ' + $_.Exception.Message)
        $diagnostics = [pscustomobject]@{
            status = 'incomplete'; problems = @('Collector could not classify the retained raw logs.')
            test_events = @(); api_events = @(); other_json = @(); unexpected_events = 0
            completed_suites = 0; api_calls = $null; api_calls_observed = 0
            api_successes = 0; expected_rejections = 0; expected_standard_rejections = 0; test_operation_successes = 0; operation_events = 0
        }
    }
    try { if ((Get-Sha $Exe) -ne $ExeSha) { $collectorErrors.Add('Executable identity changed during this run.') } }
    catch { $collectorErrors.Add('Executable identity query failed: ' + $_.Exception.Message) }
    $product = if (-not $launched -or $timedOut -or $null -eq $exitCode) { 'not_completed' }
        elseif ($exitCode -ne 0 -or $diagnostics.unexpected_events -gt 0) { 'fail' } else { 'pass' }
    $result = [ordered]@{
        run_id = $RunId; suite = $Suite; configuration = $script:Config
        started_utc = $startedUtc; ended_utc = [DateTime]::UtcNow.ToString('o')
        pid = $pidValue; elapsed_ms = $stopwatch.ElapsedMilliseconds
        executable = $Exe; executable_sha256 = $ExeSha; source_sha256 = $SourceSha
        working_directory = $script:build; arguments = @($caseParent); child_environment_overrides = $environment
        timeout_seconds = $script:TimeoutSeconds; timeout = $timedOut; launched = $launched; exit_code = $exitCode
        retry_count = 0; sleeps = 0; procmon_enabled_by_collector = $false
        product_status = $product; diagnostic_status = $diagnostics.status
        collector_status = if ($collectorErrors.Count -eq 0) { 'pass' } else { 'fail' }
        collector_errors = $collectorErrors.ToArray(); diagnostics = $diagnostics
        stdout_sha256 = $stdoutSha; stderr_sha256 = $stderrSha
        original_case_parent = $caseParent; scene = @(Get-Scene $caseParent)
        passed = ($product -eq 'pass' -and $diagnostics.status -eq 'complete' -and $collectorErrors.Count -eq 0)
    }
    try { Write-Json (Join-Path $Root 'result.json') $result }
    catch {
        $collectorErrors.Add('Run record persistence failed: ' + $_.Exception.Message)
        $result.collector_errors = $collectorErrors.ToArray()
        $result.collector_status = 'fail'; $result.passed = $false
    }
    return [pscustomobject]$result
}

$build = [IO.Path]::GetFullPath($BuildDir)
if (-not (Test-Path -LiteralPath $build -PathType Container)) { throw "Build directory is missing: $build" }
$out = [IO.Path]::GetFullPath($Output)
Assert-NoReparseAncestor $out
if (Test-Path -LiteralPath $out) { throw "Output must be a new path; no existing evidence will be overwritten: $out" }
$selected = New-Object 'System.Collections.Generic.List[object]'
foreach ($selection in @([pscustomobject]@{ suite = 'foundation.files'; path = $FilesExe }, [pscustomobject]@{ suite = 'performance.export'; path = $PerfExe })) {
    if ($Test -ne 'both' -and $Test -ne $selection.suite) { continue }
    if ([string]::IsNullOrWhiteSpace($selection.path)) { throw "Provide the exact executable path for $($selection.suite)." }
    $exe = [IO.Path]::GetFullPath($selection.path)
    if (-not (Test-Path -LiteralPath $exe -PathType Leaf)) { throw "Executable is missing: $exe" }
    $selected.Add([pscustomobject]@{ suite = $selection.suite; path = $exe; sha256 = Get-Sha $exe })
}
$source = Get-SourceIdentity
$libraryInputs = @(Get-ChildItem -LiteralPath $build -Filter '*.lib' -Recurse -File | Where-Object { $_.BaseName -match 'foundation|telemetry' } | Sort-Object FullName | ForEach-Object {
    [pscustomobject]@{ path = $_.FullName; sha256 = Get-Sha $_.FullName; bytes = $_.Length }
})
$cache = Join-Path $build 'CMakeCache.txt'
[void][IO.Directory]::CreateDirectory($out)
$manifest = [ordered]@{
    schema_version = 1; purpose = 'Fix1 bounded independent diagnostic batch, not V05 final acceptance'
    created_utc = [DateTime]::UtcNow.ToString('o'); repository = $repo; output = $out
    build_directory = $build; configuration = $Config; suites = @($selected.ToArray())
    planned_processes_per_suite = $RunCount; planned_processes = $RunCount * $selected.Count
    source_identity = $source; library_inputs = $libraryInputs
    cmake_cache_sha256 = if (Test-Path -LiteralPath $cache -PathType Leaf) { Get-Sha $cache } else { $null }
    collector = [ordered]@{ path = $PSCommandPath; sha256 = Get-Sha $PSCommandPath; powershell = $PSVersionTable.PSVersion.ToString(); process_model = 'one child per case parent, asynchronous stdout/stderr capture' }
    api_diagnostics = $apiEnabled; timeout_seconds = $TimeoutSeconds
    stop_on_unexpected_failure = $StopOnUnexpectedFailure.IsPresent
    external_trace = 'not started by this collector; record external trace identity separately'
    parent_environment_mutated = $false; retries = $false; sleep = $false
}
Write-Json (Join-Path $out 'manifest.json') $manifest
$results = New-Object 'System.Collections.Generic.List[object]'
$batchErrors = New-Object 'System.Collections.Generic.List[string]'
$batchRunId = [Guid]::NewGuid().ToString('N')
foreach ($selection in $selected) {
    for ($index = 1; $index -le $RunCount; ++$index) {
        $caseName = $selection.suite.Replace('.', '-') + '-' + $index.ToString('000')
        $runRoot = Join-Path $out $caseName
        try {
            $runId = $batchRunId + '-' + $caseName
            $result = Invoke-IndependentRun $selection.suite $selection.path $selection.sha256 $runRoot $runId $source.sha256
            $results.Add($result)
            [IO.File]::AppendAllText((Join-Path $out 'runs.jsonl'), (($result | ConvertTo-Json -Depth 30 -Compress) + "`n"), $utf8)
            Write-Output "$caseName product=$($result.product_status) diagnostic=$($result.diagnostic_status) collector=$($result.collector_status)"
            if ($result.collector_status -eq 'fail' -or ($StopOnUnexpectedFailure -and -not $result.passed)) { break }
        } catch {
            $failure = [ordered]@{ kind = 'collector_failure'; suite = $selection.suite; index = $index; run_directory = $runRoot; utc = [DateTime]::UtcNow.ToString('o'); message = $_.Exception.Message }
            $batchErrors.Add($_.Exception.Message)
            Write-Json (Join-Path $out ($caseName + '-collector-failure.json')) $failure
            break
        }
    }
}
$sourceAfter = $null
try {
    $sourceAfter = Get-SourceIdentity
    if ($sourceAfter.sha256 -ne $source.sha256) { $batchErrors.Add('Source identity changed during this batch; it is not a fixed-source regression result.') }
} catch { $batchErrors.Add('Final source identity query failed: ' + $_.Exception.Message) }
foreach ($input in $libraryInputs) {
    try {
        if (-not (Test-Path -LiteralPath $input.path -PathType Leaf) -or (Get-Sha $input.path) -ne $input.sha256) { $batchErrors.Add('Linked library input identity changed during batch: ' + $input.path) }
    } catch { $batchErrors.Add('Final linked library identity query failed: ' + $_.Exception.Message) }
}
$suiteSummary = @($selected | ForEach-Object {
    $suite = $_.suite
    $runs = @($results | Where-Object { $_.suite -eq $suite })
    [pscustomobject]@{
        suite = $suite; planned = $RunCount; actual = $runs.Count
        product_pass = @($runs | Where-Object { $_.product_status -eq 'pass' }).Count
        product_fail = @($runs | Where-Object { $_.product_status -eq 'fail' }).Count
        not_completed = @($runs | Where-Object { $_.product_status -eq 'not_completed' }).Count
        diagnostic_incomplete = @($runs | Where-Object { $_.diagnostic_status -ne 'complete' }).Count
        collector_failure = @($runs | Where-Object { $_.collector_status -ne 'pass' }).Count
        api_calls = if ($script:apiEnabled) { ($runs | ForEach-Object { $_.diagnostics.api_calls } | Measure-Object -Sum).Sum } else { $null }
        api_calls_observed = ($runs | ForEach-Object { $_.diagnostics.api_calls_observed } | Measure-Object -Sum).Sum
        expected_rejections = ($runs | ForEach-Object { $_.diagnostics.expected_rejections } | Measure-Object -Sum).Sum
        expected_standard_rejections = ($runs | ForEach-Object { $_.diagnostics.expected_standard_rejections } | Measure-Object -Sum).Sum
        operation_events = ($runs | ForEach-Object { $_.diagnostics.operation_events } | Measure-Object -Sum).Sum
    }
})
$summary = [ordered]@{
    schema_version = 1; ended_utc = [DateTime]::UtcNow.ToString('o'); manifest = 'manifest.json'; run_records = 'runs.jsonl'
    planned_processes = $RunCount * $selected.Count; actual_processes = @($results | Where-Object { $_.launched }).Count
    recorded_attempts = $results.Count; suites = $suiteSummary; collector_errors = $batchErrors.ToArray()
    source_identity_unchanged = ($null -ne $sourceAfter -and $sourceAfter.sha256 -eq $source.sha256)
    early_stop = ($results.Count -ne $RunCount * $selected.Count)
    passed = ($batchErrors.Count -eq 0 -and $results.Count -eq $RunCount * $selected.Count -and @($results | Where-Object { -not $_.passed }).Count -eq 0)
    root_cause_confirmed = $false; fix_confirmed = $false; v05_acceptance = $false; issue_closed = $false
    interpretation = 'Expected API rejections and Replace-not-found/Move-success are not unexpected product failures. No localized message is parsed for an error code. Raw case directories remain in place.'
}
Write-Json (Join-Path $out 'summary.json') $summary
Write-Output "Recorded $($summary.actual_processes)/$($summary.planned_processes) independent processes; summary=$out\summary.json"
if (-not $summary.passed) { exit 1 }

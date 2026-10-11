[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Executable,
    [Parameter(Mandatory = $true)][string]$OutputDirectory,
    [string]$PackageManifest,
    [switch]$RunTimeoutProbe,
    [ValidateRange(5, 600)][int]$TimeoutSeconds = 60
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
function Read-OptionalMachineMetadata {
    param([string]$ClassName, [string[]]$Properties)
    try {
        $values = Get-CimInstance -ClassName $ClassName -ErrorAction Stop
        return [pscustomobject]@{data=@($values | Select-Object -Property $Properties); error=$null}
    } catch {
        return [pscustomobject]@{
            data=@()
            error=[pscustomobject]@{class_name=$ClassName; message=$_.Exception.Message; error_id=$_.FullyQualifiedErrorId}
        }
    }
}
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
if ($PackageManifest) {
    $manifestPath = if (Test-Path -LiteralPath $PackageManifest -PathType Leaf) { $PackageManifest } else {
        Join-Path $PSScriptRoot $PackageManifest
    }
    $manifestPath = (Get-Item -LiteralPath $manifestPath -ErrorAction Stop).FullName
    $root = Split-Path $manifestPath
    $outRoot = Join-Path $root 'evidence'
    if (-not [IO.Path]::IsPathRooted($Executable)) { $Executable = Join-Path $root $Executable }
}
$executablePath = (Get-Item -LiteralPath $Executable -ErrorAction Stop).FullName
$build = Split-Path $executablePath
$imports = Get-Content -LiteralPath (Join-Path $build 'production-imports.json') -Raw | ConvertFrom-Json
$buildIdentities = Get-Content -LiteralPath (Join-Path $build 'build-identities.json') -Raw | ConvertFrom-Json
function Assert-Identity {
    param([string]$Path, [string]$ExpectedHash, [string]$Description)
    if ((Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ExpectedHash) {
        throw "$Description changed after configuration; rebuild before recording evidence."
    }
}
if ($PackageManifest) {
    $package = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if ($package.schema_version -ne 1 -or $package.configuration -ne 'Release' -or
        $package.required_debug_layer -ne $true -or $package.software_adapter_allowed -ne $false) {
        throw 'Unsupported portable package manifest or weakened hardware/validation requirements.'
    }
    foreach ($entry in $package.payload) {
        if ([IO.Path]::IsPathRooted($entry.path)) { throw 'Package payload paths must be relative.' }
        $path = [IO.Path]::GetFullPath((Join-Path $root $entry.path))
        if (-not $path.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
            throw 'Package payload escapes its own directory.'
        }
        $file = Get-Item -LiteralPath $path -ErrorAction Stop
        if ($file.Length -ne $entry.bytes) { throw 'Portable package payload size changed.' }
        Assert-Identity $path $entry.sha256 'Portable package payload'
    }
    Assert-Identity (Join-Path $build 'build-identities.json') $package.origin_build.build_identities_sha256 'Packaged build identity'
    Assert-Identity (Join-Path $build 'production-imports.json') $package.origin_build.production_imports_sha256 'Packaged production origin'
    Assert-Identity $executablePath $package.origin_build.executable_sha256 'Packaged executable'
} else {
    Assert-Identity (Join-Path $imports.production_build 'CMakeCache.txt') $imports.production_cache_sha256 'Production configuration'
    foreach ($entry in $imports.imports) { Assert-Identity $entry.archive $entry.sha256 'Production archive' }
    foreach ($entry in $imports.platform_sources) { Assert-Identity $entry.path $entry.sha256 'Production platform source' }
    if ($imports.PSObject.Properties.Name -notcontains 'foundation_sources' -or @($imports.foundation_sources).Count -eq 0) {
        throw 'Production import manifest lacks foundation source identities; rebuild production dependencies and the probe before recording current workspace evidence.'
    }
    foreach ($entry in $imports.foundation_sources) { Assert-Identity $entry.path $entry.sha256 'Production foundation source' }
    Assert-Identity $imports.dxc $imports.dxc_sha256 'DXC executable'
}
Assert-Identity $executablePath $buildIdentities.executable_sha256 'Experiment executable'
Assert-Identity (Join-Path $build 'production-imports.json') $buildIdentities.production_imports_sha256 'Production import manifest'
if (-not $PackageManifest) {
    foreach ($entry in $buildIdentities.sources) { Assert-Identity $entry.path $entry.sha256 'Experiment source' }
}
foreach ($entry in $buildIdentities.shaders) {
    $shaderPath = if ($PackageManifest) { Join-Path $build ('shaders/' + (Split-Path $entry.path -Leaf)) } else { $entry.path }
    Assert-Identity $shaderPath $entry.sha256 'Compiled shader'
}
$shaderIdentities = @()
foreach ($name in @('probe-vs.cso', 'probe-ps.cso')) {
    $path = (Get-Item -LiteralPath (Join-Path $build "shaders/$name") -ErrorAction Stop).FullName
    $shaderIdentities += [pscustomobject]@{path=$path; sha256=(Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant()}
}
$sourceIdentities = @($buildIdentities.sources)
$output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $root $OutputDirectory }))
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Evidence must stay under workspace out, or the portable package evidence directory.'
}
if (Test-Path -LiteralPath $output) { throw 'Use a new evidence directory; earlier evidence is never overwritten.' }
New-Item -ItemType Directory -Path $output | Out-Null
$osMetadata = Read-OptionalMachineMetadata 'Win32_OperatingSystem' @('Caption','Version','BuildNumber','OSArchitecture')
$cpuMetadata = Read-OptionalMachineMetadata 'Win32_Processor' @('Name','NumberOfCores','NumberOfLogicalProcessors')
$displayMetadata = Read-OptionalMachineMetadata 'Win32_VideoController' @('Name','DriverVersion','PNPDeviceID')
$machine = [ordered]@{
    recorded_utc=[DateTimeOffset]::UtcNow.ToString('o')
    computer_name=$env:COMPUTERNAME
    windows=$osMetadata.data
    cpu=$cpuMetadata.data
    display_inventory=$displayMetadata.data
    metadata_errors=@(@($osMetadata,$cpuMetadata,$displayMetadata) | Where-Object { $null -ne $_.error } | ForEach-Object { $_.error })
    optional_metadata_is_not_gpu_proof=$true
    display_inventory_is_not_selected_adapter_proof=$true
    process_is_64_bit=[Environment]::Is64BitProcess
}
$machine | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'machine.json') -Encoding UTF8
$cases = @(@{name='hardware'; injected=$false; expected_exit=0})
if ($RunTimeoutProbe) { $cases += @{name='injected-timeout'; injected=$true; expected_exit=1} }
$results = @()
foreach ($case in $cases) {
    $caseOutput = Join-Path $output $case.name
    New-Item -ItemType Directory -Path $caseOutput | Out-Null
    $stdout = Join-Path $caseOutput 'stdout.log'
    $stderr = Join-Path $caseOutput 'stderr.log'
    $arguments = @('--output', ('"' + $caseOutput + '"'), '--shaders', ('"' + (Join-Path $build 'shaders') + '"'))
    $arguments += '--require-debug-layer'
    if ($case.injected) { $arguments += '--inject-timeout' }
    $process = $null
    $exitCode = $null
    $timedOut = $false
    $failure = $null
    $report = $null
    $started = [DateTimeOffset]::UtcNow
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        $process = Start-Process -FilePath $executablePath -ArgumentList $arguments -WorkingDirectory $caseOutput `
            -WindowStyle Hidden -PassThru -RedirectStandardOutput $stdout -RedirectStandardError $stderr
        # Cache the native handle before waiting so Windows PowerShell preserves ExitCode.
        $null = $process.Handle
        $timedOut = -not $process.WaitForExit($TimeoutSeconds * 1000)
        if ($timedOut) { $process.Kill() }
        $process.WaitForExit()
        $process.Refresh()
        $exitCode = $process.ExitCode
        $resultPath = Join-Path $caseOutput 'result.json'
        if (Test-Path -LiteralPath $resultPath -PathType Leaf) {
            try { $report = Get-Content -LiteralPath $resultPath -Raw | ConvertFrom-Json }
            catch { $failure = "Invalid result.json: $($_.Exception.Message)" }
        } else { $failure = 'Missing result.json.' }
    } catch { $failure = $_.Exception.Message }
    finally {
        $watch.Stop()
        if ($process) {
            if (-not $process.HasExited) { $process.Kill(); $process.WaitForExit() }
            $process.Dispose()
        }
    }
    $checks = [ordered]@{
        process_completed=(-not $timedOut -and $null -ne $exitCode)
        expected_exit=($null -ne $exitCode -and $exitCode -eq $case.expected_exit)
        json_report=($null -ne $report)
    }
    if ($null -ne $report) {
        try {
            $checks.backend = $report.backend -eq 'D3D12'
            $checks.hardware_adapter = $report.software_adapter -eq $false
            $checks.current_scope_rtx = $report.adapter -eq 'NVIDIA GeForce RTX 5070 Ti' -and
                $report.adapter_vendor_id -eq 0x10de -and $report.adapter_device_id -eq 0x2c05
            $checks.actual_device_identity = $report.device_luid_matches_dxgi_adapter -eq $true -and
                -not [string]::IsNullOrWhiteSpace($report.adapter_luid)
            $checks.actual_driver_and_capabilities = $report.driver -match '^\d+\.\d+\.\d+\.\d+$' -and
                $report.feature_level -eq '12_0 (requested and created)' -and
                $report.shader_model -eq '6_0 (explicit CheckFeatureSupport)'
            $checks.actual_initial_presentation = $report.swapchain_initial_width -gt 0 -and
                $report.swapchain_initial_height -gt 0 -and $report.swapchain_initial_format -eq 28 -and
                $report.swapchain_initial_buffer_count -eq 3 -and $report.swapchain_initial_sample_count -eq 1 -and
                $report.swapchain_initial_sample_quality -eq 0 -and $report.swapchain_initial_swap_effect -eq 4 -and
                $report.swapchain_initial_flags -eq 0 -and $report.swapchain_initial_windowed -eq $true
            $checks.present_parameters = $report.present_sync_interval -eq 0 -and $report.present_flags -eq 0
            $checks.debug_layer = $report.debug_layer_enabled -eq $true
            $checks.clean_debug_errors = $report.debug_error_count -eq 0
            $checks.clean_debug_warnings = $report.debug_warning_count -eq 0
            if ($case.injected) {
                $checks.expected_failure = $report.status -eq 'fail' -and $report.timeout_injected -eq $true
                $checks.original_diagnostic = $report.error.Contains('injected GPU wait timeout')
                $checks.cleanup_completed = $report.cleanup_completed -eq $true
                $checks.bounded_failure = $report.elapsed_ms -lt 10000
            } else {
                $checks.shutdown_completed = $report.shutdown_completed -eq $true
                $checks.success = $report.status -eq 'pass'
                $checks.e01 = $report.e01_passed -eq $true
                $checks.e02 = $report.e02_passed -eq $true
                $checks.e03 = $report.e03_passed -eq $true
                $checks.presented_frames = $report.submitted_frames -gt 0
                $checks.real_pixels = $report.width -gt 0 -and $report.height -gt 0
                $checks.actual_restored_presentation = $report.swapchain_restored_width -eq $report.width -and
                    $report.swapchain_restored_height -eq $report.height -and $report.swapchain_restored_format -eq 28 -and
                    $report.swapchain_restored_buffer_count -eq 3 -and $report.swapchain_restored_sample_count -eq 1 -and
                    $report.swapchain_restored_sample_quality -eq 0 -and $report.swapchain_restored_swap_effect -eq 4 -and
                    $report.swapchain_restored_flags -eq 0 -and $report.swapchain_restored_windowed -eq $true
                $checks.adapter_identity = -not [string]::IsNullOrWhiteSpace($report.adapter) -and
                    -not [string]::IsNullOrWhiteSpace($report.adapter_luid)
                $checks.capability_record = -not [string]::IsNullOrWhiteSpace($report.driver) -and
                    -not [string]::IsNullOrWhiteSpace($report.feature_level) -and -not [string]::IsNullOrWhiteSpace($report.shader_model)
            }
        } catch { $failure = "Incomplete report schema: $($_.Exception.Message)"; $checks.complete_schema = $false }
    }
    $screenshots = @()
    foreach ($file in @(Get-ChildItem -LiteralPath $caseOutput -Filter '*.png' -File)) {
        $screenshots += [pscustomobject]@{path=$file.FullName; bytes=$file.Length; sha256=(Get-FileHash -LiteralPath $file.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
    }
    if (-not $case.injected) {
        foreach ($name in @('e01-linear.png', 'e01-srgb.png', 'e02-updated.png', 'e02-empty.png', 'e03-restored.png')) {
            $checks["screenshot_$name"] = @($screenshots | Where-Object { (Split-Path $_.path -Leaf) -eq $name -and $_.bytes -gt 0 }).Count -eq 1
        }
    }
    $passed = -not $failure -and @($checks.Values | Where-Object { $_ -ne $true }).Count -eq 0
    $results += [pscustomobject]@{
        case=$case.name; started_utc=$started.ToString('o'); elapsed_ms=$watch.ElapsedMilliseconds
        expected_exit_code=$case.expected_exit; exit_code=$exitCode; timed_out=$timedOut; timeout_seconds=$TimeoutSeconds
        injected_test=$case.injected; debug_layer_required=$true
        passed=$passed; checks=[pscustomobject]$checks; failure=$failure; report=$report; screenshots=$screenshots
        stdout=$stdout; stderr=$stderr
    }
}
$verification = [ordered]@{
    executable=$executablePath
    executable_sha256=(Get-FileHash -LiteralPath $executablePath -Algorithm SHA256).Hash.ToLowerInvariant()
    production_imports=$imports; build_identities=$buildIdentities; shaders=$shaderIdentities; experiment_sources=$sourceIdentities
    portable_package=([bool]$PackageManifest)
    sources_are_build_origin_records=([bool]$PackageManifest)
    machine_file=(Join-Path $output 'machine.json')
    passed=@($results | Where-Object { -not $_.passed }).Count -eq 0
    freeze_authorized=$false
    scope='Isolated discrete-adapter D3D12 R1 experiments; explicit zero-extent input and timeout are labelled contract injections, not real zero-pixel propagation or device-loss evidence.'
    remaining='T2 acceptance and the current spec R1 handoff/evidence review are required before R2. This discrete-adapter experiment does not waive unexecuted matrix or later performance requirements and is not gameplay or T3 acceptance.'
    cases=$results
}
$verification | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'verification.json') -Encoding UTF8
[pscustomobject]$verification | Select-Object passed,cases | ConvertTo-Json -Depth 12
if (-not $verification.passed) { throw "D3D12 R1 experiments failed or lack required evidence; see $output" }

[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$InputDirectory,
      [Parameter(Mandatory=$true)][string]$OutputFile,
      [Parameter(Mandatory=$true)][ValidatePattern('^[0-9]{4}-[0-9]{2}-[0-9]{2}$')][string]$AnalysisDate,
      [switch]$MetricsOnly)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$culture=[Globalization.CultureInfo]::InvariantCulture
$summaryPath=Join-Path $InputDirectory 'summary.json'
$summary=Get-Content -LiteralPath $summaryPath -Raw | ConvertFrom-Json
if(-not $summary.passed -or $summary.actual_game_processes -ne 9 -or $summary.passed_count -ne 9) {
    throw 'A complete nine-round valid exploration is required for this budget proposal.'
}
if(Test-Path -LiteralPath $OutputFile) { throw 'Analysis output must be new.' }
function Number($value) {
    $number=[double]::Parse([string]$value,$script:culture)
    if([double]::IsNaN($number) -or [double]::IsInfinity($number) -or $number -lt 0) { throw 'Invalid metric' }
    return $number
}
function Range([double[]]$values) {
    $ordered=@($values | Sort-Object)
    if($ordered.Count -ne 3) { throw 'Each scene must retain three valid rounds.' }
    return [ordered]@{ min=$ordered[0]; median=$ordered[1]; max=$ordered[2]; spread=$ordered[2]-$ordered[0] }
}
function Up([double]$value,[double]$step) { return [Math]::Round([Math]::Ceiling($value/$step)*$step,6) }
$scenes=New-Object 'System.Collections.Generic.List[object]'
$rounds=New-Object 'System.Collections.Generic.List[object]'
foreach($scene in @('static','walk','edit')) {
    $runs=@($summary.rounds | Where-Object scene -eq $scene)
    if($runs.Count -ne 3) { throw "Missing scene rounds: $scene" }
    foreach($run in $runs) {
        if(-not $run.passed -or -not $run.memory.passed) { throw 'Invalid round or memory coverage' }
        $m=$run.metrics
        $rounds.Add([pscustomobject]@{
            scene=$scene; repeat=$run.repeat; frames=Number $m.frame_count
            p95_ms=Number $m.frame_p95_ms; p99_ms=Number $m.frame_p99_ms
            first_frame_ms=Number $m.first_frame_swap_return_from_main_entry_ms
            ws_observed_max_mib=(Number $run.memory.working_set_bytes.max_observed_bytes)/1MB
            private_observed_max_mib=(Number $run.memory.private_bytes.max_observed_bytes)/1MB
            ws_growth_mib=((Number $run.memory.working_set_bytes.last_bytes)-(Number $run.memory.working_set_bytes.first_bytes))/1MB
            private_growth_mib=((Number $run.memory.private_bytes.last_bytes)-(Number $run.memory.private_bytes.first_bytes))/1MB
            memory_points=$run.memory.samples; gpu_count=Number $m.gpu_count
            gpu_missing=Number $m.gpu_missing_sample_frames
            gpu_missing_ratio=(Number $m.gpu_missing_sample_frames)/(Number $m.frame_count)
        })
    }
    $set=@($rounds | Where-Object scene -eq $scene)
    $scenes.Add([pscustomobject]@{
        scene=$scene
        p95_ms=Range @($set | ForEach-Object p95_ms)
        p99_ms=Range @($set | ForEach-Object p99_ms)
        first_frame_ms=Range @($set | ForEach-Object first_frame_ms)
        ws_observed_max_mib=Range @($set | ForEach-Object ws_observed_max_mib)
        private_observed_max_mib=Range @($set | ForEach-Object private_observed_max_mib)
    })
}
$maxWs=($rounds | Measure-Object ws_observed_max_mib -Maximum).Maximum
$maxPrivate=($rounds | Measure-Object private_observed_max_mib -Maximum).Maximum
$maxStartup=($rounds | Measure-Object first_frame_ms -Maximum).Maximum
$policies=@(
    [pscustomobject]@{ id=1; name='tight'; p95=1.05; p99=1.10; memory=1.10; startup=1.10 },
    [pscustomobject]@{ id=2; name='balanced'; p95=1.10; p99=1.20; memory=1.15; startup=1.20 },
    [pscustomobject]@{ id=3; name='wide'; p95=1.20; p99=1.30; memory=1.25; startup=1.30 })
$options=@(if(-not $MetricsOnly) { foreach($policy in $policies) {
    $limits=foreach($scene in $scenes) {
        [pscustomobject]@{ scene=$scene.scene
            frame_p95_ms=Up ($scene.p95_ms.max*$policy.p95) 0.1
            frame_p99_ms=Up ($scene.p99_ms.max*$policy.p99) 0.1 }
    }
    [pscustomobject]@{ id=$policy.id; name=$policy.name; recommended=($policy.id -eq 2)
        nominal_policy_multipliers=$policy; frame_limits=@($limits)
        ws_observed_max_limit_mib=Up ($maxWs*$policy.memory) 64
        private_observed_max_limit_mib=Up ($maxPrivate*$policy.memory) 64
        first_frame_limit_ms=Up ($maxStartup*$policy.startup) 50 }
} })
$report=[ordered]@{
    date=$AnalysisDate
    role=if($MetricsOnly) { 'Frozen GLFW repeat exploratory P95/P99 measurement; not matched Q06 acceptance' } else { 'Frozen GLFW exploratory Q07 budget proposal; not matched Q06 acceptance' }
    source_summary=$summaryPath; source_summary_sha256=(Get-FileHash -LiteralPath $summaryPath -Algorithm SHA256).Hash.ToLowerInvariant()
    source_manifest_sha256=(Get-FileHash -LiteralPath (Join-Path $InputDirectory 'manifest.json') -Algorithm SHA256).Hash.ToLowerInvariant()
    planned=9; valid=9; rounds=$rounds.ToArray(); scenes=$scenes.ToArray(); options=@($options)
    budget_approved=$false; sdl_acceptance_sampled=$false; fix1_resumed=$false
    interpretation=@(
        'Nearest-rank per round. A median of three P95 values is not a pooled percentile.'
        'Candidate headroom is a policy choice, not a confidence interval or a proven noise bound.'
        'Only three independent 30-second rounds per scene; keep all slow and unfocused frames.'
        'Memory limits apply to periodic observations in [10,40)s, not loading/export peaks, VRAM, or a leak guarantee.'
        'These old-source measurements must not be combined with current SDL as a matched migration comparison.'
        'No acceptance outcome before human budget confirmation and required same-source comparison.')
}
$utf8=New-Object Text.UTF8Encoding($false)
[IO.File]::WriteAllText([IO.Path]::GetFullPath($OutputFile),(($report | ConvertTo-Json -Depth 15)+"`n"),$utf8)
$report.scenes | Format-Table scene,@{Label='P95 min/median/max';Expression={ '{0:F3}/{1:F3}/{2:F3}' -f $_.p95_ms.min,$_.p95_ms.median,$_.p95_ms.max }},@{Label='P99 min/median/max';Expression={ '{0:F3}/{1:F3}/{2:F3}' -f $_.p99_ms.min,$_.p99_ms.median,$_.p99_ms.max }}
$report.options | Format-Table id,name,ws_observed_max_limit_mib,private_observed_max_limit_mib,first_frame_limit_ms

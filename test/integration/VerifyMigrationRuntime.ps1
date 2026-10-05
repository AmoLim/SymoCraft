[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$OldExecutable,
    [Parameter(Mandatory)][string]$NewExecutable,
    [Parameter(Mandatory)][string]$DebugExecutable,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (!$output.StartsWith((Join-Path $root 'out') + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Runtime evidence must be a new directory below this workspace out directory.'
}
if (Test-Path -LiteralPath $output) { throw 'Refusing to overwrite runtime evidence.' }
$old = (Get-Item -LiteralPath $OldExecutable).FullName
$new = (Get-Item -LiteralPath $NewExecutable).FullName
$debug = (Get-Item -LiteralPath $DebugExecutable).FullName
New-Item -ItemType Directory -Path $output | Out-Null
$cwd = New-Item -ItemType Directory -Path (Join-Path $output 'unrelated-cwd')
$results = [Collections.Generic.List[object]]::new()

function Invoke-Probe([string]$Name, [string]$Executable, [string[]]$Arguments, [int]$Expected = 0, [int]$Timeout = 120) {
    $start = [Diagnostics.ProcessStartInfo]::new()
    $start.FileName = $Executable
    $start.WorkingDirectory = $cwd.FullName
    $start.UseShellExecute = $false
    $start.CreateNoWindow = $true
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    foreach ($argument in $Arguments) { $start.ArgumentList.Add($argument) }
    $process = [Diagnostics.Process]::Start($start)
    $stdout = $process.StandardOutput.ReadToEndAsync()
    $stderr = $process.StandardError.ReadToEndAsync()
    if (!$process.WaitForExit($Timeout * 1000)) {
        $process.Kill()
        $process.WaitForExit()
        throw "Probe $Name timed out; only its owned game process was stopped."
    }
    $outText = $stdout.GetAwaiter().GetResult()
    $errText = $stderr.GetAwaiter().GetResult()
    [IO.File]::WriteAllText((Join-Path $output "$Name.stdout.log"), $outText)
    [IO.File]::WriteAllText((Join-Path $output "$Name.stderr.log"), $errText)
    $record = [ordered]@{name=$Name; executable=$Executable; exit_code=$process.ExitCode; expected=$Expected; arguments=$Arguments}
    $results.Add($record)
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $output 'probes.json') -Encoding UTF8
    if ($process.ExitCode -ne $Expected) { throw "Probe $Name returned $($process.ExitCode), expected $Expected. See retained logs." }
    Write-Host "Passed $Name (exit $($process.ExitCode))"
    return $outText
}

$identity = [ordered]@{
    old=(Get-FileHash -LiteralPath $old -Algorithm SHA256).Hash
    new=(Get-FileHash -LiteralPath $new -Algorithm SHA256).Hash
    debug=(Get-FileHash -LiteralPath $debug -Algorithm SHA256).Hash
    note='Short migration comparison, not the frozen 36-minute baseline. No user gameplay claim.'
}
$identity | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'identity.json') -Encoding UTF8
$summaryArguments = @('--world-summary','--seed','424242','--scene','regression','--checkpoint','four-chunk','--test-edits')
$before = Invoke-Probe 'old-world-summary' $old $summaryArguments
$after = Invoke-Probe 'new-world-summary' $new $summaryArguments
if ($before -cne $after) { throw 'World summary differs byte-for-byte; inspect before/after logs before continuing.' }

foreach ($probe in @(@('old-static',$old,'static'), @('new-static',$new,'static'), @('new-edit',$new,'edit'))) {
    $capture = Join-Path $output $probe[0]
    $null = Invoke-Probe $probe[0] $probe[1] @('--benchmark',$probe[2],'--output',$capture,
        '--seed','424242','--warmup-seconds','5','--sample-seconds','15','--focus-policy','allow-unfocused')
    if (!(Test-Path -LiteralPath (Join-Path $capture 'summary.yaml'))) { throw 'Missing benchmark summary.' }
}
$null = Invoke-Probe 'debug-smoke' $debug @('--scene','regression','--seed','424242','--smoke-frames','120')

$package = Split-Path -Parent $new
foreach ($fault in @('shader','texture','block-config')) {
    $destination = Join-Path $output "fault-$fault"
    Copy-Item -LiteralPath $package -Destination $destination -Recurse
    switch ($fault) {
        'shader' { $relative = 'assets/shaders/vs_BlockShader.glsl' }
        'texture' { $relative = 'assets/textures/texture_atlas.png' }
        'block-config' { $relative = 'assets/configs/blockFormats.yaml' }
    }
    [IO.File]::WriteAllText((Join-Path $destination $relative), 'deliberately invalid migration fixture')
    $null = Invoke-Probe "fault-$fault" (Join-Path $destination 'SymoCraft.exe') @('--smoke-frames','1') 3
}
Write-Host "Runtime checks finished. Evidence: $output"

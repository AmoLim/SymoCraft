[CmdletBinding()]
param(
    [Parameter(Mandatory)] [string]$Executable,
    [Parameter(Mandatory)] [string]$OutputDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out'))
$output = [IO.Path]::GetFullPath($(if ([IO.Path]::IsPathRooted($OutputDirectory)) { $OutputDirectory } else { Join-Path $root $OutputDirectory }))
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Use workspace out for evidence.' }
if (Test-Path -LiteralPath $output) { throw 'Use a fresh evidence directory.' }
$exe = (Get-Item -LiteralPath $Executable).FullName
New-Item -ItemType Directory -Path $output | Out-Null
$stdout = Join-Path $output 'stdout.log'
$stderr = Join-Path $output 'stderr.log'
$process = Start-Process -FilePath $exe -WorkingDirectory $root -PassThru -WindowStyle Hidden -RedirectStandardOutput $stdout -RedirectStandardError $stderr
try {
    $null = $process.Handle
    if (-not $process.WaitForExit(30000)) { throw 'SDL runtime input fixture timed out.' }
    $process.Refresh()
    $text = [IO.File]::ReadAllText($stdout)
    $checks = if ($text -match 'fixture: (\d+) checks passed') { [int]$Matches[1] } else { 0 }
    [ordered]@{ created_utc=[DateTime]::UtcNow.ToString('o'); executable=$exe; executable_sha256=(Get-FileHash -LiteralPath $exe -Algorithm SHA256).Hash.ToLowerInvariant(); exit_code=$process.ExitCode; checks=$checks; passed=($process.ExitCode -eq 0); synthetic_own_window_messages=$true; hardware_validation=$false } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding utf8
    if ($process.ExitCode -ne 0) { throw "SDL runtime fixture did not fully pass (exit $($process.ExitCode)): $text $([IO.File]::ReadAllText($stderr))" }
    Write-Host $text.Trim()
} finally {
    if (-not $process.HasExited) { Stop-Process -Id $process.Id -Force }
    $process.Dispose()
}

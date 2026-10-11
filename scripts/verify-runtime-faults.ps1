# Compatibility launcher only; all automation lives in the adjacent Python script.
[CmdletBinding()]
param(
    [Parameter(Mandatory)][string]$Executable,
    [Parameter(Mandatory)][string]$OutputDirectory
)
$ErrorActionPreference = 'Stop'
$python = $null
$prefix = @()
$candidates = if ($env:SYMOCRAFT_PYTHON) { @($env:SYMOCRAFT_PYTHON) } else { @('python.exe', 'py.exe') }
foreach ($candidate in $candidates) {
    $candidatePrefix = if ([IO.Path]::GetFileName($candidate) -eq 'py.exe') { @('-3') } else { @() }
    try {
        $version = & $candidate @candidatePrefix --version 2>&1
        if ($LASTEXITCODE -eq 0 -and "$version" -match '^Python 3\.(\d+)' -and [int]$Matches[1] -ge 10) {
            $python = $candidate
            $prefix = $candidatePrefix
            break
        }
    } catch { }
}
if (-not $python) { throw 'Python 3.10+ is required. Install Python or set SYMOCRAFT_PYTHON to its executable.' }
$pythonArguments = @($prefix) + @((Join-Path $PSScriptRoot 'verify-runtime-faults.py'))
foreach ($entry in $PSBoundParameters.GetEnumerator()) {
    if (@('Verbose', 'Debug', 'ErrorAction', 'WarningAction', 'InformationAction', 'ProgressAction',
          'ErrorVariable', 'WarningVariable', 'InformationVariable', 'OutVariable', 'OutBuffer', 'PipelineVariable') -contains $entry.Key) { continue }
    if ($entry.Value -is [System.Management.Automation.SwitchParameter]) {
        if ($entry.Value.IsPresent) { $pythonArguments += "-$($entry.Key)" }
    } else {
        $value = $entry.Value
        if ($entry.Key -in @('Executable', 'OutputDirectory')) {
            $value = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath([string]$value)
        }
        $pythonArguments += "-$($entry.Key)"
        $pythonArguments += @($value | ForEach-Object { [string]$_ })
    }
}
# Native stderr is data here; Python's exit status determines success.
$ErrorActionPreference = 'Continue'
& $python @pythonArguments 2>&1 | Tee-Object -Variable outputLines | ForEach-Object { Write-Output ([string]$_) }
$code = $LASTEXITCODE
$ErrorActionPreference = 'Stop'
if ($code -ne 0) { throw ($outputLines -join "`n") }

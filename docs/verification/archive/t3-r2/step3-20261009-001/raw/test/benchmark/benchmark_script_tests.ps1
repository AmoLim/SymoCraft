[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$BenchmarkScript,
    [Parameter(Mandatory = $true)][string]$FixtureExecutable,
    [Parameter(Mandatory = $true)][string]$OutputParent
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
# Some CTest launch environments contain both Path and PATH; normalize this child only.
$inheritedPath = [Environment]::GetEnvironmentVariable('PATH')
[Environment]::SetEnvironmentVariable('PATH', $null)
[Environment]::SetEnvironmentVariable('Path', $null)
[Environment]::SetEnvironmentVariable('PATH', $inheritedPath)
Import-Module (Join-Path $PSHOME 'Modules/Microsoft.PowerShell.Utility') -ErrorAction Stop
$script = (Get-Item -LiteralPath $BenchmarkScript).FullName
$exe = (Get-Item -LiteralPath $FixtureExecutable).FullName
$parent = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputParent)
$root = Join-Path $parent ([Guid]::NewGuid().ToString('N'))
$shellDirectory = Join-Path $root 'shell directory'
$nativeDirectory = Join-Path $root 'native directory'
New-Item -ItemType Directory -Path $shellDirectory,$nativeDirectory -Force | Out-Null
$savedNativeDirectory = [Environment]::CurrentDirectory
Push-Location -LiteralPath $shellDirectory
try {
    [Environment]::CurrentDirectory = $nativeDirectory
    foreach ($case in @(
        @{ Name = 'success'; Seed = 0; Code = 0; Passed = $true; Repeats = 2 },
        @{ Name = 'fast-success'; Seed = 2; Code = 0; Passed = $true; Repeats = 2 },
        @{ Name = 'focus-failure'; Seed = 4; Code = 4; Passed = $false; Repeats = 3 },
        @{ Name = 'runtime-failure'; Seed = 3; Code = 3; Passed = $false; Repeats = 3 }
    )) {
        $failure = $null
        try {
            & $script -Executable $exe -OutputDirectory $case.Name -Scenarios static -Seed $case.Seed `
                -WarmupSeconds 0 -SampleSeconds 1 -Repeats $case.Repeats -SkipGpuSensors | Out-Null
        } catch { $failure = $_.Exception.Message }
        $resultPath = Join-Path $shellDirectory "$($case.Name)/static-1/result.json"
        if (-not (Test-Path -LiteralPath $resultPath)) { throw "Relative output did not follow PowerShell location: $resultPath; $failure" }
        if (Test-Path -LiteralPath (Join-Path $nativeDirectory $case.Name)) { throw 'Output used native process directory.' }
        $result = Get-Content -Raw -LiteralPath $resultPath | ConvertFrom-Json
        if ($null -eq $result.exitCode -or $result.exitCode -ne $case.Code) { throw "Incorrect exit code: $($case.Name)" }
        if ($result.passed -ne $case.Passed) { throw "Incorrect validity: $($case.Name)" }
        if ($case.Passed) {
            if ($failure) { throw $failure }
            if ($result.frameMs.count -ne 2 -or $result.frameMs.p95 -ne 30 -or $result.gpuDrawMs.count -ne 1) { throw 'Statistics changed.' }
            if (-not (Test-Path -LiteralPath (Join-Path $shellDirectory "$($case.Name)/static-2/result.json"))) { throw 'Successful sequence stopped early.' }
        } else {
            if (-not $failure -or $result.failureReasons.Count -eq 0) { throw 'Failure lacks diagnostics.' }
            if (Test-Path -LiteralPath (Join-Path $shellDirectory "$($case.Name)/static-2")) { throw 'Failure did not stop subsequent runs.' }
            if ($case.Seed -eq 4 -and ($result.focusLostFrames -ne 1 -or $failure -notmatch 'window-not-focused')) { throw 'Focus failure was not reported.' }
        }
        Write-Output "$($case.Name): passed"
    }
    $existing = Join-Path $shellDirectory 'success'
    $before = (Get-FileHash -LiteralPath (Join-Path $existing 'machine.json')).Hash
    $refused = $false
    try { & $script -Executable $exe -OutputDirectory $existing -SkipGpuSensors | Out-Null }
    catch { $refused = $_.Exception.Message -match 'previous runs are not overwritten' }
    if (-not $refused -or (Get-FileHash -LiteralPath (Join-Path $existing 'machine.json')).Hash -ne $before) { throw 'Existing output was not protected.' }
    Write-Output "Output protection passed. PowerShell $($PSVersionTable.PSVersion); no game/GL context was used."
} finally {
    [Environment]::CurrentDirectory = $savedNativeDirectory
    Pop-Location
}

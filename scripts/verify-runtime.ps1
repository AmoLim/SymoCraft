[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Executable,
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,
    [ValidateRange(0, 10000)]
    [int]$Frames = 120,
    [ValidateRange(10, 600)]
    [int]$TimeoutSeconds = 120,
    [ValidateSet(0, 2, 3)]
    [int]$ExpectedExitCode = 0,
    [uint32]$Seed,
    [ValidateSet('terrain', 'regression')]
    [string]$Scene = 'terrain',
    [ValidateSet('spawn', 'positive-x', 'negative-x', 'four-chunk', 'wall-corner', 'low-ceiling', 'single-block')]
    [string]$Checkpoint,
    [switch]$TestEdits
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
if ($Scene -ne 'regression' -and ($Checkpoint -or $TestEdits)) {
    throw 'Checkpoint and TestEdits require the regression scene.'
}
$executablePath = (Get-Item -LiteralPath $Executable).FullName
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) {
    throw "Use a new evidence directory instead of overwriting an existing run: $output"
}
New-Item -ItemType Directory -Path $output | Out-Null
$stdout = Join-Path $output 'stdout.log'
$stderr = Join-Path $output 'stderr.log'
$start = @{
    FilePath = $executablePath
    WorkingDirectory = $output
    WindowStyle = 'Hidden'
    PassThru = $true
    RedirectStandardOutput = $stdout
    RedirectStandardError = $stderr
}
$gameArguments = @()
if ($PSBoundParameters.ContainsKey('Scene')) { $gameArguments += @('--scene', $Scene) }
if ($Frames -gt 0) { $gameArguments += @('--smoke-frames', "$Frames") }
if ($PSBoundParameters.ContainsKey('Seed')) { $gameArguments += @('--seed', "$Seed") }
if ($Checkpoint) { $gameArguments += @('--checkpoint', $Checkpoint) }
if ($TestEdits) { $gameArguments += '--test-edits' }
if ($gameArguments.Count -gt 0) { $start.ArgumentList = $gameArguments }
$timer = [Diagnostics.Stopwatch]::StartNew()
$process = Start-Process @start
$timedOut = $false
try {
    if (-not $process.WaitForExit($TimeoutSeconds * 1000)) {
        $timedOut = $true
        $process.Kill()
        $process.WaitForExit()
    }
    $timer.Stop()
    $process.Refresh()
    $exitCode = $process.ExitCode
    $text = [IO.File]::ReadAllText($stdout)
    $errors = [IO.File]::ReadAllText($stderr)
    $ready = $null -ne $text -and $text.Contains('[runtime] ready;')
    $shutdown = $null -ne $text -and $text.Contains('[runtime] shutdown complete;')
    $frameMatch = [regex]::Match([string]$text, '\[runtime\] loop finished; rendered_frames=(\d+)')
    $renderedFrames = if ($frameMatch.Success) { [int]$frameMatch.Groups[1].Value } else { $null }
    $glDiagnosticCount = [regex]::Matches($errors, 'OpenGL diagnostic').Count
    $passed = -not $timedOut -and $exitCode -eq $ExpectedExitCode
    if ($ExpectedExitCode -eq 0) {
        $passed = $passed -and $ready -and $shutdown -and $frameMatch.Success -and
            $glDiagnosticCount -eq 0 -and -not $errors.Contains('[runtime] fatal:')
        if ($Frames -gt 0) { $passed = $passed -and $renderedFrames -eq $Frames }
    }
    if ($ExpectedExitCode -eq 3) { $passed = $passed -and -not $ready -and $shutdown }
    $result = [ordered]@{
        executable = $executablePath
        workingDirectory = $output
        requestedFrames = $Frames
        arguments = $gameArguments
        renderedFrames = $renderedFrames
        glDiagnosticCount = $glDiagnosticCount
        processId = $process.Id
        timedOut = $timedOut
        exitCode = $exitCode
        expectedExitCode = $ExpectedExitCode
        ready = $ready
        shutdown = $shutdown
        elapsedSeconds = $timer.Elapsed.TotalSeconds
        passed = $passed
    }
    $result | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'result.json') -Encoding UTF8
    $result | ConvertTo-Json | Write-Output
    if (-not $passed) { throw "Runtime probe failed; see $output" }
} finally {
    if (-not $process.HasExited) {
        $process.Kill()
        $process.WaitForExit()
    }
    $process.Dispose()
}

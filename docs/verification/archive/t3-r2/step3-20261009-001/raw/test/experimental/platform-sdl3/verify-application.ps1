[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$ProtocolInspector,
    [Parameter(Mandatory=$true)][string]$OutputDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = Join-Path $root 'out'
$exe = (Get-Item -LiteralPath $Executable).FullName
$inspector = (Get-Item -LiteralPath $ProtocolInspector).FullName
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $output.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Evidence must be inside workspace out.' }
if (Test-Path -LiteralPath $output) { throw 'Choose a fresh evidence directory.' }
New-Item -ItemType Directory -Path $output | Out-Null
if (-not ('S3OwnedWindows' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class S3OwnedWindows {
    public delegate bool Enumerate(IntPtr hwnd,IntPtr data);
    [DllImport("user32.dll")] public static extern bool EnumWindows(Enumerate callback,IntPtr data);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern int GetWindowText(IntPtr hwnd,StringBuilder text,int count);
    [StructLayout(LayoutKind.Sequential)] public struct Rect { public int Left,Top,Right,Bottom; }
    [DllImport("user32.dll")] public static extern bool GetClientRect(IntPtr hwnd,out Rect rect);
    [DllImport("user32.dll")] public static extern IntPtr SetThreadDpiAwarenessContext(IntPtr context);
    public static int[] Pixels(IntPtr hwnd) {
        var prior=SetThreadDpiAwarenessContext(new IntPtr(-4));
        if(prior==IntPtr.Zero)throw new InvalidOperationException("Probe thread DPI context could not be set");
        try {Rect rect;if(!GetClientRect(hwnd,out rect))throw new InvalidOperationException("GetClientRect failed");return new[]{rect.Right-rect.Left,rect.Bottom-rect.Top};}
        finally {if(SetThreadDpiAwarenessContext(prior)==IntPtr.Zero)throw new InvalidOperationException("Probe DPI context restore failed");}
    }
    public static bool ForceSize(IntPtr hwnd) {
        var prior=SetThreadDpiAwarenessContext(new IntPtr(-4));
        if(prior==IntPtr.Zero)throw new InvalidOperationException("Probe resize DPI context could not be set");
        try {return SetWindowPos(hwnd,IntPtr.Zero,0,0,1280,720,0x0416);}
        finally {if(SetThreadDpiAwarenessContext(prior)==IntPtr.Zero)throw new InvalidOperationException("Probe resize DPI context restore failed");}
    }
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint id);
    [DllImport("user32.dll")] public static extern bool IsWindowVisible(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern bool ShowWindow(IntPtr hwnd,int mode);
    [DllImport("user32.dll")] public static extern bool PostMessage(IntPtr hwnd,uint message,IntPtr w,IntPtr l);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr hwnd);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool SetWindowPos(IntPtr hwnd,IntPtr after,int x,int y,int w,int h,uint flags);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] public static extern IntPtr CreateWindowEx(uint ex,string klass,string title,uint style,int x,int y,int w,int h,IntPtr parent,IntPtr menu,IntPtr instance,IntPtr param);
    [DllImport("user32.dll")] public static extern bool DestroyWindow(IntPtr hwnd);
    public static IntPtr Find(uint process) {
        IntPtr result=IntPtr.Zero;
        EnumWindows((hwnd,data)=>{uint owner;GetWindowThreadProcessId(hwnd,out owner);if(owner==process){var title=new StringBuilder(256);GetWindowText(hwnd,title,256);if(title.ToString()=="SymoCraft"){result=hwnd;return false;}}return true;},IntPtr.Zero);
        return result;
    }
}
'@
}
function Read-LiveLog {
    param([string]$Path)
    $stream = [IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,[IO.FileShare]::ReadWrite)
    try {
        $reader = [IO.StreamReader]::new($stream)
        try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
    } finally { $stream.Dispose() }
}
$results = @()
foreach ($case in @('paused-close','minimized-close','allow-unfocused','strict-unfocused','benchmark-minimized','benchmark-resized','benchmark-close')) {
    $directory = Join-Path $output $case
    New-Item -ItemType Directory -Path $directory | Out-Null
    $benchmark = $case -notin @('paused-close','minimized-close')
    $policy = if ($case -eq 'strict-unfocused') { 'strict' } else { 'allow-unfocused' }
    $arguments = @('--seed','424242')
    if ($benchmark) { $arguments += @('--benchmark','static','--output',('"{0}"' -f "$directory/run"),'--warmup-seconds','0','--sample-seconds','3','--width','1920','--height','1080','--vsync','0','--focus-policy',$policy) }
    $process = $null
    $sink = [IntPtr]::Zero
    $errorText = $null
    $partial = $false
    $report = $null
    $timedOut = $false
    $exitCode = $null
    $action = $false
    $forcedTermination = $false
    $pixelsBefore = $null
    $pixelsAfter = $null
    try {
        $process = Start-Process -FilePath $exe -ArgumentList $arguments -WorkingDirectory $directory -WindowStyle Hidden -PassThru -RedirectStandardOutput "$directory/stdout.log" -RedirectStandardError "$directory/stderr.log"
        $null = $process.Handle
        $deadline = [DateTime]::UtcNow.AddSeconds(40)
        $hwnd = [IntPtr]::Zero
        do {
            $hwnd = [S3OwnedWindows]::Find($process.Id)
            # Hidden helper startup can suppress the first Win32 show; reveal only this owned game HWND.
            if ($hwnd -ne [IntPtr]::Zero -and -not [S3OwnedWindows]::IsWindowVisible($hwnd)) {
                $null = [S3OwnedWindows]::ShowWindow($hwnd,5)
            }
            $ready = (Read-LiveLog "$directory/stdout.log").Contains('[runtime] ready;')
            if ($hwnd -ne [IntPtr]::Zero -and $ready) { break }
            if ($process.HasExited) { throw 'Game exited before the owned ready window was observed.' }
            Start-Sleep -Milliseconds 20
        } while ([DateTime]::UtcNow -lt $deadline)
        if (-not $ready -or $hwnd -eq [IntPtr]::Zero) { throw 'Owned ready window timed out.' }
        $pixelsBefore = [S3OwnedWindows]::Pixels($hwnd)
        if ($case -in @('paused-close','allow-unfocused','strict-unfocused')) {
            # Focus only a fixture owned by this script, never an unrelated user's window.
            $sink = [S3OwnedWindows]::CreateWindowEx(0,'STATIC','SymoCraft S3 owned focus fixture',0x10CF0000,40,40,280,140,[IntPtr]::Zero,[IntPtr]::Zero,[IntPtr]::Zero,[IntPtr]::Zero)
            if ($sink -eq [IntPtr]::Zero) { throw 'Owned focus fixture could not be created.' }
            $null = [S3OwnedWindows]::SetForegroundWindow($sink)
            Start-Sleep -Milliseconds 150
            $action = [S3OwnedWindows]::GetForegroundWindow() -eq $sink
            if (-not $action) { $partial = $true }
        }
        if ($case -in @('minimized-close','benchmark-minimized')) {
            $null = [S3OwnedWindows]::ShowWindow($hwnd,6)
            Start-Sleep -Milliseconds 150
            $action = [S3OwnedWindows]::IsIconic($hwnd)
            if (-not $action) { throw 'Owned window did not actually minimize.' }
        }
        if ($case -eq 'benchmark-resized') {
            # Restricted size-fault fixture bypasses Win32's fixed-window min/max clamp, not SDL state.
            $action = [S3OwnedWindows]::ForceSize($hwnd)
            if (-not $action) { throw 'Owned resize request failed.' }
            $pixelsAfter = [S3OwnedWindows]::Pixels($hwnd)
            if ($pixelsAfter[0] -eq $pixelsBefore[0] -and $pixelsAfter[1] -eq $pixelsBefore[1]) {
                $partial = $true
                throw 'Fixture unavailable: the fixed benchmark window rejected the forced size request; production invalidation was not exercised.'
            }
        }
        if ($case -in @('paused-close','minimized-close','benchmark-close')) {
            if (-not [S3OwnedWindows]::PostMessage($hwnd,0x0010,[IntPtr]::Zero,[IntPtr]::Zero)) { throw 'Owned WM_CLOSE failed.' }
            if ($case -eq 'benchmark-close') { $action = $true }
        }
        $timedOut = -not $process.WaitForExit(60000)
        if ($timedOut) { $process.Kill() }
        $process.WaitForExit()
        $process.Refresh()
        $exitCode = $process.ExitCode
        if ($benchmark) {
            $parserOutput = & $inspector "$directory/run" 2> "$directory/protocol.stderr.log"
            $parserExit = $LASTEXITCODE
            $parserOutput | Set-Content -LiteralPath "$directory/protocol.json" -Encoding UTF8
            $report = $parserOutput | ConvertFrom-Json
            if ($parserExit -ne 0 -or -not $report.protocol) { throw 'Structured protocol verification failed.' }
        }
    } catch { $errorText = $_.Exception.Message }
    finally {
        if ($process) {
            if (-not $process.HasExited) { $forcedTermination = $true; $process.Kill(); $process.WaitForExit() }
            $process.Refresh()
            if ($null -eq $exitCode) { $exitCode = $process.ExitCode }
            $process.Dispose()
        }
        if ($sink -ne [IntPtr]::Zero) { $null = [S3OwnedWindows]::DestroyWindow($sink) }
    }
    $stdout = if (Test-Path -LiteralPath "$directory/stdout.log") { [IO.File]::ReadAllText("$directory/stdout.log") } else { '' }
    $stderr = if (Test-Path -LiteralPath "$directory/stderr.log") { [IO.File]::ReadAllText("$directory/stderr.log") } else { '' }
    $passed = -not $timedOut -and $null -eq $errorText -and $null -ne $exitCode -and
        $stdout.Contains('[runtime] shutdown complete;') -and -not $stderr.Contains('[runtime] fatal:') -and -not $stderr.Contains('OpenGL diagnostic')
    if (-not $benchmark) { $passed = $passed -and $exitCode -eq 0 -and $action }
    elseif ($case -eq 'allow-unfocused') { $passed = $passed -and $exitCode -eq 0 -and $report.valid_run -and $report.completed -and $report.unfocused_frames -gt 0 -and $action }
    elseif ($case -eq 'strict-unfocused') { $passed = $passed -and $exitCode -eq 4 -and -not $report.valid_run -and $report.completed -and $report.window_not_focused -and $report.unfocused_frames -gt 0 -and $action }
    elseif ($case -in @('benchmark-minimized','benchmark-resized')) { $passed = $passed -and $exitCode -eq 4 -and -not $report.valid_run -and -not $report.completed -and $report.framebuffer_changed_or_minimized -and $action }
    else { $passed = $passed -and $exitCode -eq 4 -and -not $report.valid_run -and -not $report.completed -and $report.duration_not_completed -and $action }
    if ($report -and $report.runtime_error) { $passed = $false }
    $results += [pscustomobject]@{case=$case;passed=$passed;partial=$partial;actual_owned_action=$action;pixels_before=$pixelsBefore;pixels_after=$pixelsAfter;exit_code=$exitCode;timed_out=$timedOut;forced_termination=$forcedTermination;error=$errorText;protocol=$report;arguments=$arguments}
}
$summary = [ordered]@{
    scope='S3 owned Win32 window operations against the actual installed game; resize is a restricted forced-client-size fixture (SWP_NOSENDCHANGING), not user dragging a fixed benchmark window; three-second protocol diagnostics, not Q06 sampling, performance budget or physical input acceptance'
    executable=$exe;executable_sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
    protocol_inspector=$inspector;protocol_inspector_sha256=(Get-FileHash -LiteralPath $inspector).Hash.ToLowerInvariant()
    passed=(@($results|Where-Object{-not $_.passed}).Count -eq 0);cases=$results
}
$summary | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath "$output/summary.json" -Encoding UTF8
$results | Select-Object case,passed,partial,exit_code,error | Format-Table
if (-not $summary.passed) { throw "S3 application checks incomplete or failed; see $output" }

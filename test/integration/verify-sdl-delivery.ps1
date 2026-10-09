[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidateNotNullOrEmpty()][string]$GamePackage,
    [Parameter(Mandatory=$true)][ValidateNotNullOrEmpty()][string]$RunnerPackage,
    [Parameter(Mandatory=$true)][ValidateNotNullOrEmpty()][string]$OutputDirectory,
    [string]$ProtocolInspector,
    [ValidateRange(30,180)][int]$TimeoutSeconds = 90,
    [ValidateRange(1,1000)][int]$Frames = 120
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output = [IO.Path]::GetFullPath($OutputDirectory)
if (-not $output.StartsWith((Join-Path $root 'out') + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
    throw 'Evidence and dedicated fixtures must be inside this workspace out directory.'
}
if (Test-Path -LiteralPath $output) { throw "Use a fresh evidence directory: $output" }
$gamePackagePath = (Get-Item -LiteralPath $GamePackage).FullName
$runnerPackagePath = (Get-Item -LiteralPath $RunnerPackage).FullName
foreach ($package in @($gamePackagePath,$runnerPackagePath)) {
    if (-not (Test-Path -LiteralPath $package -PathType Container)) { throw "Package must be a directory: $package" }
    if ($output.StartsWith($package.TrimEnd([char[]]@('\','/')) + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) {
        throw 'Evidence cannot be inside a source package.'
    }
    $items = @((Get-Item -LiteralPath $package)) + @(Get-ChildItem -LiteralPath $package -Force -Recurse)
    foreach ($item in $items) {
        if ($item.Attributes -band [IO.FileAttributes]::ReparsePoint) { throw "Reparse package paths are not supported: $($item.FullName)" }
    }
}
$gameExe = Join-Path $gamePackagePath 'SymoCraft.exe'
$runnerExe = Join-Path $runnerPackagePath 'SymoCraftBenchmark.exe'
foreach ($file in @($gameExe,$runnerExe,(Join-Path $gamePackagePath 'assets/configs/blockFormats.yaml'))) {
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "Required package file is missing: $file" }
}
if (-not $ProtocolInspector) { $ProtocolInspector = Join-Path $root 'out/m3-t2/s5/inspect-sdl-delivery.exe' }
$inspector = (Get-Item -LiteralPath $ProtocolInspector).FullName
if (-not (Test-Path -LiteralPath $inspector -PathType Leaf)) { throw 'ProtocolInspector must be a compiled inspection executable.' }
New-Item -ItemType Directory -Path $output | Out-Null

if (-not ('S5DeliveryWindows' -as [type])) {
Add-Type -TypeDefinition @'
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Runtime.InteropServices;
using System.Text;
public static class S5DeliveryWindows {
    private delegate bool EnumProc(IntPtr hwnd,IntPtr data);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EnumProc callback,IntPtr data);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr hwnd,out uint pid);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr hwnd,StringBuilder text,int length);
    [DllImport("user32.dll",CharSet=CharSet.Unicode)] private static extern int GetClassName(IntPtr hwnd,StringBuilder text,int length);
    [DllImport("user32.dll")] private static extern IntPtr GetDlgItem(IntPtr hwnd,int id);
    [DllImport("user32.dll",CharSet=CharSet.Unicode,SetLastError=true)] private static extern IntPtr SendMessageTimeout(IntPtr hwnd,uint message,UIntPtr wparam,IntPtr lparam,uint flags,uint timeout,out UIntPtr result);
    [DllImport("user32.dll",EntryPoint="SendMessageTimeoutW",CharSet=CharSet.Unicode,SetLastError=true)] private static extern IntPtr SendString(IntPtr hwnd,uint message,UIntPtr wparam,string lparam,uint flags,uint timeout,out UIntPtr result);
    [DllImport("user32.dll",SetLastError=true)] private static extern bool PostMessage(IntPtr hwnd,uint message,IntPtr wparam,IntPtr lparam);
    [DllImport("kernel32.dll")] private static extern uint SetErrorMode(uint mode);
    [DllImport("kernel32.dll",SetLastError=true)] private static extern IntPtr CreateToolhelp32Snapshot(uint flags,uint pid);
    [DllImport("kernel32.dll",EntryPoint="Process32FirstW",CharSet=CharSet.Unicode)] private static extern bool ProcessFirst(IntPtr snapshot,ref Entry entry);
    [DllImport("kernel32.dll",EntryPoint="Process32NextW",CharSet=CharSet.Unicode)] private static extern bool ProcessNext(IntPtr snapshot,ref Entry entry);
    [DllImport("kernel32.dll")] private static extern bool CloseHandle(IntPtr handle);
    [StructLayout(LayoutKind.Sequential,CharSet=CharSet.Unicode)] private struct Entry {
        public uint size,usage,pid; public UIntPtr heap; public uint module,threads,parent; public int priority; public uint flags;
        [MarshalAs(UnmanagedType.ByValTStr,SizeConst=260)] public string executable;
    }
    public static uint[] Children(uint parent) {
        var snapshot=CreateToolhelp32Snapshot(2,0);
        if(snapshot==new IntPtr(-1))throw new Win32Exception(Marshal.GetLastWin32Error(),"Process snapshot failed");
        try {var result=new List<uint>();var entry=new Entry();entry.size=(uint)Marshal.SizeOf(typeof(Entry));
            if(!ProcessFirst(snapshot,ref entry))throw new Win32Exception(Marshal.GetLastWin32Error(),"Process enumeration failed");
            do {if(entry.parent==parent)result.Add(entry.pid);}while(ProcessNext(snapshot,ref entry));return result.ToArray();
        } finally {CloseHandle(snapshot);}
    }
    private static bool Matches(IntPtr hwnd,uint pid,string title,string klass) {
        uint owner;GetWindowThreadProcessId(hwnd,out owner);
        if(owner!=pid)return false;var text=new StringBuilder(256);GetWindowText(hwnd,text,256);
        if(text.ToString()!=title)return false;
        if(!String.IsNullOrEmpty(klass)){text.Clear();GetClassName(hwnd,text,256);if(text.ToString()!=klass)return false;}return true;
    }
    public static IntPtr Find(uint pid,string title,string klass) {
        IntPtr result=IntPtr.Zero;EnumWindows((hwnd,data)=>{if(Matches(hwnd,pid,title,klass)){result=hwnd;return false;}return true;},IntPtr.Zero);return result;
    }
    private static void OwnedRunner(IntPtr hwnd,uint pid) {
        if(!Matches(hwnd,pid,"SymoCraft Benchmark","SymoCraftBenchmarkRunner"))throw new InvalidOperationException("Runner HWND/PID/title/class no longer match");
    }
    public static void Text(IntPtr hwnd,uint pid,int id,string text) {
        OwnedRunner(hwnd,pid);var control=GetDlgItem(hwnd,id);uint owner;GetWindowThreadProcessId(control,out owner);
        if(control==IntPtr.Zero || owner!=pid)throw new InvalidOperationException("Owned runner edit control is missing");
        UIntPtr result;if(SendString(control,0x000C,UIntPtr.Zero,text,2,1000,out result)==IntPtr.Zero || result==UIntPtr.Zero)
            throw new Win32Exception(Marshal.GetLastWin32Error(),"Owned runner edit update failed");
    }
    public static void Choice(IntPtr hwnd,uint pid,int id,uint index) {
        OwnedRunner(hwnd,pid);var control=GetDlgItem(hwnd,id);uint owner;GetWindowThreadProcessId(control,out owner);
        if(control==IntPtr.Zero || owner!=pid)throw new InvalidOperationException("Owned runner combo control is missing");
        UIntPtr result;if(SendMessageTimeout(control,0x014E,new UIntPtr(index),IntPtr.Zero,2,1000,out result)==IntPtr.Zero || result.ToUInt64()!=index)
            throw new Win32Exception(Marshal.GetLastWin32Error(),"Owned runner selection failed");
    }
    public static void Start(IntPtr hwnd,uint pid) {
        OwnedRunner(hwnd,pid);var button=GetDlgItem(hwnd,110);uint owner;GetWindowThreadProcessId(button,out owner);
        if(button==IntPtr.Zero || owner!=pid)throw new InvalidOperationException("Owned runner Start button is missing");
        UIntPtr result;if(SendMessageTimeout(hwnd,0x0111,new UIntPtr(110),button,2,1000,out result)==IntPtr.Zero)
            throw new Win32Exception(Marshal.GetLastWin32Error(),"Owned runner Start failed");
    }
    public static void CloseRunner(IntPtr hwnd,uint pid) {
        OwnedRunner(hwnd,pid);if(!PostMessage(hwnd,0x0010,IntPtr.Zero,IntPtr.Zero))throw new Win32Exception(Marshal.GetLastWin32Error(),"Owned runner WM_CLOSE failed");
    }
    public static uint QuietErrors() {return SetErrorMode(0x0001|0x0002|0x8000);}
    public static void RestoreErrors(uint value) {SetErrorMode(value);}
}
'@
}
function Read-SharedText([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return '' }
    $file = [IO.File]::Open($Path,[IO.FileMode]::Open,[IO.FileAccess]::Read,([IO.FileShare]::ReadWrite -bor [IO.FileShare]::Delete))
    try {
        $reader = [IO.StreamReader]::new($file)
        try { return $reader.ReadToEnd() } finally { $reader.Dispose() }
    } finally { $file.Dispose() }
}
function Package-Identity([string]$Package) {
    $prefix = $Package.TrimEnd([char[]]@('\','/')) + [IO.Path]::DirectorySeparatorChar
    return @(Get-ChildItem -LiteralPath $Package -File -Force -Recurse | Sort-Object FullName | ForEach-Object {
        [pscustomobject]@{ path=$_.FullName.Substring($prefix.Length).Replace('\','/'); sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant(); bytes=$_.Length }
    })
}
function Copy-Package([string]$Source,[string]$Destination,[switch]$WithoutCrt) {
    $prefix = $Source.TrimEnd([char[]]@('\','/')) + [IO.Path]::DirectorySeparatorChar
    New-Item -ItemType Directory -Path $Destination | Out-Null
    foreach ($file in Get-ChildItem -LiteralPath $Source -File -Force -Recurse) {
        $relative = $file.FullName.Substring($prefix.Length)
        if ($WithoutCrt -and $relative -match '^(msvcp140.*|vcruntime140.*|concrt140)\.dll$') { continue }
        $target = [IO.Path]::GetFullPath((Join-Path $Destination $relative))
        if (-not $target.StartsWith($Destination + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Fixture copy escaped its dedicated package.' }
        $parent = [IO.Path]::GetDirectoryName($target)
        if (-not (Test-Path -LiteralPath $parent)) { New-Item -ItemType Directory -Path $parent -Force | Out-Null }
        Copy-Item -LiteralPath $file.FullName -Destination $target
    }
}
function Invoke-ProtocolInspector([string]$Mode,[string]$Path,[string]$EvidenceBase) {
    $process=$null; $actualExit=$null; $forced=$false
    try {
        $process=Start-Process -FilePath $inspector -ArgumentList @($Mode,('"{0}"' -f $Path)) -WindowStyle Hidden -PassThru -RedirectStandardOutput "$EvidenceBase.json" -RedirectStandardError "$EvidenceBase.stderr.log"
        $null=$process.Handle
        if (-not $process.WaitForExit(10000)) { $forced=$true; $process.Kill(); throw 'Protocol inspector timed out.' }
        $process.WaitForExit();$process.Refresh();$actualExit=$process.ExitCode
        if ($null -eq $actualExit -or $actualExit -ne 0) { throw "Structured protocol inspector failed with exit code $actualExit; see $EvidenceBase.stderr.log" }
        $report=(Read-SharedText "$EvidenceBase.json") | ConvertFrom-Json
        if (-not $report.protocol) { throw 'Structured protocol inspector reported incompatible protocol.' }
        return $report
    } finally {
        if ($process) {
            if (-not $process.HasExited) { $forced=$true;$process.Kill();$process.WaitForExit() }
            $process.Refresh();if($null -eq $actualExit){$actualExit=$process.ExitCode}
            [ordered]@{exit_code=$actualExit;forced_termination=$forced;mode=$Mode;path=$Path} | ConvertTo-Json | Set-Content -LiteralPath "$EvidenceBase.process.json" -Encoding UTF8
            $process.Dispose()
        }
    }
}
function Invoke-FiniteGame([string]$Executable,[string]$Directory,[string]$Cwd,[int]$ExpectedExit,[int]$RequestedFrames,[string]$Diagnostic,[switch]$CrtObservation) {
    $process = $null; $actualExit = $null; $errorText = $null; $timeout = $false; $forced = $false
    $watch = [Diagnostics.Stopwatch]::StartNew()
    try {
        $prior = [S5DeliveryWindows]::QuietErrors()
        try {
            $process = Start-Process -FilePath $Executable -ArgumentList @('--smoke-frames',"$RequestedFrames") -WorkingDirectory $Cwd -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $Directory 'stdout.log') -RedirectStandardError (Join-Path $Directory 'stderr.log')
        } finally { [S5DeliveryWindows]::RestoreErrors($prior) }
        $null = $process.Handle
        $timeout = -not $process.WaitForExit($TimeoutSeconds*1000)
        if ($timeout) { $forced=$true; $process.Kill() }
        $process.WaitForExit(); $process.Refresh(); $actualExit = $process.ExitCode
    } catch { $errorText=$_.Exception.Message }
    finally {
        if ($process) {
            if (-not $process.HasExited) { $forced=$true; $process.Kill(); $process.WaitForExit() }
            $process.Refresh(); if ($null -eq $actualExit) { $actualExit=$process.ExitCode }
            $process.Dispose()
        }
        $watch.Stop()
    }
    $stdout = Read-SharedText (Join-Path $Directory 'stdout.log')
    $stderr = Read-SharedText (Join-Path $Directory 'stderr.log')
    $match = [regex]::Match($stdout,'\[runtime\] loop finished; rendered_frames=(\d+)')
    $framesRendered = if ($match.Success) { [int]$match.Groups[1].Value } else { $null }
    $ready = $stdout.Contains('[runtime] ready;')
    $shutdown = $stdout.Contains("[runtime] shutdown complete; exit_code=$ExpectedExit")
    $gl = $stdout.Contains('OpenGL vendor:') -and $stdout.Contains('OpenGL renderer:') -and $stdout.Contains('OpenGL version:')
    $passed = -not $timeout -and -not $forced -and $null -eq $errorText -and $null -ne $actualExit -and $actualExit -eq $ExpectedExit
    if ($ExpectedExit -eq 0) { $passed = $passed -and $ready -and $shutdown -and $gl -and $framesRendered -eq $RequestedFrames -and -not $stderr.Contains('[runtime] fatal:') -and -not $stderr.Contains('OpenGL diagnostic') }
    else { $passed=$passed -and -not $ready -and $shutdown -and $gl -and $stderr.Contains('[runtime] fatal:') -and $stderr.Contains($Diagnostic) }
    $state = if ($passed) { 'passed' } else { 'failed' }
    $scope = 'Finite-frame delivery diagnostic; not a 15-minute hardware acceptance run.'
    if ($CrtObservation -and -not $timeout -and -not $forced -and $null -eq $errorText -and $null -ne $actualExit) {
        $state='unavailable'; $passed=$false
        $scope='App-local CRT files omitted only in this dedicated copy. System/WinSxS fallback is allowed; installed developer machine cannot prove clean-machine missing-CRT behavior. No system files removed.'
    }
    return [pscustomobject][ordered]@{ state=$state; passed=$passed; executable=$Executable; working_directory=$Cwd; process_exit_code=$actualExit; expected_exit_code=$ExpectedExit; requested_frames=$RequestedFrames; rendered_frames=$framesRendered; timed_out=$timeout; forced_termination=$forced; ready=$ready; shutdown=$shutdown; actual_gl_initialized=$gl; expected_diagnostic=$Diagnostic; elapsed_seconds=$watch.Elapsed.TotalSeconds; error=$errorText; scope=$scope }
}

$gameIdentity = Package-Identity $gamePackagePath
$runnerIdentity = Package-Identity $runnerPackagePath
$identity = [ordered]@{ game_package=$gamePackagePath; runner_package=$runnerPackagePath; game_files=$gameIdentity; runner_files=$runnerIdentity; verifier_sha256=(Get-FileHash -LiteralPath $PSCommandPath).Hash.ToLowerInvariant(); protocol_inspector=$inspector; protocol_inspector_sha256=(Get-FileHash -LiteralPath $inspector).Hash.ToLowerInvariant() }
$identity | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $output 'input-identity.json') -Encoding UTF8
$cases = @(
    [pscustomobject]@{ Name='installed-finite-frames'; Source=$gamePackagePath; Fault=$null; Content=$null; Diagnostic=$null; SystemCwd=$false; Unicode=$false; Crt=$false },
    [pscustomobject]@{ Name='different-working-directory'; Source=$gamePackagePath; Fault=$null; Content=$null; Diagnostic=$null; SystemCwd=$true; Unicode=$false; Crt=$false },
    [pscustomobject]@{ Name='unicode-space-package'; Source=$gamePackagePath; Fault=$null; Content=$null; Diagnostic=$null; SystemCwd=$true; Unicode=$true; Crt=$false },
    [pscustomobject]@{ Name='invalid-vertex-shader'; Source=$gamePackagePath; Fault='assets/shaders/vs_BlockShader.glsl'; Content="#version 460 core`nthis is deliberately invalid GLSL;`n"; Diagnostic='Shader compilation failed:'; SystemCwd=$false; Unicode=$false; Crt=$false },
    [pscustomobject]@{ Name='invalid-texture'; Source=$gamePackagePath; Fault='assets/textures/texture_atlas.png'; Content="This dedicated fixture is not a PNG image.`n"; Diagnostic='Cannot decode image: '; SystemCwd=$false; Unicode=$false; Crt=$false },
    [pscustomobject]@{ Name='invalid-block-config'; Source=$gamePackagePath; Fault='assets/configs/blockFormats.yaml'; Content=''; Diagnostic='Failed to load block configuration: Block configuration must be a nonempty mapping'; SystemCwd=$false; Unicode=$false; Crt=$false },
    [pscustomobject]@{ Name='missing-app-local-crt-observation'; Source=$gamePackagePath; Fault=$null; Content=$null; Diagnostic=$null; SystemCwd=$true; Unicode=$false; Crt=$true }
)
$results = @()
foreach ($case in $cases) {
    $directory = Join-Path $output $case.Name
    New-Item -ItemType Directory -Path $directory | Out-Null
    $packageName = if ($case.Unicode) { (-join @([char]0x4E2D,[char]0x6587)) + ' package with spaces' } else { 'package' }
    $package = Join-Path $directory $packageName
    $errorText=$null; $result=$null
    try {
        Copy-Package $case.Source $package -WithoutCrt:$case.Crt
        if ($case.Fault) {
            $fault = [IO.Path]::GetFullPath((Join-Path $package $case.Fault))
            if (-not $fault.StartsWith($package + [IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)) { throw 'Fault file escaped its dedicated package.' }
            if (-not (Test-Path -LiteralPath $fault -PathType Leaf)) { throw "Fault target missing before injection: $($case.Fault)" }
            [IO.File]::WriteAllText($fault,$case.Content,[Text.UTF8Encoding]::new($false))
        }
        $cwd = if ($case.SystemCwd) { [Environment]::SystemDirectory } else { $directory }
        $expected = if ($case.Fault) { 3 } else { 0 }
        $requested = if ($case.Fault) { 1 } else { $Frames }
        $result = Invoke-FiniteGame (Join-Path $package 'SymoCraft.exe') $directory $cwd $expected $requested $case.Diagnostic -CrtObservation:$case.Crt
        if ($case.Fault) {
            $result | Add-Member -NotePropertyName fixture_fault_file -NotePropertyValue $fault
            $result | Add-Member -NotePropertyName fixture_fault_sha256 -NotePropertyValue (Get-FileHash -LiteralPath $fault -Algorithm SHA256).Hash.ToLowerInvariant()
            $result | Add-Member -NotePropertyName source_asset_sha256 -NotePropertyValue (Get-FileHash -LiteralPath (Join-Path $gamePackagePath $case.Fault) -Algorithm SHA256).Hash.ToLowerInvariant()
        }
        if ($case.Crt) {
            $result | Add-Member -NotePropertyName omitted_app_local_crt -NotePropertyValue @($gameIdentity | Where-Object { $_.path -match '^(msvcp140.*|vcruntime140.*|concrt140)\.dll$' } | ForEach-Object path)
            $result | Add-Member -NotePropertyName clean_machine_missing_crt_verified -NotePropertyValue $false
        }
    } catch { $errorText=$_.Exception.Message }
    if ($null -eq $result) { $result=[pscustomobject]@{state='failed';passed=$false;error=$errorText} }
    $result | Add-Member -NotePropertyName case -NotePropertyValue $case.Name
    $result | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
    $results += $result
}

$directory = Join-Path $output 'runner-cancel'
New-Item -ItemType Directory -Path $directory | Out-Null
$unicodePrefix = -join @([char]0x4E2D,[char]0x6587)
$runnerCopy = Join-Path $directory ($unicodePrefix + ' runner package')
$gameCopy = Join-Path $directory ($unicodePrefix + ' game package')
$sessionParent = Join-Path $directory ($unicodePrefix + ' result sessions')
$runner = $null; $child = $null; $runnerExit=$null; $childExit=$null; $errorText=$null; $forced=$false; $timeout=$false; $action=$false; $protocol=$null; $session=$null; $liveStatus=$null; $liveInspections=0; $childHwnd=[IntPtr]::Zero; $runnerHwnd=[IntPtr]::Zero
$timer=[Diagnostics.Stopwatch]::StartNew()
try {
    Copy-Package $runnerPackagePath $runnerCopy
    Copy-Package $gamePackagePath $gameCopy
    New-Item -ItemType Directory -Path $sessionParent | Out-Null
    $runner = Start-Process -FilePath (Join-Path $runnerCopy 'SymoCraftBenchmark.exe') -WorkingDirectory ([Environment]::SystemDirectory) -WindowStyle Hidden -PassThru -RedirectStandardOutput (Join-Path $directory 'runner.stdout.log') -RedirectStandardError (Join-Path $directory 'runner.stderr.log')
    $null=$runner.Handle
    $deadline=[DateTime]::UtcNow.AddSeconds(20)
    do {
        $runnerHwnd=[S5DeliveryWindows]::Find($runner.Id,'SymoCraft Benchmark','SymoCraftBenchmarkRunner')
        if ($runnerHwnd -ne [IntPtr]::Zero) { break }
        if ($runner.HasExited) { throw 'Runner exited before its owned GUI HWND was observed.' }
        Start-Sleep -Milliseconds 25
    } while ([DateTime]::UtcNow -lt $deadline)
    if ($runnerHwnd -eq [IntPtr]::Zero) { throw 'Owned runner GUI HWND timed out.' }
    [S5DeliveryWindows]::Text($runnerHwnd,$runner.Id,100,(Join-Path $gameCopy 'SymoCraft.exe'))
    [S5DeliveryWindows]::Text($runnerHwnd,$runner.Id,102,$sessionParent)
    [S5DeliveryWindows]::Text($runnerHwnd,$runner.Id,106,'s5-cancellation-diagnostic-not-acceptance')
    [S5DeliveryWindows]::Choice($runnerHwnd,$runner.Id,104,1)
    [S5DeliveryWindows]::Choice($runnerHwnd,$runner.Id,105,0)
    [S5DeliveryWindows]::Start($runnerHwnd,$runner.Id)
    $deadline=[DateTime]::UtcNow.AddSeconds([Math]::Min(60,$TimeoutSeconds))
    $ready=$false
    do {
        if ($runner.HasExited) { throw 'Runner exited before a cancellable live child was observed.' }
        $sessions=@(Get-ChildItem -LiteralPath $sessionParent -Directory)
        if ($sessions.Count -gt 1) { throw 'Unexpected multiple sessions in fresh fixture.' }
        if ($sessions.Count -eq 1) { $session=$sessions[0].FullName }
        if (-not $child) {
            foreach ($childId in [S5DeliveryWindows]::Children($runner.Id)) {
                $candidate=$null
                try {
                    $candidate=[Diagnostics.Process]::GetProcessById([int]$childId)
                    $null=$candidate.Handle
                    if ($candidate.MainModule.FileName.Equals((Join-Path $gameCopy 'SymoCraft.exe'),[StringComparison]::OrdinalIgnoreCase)) { $child=$candidate; $candidate=$null; break }
                } finally { if ($candidate) { $candidate.Dispose() } }
            }
        }
        if ($child -and $session) {
            $childHwnd=[S5DeliveryWindows]::Find($child.Id,'SymoCraft',$null)
            $attempt=Join-Path $session 'static-1-attempt-1'
            $ready=(Read-SharedText (Join-Path $attempt 'stdout.log')).Contains('[runtime] ready;')
            $statusPath=Join-Path $attempt 'capture/status.yaml'
            if ($ready -and $childHwnd -ne [IntPtr]::Zero -and (Test-Path -LiteralPath $statusPath -PathType Leaf)) {
                ++$liveInspections
                $liveStatus=Invoke-ProtocolInspector '--live-status' $statusPath (Join-Path $directory ('live-status-{0:d3}' -f $liveInspections))
                $ready=$liveStatus.live_phase
            } else { $ready=$false }
            if ($ready) { break }
            if ($child.HasExited) { throw 'Runner child exited before live protocol status and its owned HWND were observed.' }
        }
        Start-Sleep -Milliseconds 25
    } while ([DateTime]::UtcNow -lt $deadline)
    if (-not $ready -or -not $child -or -not $session) { throw 'Actual cancellable runner child/live-status readiness timed out.' }
    # Allow rendering after readiness; only the terminal structured summary proves actual collected frames.
    Start-Sleep -Milliseconds 250
    [S5DeliveryWindows]::CloseRunner($runnerHwnd,$runner.Id)
    $action=$true
    $timeout=-not $runner.WaitForExit(15000)
    if ($timeout) { throw 'Runner failed to stop normally within 15 seconds after its own WM_CLOSE.' }
    $runner.WaitForExit();$runner.Refresh();$runnerExit=$runner.ExitCode
    if (-not $child.WaitForExit(1000)) { throw 'Owned game child survived normal runner exit.' }
    $child.WaitForExit();$child.Refresh();$childExit=$child.ExitCode
    $protocol=Invoke-ProtocolInspector '--runner' $session (Join-Path $directory 'protocol')
    if ($null -eq $runnerExit -or $runnerExit -ne 0 -or $null -eq $childExit -or $childExit -ne 4) { throw 'Actual normal runner/game exit codes must be 0/4.' }
    if (-not (Read-SharedText (Join-Path $session 'static-1-attempt-1/stdout.log')).Contains('[runtime] shutdown complete; exit_code=4')) { throw 'Game normal cleanup marker is missing.' }
    $lock=Join-Path $session 'session.lock'
    $lockCheck=[IO.File]::Open($lock,[IO.FileMode]::Open,[IO.FileAccess]::ReadWrite,[IO.FileShare]::None)
    $lockCheck.Dispose()
} catch { $errorText=$_.Exception.Message }
finally {
    foreach ($process in @($child,$runner)) {
        if ($process) {
            if (-not $process.HasExited) { $forced=$true; $process.Kill(); $process.WaitForExit() }
            $process.Refresh()
            if ($process -eq $child) { $childExit=$process.ExitCode } else { $runnerExit=$process.ExitCode }
        }
    }
    $timer.Stop()
}
$passed=$null -eq $errorText -and $action -and -not $timeout -and -not $forced -and $null -ne $runnerExit -and $runnerExit -eq 0 -and $null -ne $childExit -and $childExit -eq 4
$runnerResult=[pscustomobject][ordered]@{ case='runner-cancel';state=$(if($passed){'passed'}else{'failed'});passed=$passed;runner_executable=(Join-Path $runnerCopy 'SymoCraftBenchmark.exe');game_executable=(Join-Path $gameCopy 'SymoCraft.exe');runner_process_id=$(if($runner){$runner.Id}else{$null});game_process_id=$(if($child){$child.Id}else{$null});runner_hwnd=$runnerHwnd.ToInt64();game_hwnd=$childHwnd.ToInt64();runner_exit_code=$runnerExit;game_exit_code=$childExit;own_runner_close_sent=$action;timed_out=$timeout;fixture_forced_termination=$forced;session_directory=$session;live_status_before_cancel=$liveStatus;protocol=$protocol;elapsed_seconds=$timer.Elapsed.TotalSeconds;error=$errorText;scope='Cancellation diagnostic for the existing GUI quick profile. Sending its owned WM_CLOSE/stop token requires structured live warmup/sampling readiness; a successful result also requires the final summary to prove actual frames and normal cleanup. This is not Q06 sampling or Q07 budget acceptance.' }
$runnerResult | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $directory 'result.json') -Encoding UTF8
$results += $runnerResult
foreach ($process in @($child,$runner)) { if($process){$process.Dispose()} }
$gameAfter=Package-Identity $gamePackagePath
$runnerAfter=Package-Identity $runnerPackagePath
$unchanged=($gameIdentity | ConvertTo-Json -Depth 6 -Compress) -eq ($gameAfter | ConvertTo-Json -Depth 6 -Compress) -and ($runnerIdentity | ConvertTo-Json -Depth 6 -Compress) -eq ($runnerAfter | ConvertTo-Json -Depth 6 -Compress)
$failed=@($results | Where-Object state -eq 'failed')
$unavailable=@($results | Where-Object state -eq 'unavailable')
$summary=[ordered]@{ game_package=$gamePackagePath;runner_package=$runnerPackagePath;source_packages_unchanged=$unchanged;total_cases=$results.Count;passed_cases=@($results | Where-Object state -eq 'passed').Count;failed_cases=$failed.Count;unavailable_cases=$unavailable.Count;verified_cases_passed=$failed.Count -eq 0 -and $unchanged;all_requested_checks_passed=$failed.Count -eq 0 -and $unavailable.Count -eq 0 -and $unchanged;hardware_acceptance_passed=$false;q06_acceptance_sample=$false;q07_budget_acceptance=$false;y9000p_checked=$false;fixtures_and_first_failures_retained=$true;cases=$results }
$summary | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding UTF8
$summary | ConvertTo-Json -Depth 12 | Write-Output
if ($failed.Count -ne 0 -or -not $unchanged) { throw "SDL delivery verification failed; inspect retained evidence: $output" }
if ($unavailable.Count -ne 0) { Write-Warning 'Missing-CRT clean-machine behavior remains unverified; this run is not overall delivery acceptance.' }

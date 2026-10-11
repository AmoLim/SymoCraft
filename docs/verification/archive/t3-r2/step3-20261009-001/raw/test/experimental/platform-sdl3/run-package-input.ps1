[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$OutputDirectory)
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
$package=$PSScriptRoot
$identityPath=Join-Path $package 'package-identity.json'
$identity=Get-Content -LiteralPath $identityPath -Raw | ConvertFrom-Json
foreach($entry in $identity.files){
    $path=Join-Path $package $entry.path
    if((Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256){throw "Candidate file differs from its manifest: $($entry.path)"}
}
$output=[IO.Path]::GetFullPath($OutputDirectory)
if(Test-Path -LiteralPath $output){throw 'Choose a fresh observation directory.'}
New-Item -ItemType Directory -Path $output | Out-Null
$exe=Join-Path $package 'SymoCraftPlatformInputTests.exe'
[ordered]@{
    started_utc=[DateTime]::UtcNow.ToString('o');machine=$env:COMPUTERNAME
    package_identity_sha256=(Get-FileHash -LiteralPath $identityPath).Hash.ToLowerInvariant()
    input_executable_sha256=(Get-FileHash -LiteralPath $exe).Hash.ToLowerInvariant()
    game_executable_sha256=(Get-FileHash -LiteralPath (Join-Path $package 'SymoCraft.exe')).Hash.ToLowerInvariant()
    production_provenance=$identity.production_imports
    scope='Portable verified candidate; provenance is the original build, not a rebuild on this machine. Physical observations are separate from human acceptance.'
    human_acceptance_passed=$false
}|ConvertTo-Json -Depth 9|Set-Content -LiteralPath "$output/manual-session-identity.json" -Encoding UTF8
$names=@('SDL_VIDEO_DRIVER','SDL_OPENGL_LIBRARY','SDL_VULKAN_LIBRARY','SDL3_DYNAMIC_API','SDL_WINDOWS_RAW_KEYBOARD')
$saved=@{}
foreach($name in $names){$saved[$name]=[Environment]::GetEnvironmentVariable($name,'Process')}
try{
    foreach($name in $names){Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue}
    Write-Host 'Click the color window. F5 Reset; F6 Normal/Hidden/Lock; F7 second Capture. Test Escape last.'
    & $exe '--case' 'hardware-manual' '--output-directory' $output
    $exitCode=$LASTEXITCODE
    [ordered]@{finished_utc=[DateTime]::UtcNow.ToString('o');exit_code=$exitCode;human_acceptance_passed=$false}|
        ConvertTo-Json|Set-Content -LiteralPath "$output/manual-launch-result.json" -Encoding UTF8
    if($exitCode -ne 0){throw "Observation tool failed with exit $exitCode; see $output"}
}finally{
    foreach($name in $names){
        if($null -eq $saved[$name]){Remove-Item -LiteralPath "Env:$name" -ErrorAction SilentlyContinue}
        else{[Environment]::SetEnvironmentVariable($name,$saved[$name],'Process')}
    }
}

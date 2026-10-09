[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string]$Archive,
    [Parameter(Mandatory = $true)][string]$VendorDirectory,
    [Parameter(Mandatory = $true)][string]$OutputFile
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.IO.Compression.FileSystem
$vendor = (Resolve-Path -LiteralPath $VendorDirectory).Path
$zip = [IO.Compression.ZipFile]::OpenRead((Resolve-Path -LiteralPath $Archive).Path)
$files = [Collections.Generic.List[object]]::new()
$archivePaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::Ordinal)
try {
    foreach ($entry in $zip.Entries) {
        if ($entry.FullName.EndsWith('/')) { continue }
        $slash = $entry.FullName.IndexOf('/')
        if ($slash -lt 0) { throw 'Expected one top-level archive directory.' }
        $relative = $entry.FullName.Substring($slash + 1)
        [void]$archivePaths.Add($relative)
        $stream = $entry.Open()
        $sha = [Security.Cryptography.SHA256]::Create()
        try { $digest = [BitConverter]::ToString($sha.ComputeHash($stream)).Replace('-', '').ToLowerInvariant() }
        finally { $sha.Dispose(); $stream.Dispose() }
        $local = Join-Path $vendor $relative
        $exists = Test-Path -LiteralPath $local -PathType Leaf
        $localHash = if ($exists) { (Get-FileHash -LiteralPath $local -Algorithm SHA256).Hash.ToLowerInvariant() } else { $null }
        $files.Add([ordered]@{
            path = $relative; archive_sha256 = $digest; local_sha256 = $localHash
            status = if (-not $exists) { 'missing' } elseif ($digest -eq $localHash) { 'identical' } else { 'different' }
        })
    }
} finally { $zip.Dispose() }
$extra = @(Get-ChildItem -LiteralPath $vendor -File -Recurse -Force | ForEach-Object {
    $relative = $_.FullName.Substring($vendor.Length + 1).Replace('\', '/')
    if (-not $archivePaths.Contains($relative)) { $relative }
})
$result = [ordered]@{
    captured_at = (Get-Date).ToString('o')
    archive_sha256 = (Get-FileHash -LiteralPath $Archive -Algorithm SHA256).Hash.ToLowerInvariant()
    archive_files = $files.Count
    identical = @($files | Where-Object status -eq 'identical').Count
    different = @($files | Where-Object status -eq 'different').Count
    missing = @($files | Where-Object status -eq 'missing').Count
    extra = $extra
    files = @($files.ToArray())
}
$result | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath $OutputFile -Encoding UTF8
[pscustomobject]$result | Select-Object archive_sha256,archive_files,identical,different,missing,extra | ConvertTo-Json -Depth 3

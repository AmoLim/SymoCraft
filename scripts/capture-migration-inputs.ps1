[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Split-Path $PSScriptRoot)).Path
$output = [IO.Path]::GetFullPath((Join-Path $root $OutputDirectory))
if (-not $output.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'The snapshot must stay inside the workspace.'
}
if (Test-Path -LiteralPath $output) {
    throw 'Use a new directory; existing migration evidence is never overwritten.'
}

Push-Location $root
try {
    $paths = @('CMakeLists.txt', 'CMakePresets.json',
        'docs/project/benchmark/使用与交付.md', 'docs/legacy/third-party-inventory.md') + @(
        & rg --files --hidden -g '!.git/**' -g '!test/experimental/**' game cmake test scripts tools/benchmark vendor assets
    )
    if ($LASTEXITCODE -ne 0) { throw 'Input inventory failed.' }
    $source = Join-Path $output 'source'
    New-Item -ItemType Directory -Path $source | Out-Null
    $files = @($paths | Sort-Object -Unique | ForEach-Object {
        $original = Get-Item -LiteralPath $_
        $hash = (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant()
        $destination = Join-Path $source $_
        New-Item -ItemType Directory -Path (Split-Path $destination) -Force | Out-Null
        Copy-Item -LiteralPath $_ -Destination $destination
        if ((Get-FileHash -LiteralPath $destination -Algorithm SHA256).Hash.ToLowerInvariant() -ne $hash -or
            (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash.ToLowerInvariant() -ne $hash) {
            throw "Input changed during capture: $_"
        }
        [ordered]@{path = $_.Replace('\', '/'); bytes = $original.Length; sha256 = $hash}
    })
    $head = & git rev-parse HEAD
    if ($LASTEXITCODE -ne 0) { throw 'Cannot read Git identity.' }
    & git status --porcelain=v1 | Set-Content -LiteralPath (Join-Path $output 'git-status.txt') -Encoding UTF8
    & git diff --binary -- . ':!vendor/sdl3' | Set-Content -LiteralPath (Join-Path $output 'tracked-changes.patch') -Encoding UTF8
    if ($LASTEXITCODE -ne 0) { throw 'Cannot capture tracked changes.' }
    [ordered]@{
        captured_at = (Get-Date).ToString('o')
        git_head = $head
        scope = 'Conservative root CMake/presets, game, cmake, test (excluding isolated experimental probes), scripts, tools/benchmark, vendor, assets and the two installed benchmark documents. Includes dirty and untracked inputs; not a compiler-read trace.'
        restore_policy = 'Reference only. Never reset or overwrite the live worktree automatically.'
        count = $files.Count
        files = $files
    } | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $output 'build-inputs.json') -Encoding UTF8
    [ordered]@{
        output = $output
        count = $files.Count
        manifest_sha256 = (Get-FileHash -LiteralPath (Join-Path $output 'build-inputs.json') -Algorithm SHA256).Hash.ToLowerInvariant()
        byte_verified = $true
    } | ConvertTo-Json
} finally {
    Pop-Location
}

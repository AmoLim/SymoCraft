Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$output = Join-Path $root 'out/m3-t2'
$evidence = Join-Path $root 'docs/milestones/m3-t2/evidence'
if (Test-Path -LiteralPath $evidence) { throw 'Do not overwrite curated preparation evidence.' }
New-Item -ItemType Directory -Path $evidence | Out-Null
function Digest([string]$Path) {
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}
Push-Location $root
try {
    $comparison = Get-Content -LiteralPath (Join-Path $output 'sdl-official-vendor-final-comparison.json') -Raw | ConvertFrom-Json
    if ($comparison.different -or $comparison.missing -or $comparison.extra.Count) { throw 'SDL source comparison failed.' }
    [ordered]@{
        captured_at = (Get-Date).ToString('o')
        version = '3.4.18'; tag = 'release-3.4.18'
        commit = (Get-Content -LiteralPath 'vendor/sdl3/.git-hash' -Raw).Trim()
        revision = (Get-Content -LiteralPath 'vendor/sdl3/REVISION.txt' -Raw).Trim()
        source_url = 'https://github.com/libsdl-org/SDL/releases/download/release-3.4.18/SDL3-3.4.18.zip'
        archive_sha256 = $comparison.archive_sha256
        vendor_files = $comparison.archive_files; byte_identical_files = $comparison.identical
        comparison_path = 'out/m3-t2/sdl-official-vendor-final-comparison.json'
        comparison_sha256 = Digest (Join-Path $output 'sdl-official-vendor-final-comparison.json')
        license_path = 'vendor/sdl3/LICENSE.txt'; license_sha256 = Digest 'vendor/sdl3/LICENSE.txt'
        vendor_source_modified = $false; old_sdl_directory_retained = $false
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $evidence 'sdl-provenance.json') -Encoding UTF8

    $inputs = @('cmake/ThirdPartyHeaders.cmake', 'game/modules/platform/include/symocraft/platform/window.h',
        'scripts/capture-migration-inputs.ps1', 'scripts/verify-runtime-faults.ps1') + @(
        & rg --files test/experimental vendor/glad vendor/KHR
    )
    if ($LASTEXITCODE -ne 0) { throw 'Preparation input inventory failed.' }
    $files = @($inputs | Sort-Object -Unique | ForEach-Object {
        [ordered]@{ path = $_.Replace('\', '/'); sha256 = Digest $_ }
    })
    [ordered]@{ captured_at = (Get-Date).ToString('o'); scope = 'Preparation sources and helpers, GLAD/header inputs; full SDL source is byte-bound by the official archive manifest. Not a compiler-read trace.'; files = $files } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'preparation-inputs.json') -Encoding UTF8

    $package = Join-Path $output 'install/glfw-release'
    $archive = Join-Path $output 'Symocraft-M3-T2-GLFW-baseline-windows-x64.zip'
    if (Test-Path -LiteralPath $archive) { throw 'Baseline archive already exists.' }
    $installed = @(Get-ChildItem -LiteralPath $package -File -Recurse | ForEach-Object {
        [ordered]@{ path = $_.FullName.Substring($package.Length + 1).Replace('\', '/'); sha256 = Digest $_.FullName }
    })
    Compress-Archive -Path (Join-Path $package '*') -DestinationPath $archive
    [ordered]@{
        captured_at = (Get-Date).ToString('o'); git_head = (& git rev-parse HEAD); dirty_input = $true
        baseline_manifest = 'out/m3-t2/baseline-input-001/build-inputs.json'
        baseline_manifest_sha256 = Digest (Join-Path $output 'baseline-input-001/build-inputs.json')
        snapshot_exclusions = 'out/m3-t2/baseline-input-001/snapshot-exclusions.json'
        install_supplement = 'out/m3-t2/baseline-input-001/install-input-supplement.json'
        debug_game_sha256 = Digest (Join-Path $output 'build/glfw-debug/bin/SymoCraft.exe')
        release_game_sha256 = Digest (Join-Path $package 'SymoCraft.exe')
        runner_sha256 = Digest (Join-Path $package 'SymoCraftBenchmark.exe')
        platform_archive_sha256 = Digest (Join-Path $output 'build/glfw-release/game/modules/platform/symocraft_platform.lib')
        zip_path = 'out/m3-t2/Symocraft-M3-T2-GLFW-baseline-windows-x64.zip'; zip_sha256 = Digest $archive
        installed_files = $installed
    } | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $evidence 'glfw-baseline-identity.json') -Encoding UTF8

    $copies = [ordered]@{
        'glfw-debug-test-retry.log' = 'glfw-debug-test.log'
        'glfw-release-test.log' = 'glfw-release-test.log'
        'glfw-release-install-retry.log' = 'glfw-release-install.log'
        'sdl-gl-debug-final-test-002.log' = 'sdl-gl-debug-test.log'
        'sdl-vulkan-debug-final-test-002.log' = 'sdl-vulkan-debug-test.log'
        'sdl-gl-release-clean-final-test.log' = 'sdl-gl-release-test.log'
        'sdl-gl-release-final-install.log' = 'sdl-gl-release-install.log'
        'sdl-gl-release-final-dependents.log' = 'sdl-gl-release-dependents.log'
        'sdl-vulkan-debug-dependents.log' = 'sdl-vulkan-debug-dependents.log'
        'probe/gl-debug-runtime-final/summary.json' = 'sdl-gl-debug-probes.json'
        'probe/vulkan-debug-runtime-final/summary.json' = 'sdl-vulkan-debug-probes.json'
        '独立部署 含空格/runtime/summary.json' = 'sdl-release-installed-probes.json'
        'runtime/glfw-debug-001/result.json' = 'glfw-debug-smoke.json'
        'runtime/glfw-release-001/result.json' = 'glfw-release-smoke.json'
        'runtime/glfw-faults-002/summary.json' = 'glfw-faults.json'
        'glfw-api-runtime-final/result.json' = 'glfw-api-probe.json'
        'glfw-api-runtime-final/stdout.log' = 'glfw-api-probe.stdout.log'
        'probe/gl-debug/Testing/Temporary/LastTest.log' = 'sdl-input-candidate.log'
    }
    foreach ($item in $copies.GetEnumerator()) {
        Copy-Item -LiteralPath (Join-Path $output $item.Key) -Destination (Join-Path $evidence $item.Value)
    }
    $identities = @('probe/gl-debug/SymoCraftSdl3Probe.exe', 'probe/vulkan-debug/SymoCraftSdl3Probe.exe',
        'install/sdl-gl-probe-release/SymoCraftSdl3Probe.exe') | ForEach-Object {
        [ordered]@{ path = "out/m3-t2/$_"; sha256 = Digest (Join-Path $output $_) }
    }
    [ordered]@{ captured_at = (Get-Date).ToString('o'); executables = @($identities) } |
        ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $evidence 'sdl-probe-identities.json') -Encoding UTF8
    if (@(Get-ChildItem -LiteralPath $evidence -File -Recurse | Where-Object Extension -in '.exe','.dll','.lib','.pdb','.obj','.zip').Count) {
        throw 'Binary artifact found in documentation evidence.'
    }
    Write-Output "Curated preparation evidence: $evidence"
} finally { Pop-Location }

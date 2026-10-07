[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$Executable,
    [Parameter(Mandatory = $true)]
    [ValidateNotNullOrEmpty()]
    [string]$OutputDirectory
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'

$executableItem = Get-Item -LiteralPath $Executable
if ($executableItem.PSIsContainer) {
    throw "Executable must be a file: $Executable"
}
$sourceExecutable = $executableItem.FullName
$sourceAssets = (Get-Item -LiteralPath (Join-Path $executableItem.DirectoryName 'assets')).FullName
if (-not (Test-Path -LiteralPath $sourceAssets -PathType Container)) {
    throw "The executable's adjacent assets directory is missing: $sourceAssets"
}
$probeScript = Join-Path $PSScriptRoot 'verify-runtime.ps1'
if (-not (Test-Path -LiteralPath $probeScript -PathType Leaf)) {
    throw "Runtime probe script is missing: $probeScript"
}

$output = [IO.Path]::GetFullPath($OutputDirectory)
if (Test-Path -LiteralPath $output) {
    throw "Use a new output directory; existing evidence will not be overwritten: $output"
}
$separator = [IO.Path]::DirectorySeparatorChar
$sourceAssetsPrefix = $sourceAssets.TrimEnd([char[]]@('\', '/')) + $separator
if ($output.StartsWith($sourceAssetsPrefix, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'OutputDirectory cannot be inside source assets because fixtures copy that directory.'
}

$cases = @(
    [pscustomobject]@{
        Name = 'invalid-vertex-shader'
        Asset = 'shaders/vs_BlockShader.glsl'
        Content = "#version 460 core`nthis is deliberately invalid GLSL;`n"
        Diagnostic = 'Shader compilation failed:'
    },
    [pscustomobject]@{
        Name = 'invalid-texture-image'
        Asset = 'textures/texture_atlas.png'
        Content = "This dedicated test fixture is not a PNG image.`n"
        Diagnostic = 'Cannot decode image: '
    },
    [pscustomobject]@{
        Name = 'empty-block-configuration'
        Asset = 'configs/blockFormats.yaml'
        Content = ''
        Diagnostic = 'Failed to load block configuration: Block configuration must be a nonempty mapping'
    }
)

foreach ($case in $cases) {
    $sourceAsset = Join-Path $sourceAssets $case.Asset
    if (-not (Test-Path -LiteralPath $sourceAsset -PathType Leaf)) {
        throw "Required source asset is missing before fault injection: $($case.Asset)"
    }
    $case | Add-Member -NotePropertyName SourceSha256 `
        -NotePropertyValue (Get-FileHash -LiteralPath $sourceAsset -Algorithm SHA256).Hash
}

New-Item -ItemType Directory -Path $output | Out-Null
$utf8 = [Text.UTF8Encoding]::new($false)
$results = @()

foreach ($case in $cases) {
    $fixture = Join-Path $output $case.Name
    $package = Join-Path $fixture 'package'
    $evidence = Join-Path $fixture 'evidence'
    New-Item -ItemType Directory -Path $fixture | Out-Null
    New-Item -ItemType Directory -Path $package | Out-Null
    $fixtureExecutable = Join-Path $package $executableItem.Name
    $fixtureAssets = Join-Path $package 'assets'
    Copy-Item -LiteralPath $sourceExecutable -Destination $fixtureExecutable
    Copy-Item -LiteralPath $sourceAssets -Destination $fixtureAssets -Recurse

    $faultFile = [IO.Path]::GetFullPath((Join-Path $fixtureAssets $case.Asset))
    $fixturePrefix = [IO.Path]::GetFullPath($package).TrimEnd([char[]]@('\', '/')) + $separator
    if (-not $faultFile.StartsWith($fixturePrefix, [StringComparison]::OrdinalIgnoreCase)) {
        throw "Refusing to alter a file outside the dedicated fixture: $faultFile"
    }
    [IO.File]::WriteAllText($faultFile, $case.Content, $utf8)

    $probeError = $null
    try {
        & $probeScript -Executable $fixtureExecutable -OutputDirectory $evidence `
            -Frames 1 -ExpectedExitCode 3 | Out-Null
    } catch {
        $probeError = $_.Exception.Message
    }

    $issues = [Collections.Generic.List[string]]::new()
    if ($null -ne $probeError) {
        $issues.Add("Runtime probe failed: $probeError")
    }
    $resultFile = Join-Path $evidence 'result.json'
    $stdoutFile = Join-Path $evidence 'stdout.log'
    $stderrFile = Join-Path $evidence 'stderr.log'
    $probeResult = $null
    $stdout = ''
    $stderr = ''
    if (Test-Path -LiteralPath $resultFile -PathType Leaf) {
        $probeResult = Get-Content -LiteralPath $resultFile -Raw | ConvertFrom-Json
        if ($probeResult.exitCode -ne 3 -or $probeResult.timedOut -or
            $probeResult.ready -or -not $probeResult.shutdown -or -not $probeResult.passed) {
            $issues.Add('The game did not fail before readiness with exit code 3 and a clean shutdown marker.')
        }
    } else {
        $issues.Add('The runtime probe did not produce result.json.')
    }
    if (Test-Path -LiteralPath $stdoutFile -PathType Leaf) {
        $stdout = [string](Get-Content -LiteralPath $stdoutFile -Raw)
    }
    if (Test-Path -LiteralPath $stderrFile -PathType Leaf) {
        $stderr = [string](Get-Content -LiteralPath $stderrFile -Raw)
    }
    foreach ($marker in @('OpenGL vendor:', 'OpenGL renderer:', 'OpenGL version:')) {
        if (-not $stdout.Contains($marker)) {
            $issues.Add("Missing proof of actual OpenGL initialization: $marker")
        }
    }
    if (-not $stderr.Contains('[runtime] fatal:') -or -not $stderr.Contains($case.Diagnostic)) {
        $issues.Add("Missing expected fault diagnostic: $($case.Diagnostic)")
    }
    if (-not $stdout.Contains('[runtime] shutdown complete; exit_code=3')) {
        $issues.Add('Missing the final runtime cleanup marker with exit code 3.')
    }
    $sourceHashAfter = (Get-FileHash -LiteralPath (Join-Path $sourceAssets $case.Asset) -Algorithm SHA256).Hash
    if ($sourceHashAfter -ne $case.SourceSha256) {
        $issues.Add('The source asset changed during the probe; investigate before accepting this evidence.')
    }

    $caseResult = [ordered]@{
        case = $case.Name
        modifiedFixtureAsset = $faultFile
        expectedDiagnostic = $case.Diagnostic
        sourceSha256Before = $case.SourceSha256
        sourceSha256After = $sourceHashAfter
        runtimeResult = $probeResult
        passed = $issues.Count -eq 0
        issues = @($issues.ToArray())
    }
    $caseResult | ConvertTo-Json -Depth 8 |
        Set-Content -LiteralPath (Join-Path $fixture 'fault-result.json') -Encoding UTF8
    $results += [pscustomobject]$caseResult
}

$failedCases = @($results | Where-Object { -not $_.passed })
$summary = [ordered]@{
    sourceExecutable = $sourceExecutable
    sourceAssets = $sourceAssets
    checkedSourceAssetsModified = @($results | Where-Object { $_.sourceSha256Before -ne $_.sourceSha256After }).Count -ne 0
    fixtureDirectoriesRetained = $true
    passed = $failedCases.Count -eq 0
    cases = $results
}
$summary | ConvertTo-Json -Depth 10 |
    Set-Content -LiteralPath (Join-Path $output 'summary.json') -Encoding UTF8
$summary | ConvertTo-Json -Depth 10 | Write-Output
if ($failedCases.Count -ne 0) {
    throw "Runtime fault verification failed for $($failedCases.Count) case(s); see $output"
}

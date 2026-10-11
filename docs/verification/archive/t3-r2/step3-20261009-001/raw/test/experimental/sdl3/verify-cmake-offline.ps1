[CmdletBinding()]
param(
    [string]$CMakePath = 'out/m3-t2/tools/cmake-3.22.6/portable/cmake-3.22.6-windows-x86_64/bin/cmake.exe',
    [string]$BuildRoot,
    [ValidateSet('Debug', 'Release')]
    [string[]]$Configurations = @('Debug', 'Release'),
    [ValidateSet('gl', 'vulkan')]
    [string[]]$Modes = @('gl', 'vulkan'),
    [string]$HeadersRoot = 'vendor/sdl3/src/video/khronos',
    [string]$NinjaPath = 'E:/Applications/JetBrains/CLion/bin/ninja/win/x64/ninja.exe',
    [string]$VisualStudioPath = 'E:/Applications/Microsoft VS/2022',
    [ValidatePattern('^14\.[0-9]+(\.[0-9]+)?$')]
    [string]$ToolsetVersion = '14.38',
    [ValidateRange(1, 64)]
    [int]$Jobs = 4
)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
$outRoot = [IO.Path]::GetFullPath((Join-Path $root 'out/m3-t2'))
if ([Environment]::OSVersion.Platform -ne [PlatformID]::Win32NT -or -not [Environment]::Is64BitProcess) {
    throw 'This audit requires 64-bit Windows PowerShell and the declared MSVC/Windows SDK toolchain.'
}

function Resolve-WorkspacePath([string]$Path) {
    if (-not [IO.Path]::IsPathRooted($Path)) { $Path = Join-Path $root $Path }
    return [IO.Path]::GetFullPath($Path)
}

function Save-Json([object]$Value, [string]$Path) {
    ConvertTo-Json -InputObject $Value -Depth 12 | Set-Content -LiteralPath $Path -Encoding UTF8
}

function Invoke-Logged([string]$Program, [string[]]$Arguments, [string]$Log) {
    Write-Host "Audit step: $(Split-Path $Program -Leaf) $($Arguments[0])"
    $previousPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = @(& $Program @Arguments 2>&1 | ForEach-Object { $_.ToString() })
        $code = $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $previousPreference
    }
    @("Program: $Program", "Arguments: $($Arguments -join ' ')", "ExitCode: $code", '') + $output |
        Set-Content -LiteralPath $Log -Encoding UTF8
    if ($code -ne 0) { throw "Audit step failed ($code); see $Log" }
    return $output
}

function Get-InputManifest {
    $directories = @(
        (Join-Path $root 'vendor/sdl3'),
        (Join-Path $root 'vendor/glad'),
        (Join-Path $root 'vendor/KHR'),
        (Join-Path $root 'game/modules/platform/include')
    )
    $files = @((Join-Path $root 'cmake/ThirdPartyHeaders.cmake'))
    foreach ($name in @('CMakeLists.txt', 'main.cpp', 'input_candidate.h', 'input_candidate_tests.cpp', 'runtime_input_tests.cpp', 'build.ps1', 'verify-cmake-offline.ps1')) {
        $files += Join-Path $PSScriptRoot $name
    }
    foreach ($directory in $directories) {
        if (Test-Path -LiteralPath $directory -PathType Container) {
            $files += @(Get-ChildItem -LiteralPath $directory -File -Recurse | Select-Object -ExpandProperty FullName)
        }
    }
    $manifest = [ordered]@{}
    foreach ($file in @($files | Sort-Object -Unique)) {
        if (Test-Path -LiteralPath $file -PathType Leaf) {
            $manifest[$file.Substring($root.Length + 1).Replace('\', '/')] = (Get-FileHash -LiteralPath $file -Algorithm SHA256).Hash
        }
    }
    return $manifest
}

function Compare-Manifests([object]$Before, [object]$After) {
    $changed = @()
    foreach ($key in @(@($Before.Keys) + @($After.Keys) | Sort-Object -Unique)) {
        if (-not $Before.Contains($key) -or -not $After.Contains($key) -or $Before[$key] -ne $After[$key]) { $changed += $key }
    }
    return $changed
}

function Audit-Trace([string]$Path) {
    $networkCommands = @()
    $processCalls = @()
    $commands = 0
    foreach ($line in [IO.File]::ReadLines($Path)) {
        $entry = $line | ConvertFrom-Json
        if (-not $entry.PSObject.Properties['cmd']) { continue }
        ++$commands
        $command = $entry.cmd.ToLowerInvariant()
        $arguments = @($entry.args)
        $joined = $arguments -join ' '
        $isNetwork = ($command -eq 'file' -and $arguments.Count -gt 0 -and $arguments[0] -eq 'DOWNLOAD') -or
            ($command -match '^(externalproject_add|fetchcontent_declare|fetchcontent_populate|fetchcontent_makeavailable)$') -or
            ($command -eq 'execute_process' -and $joined -match '(?i)(https?://|\b(curl|wget|Invoke-WebRequest)\b|\bgit(?:\.exe)?\b[^;]*(?:\bclone\b|\bfetch\b|\bpull\b))')
        if ($isNetwork) { $networkCommands += [ordered]@{ file = $entry.file; line = $entry.line; command = $command; args = $arguments } }
        if ($command -eq 'execute_process') { $processCalls += [ordered]@{ file = $entry.file; line = $entry.line; args = $arguments } }
    }
    return [ordered]@{ executed_commands = $commands; network_or_population_commands = $networkCommands; execute_process_calls = $processCalls }
}

$cmake = (Get-Item -LiteralPath (Resolve-WorkspacePath $CMakePath)).FullName
$ctest = (Get-Item -LiteralPath (Join-Path (Split-Path $cmake) 'ctest.exe')).FullName
$ninja = (Get-Item -LiteralPath $NinjaPath).FullName
$headers = (Get-Item -LiteralPath (Resolve-WorkspacePath $HeadersRoot)).FullName
if ($Modes -contains 'vulkan' -and -not (Test-Path -LiteralPath (Join-Path $headers 'vulkan/vulkan_core.h'))) {
    throw 'The declared Vulkan header source must contain vulkan/vulkan_core.h; this audit does not install an SDK.'
}
if (-not $BuildRoot) { $BuildRoot = "out/m3-t2/cmake-3.22-offline-audit-$(Get-Date -Format 'yyyyMMdd-HHmmss')" }
$auditRoot = Resolve-WorkspacePath $BuildRoot
if (-not $auditRoot.StartsWith($outRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
    throw 'Use a fresh audit directory below workspace out/m3-t2.'
}
if (Test-Path -LiteralPath $auditRoot) { throw 'The audit root must not exist; do not overwrite previous evidence or caches.' }
New-Item -ItemType Directory -Path $auditRoot | Out-Null

$summary = [ordered]@{
    scope = 'Isolated SDL preparation project, not production migration acceptance'
    network_limit = 'The user network remains connected. Process-only proxy refusal and Git file-only transport are partial constraints, not a network sandbox. Trace/static/generated-rule inspection proves no download step for the tested configure/build path; tools ignoring these settings are not blocked.'
    cmake_path = $cmake
    cmake_sha256 = (Get-FileHash -LiteralPath $cmake -Algorithm SHA256).Hash
    cmake_version = ''
    sdl_version_macros = @((Select-String -LiteralPath (Join-Path $root 'vendor/sdl3/include/SDL3/SDL_version.h') -Pattern '^#define SDL_(MAJOR|MINOR|MICRO)_VERSION ').Line)
    sdl_revision_macros = @((Select-String -LiteralPath (Join-Path $root 'vendor/sdl3/include/SDL3/SDL_revision.h') -Pattern '^#define SDL_REVISION ').Line)
    ninja_path = $ninja
    ninja_sha256 = (Get-FileHash -LiteralPath $ninja -Algorithm SHA256).Hash
    ninja_version = ''
    compiler = ''
    compiler_sha256 = ''
    msvc_toolset = $ToolsetVersion
    windows_sdk_version = ''
    vulkan_headers = $headers
    vulkan_header_version = (Select-String -LiteralPath (Join-Path $headers 'vulkan/vulkan_core.h') -Pattern '^#define VK_HEADER_VERSION ').Line
    vulkan_headers_are_installed_sdk = $false
    continuous_input_comparison = 'Explicitly OFF; this independent probe has no frozen GLFW/camera archive prerequisite'
    executed_ctest_filter = '^sdl3-input-candidate$; runtime/window targets may compile but are never launched by this audit'
    proxy = 'http://127.0.0.1:1'
    git_allow_protocol = 'file'
    static_cmake_files = @()
    cases = @()
    completed = $false
    error = $null
}
$savedEnvironment = @{}
Get-ChildItem Env: | ForEach-Object { $savedEnvironment[$_.Name] = $_.Value }
$locationPushed = $false
try {
    $summary.cmake_version = (Invoke-Logged $cmake @('--version') (Join-Path $auditRoot 'cmake-version.log')) -join "`n"
    if ($summary.cmake_version -notmatch '^cmake version 3\.22\.') { throw 'This evidence must use actual CMake 3.22.x.' }
    $summary.ninja_version = (Invoke-Logged $ninja @('--version') (Join-Path $auditRoot 'ninja-version.log')) -join "`n"
    Import-Module (Join-Path $VisualStudioPath 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll') -ErrorAction Stop
    Enter-VsDevShell -VsInstallPath $VisualStudioPath -SkipAutomaticLocation `
        -DevCmdArguments "-arch=x64 -host_arch=x64 -vcvars_ver=$ToolsetVersion" | Out-Null
    $summary.compiler = (Get-Command cl.exe -CommandType Application).Source
    $summary.compiler_sha256 = (Get-FileHash -LiteralPath $summary.compiler -Algorithm SHA256).Hash
    $summary.windows_sdk_version = $env:WindowsSDKVersion
    $env:PATH = "$(Split-Path $cmake);$(Split-Path $ninja);$env:PATH"
    $env:VSLANG = '1033'
    foreach ($name in @('http_proxy', 'https_proxy', 'all_proxy', 'HTTP_PROXY', 'HTTPS_PROXY', 'ALL_PROXY')) {
        [Environment]::SetEnvironmentVariable($name, 'http://127.0.0.1:1', 'Process')
    }
    foreach ($name in @('NO_PROXY', 'no_proxy')) { [Environment]::SetEnvironmentVariable($name, '', 'Process') }
    $env:GIT_ALLOW_PROTOCOL = 'file'
    Push-Location $root
    $locationPushed = $true

    $initialManifest = Get-InputManifest
    Save-Json $initialManifest (Join-Path $auditRoot 'inputs-sha256.json')
    $cmakeSources = @((Join-Path $PSScriptRoot 'CMakeLists.txt'), (Join-Path $root 'test/experimental/input-comparison/comparison.cmake'),
        (Join-Path $root 'cmake/ThirdPartyHeaders.cmake'), (Join-Path $root 'vendor/sdl3/CMakeLists.txt'))
    $cmakeSources += @(Get-ChildItem -LiteralPath (Join-Path $root 'vendor/sdl3/cmake') -File -Recurse |
        Where-Object { $_.Name -eq 'CMakeLists.txt' -or $_.Name -match '\.cmake(?:\.in)?$' } | Select-Object -ExpandProperty FullName)
    $cmakeSources = @($cmakeSources | Sort-Object -Unique)
    $summary.static_cmake_files = $cmakeSources
    $staticMatches = @(Select-String -LiteralPath $cmakeSources -Pattern '(?i)FetchContent|ExternalProject|file\s*\(\s*DOWNLOAD|DOWNLOAD_COMMAND|\b(curl|wget)\b|git\s+(clone|fetch|pull)' |
        Select-Object Path, LineNumber, Line)
    Save-Json $staticMatches (Join-Path $auditRoot 'static-download-audit.json')

    foreach ($configuration in $Configurations) {
        foreach ($mode in $Modes) {
            $caseName = "$($configuration.ToLowerInvariant())-$mode"
            $caseRoot = Join-Path $auditRoot $caseName
            $buildPath = Join-Path $caseRoot 'build'
            $installPath = Join-Path $caseRoot 'install'
            $queryRoot = Join-Path $buildPath '.cmake/api/v1/query/client-sdl3-audit'
            New-Item -ItemType Directory -Path $queryRoot -Force | Out-Null
            Save-Json @{ requests = @(@{ kind = 'codemodel'; version = 2 }, @{ kind = 'cache'; version = 2 }, @{ kind = 'cmakeFiles'; version = 1 }) } (Join-Path $queryRoot 'query.json')
            $case = [ordered]@{ name = $caseName; configuration = $configuration; mode = $mode; completed = $false; changed_inputs = @(); trace = $null; targets = @(); source_coverage = @{}; ctest_count = 0; synthetic_checks = 0; install_files = @(); cache_options = @{} }
            $summary.cases += $case
            $vulkan = if ($mode -eq 'vulkan') { 'ON' } else { 'OFF' }
            $declaredHeaders = if ($mode -eq 'vulkan') { $headers.Replace('\', '/') } else { '' }
            $tracePath = Join-Path $caseRoot 'configure-trace.jsonl'
            # CMake 3.22 writes compiler paths into quoted generated CMake source.
            $compilerCachePath = $summary.compiler.Replace('\', '/')
            $ninjaCachePath = $ninja.Replace('\', '/')
            $installCachePath = $installPath.Replace('\', '/')
            $configureArguments = @('--trace-expand', '--trace-format=json-v1', "--trace-redirect=$tracePath",
                '-S', $PSScriptRoot, '-B', $buildPath, '-G', 'Ninja',
                "-DCMAKE_BUILD_TYPE=$configuration", "-DCMAKE_TRY_COMPILE_CONFIGURATION=$configuration", "-DCMAKE_MAKE_PROGRAM=$ninjaCachePath",
                "-DCMAKE_C_COMPILER=$compilerCachePath", "-DCMAKE_CXX_COMPILER=$compilerCachePath", "-DCMAKE_INSTALL_PREFIX=$installCachePath",
                "-DSYMOCRAFT_SDL3_PROBE_VULKAN=$vulkan", "-DSYMOCRAFT_VULKAN_HEADERS_ROOT=$declaredHeaders",
                '-DSYMOCRAFT_SDL3_INPUT_COMPARISON=OFF',
                '-DFETCHCONTENT_FULLY_DISCONNECTED=ON', '-DFETCHCONTENT_UPDATES_DISCONNECTED=ON')
            Invoke-Logged $cmake $configureArguments (Join-Path $caseRoot 'configure.log') | Out-Null
            $case.trace = Audit-Trace $tracePath
            Save-Json $case.trace (Join-Path $caseRoot 'trace-audit.json')
            if ($case.trace.network_or_population_commands.Count -gt 0) { throw "Executed download/population command in $caseName; inspect trace-audit.json before continuing." }
            $ruleFiles = @((Join-Path $buildPath 'build.ninja'), (Join-Path $buildPath 'CMakeFiles/rules.ninja'))
            $ruleMatches = @(Select-String -LiteralPath $ruleFiles -Pattern '(?i)FetchContent|ExternalProject|file\s*\(\s*DOWNLOAD|(?:^|\s)(?:curl|wget)(?:\.exe)?\s|\bgit(?:\.exe)?\s+(?:clone|fetch|pull)' |
                Select-Object Path, LineNumber, Line)
            Save-Json $ruleMatches (Join-Path $caseRoot 'generated-rule-download-audit.json')
            if ($ruleMatches.Count -gt 0) { throw "Possible download step in generated rules for $caseName." }
            $replyPath = Join-Path $buildPath '.cmake/api/v1/reply'
            $targetReplies = @(Get-ChildItem -LiteralPath $replyPath -Filter 'target-*.json' | ForEach-Object { Get-Content -LiteralPath $_.FullName -Raw | ConvertFrom-Json })
            $case.targets = @($targetReplies | ForEach-Object { $_.name } | Sort-Object -Unique)
            $coveredSources = @()
            $generatedSources = @()
            $missingSources = @()
            foreach ($target in $targetReplies) {
                foreach ($source in $target.sources) {
                    $sourcePath = $source.path
                    if (-not [IO.Path]::IsPathRooted($sourcePath)) { $sourcePath = Join-Path $PSScriptRoot $sourcePath }
                    $sourcePath = [IO.Path]::GetFullPath($sourcePath)
                    if ($sourcePath.StartsWith($buildPath + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
                        $generatedSources += $sourcePath
                    } elseif ($sourcePath.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
                        $key = $sourcePath.Substring($root.Length + 1).Replace('\', '/')
                        if ($initialManifest.Contains($key)) { $coveredSources += $key } else { $missingSources += $sourcePath }
                    } else {
                        $missingSources += $sourcePath
                    }
                }
            }
            $case.source_coverage = [ordered]@{ manifest_covered = @($coveredSources | Sort-Object -Unique); generated_build_inputs = @($generatedSources | Sort-Object -Unique); missing_manifest_inputs = @($missingSources | Sort-Object -Unique) }
            if ($missingSources.Count -gt 0) { throw "CMake file API found untracked source inputs in $caseName." }
            $cache = Get-Content -LiteralPath (Join-Path $buildPath 'CMakeCache.txt')
            foreach ($line in $cache) {
                if ($line -match '^((?:SDL_[A-Z0-9_]+|SYMOCRAFT_[A-Z0-9_]+|CMAKE_BUILD_TYPE|CMAKE_MSVC_RUNTIME_LIBRARY)):[^=]+=(.*)$') {
                    $case.cache_options[$Matches[1]] = $Matches[2]
                }
            }
            Invoke-Logged $cmake @('--build', $buildPath, '--parallel', "$Jobs") (Join-Path $caseRoot 'build.log') | Out-Null
            $testOutput = Invoke-Logged $ctest @('--test-dir', $buildPath, '-C', $configuration,
                '-R', '^sdl3-input-candidate$', '-V', '--output-on-failure', '--no-tests=error') (Join-Path $caseRoot 'ctest.log')
            $testText = $testOutput -join "`n"
            if ($testText -notmatch 'out of (\d+)') { throw 'The CTest output did not report its test count.' }
            $case.ctest_count = [int]$Matches[1]
            if ($testText -notmatch 'synthetic input candidate:\s*(\d+) checks passed') { throw 'The synthetic candidate did not report its check count.' }
            $case.synthetic_checks = [int]$Matches[1]
            Invoke-Logged $cmake @('--install', $buildPath, '--config', $configuration) (Join-Path $caseRoot 'install.log') | Out-Null
            $case.install_files = @(Get-ChildItem -LiteralPath $installPath -File -Recurse | ForEach-Object {
                [ordered]@{ path = $_.FullName.Substring($installPath.Length + 1); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash }
            })
            $afterManifest = Get-InputManifest
            $case.changed_inputs = @(Compare-Manifests $initialManifest $afterManifest)
            Save-Json $afterManifest (Join-Path $caseRoot 'inputs-after-sha256.json')
            if ($case.changed_inputs.Count -gt 0) { throw "Inputs changed during the audit; do not compare $caseName to earlier cases." }
            $case.completed = $true
            Save-Json $summary (Join-Path $auditRoot 'summary.json')
            Write-Host "Passed: $caseName (configure/build/pure CTest/install; no GUI launched)"
        }
    }
    $summary.completed = $true
} catch {
    $summary.error = $_.Exception.Message
    throw
} finally {
    Save-Json $summary (Join-Path $auditRoot 'summary.json')
    if ($locationPushed) { Pop-Location }
    foreach ($entry in @(Get-ChildItem Env:)) {
        if (-not $savedEnvironment.ContainsKey($entry.Name)) { Remove-Item -LiteralPath "Env:$($entry.Name)" }
    }
    foreach ($name in $savedEnvironment.Keys) { [Environment]::SetEnvironmentVariable($name, $savedEnvironment[$name], 'Process') }
    Write-Host "Audit evidence: $auditRoot"
}

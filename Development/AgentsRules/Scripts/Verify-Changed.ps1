[CmdletBinding()]
param([ValidateSet('Fast','Deep')][string]$Mode = 'Fast')

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$quiet = Join-Path $PSScriptRoot 'Run-Quiet.ps1'
$compdb = Join-Path $root 'build/clangd'

function Full-Path([string]$Path, [string]$Base=$root) {
    if (-not [IO.Path]::IsPathRooted($Path)) { $Path = Join-Path $Base $Path }
    [IO.Path]::GetFullPath($Path)
}

Push-Location $root
try {
    $paths = @()
    foreach ($arguments in @(
        @('diff','--name-only','--diff-filter=ACMRD'),
        @('diff','--cached','--name-only','--diff-filter=ACMRD'),
        @('ls-files','--others','--exclude-standard')
    )) {
        $paths += @(& git -c core.quotepath=false @arguments)
        if ($LASTEXITCODE -ne 0) { throw 'Cannot determine changed files from Git.' }
    }
    $changed = @($paths | Where-Object {
        $_ -match '\.(cpp|cc|cxx|cppm|ixx|h|hh|hpp|hxx|inl|ipp|tpp)$' -and
        $_ -notlike 'Development/Tasks/*'
    } | Sort-Object -Unique)
}
finally { Pop-Location }

if (-not $changed.Count) {
    '{"status":"SKIPPED","reason":"NO_CPP_CHANGES","translation_units":0,"checks":0}'
    exit 0
}

$headers = @($changed | Where-Object { $_ -match '\.(h|hh|hpp|hxx|inl|ipp|tpp)$' })
$sources = @($changed | Where-Object { $_ -match '\.(cpp|cc|cxx|cppm|ixx)$' })
$dbPath = Join-Path $compdb 'compile_commands.json'
if (-not (Test-Path -LiteralPath $dbPath)) { throw 'BLOCKED: compilation database missing; run Prepare-Clangd.ps1.' }
$entries = ConvertFrom-Json -InputObject (
    Get-Content -LiteralPath $dbPath -Raw
)
$units = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($entry in $entries) {
    $file = Full-Path $entry.file $entry.directory
    if ($file -match '\.(cpp|cc|cxx|cppm|ixx)$') {
        if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { throw "BLOCKED: stale compilation database: $file" }
        [void]$units.Add($file)
    }
}
$selected = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
foreach ($relative in $sources) {
    $file = Full-Path $relative
    if (-not (Test-Path -LiteralPath $file -PathType Leaf)) { continue }
    if (-not $units.Contains($file)) { throw "BLOCKED: changed/untracked source absent from compilation database: $relative" }
    [void]$selected.Add($file)
}

if ($headers.Count) {
    Import-VsDevEnvironment
    $scanner = Resolve-Tool 'clang-scan-deps.exe' @((Join-Path $env:ProgramFiles 'LLVM/bin/clang-scan-deps.exe'))
    Set-AgentRuntimeEnvironment
    $dependencyPath = Join-Path $root ".ai/tmp/changed-dependencies-$PID.json"
    $filteredDbPath = Join-Path $root ".ai/tmp/changed-cpp-$PID.json"

    $scanEntries = @(
        foreach ($entry in $entries) {
            if ([string]$entry.file -match '\.(cpp|cc|cxx|cppm|ixx)$') {
                $entry
            }
        }
    )

    if ($scanEntries.Count -eq 0) {
        throw 'BLOCKED: no C/C++ translation units for dependency discovery.'
    }

    [IO.File]::WriteAllText(
        $filteredDbPath,
        (ConvertTo-Json -InputObject $scanEntries -Depth 32),
        (New-Object System.Text.UTF8Encoding($false))
    )
    try {
        $quote = { param($value) "'" + $value.Replace("'", "''") + "'" }
        $cmd = '& ' + (& $quote $scanner) +
            ' -compilation-database ' + (& $quote $filteredDbPath) +
            ' -format experimental-full -j 1 -o ' +
            (& $quote $dependencyPath)
        & $quiet -Name 'changed-dependencies' -TimeoutSec 600 -MaxJobMemoryGB 4 -Command $cmd
        if ($LASTEXITCODE -ne 0) { throw 'BLOCKED: compiler dependency discovery failed; consumers are unknown.' }
        $dependencies = Get-Content -LiteralPath $dependencyPath -Raw | ConvertFrom-Json
        $headerPaths = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($relative in $headers) { [void]$headerPaths.Add((Full-Path $relative)) }
        $seen = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        $consumed = [Collections.Generic.HashSet[string]]::new([StringComparer]::OrdinalIgnoreCase)
        foreach ($unit in $dependencies.'translation-units') {
            foreach ($command in $unit.commands) {
                $inputFile = Full-Path $command.'input-file'
                [void]$seen.Add($inputFile)
                foreach ($dependency in $command.'file-deps') {
                    $dependency = Full-Path $dependency
                    if ($headerPaths.Contains($dependency)) {
                        [void]$consumed.Add($dependency)
                        [void]$selected.Add($inputFile)
                    }
                }
            }
        }
        if ($dependencies.modules.Count) {
            # Imported modules can own header dependencies. Do not under-select.
            foreach ($file in $units) { [void]$selected.Add($file) }
            foreach ($module in $dependencies.modules) {
                foreach ($dependency in $module.'file-deps') {
                    $dependency = Full-Path $dependency
                    if ($headerPaths.Contains($dependency)) { [void]$consumed.Add($dependency) }
                }
            }
        }
        foreach ($file in $units) { if (-not $seen.Contains($file)) { throw "BLOCKED: incomplete dependency scan: $file" } }
        foreach ($header in $headerPaths) { if (-not $consumed.Contains($header)) { throw "BLOCKED: no compiled consumer for changed header: $header. Build public-header self-containment before qualifying it." } }
    }
    finally { 
        if (Test-Path -LiteralPath $filteredDbPath) {
    Remove-Item -LiteralPath $filteredDbPath -Force
}
     }
}

if (-not $selected.Count) {
    '{"status":"SKIPPED","reason":"ONLY_DELETED_SOURCES_REQUIRE_CLEAN_BUILD","translation_units":0,"checks":0}'
    exit 0
}
$clangTidy = Resolve-Tool 'clang-tidy.exe' @((Join-Path $env:ProgramFiles 'LLVM/bin/clang-tidy.exe'))
$cppcheck = $null
if ($Mode -eq 'Deep') { $cppcheck = Resolve-Tool 'cppcheck.exe' @((Join-Path $env:ProgramFiles 'Cppcheck/cppcheck.exe')) }
$failures = @()
$checks = 0
$headerFilter = '^' + (
    [regex]::Escape(
        [IO.Path]::GetFullPath($root).TrimEnd([char[]]@('\', '/'))
    ).Replace('\\', '[/\\]')
) + '[/\\]'
foreach ($full in ($selected | Sort-Object)) {
    $rootFull = [IO.Path]::GetFullPath($root).TrimEnd([char[]]@('\', '/'))
$rootPrefix = $rootFull + [IO.Path]::DirectorySeparatorChar
$fullCanonical = [IO.Path]::GetFullPath($full)

if (-not $fullCanonical.StartsWith(
    $rootPrefix,
    [StringComparison]::OrdinalIgnoreCase
)) {
    throw "BLOCKED: source outside project root: $fullCanonical"
}

$safe = $fullCanonical.Substring($rootPrefix.Length) -replace '[^A-Za-z0-9_.-]', '_'
    $quote = { param($value) "'" + $value.Replace("'", "''") + "'" }
    $cmd = '& ' + (& $quote $clangTidy) + ' ' + (& $quote $full) + ' -p ' + (& $quote $compdb) +
        ' -header-filter=' + (& $quote $headerFilter) +
        " -checks='-*,clang-analyzer-*,bugprone-*' -warnings-as-errors='*'"
    & $quiet -Name "tidy-$safe" -TimeoutSec 180 -MaxJobMemoryGB 3 -Command $cmd
    $checks++
    if ($LASTEXITCODE -ne 0) { $failures += "${full}:clang-tidy" }
    if ($Mode -eq 'Deep') {
        $cmd = '& ' + (& $quote $cppcheck) + ' --project=' + (& $quote $dbPath) +
            ' --file-filter=' + (& $quote $full) + ' --enable=warning,performance,portability --inline-suppr --error-exitcode=1'
        & $quiet -Name "cppcheck-$safe" -TimeoutSec 120 -MaxJobMemoryGB 2 -Command $cmd
        $checks++
        if ($LASTEXITCODE -ne 0) { $failures += "${full}:cppcheck" }
    }
}
[ordered]@{status=$(if($failures.Count){'FAIL'}else{'PASS'});headers=$headers.Count;
    translation_units=$selected.Count;checks=$checks;failures=$failures} | ConvertTo-Json -Compress
if ($failures.Count) { exit 1 }

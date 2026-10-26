[CmdletBinding()]
param(
    [ValidateSet('Configure','Build','Test','Full')]
    [string]$Action = 'Build',
    [ValidateSet('base','runtime','full')]
    [string]$Profile = 'full',
    [ValidateSet('Debug','Release')]
    [string]$Configuration = 'Debug',
    [string]$Target,
    [string]$TestRegex,
    [switch]$Clean,
    [string]$BuildDirectory,
    [string]$RuntimeDirectory = '.ai',
    [switch]$ShortOutput,
    [ValidateRange(1,32)][int]$BuildJobs = 4,
    [ValidateRange(30,7200)]
    [int]$TimeoutSec = 1800
)

$ErrorActionPreference = 'Stop'
if ($Action -eq 'Full') {
    if ($PSBoundParameters.ContainsKey('Target') -or $PSBoundParameters.ContainsKey('TestRegex') -or
        $Profile -ne 'full' -or ($PSBoundParameters.ContainsKey('Clean') -and -not $Clean)) {
        throw 'Full qualification requires profile full, clean inputs and no Target/TestRegex filters.'
    }
    $Clean = $true
}
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$quiet = Join-Path $PSScriptRoot 'Run-Quiet.ps1'
$configKey = $Configuration.ToLowerInvariant()
if (-not $BuildDirectory) { $BuildDirectory = "build/$Profile-$configKey" }
$build = [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
Set-AgentRuntimeEnvironment -RuntimeDirectory $RuntimeDirectory

$options = switch ($Profile) {
    'base'    { @('-DEPIDEMIC_BUILD_RUNTIME=OFF','-DEPIDEMIC_BUILD_FRAMEWORK=OFF') }
    'runtime' { @('-DEPIDEMIC_BUILD_RUNTIME=ON','-DEPIDEMIC_BUILD_FRAMEWORK=OFF') }
    'full'    { @('-DEPIDEMIC_BUILD_RUNTIME=ON','-DEPIDEMIC_BUILD_FRAMEWORK=ON') }
}

function Invoke-Step([string]$Name,[string]$Command,[int]$Seconds=$TimeoutSec,[double]$MemoryGB=8) {
    & $quiet -Name $Name -TimeoutSec $Seconds -MaxJobMemoryGB $MemoryGB -Command $Command `
        -RuntimeDirectory $RuntimeDirectory -LogDir "$RuntimeDirectory/logs" -ShortOutput:$ShortOutput
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

if (-not $build.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid build path.' }
if ($Clean -and (Test-Path -LiteralPath $build)) { Remove-Item -LiteralPath $build -Recurse -Force }
$cache = Join-Path $build 'CMakeCache.txt'
if ($Action -eq 'Configure' -or -not (Test-Path -LiteralPath $cache)) {
    $opt = $options -join ' '
    Invoke-Step "configure-$Profile-$configKey" ("cmake -S . -B `"$build`" -G `"Visual Studio 18 2026`" -A x64 -DBUILD_TESTING=ON -DEPIDEMIC_WARNINGS_AS_ERRORS=ON -DEPIDEMIC_ARCHITECTURE_FREEZE_CHECKS=ON $opt") 900 4
    if ($Action -eq 'Configure') { exit 0 }
}

$buildCommand = "cmake --build `"$build`" --config $Configuration --parallel $BuildJobs"
if ($Target) { $buildCommand += " --target `"$Target`"" }
Invoke-Step "build-$Profile-$configKey" $buildCommand $TimeoutSec 8

if ($Action -eq 'Build') { exit 0 }

if ($Action -eq 'Full') {
    Invoke-Step 'architecture-self-test' 'cmake -P cmake/ArchitectureFreezeSelfTest.cmake' 300 2
    Invoke-Step "public-headers-$Profile-$configKey" ("cmake --build `"$build`" --config $Configuration --target EpidemicPublicHeaderSelfContainment --parallel $BuildJobs") $TimeoutSec 8
}

$testCommand = "ctest --test-dir `"$build`" -C $Configuration --output-on-failure --no-tests=error --timeout 300"
if ($TestRegex) { $testCommand += " -R `"$TestRegex`"" }
Invoke-Step "test-$Profile-$configKey" $testCommand $TimeoutSec 8

if ($Action -eq 'Full') {
    & (Join-Path $PSScriptRoot 'Verify-Changed.ps1') -Mode Deep
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    Push-Location $root
    try { git -c core.safecrlf=false diff --check; if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE } }
    finally { Pop-Location }
}

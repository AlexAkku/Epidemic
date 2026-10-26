[CmdletBinding()]
param([switch]$Clean, [string]$BuildDirectory = 'build/clangd',
    [string]$RuntimeDirectory = '.ai', [switch]$ShortOutput)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$build = [IO.Path]::GetFullPath((Join-Path $root $BuildDirectory))
Set-AgentRuntimeEnvironment -RuntimeDirectory $RuntimeDirectory
$quiet = Join-Path $PSScriptRoot 'Run-Quiet.ps1'

Import-VsDevEnvironment
Resolve-Tool 'cmake.exe' | Out-Null
Resolve-Tool 'ninja.exe' | Out-Null
Resolve-Tool 'clang-cl.exe' @((Join-Path $env:ProgramFiles 'LLVM/bin/clang-cl.exe')) | Out-Null

if (-not $build.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid build path.' }
if ($Clean -and (Test-Path -LiteralPath $build)) { Remove-Item -LiteralPath $build -Recurse -Force }
New-Item -ItemType Directory -Force -Path $build | Out-Null

$cmd = @(
    'cmake -S .',
    ('-B "' + $build + '"'),
    '-G Ninja',
    '-DCMAKE_BUILD_TYPE=Debug',
    '-DCMAKE_CXX_COMPILER=clang-cl',
    '-DCMAKE_EXPORT_COMPILE_COMMANDS=ON',
    '-DBUILD_TESTING=ON',
    '-DEPIDEMIC_BUILD_RUNTIME=ON',
    '-DEPIDEMIC_BUILD_FRAMEWORK=ON',
    '-DEPIDEMIC_WARNINGS_AS_ERRORS=OFF',
    '-DEPIDEMIC_ARCHITECTURE_FREEZE_CHECKS=ON'
) -join ' '

& $quiet -Name 'clangd-configure' -TimeoutSec 600 -MaxJobMemoryGB 4 -Command $cmd `
    -RuntimeDirectory $RuntimeDirectory -LogDir "$RuntimeDirectory/logs" -ShortOutput:$ShortOutput
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$db = Join-Path $build 'compile_commands.json'
if (-not (Test-Path -LiteralPath $db -PathType Leaf)) { throw 'CMake did not generate compile_commands.json.' }
$nativeBuild = Join-Path (Split-Path -Parent $build) 'auditor-clangd'
$nativeSource = Join-Path $root '.epidemic-auditor/native'
if (Test-Path -LiteralPath (Join-Path $nativeSource 'CMakeLists.txt')) {
    $nativeCommand = ('cmake -S "' + $nativeSource + '" -B "' + $nativeBuild +
        '" -G Ninja -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang-cl -DCMAKE_EXPORT_COMPILE_COMMANDS=ON')
    & $quiet -Name 'auditor-clangd-configure' -TimeoutSec 600 -MaxJobMemoryGB 4 -Command $nativeCommand `
        -RuntimeDirectory $RuntimeDirectory -LogDir "$RuntimeDirectory/logs" -ShortOutput:$ShortOutput
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    $nativeDb = Join-Path $nativeBuild 'compile_commands.json'
    if (-not (Test-Path -LiteralPath $nativeDb -PathType Leaf)) { throw 'Auditor CMake did not generate compile_commands.json.' }
    $commands = @((Get-Content -LiteralPath $db -Raw | ConvertFrom-Json)) +
                @((Get-Content -LiteralPath $nativeDb -Raw | ConvertFrom-Json))
    foreach ($entry in $commands) {
        # MSVC /Gh and /GH install physical hooks, but clang's analysis driver
        # does not implement them. Remove only these standalone flags from the
        # analysis database; leave the actual CMake/compiler builds unchanged.
        if ($entry.PSObject.Properties['command']) {
            $entry.command = [regex]::Replace($entry.command, '(?<!\S)/G[hH](?=\s|$)', '')
        }
        if ($entry.PSObject.Properties['arguments']) {
            $entry.arguments = @($entry.arguments | Where-Object { $_ -cnotin @('/Gh', '/GH') })
        }
    }
    [IO.File]::WriteAllText($db, (ConvertTo-Json -InputObject $commands -Depth 32),
        (New-Object System.Text.UTF8Encoding($false)))
}
Write-Output "clangd database: $db"

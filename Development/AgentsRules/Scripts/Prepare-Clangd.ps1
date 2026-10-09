[CmdletBinding()]
param([switch]$Clean)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$build = [IO.Path]::GetFullPath((Join-Path $root 'build/clangd'))
Set-AgentRuntimeEnvironment
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
    '-DEPIDEMIC_ARCHITECTURE_FREEZE_CHECKS=OFF'
) -join ' '

& $quiet -Name 'clangd-configure' -TimeoutSec 600 -MaxJobMemoryGB 4 -Command $cmd
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$db = Join-Path $build 'compile_commands.json'
if (-not (Test-Path -LiteralPath $db -PathType Leaf)) { throw 'CMake did not generate compile_commands.json.' }
Write-Output "clangd database: $db"

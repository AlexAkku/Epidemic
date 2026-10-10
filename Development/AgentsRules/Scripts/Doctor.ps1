[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot

$checks = [ordered]@{}
function Probe([string]$Name,[scriptblock]$Action,[bool]$Required=$true) {
    try { $value = & $Action; $checks[$Name] = [ordered]@{status='AVAILABLE';detail=$value;required=$Required} }
    catch { $checks[$Name] = [ordered]@{status='MISSING';detail=$_.Exception.Message;required=$Required} }
}
Probe 'git' { (Resolve-Tool 'git.exe') }
Probe 'pwsh' { (Resolve-Tool 'pwsh.exe' @((Join-Path $env:ProgramFiles 'PowerShell/7/pwsh.exe'))) }
Probe 'cmake' { (Resolve-Tool 'cmake.exe') }
Probe 'node' { (Resolve-Tool 'node.exe') }
Probe 'python' { (Resolve-Tool 'python.exe') }
Probe 'rg' { (Resolve-Tool 'rg.exe') }
Probe 'ninja' { (Resolve-Tool 'ninja.exe') }
Probe 'clangd' { (Resolve-Tool 'clangd.exe' @((Join-Path $env:ProgramFiles 'LLVM/bin/clangd.exe'))) }
Probe 'clang-tidy' { (Resolve-Tool 'clang-tidy.exe' @((Join-Path $env:ProgramFiles 'LLVM/bin/clang-tidy.exe'))) }
Probe 'clang-scan-deps' { (Resolve-Tool 'clang-scan-deps.exe' @((Join-Path $env:ProgramFiles 'LLVM/bin/clang-scan-deps.exe'))) }
Probe 'cppcheck' { (Resolve-Tool 'cppcheck.exe' @((Join-Path $env:ProgramFiles 'Cppcheck/cppcheck.exe'))) }
Probe 'visual-studio-cpp' { Import-VsDevEnvironment; 'ready' }
Probe 'clangd-mcp' { if (-not (Test-Path (Join-Path $root '.ai/tools/clangd-mcp-server/dist/index.js'))) { throw 'not deployed' }; 'deployed' }
Probe 'compile-db' { if (-not (Test-Path (Join-Path $root 'build/clangd/compile_commands.json'))) { throw 'not prepared' }; 'present; freshness not tested' } $false
Probe 'retrieval-venv' { if (-not (Test-Path (Join-Path $root '.ai/retrieval/.venv/Scripts/python.exe'))) { throw 'not deployed' }; 'present; dependencies checked separately' } $false
$retrieval = [ordered]@{status='NOT_READY';detail='Retrieval venv missing';inference='NOT_TESTED'}
if ($checks['retrieval-venv'].status -eq 'AVAILABLE') {
    $python = Join-Path $root '.ai/retrieval/.venv/Scripts/python.exe'
    $script = Join-Path $PSScriptRoot '../Retrieval/retrieve.py'
    $quote = { param($value) "'" + $value.Replace("'", "''") + "'" }
    try {
        $summary = & (Join-Path $PSScriptRoot 'Run-Quiet.ps1') -Name 'retrieval-status' `
            -TimeoutSec 60 -MaxJobMemoryGB 2 -Command ('& ' + (& $quote $python) + ' -B ' + (& $quote $script) + ' status')
        $code = $LASTEXITCODE
        if ($code -notin @(0,2)) { throw "Readiness probe failed (exit $code): $summary" }
        $retrieval = Get-Content -LiteralPath (Join-Path $root '.ai/logs/retrieval-status.stdout.log') -Raw | ConvertFrom-Json
        if ($retrieval.status -notin @('PREREQUISITES_READY','NOT_READY')) { throw 'Invalid readiness response' }
    }
    catch { $retrieval = [ordered]@{status='NOT_CHECKED';detail=$_.Exception.Message;inference='NOT_TESTED'} }
}

$requiredFail = @($checks.GetEnumerator() | Where-Object { $_.Value.required -and $_.Value.status -ne 'AVAILABLE' }).Count
[ordered]@{tools_status=$(if($requiredFail){'MISSING_REQUIRED'}else{'AVAILABLE'});
    checks=$checks;retrieval=$retrieval;qualification='NOT_RUN'} | ConvertTo-Json -Depth 7
if ($requiredFail) { exit 1 }
# Setup calls this script in-process and forwards LASTEXITCODE. Optional
# retrieval NOT_READY must not leak the readiness probe's exit code into setup.
exit 0

[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][ValidateNotNullOrEmpty()][string]$Query,
    [ValidateRange(1,5)][int]$Top = 3,
    [switch]$StartDaemon,
    [ValidateRange(1,86400)][int]$TimeoutSec = 300,
    [ValidateRange(0.25,24)][double]$MaxJobMemoryGB = 8
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$python = Join-Path $root '.ai/retrieval/.venv/Scripts/python.exe'
$daemon = Join-Path $PSScriptRoot '../Retrieval/daemon.py'
# Invoking Search-Code explicitly authorizes one bounded local query.
# StartDaemon remains accepted for compatibility; no persistent daemon is started.
if (-not (Test-Path -LiteralPath $python)) { throw 'Retrieval environment is missing; run Setup-AgentTools.ps1. Models/index remain manual.' }
Set-AgentRuntimeEnvironment
$env:OMP_NUM_THREADS = '2'
$env:MKL_NUM_THREADS = '2'
$env:TOKENIZERS_PARALLELISM = 'false'
$env:HF_HUB_OFFLINE = '1'
$env:TRANSFORMERS_OFFLINE = '1'
$quote = { param($value) "'" + $value.Replace("'", "''") + "'" }
$command = '& ' + (& $quote $python) + ' -B ' + (& $quote $daemon) +
    ' --query ' + (& $quote $Query) + ' --top ' + $Top
# The wrapper owns the local process tree, memory ceiling, timeout and cleanup.
# Ollama is external; one-query mode uses keep_alive=0, never starts/downloads models.
& (Join-Path $PSScriptRoot 'Run-Quiet.ps1') -Name 'retrieval-query' -Command $command `
    -TimeoutSec $TimeoutSec -MaxJobMemoryGB $MaxJobMemoryGB
$resultCode = $LASTEXITCODE
if ($resultCode -in @(0,2)) {
    Get-Content -LiteralPath (Join-Path $root '.ai/logs/retrieval-query.stdout.log') -Raw
}
exit $resultCode

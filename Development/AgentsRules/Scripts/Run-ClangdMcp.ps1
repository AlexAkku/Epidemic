[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')

try {
    $root = Get-ProjectRoot
    Set-AgentRuntimeEnvironment
    $toolRoot = Join-Path $root '.ai/tools/clangd-mcp-server'
    $manifestPath = Join-Path $toolRoot 'package.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw 'clangd MCP is not deployed; run Setup-AgentTools.ps1.'
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    $entry = [IO.Path]::GetFullPath((Join-Path $toolRoot $manifest.main))
    if (-not (Test-Path -LiteralPath $entry -PathType Leaf)) { throw 'clangd MCP is not built; rerun setup.' }

    $env:CLANGD_PATH = Resolve-Tool 'clangd.exe' @(
        (Join-Path $env:ProgramFiles 'LLVM/bin/clangd.exe')
    )
    $env:PROJECT_ROOT = $root
    $env:COMPILE_COMMANDS_DIR = Join-Path $root 'build/clangd'
    $env:CLANGD_ARGS = '--background-index=0 -j=1 --pch-storage=disk'
    $env:CLANGD_LOG_LEVEL = 'error'

    if (-not (Test-Path -LiteralPath (Join-Path $env:COMPILE_COMMANDS_DIR 'compile_commands.json'))) {
        throw 'clangd compilation database is missing; run Prepare-Clangd.ps1.'
    }

    Push-Location $root
    try { & node $entry; exit $LASTEXITCODE }
    finally { Pop-Location }
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}

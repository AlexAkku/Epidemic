[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ai-tools-common.ps1')

try {
    $projectRoot = Get-AiProjectRoot
    $entrypoint = Get-AiClangdMcpEntrypoint

    $env:CLANGD_PATH = Resolve-AiClangd
    $env:PROJECT_ROOT = $projectRoot
    $env:COMPILE_COMMANDS_DIR = $projectRoot

    # Short-lived query-clangd invocation:
    # no repeated whole-project background indexing,
    # one worker, disk-backed PCH to reduce memory pressure.
    if ($env:AI_CLANGD_EPHEMERAL -eq '1') {
        if (-not $env:CLANGD_ARGS) {
            $env:CLANGD_ARGS = '--background-index=0 -j=1 --pch-storage=disk'
        }
    }
    else {
        # Persistent MCP server.
        if (-not $env:CLANGD_ARGS) {
            $env:CLANGD_ARGS = '--background-index -j=2 --pch-storage=disk'
        }
        elseif ($env:CLANGD_ARGS -notmatch '--background-index') {
            $env:CLANGD_ARGS += ' --background-index'
        }
    }

    if (-not $env:CLANGD_LOG_LEVEL) {
        $env:CLANGD_LOG_LEVEL = 'error'
    }

    Push-Location $projectRoot
    try {
        & node $entrypoint
        $serverExitCode = $LASTEXITCODE
    }
    finally {
        Pop-Location
    }

    exit $serverExitCode
}
catch {
    [Console]::Error.WriteLine($_.Exception.Message)
    exit 1
}
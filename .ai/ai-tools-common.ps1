Set-StrictMode -Version Latest

function Get-AiProjectRoot {
    return Split-Path -Parent $PSScriptRoot
}

function Resolve-AiClangd {
    if ($env:CLANGD_PATH) {
        $explicit = Get-Command $env:CLANGD_PATH -CommandType Application -ErrorAction SilentlyContinue
        if (-not $explicit) { throw 'CLANGD_PATH does not resolve to an executable.' }
        return $explicit.Source
    }
    $available = Get-Command clangd -CommandType Application -ErrorAction SilentlyContinue
    if ($available) { return $available.Source }
    foreach ($programRoot in @($env:ProgramW6432, $env:ProgramFiles)) {
        if ($programRoot) {
            $candidate = Join-Path $programRoot 'LLVM/bin/clangd.exe'
            if (Test-Path -LiteralPath $candidate -PathType Leaf) { return $candidate }
        }
    }
    throw 'clangd is missing: add its bin directory to PATH or set CLANGD_PATH.'
}

function Get-AiClangdMcpEntrypoint {
    $serverRoot = Join-Path (Get-AiProjectRoot) '.tools/clangd-mcp-server'
    $manifestPath = Join-Path $serverRoot 'package.json'
    if (-not (Test-Path -LiteralPath $manifestPath -PathType Leaf)) {
        throw 'clangd MCP package.json is missing under .tools/clangd-mcp-server.'
    }
    $manifest = Get-Content -LiteralPath $manifestPath -Raw | ConvertFrom-Json
    if (-not $manifest.main) { throw 'clangd MCP package does not declare a Node entrypoint.' }
    $entrypoint = [IO.Path]::GetFullPath((Join-Path $serverRoot $manifest.main))
    if (-not $entrypoint.StartsWith($serverRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) {
        throw 'clangd MCP entrypoint resolves outside its package.'
    }
    if (-not (Test-Path -LiteralPath $entrypoint -PathType Leaf)) {
        throw 'clangd MCP entrypoint is missing; run npm run build in its package.'
    }
    return $entrypoint
}

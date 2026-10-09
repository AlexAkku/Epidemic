[CmdletBinding()]
param(
    [switch]$SkipSystemInstall,
    [switch]$SkipRetrieval,
    [switch]$SkipClangdConfigure
)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$rules = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
$ai = Join-Path $root '.ai'
Set-AgentRuntimeEnvironment

function Refresh-ProcessPath {
    $machine = [Environment]::GetEnvironmentVariable('Path','Machine')
    $user = [Environment]::GetEnvironmentVariable('Path','User')
    $env:Path = @($machine,$user) -join ';'
}

function Ensure-WingetTool([string]$Command,[string]$PackageId) {
    if (Get-Command $Command -ErrorAction SilentlyContinue) { return }
    if ($SkipSystemInstall) { throw "Missing $Command; install package $PackageId or rerun without -SkipSystemInstall." }
    if (-not (Get-Command winget.exe -ErrorAction SilentlyContinue)) { throw "Missing $Command and winget is unavailable. Install $PackageId manually." }
    & winget.exe install --id $PackageId --exact --silent --accept-package-agreements --accept-source-agreements
    if ($LASTEXITCODE -ne 0) { throw "winget failed for $PackageId" }
    Refresh-ProcessPath
}

New-Item -ItemType Directory -Force -Path $ai,(Join-Path $root 'build'),(Join-Path $ai 'logs'),(Join-Path $ai 'tmp'),(Join-Path $ai 'tools'),(Join-Path $ai 'retrieval') | Out-Null

# Keep all runtime state out of Git.
$gitignore = Join-Path $root '.gitignore'
if (-not (Test-Path -LiteralPath $gitignore)) { New-Item -ItemType File -Path $gitignore | Out-Null }
if (-not (Select-String -LiteralPath $gitignore -Pattern '^/\.ai/$' -Quiet)) {
    Add-Content -LiteralPath $gitignore -Value "`n# Disposable agent/build runtime`n/.ai/"
}

# Deploy the two root configurations that Codex/clangd discover automatically.
New-Item -ItemType Directory -Force -Path (Join-Path $root '.codex') | Out-Null
Copy-Item -LiteralPath (Join-Path $rules 'Templates/codex.config.toml') -Destination (Join-Path $root '.codex/config.toml') -Force
Copy-Item -LiteralPath (Join-Path $rules 'Templates/clangd') -Destination (Join-Path $root '.clangd') -Force
Copy-Item -LiteralPath (Join-Path $rules 'Templates/AGENTS.root.md') -Destination (Join-Path $root 'AGENTS.md') -Force

Ensure-WingetTool 'git.exe' 'Git.Git'
Ensure-WingetTool 'pwsh.exe' 'Microsoft.PowerShell'
Ensure-WingetTool 'cmake.exe' 'Kitware.CMake'
Ensure-WingetTool 'ninja.exe' 'Ninja-build.Ninja'
Ensure-WingetTool 'node.exe' 'OpenJS.NodeJS.LTS'
Ensure-WingetTool 'python.exe' 'Python.Python.3.13'
Ensure-WingetTool 'rg.exe' 'BurntSushi.ripgrep.MSVC'
Ensure-WingetTool 'clangd.exe' 'LLVM.LLVM'
Ensure-WingetTool 'cppcheck.exe' 'Cppcheck.Cppcheck'

$trackedAi = @(git -C $root ls-files .ai 2>$null)
if ($trackedAi.Count -gt 0) { Write-Warning "Legacy .ai files are still tracked by Git. Remove them from the tracked tree before the cleanup commit; runtime .ai must be disposable." }

# Fail early if the required C++ build environment itself is absent.
Import-VsDevEnvironment
$pythonExe = Resolve-Tool 'python.exe'

# clangd MCP: pinned source revision, disposable under .ai/tools.
$mcp = Join-Path $ai 'tools/clangd-mcp-server'
$pin = 'be6bf91e3270ec2ce497358046436d7b17c38951'
if (-not (Test-Path -LiteralPath (Join-Path $mcp '.git'))) {
    if (Test-Path -LiteralPath $mcp) { throw "Existing non-Git tool directory: $mcp. Inspect it before retrying." }
    & git clone --filter=blob:none https://github.com/felipeerias/clangd-mcp-server.git $mcp
    if ($LASTEXITCODE -ne 0) { throw 'clangd-mcp-server clone failed.' }
}
Push-Location $mcp
try {
    & git fetch --depth 1 origin $pin
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP fetch failed.' }
    & git checkout --detach $pin
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP checkout failed.' }
    # The pinned upstream adds a Linux-only clangd option on Windows.
    # Keep this compatibility adjustment reproducible after every deployment.
    $configPath = Join-Path $mcp 'src/config-detector.ts'
    $configSource = [IO.File]::ReadAllText($configPath)
    $trimCondition = "if (!args.some(arg => arg.includes('malloc-trim'))) {"
    $windowsTrimCondition = "if (process.platform !== 'win32' && !args.some(arg => arg.includes('malloc-trim'))) {"
    if ($configSource.Contains($trimCondition)) {
        [IO.File]::WriteAllText($configPath, $configSource.Replace($trimCondition, $windowsTrimCondition), [Text.UTF8Encoding]::new($false))
    } elseif (-not $configSource.Contains($windowsTrimCondition)) { throw 'Pinned clangd MCP compatibility patch no longer matches.' }
    # Upgrade the runtime SDK within its existing major version; the upstream
    # lock pins releases affected by published SDK and HTTP dependency advisories.
    & npm.cmd install --package-lock-only --ignore-scripts --save-exact '@modelcontextprotocol/sdk@1.31.0'
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP runtime SDK lock update failed.' }
    & npm.cmd audit fix --package-lock-only --omit=dev --ignore-scripts
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP runtime dependency remediation failed.' }
    & npm.cmd ci
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP npm ci failed.' }
    & npm.cmd run build
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP build failed.' }
    & npm.cmd prune --omit=dev --ignore-scripts
    if ($LASTEXITCODE -ne 0) { throw 'clangd MCP runtime pruning failed.' }
}
finally { Pop-Location }

if (-not $SkipRetrieval) {
    $venv = Join-Path $ai 'retrieval/.venv'
    if (-not (Test-Path -LiteralPath (Join-Path $venv 'Scripts/python.exe'))) {
        & $pythonExe -m venv $venv
        if ($LASTEXITCODE -ne 0) { throw 'Retrieval venv creation failed.' }
    }
    $py = Join-Path $venv 'Scripts/python.exe'
    & $py -m pip install --disable-pip-version-check -r (Join-Path $rules 'Retrieval/requirements.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Retrieval Python dependencies failed.' }

    # ----------------------------------------------------------------------
    # Neural/model installation intentionally disabled.
    # Model deployment and index construction are explicit manual operations.
    # Expected paths are documented in Development/AgentsRules/Retrieval/README.md.
    #
    # winget install --id Ollama.Ollama --exact --silent
    # ollama pull qwen3-embedding:4b-q8_0
    # & $py (Join-Path $rules 'Retrieval/retrieve.py') download-reranker
    # ----------------------------------------------------------------------
}

# Reuse the component's installer without launching its application.
$auditorSetup = Join-Path $root '.epidemic-auditor/Start-Auditor.ps1'
if (Test-Path -LiteralPath $auditorSetup) {
    Push-Location $root
    try { & $auditorSetup -SetupOnly }
    finally { Pop-Location }
}

if (-not $SkipClangdConfigure) {
    & (Join-Path $PSScriptRoot 'Prepare-Clangd.ps1')
    if ($LASTEXITCODE -ne 0) { throw 'clangd compilation database preparation failed.' }
}

& (Join-Path $PSScriptRoot 'Doctor.ps1')
exit $LASTEXITCODE

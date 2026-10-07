[CmdletBinding()]
param([switch]$MS5)

$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'ai-tools-common.ps1')
$projectRoot = Get-AiProjectRoot
$script:AiFailures = 0

function Invoke-AiCheck {
    param([string]$Name, [scriptblock]$Action)
    try {
        & $Action
        Write-Host ($Name + ': PASS')
    } catch {
        $script:AiFailures++
        Write-Host ($Name + ': FAIL - ' + $_.Exception.Message)
    }
}

function Invoke-AiVersion {
    param([string]$Command, [string[]]$VersionArgs)
    & $Command @VersionArgs
    if ($LASTEXITCODE -ne 0) { throw ($Command + ' returned exit code ' + $LASTEXITCODE) }
}

Push-Location $projectRoot
try {
    Invoke-AiCheck 'clangd' { Invoke-AiVersion (Resolve-AiClangd) @('--version') }
    Invoke-AiCheck 'ripgrep' { Invoke-AiVersion 'rg' @('--version') }
    Invoke-AiCheck 'node' { Invoke-AiVersion 'node' @('--version') }
    if (-not $MS5) { Invoke-AiCheck 'Repomix' {
        if (-not (Test-Path 'node_modules/repomix/package.json')) { throw 'Local repomix dependency is missing.' }
        Invoke-AiVersion 'npx.cmd' @('--no-install', 'repomix', '--version')
    } }
    Invoke-AiCheck 'ast-grep' {
        if (-not (Test-Path 'node_modules/@ast-grep/cli/package.json')) { throw 'Local @ast-grep/cli dependency is missing.' }
        Invoke-AiVersion 'npx.cmd' @('--no-install', 'ast-grep', '--version')
    }
    Invoke-AiCheck 'compile_commands.json' {
        if (-not (Test-Path -LiteralPath 'compile_commands.json' -PathType Leaf)) { throw 'Compilation database is missing.' }
        $raw = Get-Content -LiteralPath 'compile_commands.json' -Raw
        if (-not $raw.TrimStart().StartsWith('[')) { throw 'Compilation database must be a JSON array.' }
        $database = ConvertFrom-Json -InputObject $raw
        if ($database.Count -eq 0) { throw 'Compilation database is empty.' }
        $projectUnits = 0
        foreach ($unit in $database) {
            if (-not $unit.file -or -not $unit.directory) { throw 'Translation unit is missing file/directory.' }
            if (-not ($unit.PSObject.Properties['command'] -or $unit.PSObject.Properties['arguments'])) { throw 'Translation unit has no compiler invocation.' }
            if (-not (Test-Path -LiteralPath $unit.directory -PathType Container)) { throw ('Missing compile directory: ' + $unit.directory) }
            $unitFile = $unit.file
            if (-not [IO.Path]::IsPathRooted($unitFile)) { $unitFile = Join-Path $unit.directory $unitFile }
            $unitFile = [IO.Path]::GetFullPath($unitFile)
            if (-not (Test-Path -LiteralPath $unitFile -PathType Leaf)) { throw ('Missing translation unit: ' + $unitFile) }
            if ($unitFile.StartsWith($projectRoot + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase) -and
                $unitFile.Substring($projectRoot.Length + 1) -match '^Engine(Base|Runtime|Framework)[\\/]') { $projectUnits++ }
        }
        if ($projectUnits -eq 0) { throw 'Compilation database contains no project source files.' }
        Write-Host ('Translation units: ' + $database.Count + '; project source units: ' + $projectUnits)
    }
    Invoke-AiCheck 'clangd MCP' {
        Write-Host ('Node entrypoint: ' + (Get-AiClangdMcpEntrypoint))
        if (-not (Test-Path '.tools/clangd-mcp-server/node_modules/@modelcontextprotocol/sdk/package.json')) {
            throw 'clangd MCP runtime dependency is missing.'
        }
        Invoke-AiVersion 'node' @('--check', (Get-AiClangdMcpEntrypoint))
    }
    if ($MS5) {
        Invoke-AiCheck 'clang-tidy' {
            Invoke-AiVersion (Join-Path (Split-Path -Parent (Resolve-AiClangd)) 'clang-tidy.exe') @('--version')
        }
        Invoke-AiCheck 'Cppcheck' {
            $checker = Get-Command cppcheck -CommandType Application -ErrorAction SilentlyContinue
            $checkerPath = if ($checker) { $checker.Source } else { Join-Path $env:ProgramFiles 'Cppcheck/cppcheck.exe' }
            Invoke-AiVersion $checkerPath @('--version')
        }
        Invoke-AiCheck 'Graft' { Invoke-AiVersion 'graft.cmd' @('--version') }
        Invoke-AiCheck 'Graft MCP configuration' {
            $configPaths = @('.codex/config.toml', (Join-Path $env:USERPROFILE '.codex/config.toml'))
            $registrations = 0
            foreach ($configPath in $configPaths) {
                if (Test-Path -LiteralPath $configPath) {
                    $configText = Get-Content -LiteralPath $configPath -Raw
                    $registrations += [regex]::Matches($configText, '(?m)^\s*\[mcp_servers\.graft\]\s*$').Count
                }
            }
            if ($registrations -ne 1) { throw "Expected one Graft MCP registration, found $registrations" }
        }
        Invoke-AiCheck 'Ollama/Qwen' {
            $ollama = Get-Command ollama -CommandType Application -ErrorAction SilentlyContinue
            $ollamaPath = if ($ollama) { $ollama.Source } else { Join-Path $env:LOCALAPPDATA 'Programs/Ollama/ollama.exe' }
            Invoke-AiVersion $ollamaPath @('--version')
            $models = Invoke-RestMethod 'http://127.0.0.1:11434/api/tags'
            if (-not @($models.models | Where-Object name -EQ 'qwen3-embedding:4b').Count) { throw 'qwen3-embedding:4b missing' }
        }
        Invoke-AiCheck 'Hybrid retrieval index' {
            $retrievalPython = Join-Path $projectRoot '.ai/retrieval/.venv/Scripts/python.exe'
            Invoke-AiVersion $retrievalPython @('.ai/retrieval/retrieve.py', 'check')
            if (-not (Test-Path '.ai/retrieval/models/bge-reranker-v2-m3/model.safetensors')) { throw 'Local BGE weights missing' }
        }
    }
} finally { Pop-Location }

if ($script:AiFailures -gt 0) { exit 1 }
exit 0

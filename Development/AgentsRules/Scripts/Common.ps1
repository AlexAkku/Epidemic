Set-StrictMode -Version Latest

function Get-ProjectRoot {
    return [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))
}

function Set-AgentRuntimeEnvironment {
    $runtime = Join-Path (Get-ProjectRoot) '.ai'
    $temporary = Join-Path $runtime 'tmp'
    New-Item -ItemType Directory -Force -Path $temporary | Out-Null
    $env:TEMP = $temporary
    $env:TMP = $temporary
    $env:PYTHONDONTWRITEBYTECODE = '1'
    $env:PIP_CACHE_DIR = Join-Path $runtime 'cache/pip'
    $env:npm_config_cache = Join-Path $runtime 'cache/npm'
    $env:HF_HOME = Join-Path $runtime 'cache/huggingface'
    $env:TORCH_HOME = Join-Path $runtime 'cache/torch'
}

function Resolve-Tool([string]$Name, [string[]]$Fallbacks = @()) {
    $found = Get-Command $Name -CommandType Application -ErrorAction SilentlyContinue | Select-Object -First 1
    if ($found) { return $found.Source }
    foreach ($candidate in $Fallbacks) {
        if ($candidate -and (Test-Path -LiteralPath $candidate -PathType Leaf)) {
            return [IO.Path]::GetFullPath($candidate)
        }
    }
    throw "Required tool is missing: $Name"
}

function Import-VsDevEnvironment {
    $vswhere = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
    if (-not (Test-Path -LiteralPath $vswhere)) { throw 'vswhere.exe not found; install Visual Studio C++ Build Tools.' }
    $install = & $vswhere -latest -products * -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
    if ($LASTEXITCODE -ne 0 -or [string]::IsNullOrWhiteSpace($install)) { throw 'Visual Studio C++ toolchain not found.' }
    $devcmd = Join-Path $install.Trim() 'Common7/Tools/VsDevCmd.bat'
    if (-not (Test-Path -LiteralPath $devcmd)) { throw "VsDevCmd.bat not found: $devcmd" }
    $lines = & cmd.exe /d /s /c "`"$devcmd`" -arch=x64 -host_arch=x64 >nul && set"
    if ($LASTEXITCODE -ne 0) { throw 'VsDevCmd failed.' }
    foreach ($line in $lines) {
        $i = $line.IndexOf('=')
        if ($i -le 0) { continue }
        [Environment]::SetEnvironmentVariable($line.Substring(0,$i), $line.Substring($i+1), 'Process')
    }
}

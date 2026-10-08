param(
    [Parameter(Mandatory=$true, Position=0)]
    [ValidateSet("configure", "build", "discover", "test-all", "test", "run", "build-run", "static")]
    [string]$Command,

    [ValidateSet("Fast", "Full", "Deep")]
    [string]$Mode = "Fast",

    [Parameter(ValueFromRemainingArguments=$true)]
    [string[]]$Names
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root
$env:PYTHONDONTWRITEBYTECODE = '1'
$Python = Join-Path (Split-Path -Parent $Root) '.ai/venv/auditor/Scripts/python.exe'
if (-not (Test-Path $Python)) {
    throw "Auditor environment is missing at .ai/venv/auditor; install requirements there before running."
}

$ArgsList = @("-m", "ep_auditor.cli", "--config", "auditor.config.json", "--mode", $Mode, $Command)
if ($Names) { $ArgsList += $Names }
& $Python @ArgsList
exit $LASTEXITCODE

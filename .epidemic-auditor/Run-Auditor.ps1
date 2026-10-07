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
$Python = Join-Path $Root ".venv\Scripts\python.exe"
if (-not (Test-Path $Python)) {
    throw "Run Start-Auditor.ps1 once to create the local environment."
}

$ArgsList = @("-m", "ep_auditor.cli", "--config", "auditor.config.json", "--mode", $Mode, $Command)
if ($Names) { $ArgsList += $Names }
& $Python @ArgsList
exit $LASTEXITCODE

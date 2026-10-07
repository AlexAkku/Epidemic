$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root
$Python = Join-Path $Root ".venv\Scripts\python.exe"
if (-not (Test-Path $Python)) {
    throw "Run Start-Auditor.ps1 once to create the local environment."
}
& $Python -m unittest discover -s tests -v
exit $LASTEXITCODE

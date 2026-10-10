$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:TEMP = Join-Path (Split-Path -Parent $Root) '.ai/tmp'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force -Path $env:TEMP | Out-Null
if (-not $env:EP_AUDITOR_FIXTURE_EVIDENCE) {
    $env:EP_AUDITOR_FIXTURE_EVIDENCE = Join-Path (Split-Path -Parent $Root) (
        '.ai/auditor/test-results/self-test-' + [Guid]::NewGuid().ToString('N'))
}
New-Item -ItemType Directory -Force -Path $env:EP_AUDITOR_FIXTURE_EVIDENCE | Out-Null
Write-Output ('Auditor test captures: ' + $env:EP_AUDITOR_FIXTURE_EVIDENCE)
$Python = Join-Path (Split-Path -Parent $Root) '.ai/venv/auditor/Scripts/python.exe'
if (-not (Test-Path $Python)) {
    throw "Auditor environment is missing at .ai/venv/auditor; install requirements there before testing."
}
& $Python -m unittest discover -s tests -v
exit $LASTEXITCODE

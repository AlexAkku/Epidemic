$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $MyInvocation.MyCommand.Path
Set-Location $Root

$Python = Get-Command py -ErrorAction SilentlyContinue
if ($Python) {
    $PythonExe = "py"
    $PythonArgs = @("-3")
} else {
    $PythonCmd = Get-Command python -ErrorAction SilentlyContinue
    if (-not $PythonCmd) {
        throw "Python 3 was not found. Install Python 3.11+ and retry."
    }
    $PythonExe = "python"
    $PythonArgs = @()
}

$VenvPython = Join-Path $Root ".venv\Scripts\python.exe"
if (-not (Test-Path $VenvPython)) {
    & $PythonExe @PythonArgs -m venv ".venv"
}

& $VenvPython -m pip install --disable-pip-version-check -r "requirements.txt"
& $VenvPython -m ep_auditor --config "auditor.config.json"

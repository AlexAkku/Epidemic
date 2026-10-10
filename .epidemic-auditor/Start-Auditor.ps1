param([switch]$SetupOnly)
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

$Runtime = Join-Path (Split-Path -Parent $Root) '.ai'
$Venv = Join-Path $Runtime 'venv/auditor'
$env:PYTHONDONTWRITEBYTECODE = '1'
$env:PIP_CACHE_DIR = Join-Path $Runtime 'cache/pip'
$env:TEMP = Join-Path $Runtime 'tmp'
$env:TMP = $env:TEMP
New-Item -ItemType Directory -Force -Path $env:TEMP,(Split-Path -Parent $Venv) | Out-Null
$VenvPython = Join-Path $Venv 'Scripts/python.exe'
if (-not (Test-Path $VenvPython)) {
    & $PythonExe @PythonArgs -m venv $Venv
    if ($LASTEXITCODE -ne 0) { throw 'Auditor environment creation failed.' }
}

& $VenvPython -m pip install --disable-pip-version-check -r "requirements.txt"
if ($LASTEXITCODE -ne 0) { throw 'Auditor dependency installation failed.' }
if ($SetupOnly) {
    # Deleting .ai must not require relinking an unchanged native executable.
    $names = @('EpidemicAuditor.exe','EpidemicAuditorObservation.exe','EpidemicAuditorObservation.pdb',
               'EpidemicAuditorEtwBroker.exe','EpidemicAuditorEtwBroker.pdb')
    foreach ($configuration in @('Debug','Release','RelWithDebInfo','MinSizeRel')) {
        $native = Join-Path (Split-Path -Parent $Root) "build/auditor-native/bin/$configuration"
        $missing = @($names | Where-Object { -not (Test-Path -LiteralPath (Join-Path $native $_) -PathType Leaf) })
        if ($missing.Count) { continue }
        $bundle = Join-Path $Runtime 'tools/auditor'
        New-Item -ItemType Directory -Force -Path $bundle | Out-Null
        foreach ($name in $names) { Copy-Item -LiteralPath (Join-Path $native $name) -Destination (Join-Path $bundle $name) -Force }
        & $VenvPython -B (Join-Path $Root 'native/bundle_runtime.py') --output $bundle --executable (Join-Path $bundle 'EpidemicAuditor.exe')
        if ($LASTEXITCODE -ne 0) { throw 'Auditor runtime bundle restoration failed.' }
        break
    }
    return
}
& $VenvPython -m ep_auditor --config "auditor.config.json"

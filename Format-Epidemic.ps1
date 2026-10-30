param(
    [switch]$Apply,
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$script = Join-Path $root 'Format-Epidemic.py'

if (Get-Command py -ErrorAction SilentlyContinue) {
    $python = 'py'
    $pythonArgs = @('-3')
} elseif (Get-Command python -ErrorAction SilentlyContinue) {
    $python = 'python'
    $pythonArgs = @()
} else {
    throw 'Python 3 is required to run the guarded formatter.'
}

$mode = if ($SelfTest) { '--self-test' } elseif ($Apply) { '--apply' } else { '--check' }
& $python @pythonArgs $script $mode
if ($LASTEXITCODE -ne 0) {
    throw "Formatter failed with exit code $LASTEXITCODE"
}

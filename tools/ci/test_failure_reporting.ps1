$ErrorActionPreference = 'Stop'
$runner = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'build.ps1'
$tokens = $null; $parseErrors = $null
$ast = [System.Management.Automation.Language.Parser]::ParseFile($runner, [ref]$tokens, [ref]$parseErrors)
if ($parseErrors.Count) { throw 'Runner has parse errors' }
foreach ($name in @('Add-Issue', 'Assert-ProcessSuccess')) {
    $definition = $ast.Find({ param($node) $node -is [System.Management.Automation.Language.FunctionDefinitionAst] -and $node.Name -eq $name }, $true)
    if ($null -eq $definition) { throw "Missing runner function: $name" }
    Invoke-Expression $definition.Extent.Text
}
foreach ($timedOut in @($false, $true)) {
    $script:Issues = @()
    $result = [pscustomobject]@{ ExitCode=8; TimedOut=$timedOut; Label='fixture-tests'; LogPath='fixture-test.log' }
    $caught = $null
    try { Assert-ProcessSuccess $result 'ctest' } catch { $caught = $_.Exception.Message }
    $expectedRule = if ($timedOut) { 'timeout' } else { 'exit_code' }
    if ($script:Issues.Count -ne 1 -or $script:Issues[0].rule -ne $expectedRule -or
        $script:Issues[0].log_path -ne 'fixture-test.log' -or $caught -ne 'fixture-tests failed. Full log: fixture-test.log') {
        throw "Failure report must preserve the real exit/timeout rule, log path and cause: $caught"
    }
}
$script:Issues = @()
Assert-ProcessSuccess ([pscustomobject]@{ ExitCode=0; TimedOut=$false }) 'ctest'
if ($script:Issues.Count) { throw 'Success must not report a failure' }
Write-Output 'PASS: runner preserves exit-code/timeout diagnostics and emits no false success-path issue.'

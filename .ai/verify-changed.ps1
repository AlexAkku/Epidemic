param(
    [string]$LogDir = "_ai_logs/static"
)

$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$logRoot = [System.IO.Path]::GetFullPath((Join-Path $root $LogDir))
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

Push-Location $root
try {
    $changed = @(
        git -c core.safecrlf=false diff --name-only --diff-filter=ACMR 2>$null
        git -c core.safecrlf=false diff --cached --name-only --diff-filter=ACMR 2>$null
    ) |
        Where-Object { $_ -match '\.(cpp|cc|cxx)$' } |
        Sort-Object -Unique
}
finally {
    Pop-Location
}

if ($changed.Count -eq 0) {
    [ordered]@{
        status = "PASS"
        translation_units = 0
        checks = 0
        failures = @()
    } | ConvertTo-Json -Depth 5 -Compress
    exit 0
}

$clangd = "C:\Program Files\LLVM\bin\clangd.exe"
$clangTidy = "C:\Program Files\LLVM\bin\clang-tidy.exe"
$cppcheck = "C:\Program Files\Cppcheck\cppcheck.exe"

foreach ($tool in @($clangd, $clangTidy, $cppcheck)) {
    if (-not (Test-Path -LiteralPath $tool)) {
        throw "Required static-analysis tool not found: $tool"
    }
}

function New-NativeCheckJob {
    param(
        [string]$ToolName,
        [string]$ToolPath,
        [string[]]$Arguments,
        [string]$RelativeFile,
        [string]$SafeName
    )

    return Start-Job -ArgumentList @(
        $root,
        $logRoot,
        $ToolName,
        $ToolPath,
        $Arguments,
        $RelativeFile,
        $SafeName
    ) -ScriptBlock {
        param($root,$logRoot,$toolName,$toolPath,$arguments,$relativeFile,$safeName)

        Set-Location -LiteralPath $root
        $stdout = Join-Path $logRoot "$safeName.$toolName.stdout.log"
        $stderr = Join-Path $logRoot "$safeName.$toolName.stderr.log"

        $process = Start-Process `
            -FilePath $toolPath `
            -WorkingDirectory $root `
            -ArgumentList $arguments `
            -RedirectStandardOutput $stdout `
            -RedirectStandardError $stderr `
            -Wait `
            -PassThru `
            -NoNewWindow

        [pscustomobject]@{
            file = $relativeFile
            tool = $toolName
            exit_code = $process.ExitCode
            stdout_log = $stdout
            stderr_log = $stderr
        }
    }
}

$jobs = @()

foreach ($relativeFile in $changed) {
    $file = [System.IO.Path]::GetFullPath((Join-Path $root $relativeFile))
    $safe = $relativeFile -replace '[^A-Za-z0-9_.-]', '_'

    $jobs += New-NativeCheckJob `
        -ToolName "clangd" `
        -ToolPath $clangd `
        -RelativeFile $relativeFile `
        -SafeName $safe `
        -Arguments @(
            "--check=`"$file`"",
            "--compile-commands-dir=`"$root`"",
            "--tweaks=none",
            "--log=error"
        )

    $jobs += New-NativeCheckJob `
        -ToolName "clang-tidy" `
        -ToolPath $clangTidy `
        -RelativeFile $relativeFile `
        -SafeName $safe `
        -Arguments @(
            "`"$file`"",
            "-p=`"$root`"",
            "-checks=-*,clang-analyzer-*,bugprone-*"
        )

    $jobs += New-NativeCheckJob `
        -ToolName "cppcheck" `
        -ToolPath $cppcheck `
        -RelativeFile $relativeFile `
        -SafeName $safe `
        -Arguments @(
            "--enable=warning,performance,portability",
            "--std=c++20",
            "--inline-suppr",
            "--error-exitcode=1",
            "`"$file`""
        )
}

Wait-Job $jobs | Out-Null
$results = @(Receive-Job $jobs)
Remove-Job $jobs -Force

function Get-Diagnostics([object]$Result) {
    $lines = @()
    foreach ($path in @($Result.stderr_log, $Result.stdout_log)) {
        if (-not (Test-Path -LiteralPath $path)) { continue }
        $lines += @(
            Select-String `
                -LiteralPath $path `
                -Pattern '(?i)(fatal error|\berror\b|\bwarning\b|exception|C[0-9]{4}|\[[A-Za-z0-9_-]+\])' `
                -ErrorAction SilentlyContinue |
            Select-Object -First 6 |
            ForEach-Object {
                $text = $_.Line.Trim()
                if ($text.Length -gt 500) { $text = $text.Substring(0,500) }
                $text
            }
        )
    }

    return @($lines | Select-Object -Unique | Select-Object -First 8)
}

$failures = @()
foreach ($result in $results) {
    if ($result.exit_code -eq 0) { continue }

    $failures += [ordered]@{
        file = $result.file
        tool = $result.tool
        exit_code = $result.exit_code
        diagnostics = @(Get-Diagnostics $result)
        stdout_log = $result.stdout_log
        stderr_log = $result.stderr_log
    }
}

[ordered]@{
    status = $(if ($failures.Count -eq 0) { "PASS" } else { "FAIL" })
    translation_units = $changed.Count
    checks = $results.Count
    failures = $failures
} | ConvertTo-Json -Depth 8 -Compress

if ($failures.Count -gt 0) { exit 1 }
exit 0

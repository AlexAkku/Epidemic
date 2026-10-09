[CmdletBinding(SupportsShouldProcess=$true, ConfirmImpact='High')]
param([switch]$Force)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'Common.ps1')
$root = Get-ProjectRoot
$ai = [IO.Path]::GetFullPath((Join-Path $root '.ai'))
if (-not $ai.StartsWith($root + [IO.Path]::DirectorySeparatorChar, [StringComparison]::OrdinalIgnoreCase)) { throw 'Invalid .ai path.' }
if ((Test-Path -LiteralPath $ai) -and ((Get-Item -LiteralPath $ai -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)) {
    throw 'Refusing to reset a redirected .ai directory.'
}

if (-not (Test-Path -LiteralPath $ai)) { Write-Output '.ai already absent'; exit 0 }
if ($Force -or $PSCmdlet.ShouldProcess($ai, 'delete all disposable agent/tool state; preserve build/')) {
    # Recognize only this checkout's disposable tool entry points. A PID file
    # is insufficient: manual indexers and MCP workers may not have one.
    $python = [regex]::Escape((Join-Path $ai 'retrieval/.venv/Scripts/python.exe').Replace('/', '\'))
    $mcpEntry = [regex]::Escape((Join-Path $ai 'tools/clangd-mcp-server/dist/index.js').Replace('/', '\'))
    $processes = @(Get-CimInstance Win32_Process)
    $owned = @($processes | Where-Object {
        ($_.Name -eq 'python.exe' -and $_.CommandLine -match ('^"?' + $python + '"?(?:\s|$)')) -or
        ($_.Name -eq 'node.exe' -and $_.CommandLine -match ('(?:^|\s|")' + $mcpEntry + '(?:"|\s|$)'))
    })
    $ownedIds = [Collections.Generic.HashSet[uint32]]::new()
    foreach ($proc in $owned) { [void]$ownedIds.Add($proc.ProcessId) }
    do {
        $children = @($processes | Where-Object { $ownedIds.Contains($_.ParentProcessId) -and -not $ownedIds.Contains($_.ProcessId) })
        foreach ($child in $children) { [void]$ownedIds.Add($child.ProcessId); $owned += $child }
    } while ($children.Count)
    [array]::Reverse($owned)
    foreach ($proc in $owned) {
        $current = Get-CimInstance Win32_Process -Filter "ProcessId=$($proc.ProcessId)"
        if ($current -and $current.CreationDate -eq $proc.CreationDate) {
            Stop-Process -Id $proc.ProcessId -Force -ErrorAction Stop
        }
    }
    Remove-Item -LiteralPath $ai -Recurse -Force
    Write-Output "Deleted $ai"
}

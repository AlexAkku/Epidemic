[CmdletBinding()]
param(
    [string]$EmsdkPath,
    [string]$AscentSource,
    [string]$AssetPython='python',
    [switch]$Serve,
    [ValidateRange(1024,65535)][int]$Port=8088
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..'))
if(-not $EmsdkPath){$EmsdkPath=Join-Path $root '.ai/tools/emsdk'}
$EmsdkPath=(Resolve-Path -LiteralPath $EmsdkPath).Path
if(-not (Test-Path -LiteralPath (Join-Path $EmsdkPath 'upstream/emscripten/emcc.py'))){
    throw 'Install and activate Emscripten 6.0.13 first; see Documentation/web-platform.md.'
}
$env:EMSDK_QUIET='1'
. (Join-Path $EmsdkPath 'emsdk_env.ps1') | Out-Null
$env:EMSDK=$EmsdkPath
$env:EM_CONFIG=Join-Path $EmsdkPath '.emscripten'
$quiet=Join-Path $root 'Development/AgentsRules/Scripts/Run-Quiet.ps1'
function Quote-Argument([string]$value){return "'"+$value.Replace("'","''")+"'"}
function Invoke-WebStep([string]$name,[string[]]$arguments){
    $command=($arguments | ForEach-Object {Quote-Argument $_}) -join ' '
    & $quiet -Name $name -Command ("& "+$command) -TimeoutSec 1800 -MaxJobMemoryGB 8
    if($LASTEXITCODE -ne 0){throw "$name failed; see the reported .ai/logs path"}
}
Push-Location $root
try{
    $configure=@('cmake','--preset','web-release')
    if($AscentSource){$AscentSource=(Resolve-Path -LiteralPath $AscentSource).Path}
    # Explicitly clear a previous optional adapter when the parameter is omitted.
    $configure+="-DEPIDEMIC_WEB_ASCENT_SOURCE=$AscentSource"
    Invoke-WebStep 'web-configure' $configure
    $targets=@('EpidemicWebPlatformApp','EpidemicWebContracts')
    if($AscentSource){
        $targets+='AscentWebDiagnostic';$targets+='AscentWebGame'
        $exporter=Join-Path (Split-Path -Parent $AscentSource) 'Tools/Export-WebAssets.py'
        Invoke-WebStep 'web-assets' @($AssetPython,$exporter,'--game',$AscentSource,'--output',(Join-Path $root 'build/web-release/web'))
    }
    Invoke-WebStep 'web-build' (@('cmake','--build','build/web-release','--parallel','4','--target')+$targets)
    Invoke-WebStep 'web-contracts' @($env:EMSDK_NODE,'build/web-release/web/web-contracts.js')
    Write-Output "Web output: $(Join-Path $root 'build/web-release/web')"
    if($Serve){& $env:EMSDK_PYTHON (Join-Path $root 'tools/Serve-Web.py') --root (Join-Path $root 'build/web-release/web') --port $Port}
}finally{Pop-Location}

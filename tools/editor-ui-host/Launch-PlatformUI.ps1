[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$SdkRoot,[switch]$Smoke)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$sdk=(Resolve-Path -LiteralPath $SdkRoot).Path
$sandbox=Join-Path $root 'Temp/U/intellij'
$out=Join-Path $root 'Temp/U/platform'
$build=Join-Path $root 'Temp/b/ui-host'
$profile=Join-Path $env:LOCALAPPDATA 'EpidemicEngine/EditorUI'
if($Smoke){$profile=Join-Path $out 'profile'}
# Reuse only the disposable IDE container explicitly trusted by the owner in U1.
if(-not(Test-Path "$sandbox/workspace/.idea/misc.xml")){throw 'Trusted U1 SDK container is missing; run the isolated U1 setup first.'}
New-Item -ItemType Directory -Force "$sandbox/plugins/epidemic-editor-platform/lib",$out,$profile | Out-Null
$oldProbe=[IO.Path]::GetFullPath((Join-Path $sandbox 'plugins/epidemic-u1'))
$expectedPlugins=[IO.Path]::GetFullPath((Join-Path $root 'Temp/U/intellij/plugins'))+[IO.Path]::DirectorySeparatorChar
if(-not $oldProbe.StartsWith($expectedPlugins,[StringComparison]::OrdinalIgnoreCase)){throw 'Unexpected probe target'}
if(Test-Path -LiteralPath $oldProbe){Remove-Item -LiteralPath $oldProbe -Recurse -Force}
Copy-Item "$out/epidemic-editor-platform.jar" "$sandbox/plugins/epidemic-editor-platform/lib" -Force
[IO.File]::WriteAllText("$out/platform-status.log",'', [Text.UTF8Encoding]::new($false))
$options=@(Get-Content "$sdk/bin/idea64.exe.vmoptions")+@(
    "-Didea.config.path=$sandbox/config","-Didea.system.path=$sandbox/system","-Didea.plugins.path=$sandbox/plugins","-Didea.log.path=$sandbox/logs",'-Didea.paths.selector=EpidemicU1Probe',
    "-XX:ErrorFile=$out/hs_err_pid%p.log","-XX:HeapDumpPath=$out/java_error.hprof",
    '-Depidemic.ui.platform.enabled=true',"-Depidemic.ui.backend=$build/Debug/EpidemicEditorBackend.exe", "-Depidemic.u1.native=$build/Debug/EpidemicUiViewport.dll",
    "-Depidemic.ui.profile=$profile","-Depidemic.ui.output=$out"
)
if($Smoke){$options+='-Depidemic.ui.platform.smoke=true'}
[IO.File]::WriteAllLines("$out/platform.vmoptions",$options,[Text.UTF8Encoding]::new($false))
$previousOptions=$env:IDEA_VM_OPTIONS;$previousJdk=$env:IDEA_JDK
try{
    $env:IDEA_VM_OPTIONS="$out/platform.vmoptions";$env:IDEA_JDK="$sdk/jbr"
    $style=if($Smoke){'Hidden'}else{'Normal'}
    $owned=Start-Process -FilePath "$sdk/bin/idea64.exe" -ArgumentList @(('"'+"$sandbox/workspace"+'"')) -WindowStyle $style -PassThru
    [IO.File]::WriteAllText("$out/owned-pid.txt",[string]$owned.Id)
    $owned.WaitForExit()
    if($owned.ExitCode){throw "Platform process exit $($owned.ExitCode)"}
    if($Smoke){$status=Get-Content "$out/platform-status.log" -Raw;if($status -notmatch 'PASS Platform native shutdown; surfaces=0' -or $status -match 'FAIL Platform'){throw 'Platform smoke incomplete; inspect platform-status.log and SDK idea.log'}}
}finally{$env:IDEA_VM_OPTIONS=$previousOptions;$env:IDEA_JDK=$previousJdk}

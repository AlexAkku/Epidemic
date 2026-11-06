[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$SdkRoot,
    [Parameter(Mandatory=$true)][string]$JdkRoot,
    [switch]$Test,
    [switch]$Run,
    [switch]$Smoke,
    [switch]$NativeReady
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$sdk=(Resolve-Path -LiteralPath $SdkRoot).Path
$jdk=(Resolve-Path -LiteralPath $JdkRoot).Path
if((Get-Content "$sdk/build.txt" -Raw).Trim() -ne 'IU-253.31033.145'){throw 'Qualify this adapter against the pinned Platform SDK 253.31033.145.'}
if((Get-Content "$jdk/release" -Raw) -notmatch 'JAVA_RUNTIME_VERSION="21.0.7\+6-LTS"'){throw 'Pinned compile JDK: Microsoft OpenJDK 21.0.7+6-LTS.'}
if((Get-Content "$sdk/jbr/release" -Raw) -notmatch 'JAVA_RUNTIME_VERSION="21.0.10\+1-b1163.108"'){throw 'Pinned SDK runtime differs.'}
if(-not $NativeReady){
    & "$PSScriptRoot/Build-EditorUI.ps1" -JdkRoot $jdk -Test:$Test
    if($LASTEXITCODE){throw 'Native/common UI build failed'}
}
$common=Join-Path $root 'Temp/U/ui-host/classes'
$out=Join-Path $root 'Temp/U/platform'
$classes=Join-Path $out 'classes'
$stage=Join-Path $out 'package'
New-Item -ItemType Directory -Force $classes,"$stage/epidemic/editor","$stage/epidemic/u1","$stage/META-INF" | Out-Null
$sources=@(Get-ChildItem "$PSScriptRoot/java-intellij" -Recurse -Filter '*.java'|ForEach-Object FullName)
& "$jdk/bin/javac.exe" --release 21 -encoding UTF-8 -Xlint:deprecation -classpath "$common;$sdk/lib/*" -d $classes $sources
if($LASTEXITCODE){throw 'IntelliJ Platform adapter compile failed'}
# The diagnostic Swing dock/window implementation is intentionally absent from this plugin.
Get-ChildItem "$common/epidemic/editor" -Filter '*.class' | Where-Object { $_.Name -notmatch '^(EditorApp|DockSpace|DockLayout|UiTests|IslandsTabsUI)(\$|\.)' } | Copy-Item -Destination "$stage/epidemic/editor" -Force
Copy-Item "$common/epidemic/u1/NativeViewport*.class" "$stage/epidemic/u1" -Force
Copy-Item "$PSScriptRoot/resources/epidemic/editor/*.tsv" "$stage/epidemic/editor" -Force
Copy-Item "$classes/epidemic/editor/platform" "$stage/epidemic/editor" -Recurse -Force
Copy-Item "$PSScriptRoot/resources-intellij/META-INF/plugin.xml" "$stage/META-INF/plugin.xml" -Force
& "$jdk/bin/jar.exe" --create --file "$out/epidemic-editor-platform.jar" -C $stage .
if($LASTEXITCODE){throw 'Platform plugin packaging failed'}
$entries=& "$jdk/bin/jar.exe" --list --file "$out/epidemic-editor-platform.jar"
if($entries -match 'epidemic/editor/(DockSpace|EditorApp|DockLayout).*\.class'){throw 'Competing Swing owner leaked into Platform package'}
Write-Output 'PASS Platform plugin compile/package; ToolWindowManager/FileEditorProvider/ActionManager; custom Swing dock and app excluded'
if($Run -or $Smoke){
    & "$PSScriptRoot/Launch-PlatformUI.ps1" -SdkRoot $sdk -Smoke:$Smoke
    if($LASTEXITCODE){throw 'Platform host failed'}
}

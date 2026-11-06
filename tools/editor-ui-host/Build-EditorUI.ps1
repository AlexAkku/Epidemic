[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$JdkRoot,
    [switch]$Test
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$jdk=(Resolve-Path -LiteralPath $JdkRoot).Path
if((Get-Content "$jdk/release" -Raw) -notmatch 'JAVA_RUNTIME_VERSION="21.0.7\+6-LTS"'){
    throw 'This UI module is pinned to Microsoft OpenJDK 21.0.7+6-LTS.'
}
$env:JAVA_HOME=$jdk.Replace('\','/')
$build=Join-Path $root 'Temp/b/ui-host'
$out=Join-Path $root 'Temp/U/ui-host'
$classes=[IO.Path]::GetFullPath((Join-Path $out 'classes'))
if($classes -ne [IO.Path]::GetFullPath((Join-Path $root 'Temp/U/ui-host/classes'))){throw 'Unexpected common classes directory'}
if(Test-Path -LiteralPath $classes){Remove-Item -LiteralPath $classes -Recurse -Force}
New-Item -ItemType Directory -Force "$out/classes", "$out/logs", "$out/profile" | Out-Null
& cmake -S $PSScriptRoot -B $build -G 'Visual Studio 18 2026' -A x64 -DEPIDEMIC_UI_NATIVE_VIEWPORT=ON "-DJAVA_HOME=$env:JAVA_HOME"
if($LASTEXITCODE){throw 'UI configure failed'}
& cmake --build $build --config Debug --target EpidemicEditorBackend EpidemicUiViewport --parallel 3
if($LASTEXITCODE){throw 'UI native build failed'}
$sources=@(Get-ChildItem "$PSScriptRoot/java" -Recurse -Filter '*.java' | ForEach-Object FullName)
& "$jdk/bin/javac.exe" --release 21 -encoding UTF-8 -d "$out/classes" $sources
if($LASTEXITCODE){throw 'UI Java compile failed'}
if(Test-Path "$PSScriptRoot/resources") { Copy-Item "$PSScriptRoot/resources/*" "$out/classes" -Recurse -Force }
$profile=Join-Path $env:LOCALAPPDATA 'EpidemicEngine/EditorUI'
if($Test){$profile=Join-Path $out 'profile'}
$arguments=@("-Depidemic.ui.backend=$build/Debug/EpidemicEditorBackend.exe", "-Depidemic.u1.native=$build/Debug/EpidemicUiViewport.dll", "-Depidemic.ui.profile=$profile", "-Depidemic.ui.logs=$out/logs", '-cp', "$out/classes")
if($Test){
    & "$jdk/bin/java.exe" @arguments epidemic.editor.UiTests "$root/Temp/U/ui-tests"
    if($LASTEXITCODE){throw 'UI contract tests failed'}
}

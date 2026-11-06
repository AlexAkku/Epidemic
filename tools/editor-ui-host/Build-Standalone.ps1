[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$JdkRoot,
    [switch]$Resolve,
    [switch]$UpdateLock,
    [switch]$NativeReady,
    [switch]$StandardOpenJdk,
    [ValidateSet('Debug','Release')][string]$Configuration='Release',
    [switch]$NoPackage,
    [switch]$Run,
    [switch]$Smoke
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$jdk=(Resolve-Path -LiteralPath $JdkRoot).Path
if((Get-Content "$jdk/release" -Raw) -notmatch 'JAVA_RUNTIME_VERSION="21.0.7\+6-LTS"'){throw 'Pinned JDK: Microsoft OpenJDK 21.0.7+6-LTS'}
$productRoot=Join-Path $root 'Temp/U/standalone'
$classes=Join-Path $root 'Temp/U/standalone-classes'
$build=Join-Path $root 'Temp/b/ui-host'
if(-not $NoPackage){
    $runningImage=[IO.Path]::GetFullPath((Join-Path $root 'Temp/U/distribution/EpidemicEditor/EpidemicEditor.exe'))
    $running=@(Get-CimInstance Win32_Process -Filter "Name='EpidemicEditor.exe'" | Where-Object {$_.ExecutablePath -eq $runningImage})
    if($running.Count){throw 'Close the running standalone EpidemicEditor before rebuilding its app image. No distribution files were changed.'}
}
$runtime=if($StandardOpenJdk){Join-Path $root 'Temp/U/standalone-runtime'}else{& "$PSScriptRoot/Resolve-Runtime.ps1"}
if([IO.Path]::GetFullPath($classes) -ne [IO.Path]::GetFullPath((Join-Path $root 'Temp/U/standalone-classes'))){throw 'Unexpected generated classes directory'}
if(Test-Path -LiteralPath $classes){Remove-Item -LiteralPath $classes -Recurse -Force}
New-Item -ItemType Directory -Force "$productRoot/lib","$productRoot/native","$productRoot/bin","$productRoot/plugins","$productRoot/licenses",$classes | Out-Null
if($Resolve -or -not(Test-Path "$productRoot/lib/com.jetbrains.intellij.platform.bootstrap-253.31033.145.jar")){
    $libraryRoot=[IO.Path]::GetFullPath((Join-Path $productRoot 'lib'))
    if($libraryRoot -ne [IO.Path]::GetFullPath((Join-Path $root 'Temp/U/standalone/lib'))){throw 'Unexpected generated library directory'}
    Remove-Item -LiteralPath $libraryRoot -Recurse -Force
    New-Item -ItemType Directory -Force $libraryRoot | Out-Null
    $previousJava=$env:JAVA_HOME
    try{$env:JAVA_HOME=$jdk;& mvn.cmd -B -f "$PSScriptRoot/standalone/pom.xml" "-Dmaven.repo.local=$root/Temp/U/maven" "-DoutputDirectory=$productRoot/lib" org.apache.maven.plugins:maven-dependency-plugin:3.8.1:copy-dependencies '-DincludeScope=runtime' '-Dmdep.prependGroupId=true';if($LASTEXITCODE){throw 'Open Platform module resolution failed'}}finally{$env:JAVA_HOME=$previousJava}
}
if(-not $NativeReady){
    & cmake -S $PSScriptRoot -B $build -G 'Visual Studio 18 2026' -A x64 -DEPIDEMIC_UI_NATIVE_VIEWPORT=ON "-DJAVA_HOME=$($jdk.Replace('\','/'))"
    if($LASTEXITCODE){throw 'Native configure failed'}
    & cmake --build $build --config $Configuration --target EpidemicEditorBackend EpidemicUiViewport --parallel 3
    if($LASTEXITCODE){throw 'Native build failed'}
}
if($UpdateLock){& python "$PSScriptRoot/standalone/lock_platform.py" --update}else{& python "$PSScriptRoot/standalone/lock_platform.py"}
if($LASTEXITCODE){throw 'Runtime dependency lock/license collection failed'}
Copy-Item "$PSScriptRoot/standalone/licenses/*" "$productRoot/licenses" -Recurse -Force
$retiredNative=Join-Path $productRoot 'native/EpidemicEditorDesktop.exe'
if(Test-Path -LiteralPath $retiredNative){Remove-Item -LiteralPath $retiredNative -Force}
Copy-Item "$build/$Configuration/EpidemicEditorBackend.exe","$build/$Configuration/EpidemicUiViewport.dll" "$productRoot/native" -Force
$sources=@(Get-ChildItem "$PSScriptRoot/java" -Recurse -Filter '*.java' | Where-Object Name -notin @('EditorApp.java','DockSpace.java','DockLayout.java','UiTests.java','AssetTests.java','IslandsTabsUI.java') | ForEach-Object FullName)
$sources+=@(Get-ChildItem "$PSScriptRoot/java-intellij","$PSScriptRoot/standalone/java","$PSScriptRoot/standalone/upstream" -Recurse -Filter '*.java' | ForEach-Object FullName)
& "$jdk/bin/javac.exe" --release 21 -encoding UTF-8 -Xlint:deprecation -classpath "$productRoot/lib/*" -d $classes $sources
if($LASTEXITCODE){throw 'Standalone product compilation failed'}
Copy-Item "$PSScriptRoot/resources/*","$PSScriptRoot/standalone/resources/*" $classes -Recurse -Force
& python "$PSScriptRoot/standalone/prepare_resources.py" $classes "$productRoot/lib"
if($LASTEXITCODE){throw 'Product resource assembly failed'}
$classPathHeader='Class-Path: '+((Get-ChildItem "$productRoot/lib" -Filter '*.jar'|Where-Object Name -ne 'epidemic.jar'|Sort-Object Name|ForEach-Object Name)-join ' ')
$manifestLines=[Collections.Generic.List[string]]::new();$manifestLines.Add('Manifest-Version: 1.0');$manifestLines.Add('Main-Class: epidemic.launcher.Main')
while($classPathHeader.Length -gt 70){$manifestLines.Add($classPathHeader.Substring(0,70));$classPathHeader=' '+$classPathHeader.Substring(70)}
$manifestLines.Add($classPathHeader)
$manifest=($manifestLines -join "`r`n")+"`r`n`r`n"
[IO.File]::WriteAllText("$productRoot/product.mf",$manifest,[Text.UTF8Encoding]::new($false))
& "$jdk/bin/jar.exe" --create --file "$productRoot/lib/epidemic.jar" --manifest "$productRoot/product.mf" -C $classes .
if($LASTEXITCODE){throw 'Product jar failed'}
[IO.File]::WriteAllText("$productRoot/build.txt",'EP-253.31033.145')
[IO.File]::WriteAllText("$productRoot/product-info.json",(@{name='Epidemic Engine Editor';version='0.3';buildNumber='253.31033.145';productCode='EP';dataDirectoryName='EpidemicEditor';vendor='Epidemic Engine';launch=@()} | ConvertTo-Json -Depth 4),[Text.UTF8Encoding]::new($false))
[IO.File]::WriteAllText("$productRoot/bin/idea.properties",'# Product bootstrap supplies isolated paths.')
# Both the companion and JNI use the current MSVC STL. JBR carries an older
# app-local CRT which must not shadow that toolset (notably std::mutex/future).
# Build a derived product runtime; the checksum-verified upstream cache is intact.
if($StandardOpenJdk -and -not(Test-Path "$runtime/bin/java.exe")){
    & "$jdk/bin/jlink.exe" --add-modules ALL-MODULE-PATH --strip-debug --no-header-files --no-man-pages --output $runtime
    if($LASTEXITCODE){throw 'OpenJDK runtime image failed'}
}
. "$root/Development/AgentsRules/Scripts/Common.ps1"
Import-VsDevEnvironment
$crt=Get-ChildItem -LiteralPath (Join-Path $env:VCToolsRedistDir 'x64') -Directory -Filter 'Microsoft.VC*.CRT' | Select-Object -First 1
if(-not $crt){throw 'Matching x64 MSVC redistributable is missing'}
$crtInfo=[Diagnostics.FileVersionInfo]::GetVersionInfo((Join-Path $crt.FullName 'msvcp140.dll'))
$crtVersion=[version]"$($crtInfo.FileMajorPart).$($crtInfo.FileMinorPart).$($crtInfo.FileBuildPart).$($crtInfo.FilePrivatePart)"
if($crtVersion -lt [version]($env:VCToolsVersion.Trim())){throw 'Redistributable predates the active MSVC toolset'}
$derived=[IO.Path]::GetFullPath((Join-Path $root 'Temp/U/product-runtime'))
if($derived -ne [IO.Path]::GetFullPath((Join-Path $root 'Temp/U/product-runtime'))){throw 'Unexpected product runtime directory'}
$runtimeSignature=[IO.Path]::GetFullPath($runtime)+'|'+(Get-FileHash -LiteralPath "$runtime/release" -Algorithm SHA256).Hash
$marker=Join-Path $derived '.source-runtime'
if(-not(Test-Path "$derived/bin/java.exe") -or -not(Test-Path -LiteralPath $marker) -or [IO.File]::ReadAllText($marker) -ne $runtimeSignature){
    if(Test-Path -LiteralPath $derived){
        if(([IO.File]::GetAttributes($derived) -band [IO.FileAttributes]::ReparsePoint) -ne 0){throw 'Product runtime cache cannot be a reparse point'}
        Remove-Item -LiteralPath $derived -Recurse -Force
    }
    New-Item -ItemType Directory -Force $derived | Out-Null
    foreach($entry in Get-ChildItem -LiteralPath $runtime -Force){Copy-Item -LiteralPath $entry.FullName -Destination $derived -Recurse -Force}
    [IO.File]::WriteAllText($marker,$runtimeSignature)
}
foreach($dll in Get-ChildItem -LiteralPath $crt.FullName -Filter '*.dll'){
    Copy-Item -LiteralPath $dll.FullName -Destination "$derived/bin" -Force
    Copy-Item -LiteralPath $dll.FullName -Destination "$productRoot/native" -Force
}
$runtime=$derived
Write-Output "PASS app-local MSVC CRT $crtVersion in product runtime and native directory; upstream JBR unchanged"
$options=@('-Xmx2048m','-Djava.system.class.loader=com.intellij.util.lang.PathClassLoader','-Dfile.encoding=UTF-8','--enable-native-access=ALL-UNNAMED')
foreach($package in Get-Content "$PSScriptRoot/standalone/jvm-opens.txt"){$options+="--add-opens=$package=ALL-UNNAMED"}
if(-not $NoPackage){
    if($StandardOpenJdk -and -not(Test-Path "$runtime/bin/java.exe")){
        & "$jdk/bin/jlink.exe" --add-modules ALL-MODULE-PATH --strip-debug --no-header-files --no-man-pages --output $runtime
        if($LASTEXITCODE){throw 'OpenJDK runtime image failed'}
    }
    $destination=Join-Path $root 'Temp/U/distribution'
    $image=[IO.Path]::GetFullPath((Join-Path $destination 'EpidemicEditor'))
    if($image -ne [IO.Path]::GetFullPath((Join-Path $root 'Temp/U/distribution/EpidemicEditor'))){throw 'Unexpected generated product path'}
    # Recheck after compilation: the user may have launched the previous image meanwhile.
    $running=@(Get-CimInstance Win32_Process -Filter "Name='EpidemicEditor.exe'" | Where-Object {$_.ExecutablePath -eq (Join-Path $image 'EpidemicEditor.exe')})
    if($running.Count){throw 'Standalone app started during the build; close it before packaging. Existing distribution was preserved.'}
    if(Test-Path -LiteralPath $image){Remove-Item -LiteralPath $image -Recurse -Force}
    $arguments=@('--type','app-image','--name','EpidemicEditor','--app-version','0.3.0','--vendor','Epidemic Engine','--dest',$destination,'--input',$productRoot,'--main-jar','lib/epidemic.jar','--main-class','epidemic.launcher.Main','--runtime-image',$runtime)
    $packageInput=Join-Path $root 'Temp/U/standalone-image-input'
    if([IO.Path]::GetFullPath($packageInput) -ne [IO.Path]::GetFullPath((Join-Path $root 'Temp/U/standalone-image-input'))){throw 'Unexpected generated packaging directory'}
    if(Test-Path -LiteralPath $packageInput){Remove-Item -LiteralPath $packageInput -Recurse -Force}
    New-Item -ItemType Directory -Force $packageInput | Out-Null
    foreach($directory in @('lib','native','bin','licenses')){Copy-Item -LiteralPath "$productRoot/$directory" -Destination $packageInput -Recurse -Force}
    Copy-Item -LiteralPath "$productRoot/build.txt" -Destination $packageInput -Force
    Copy-Item -LiteralPath "$productRoot/product-info.json" -Destination $packageInput -Force
    $arguments[$arguments.IndexOf('--input')+1]=$packageInput
    foreach($option in $options){$arguments+=@('--java-options',$option)}
    & "$jdk/bin/jpackage.exe" @arguments
    if($LASTEXITCODE){throw 'Standalone EXE image failed'}
    Write-Output "PASS standalone Windows application: $image/EpidemicEditor.exe"
}
if($Run -or $Smoke){
    if($NoPackage){$java="$runtime/bin/java.exe";$arguments=$options+@('-cp',"$productRoot/lib/epidemic.jar;$productRoot/lib/*",'epidemic.launcher.Main');if($Smoke){$arguments+='--smoke'};& $java @arguments;if($LASTEXITCODE){throw 'Standalone bootstrap failed'}}
    else{$style=if($Smoke){'Hidden'}else{'Normal'};$launch=@{FilePath="$image/EpidemicEditor.exe";WindowStyle=$style;PassThru=$true};if($Smoke){$launch.ArgumentList=@('--smoke')};$owned=Start-Process @launch;$owned.WaitForExit();if($owned.ExitCode){throw "Standalone EXE exited $($owned.ExitCode)"}}
}

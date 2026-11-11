[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$cache=Join-Path $root 'Temp/U/runtimes'
$name='jbr_jcef-21.0.10-windows-x64-b1163.108'
# Public JBR with JCEF, not a runtime copied from an installed IDE. SHA512 is
# published alongside this exact release by JetBrains (retained here as a pin).
$url="https://cache-redirector.jetbrains.com/intellij-jbr/$name.tar.gz"
$expected='F231D37BF2F358486BC38C21931651B2240A5D13C86B6B86467E43BD5C4DB50E4E5585F1E4515AB7FD7B70B05EFEAAEF644A692799F52BC0270D9E8ABA6CB34A'
$archive=Join-Path $cache "$name.tar.gz"
$runtime=Join-Path $cache $name
New-Item -ItemType Directory -Force $cache | Out-Null
if(-not(Test-Path -LiteralPath $archive)){
    $download=$archive+'.download'
    Invoke-WebRequest $url -OutFile $download -UseBasicParsing -TimeoutSec 180
    if((Get-FileHash -LiteralPath $download -Algorithm SHA512).Hash -ne $expected){throw 'Public JBR download checksum mismatch'}
    Move-Item -LiteralPath $download -Destination $archive
}
if((Get-FileHash -LiteralPath $archive -Algorithm SHA512).Hash -ne $expected){throw 'Cached public JBR checksum mismatch'}
# Re-extract the verified archive so a modified/stale unpacked cache cannot
# enter the product. The public archive remains the offline source of truth.
$expectedRuntime=[IO.Path]::GetFullPath((Join-Path $cache $name))
if([IO.Path]::GetFullPath($runtime) -ne $expectedRuntime){throw 'Unexpected JBR extraction path'}
if(Test-Path -LiteralPath $runtime){
    $item=Get-Item -LiteralPath $runtime
    if($item.FullName -ne $expectedRuntime -or ($item.Attributes -band [IO.FileAttributes]::ReparsePoint)){
        throw 'JBR cache cannot be a reparse point'
    }
    Remove-Item -LiteralPath $runtime -Recurse -Force
}
& tar.exe -xzf $archive -C $cache
if($LASTEXITCODE){throw 'Public JBR extraction failed'}
$release=Get-Content -LiteralPath "$runtime/release" -Raw
if($release -notmatch 'JAVA_RUNTIME_VERSION="21.0.10\+1-b1163.108"' -or $release -notmatch 'IMPLEMENTOR="JetBrains s.r.o."' -or $release -notmatch 'OS_ARCH="x86_64"'){
    throw 'Unexpected public JBR release'
}
if(-not(Test-Path -LiteralPath "$runtime/bin/server/jvm.dll")){throw 'Incomplete public JBR runtime'}
foreach($required in @('bin/jcef.dll','bin/libcef.dll','bin/chrome_elf.dll','bin/cef_server.exe','bin/jcef_helper.exe','legal/java.base/LICENSE')){
    if(-not(Test-Path -LiteralPath (Join-Path $runtime $required))){throw "Incomplete public JCEF runtime: $required"}
}
Write-Output $runtime

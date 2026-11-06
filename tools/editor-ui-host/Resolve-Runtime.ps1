[CmdletBinding()]
param()
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$cache=Join-Path $root 'Temp/U/runtimes'
$name='jbr-21.0.10-windows-x64-b1163.108'
# Public vanilla JBR, not a runtime copied from an installed IDE. SHA512 is
# published alongside this exact release by JetBrains (retained here as a pin).
$url="https://cache-redirector.jetbrains.com/intellij-jbr/$name.tar.gz"
$expected='F9342C1526430FEB5548A848FBDFE8D594ADF797713A6240D9B6371065F0495A561DD16B1D2DCFE627D093A79BA3C9311C0ABD3E7AB1F1F69BCA8DE9E2E2964E'
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
if(-not(Test-Path -LiteralPath "$runtime/bin/java.exe")){
    & tar.exe -xzf $archive -C $cache
    if($LASTEXITCODE){throw 'Public JBR extraction failed'}
}
$release=Get-Content -LiteralPath "$runtime/release" -Raw
if($release -notmatch 'JAVA_RUNTIME_VERSION="21.0.10\+1-b1163.108"' -or $release -notmatch 'IMPLEMENTOR="JetBrains s.r.o."' -or $release -notmatch 'OS_ARCH="x86_64"'){
    throw 'Unexpected public JBR release'
}
if(-not(Test-Path -LiteralPath "$runtime/bin/server/jvm.dll")){throw 'Incomplete public JBR runtime'}
Write-Output $runtime

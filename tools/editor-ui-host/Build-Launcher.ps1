[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$JdkRoot)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$name='cef_binary_137.0.17+gf354b0e+chromium-137.0.7151.104_windows64_minimal'
$cache=Join-Path $root '.ai/cef-sdk'
$archive=Join-Path $cache "$name.tar.bz2"
$expected='76BCC2F4E778679408E011FAB5E010E65D0AA9B989C8BD9D8C9A1E1CF3CE8E2241BB4A45105DE4B9147BAACC380BC0EAA7BFDD88BDB2F78D28C6BC70DD153153'
New-Item -ItemType Directory -Force $cache | Out-Null
if(-not(Test-Path -LiteralPath $archive)){
    $download=$archive+'.download'
    Invoke-WebRequest -UseBasicParsing "https://cef-builds.spotifycdn.com/$name.tar.bz2" -OutFile $download -TimeoutSec 300
    if((Get-FileHash -LiteralPath $download -Algorithm SHA512).Hash -ne $expected){throw 'CEF SDK checksum mismatch'}
    Move-Item -LiteralPath $download -Destination $archive
}
if((Get-FileHash -LiteralPath $archive -Algorithm SHA512).Hash -ne $expected){throw 'Cached CEF SDK checksum mismatch'}
& tar.exe -xjf $archive -C $cache "$name/include" "$name/cmake" "$name/Release/cef_sandbox.lib" "$name/LICENSE.txt"
if($LASTEXITCODE){throw 'CEF SDK extraction failed'}
$JdkRoot=$JdkRoot.Replace('\','/')
$cache=$cache.Replace('\','/')
$build=Join-Path $root 'Temp/b/editor-launcher'
& cmake -S "$PSScriptRoot/standalone/native-launcher" -B $build -G 'Visual Studio 18 2026' -A x64 "-DCEF_SDK=$cache/$name" "-DJAVA_HOME=$JdkRoot" | ForEach-Object { Write-Host $_ }
if($LASTEXITCODE){throw 'Native launcher configure failed'}
& cmake --build $build --config Release --target EpidemicEditor --parallel 2 | ForEach-Object { Write-Host $_ }
if($LASTEXITCODE){throw 'Native sandbox launcher build failed'}
Write-Output "$build/Release/EpidemicEditor.exe"

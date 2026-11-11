[CmdletBinding()]
param([switch]$Resolve)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$work=Join-Path $root '.ai/graph-web'
New-Item -ItemType Directory -Force $work | Out-Null
Copy-Item "$PSScriptRoot/graph-web/package.json","$PSScriptRoot/graph-web/package-lock.json","$PSScriptRoot/graph-web/canvas.js" $work -Force
Push-Location $work
try {
    $options=@('ci','--ignore-scripts','--no-audit','--no-fund','--cache',(Join-Path $root '.ai/npm-cache'))
    if(-not $Resolve){$options+='--offline'}
    & npm.cmd @options
    if($LASTEXITCODE){throw 'Pinned Graph dependencies unavailable; use -Resolve once to populate the offline cache'}
    $output=Join-Path $work 'out'
    if(Test-Path -LiteralPath $output){
        $resolved=(Get-Item -LiteralPath $output)
        if($resolved.FullName -ne [IO.Path]::GetFullPath((Join-Path $root '.ai/graph-web/out')) -or
           ($resolved.Attributes -band [IO.FileAttributes]::ReparsePoint)){throw 'Unexpected Graph output path'}
        Remove-Item -LiteralPath $output -Recurse -Force
    }
    New-Item -ItemType Directory -Force "$output/graph","$output/licenses/rete" | Out-Null
    & node.exe node_modules/esbuild/bin/esbuild canvas.js --bundle --minify --legal-comments=linked "--outfile=$output/graph/canvas.js"
    if($LASTEXITCODE){throw 'Graph canvas compilation failed'}
    Copy-Item package.json,package-lock.json,canvas.js "$output/licenses/rete" -Force
    # Keep the full unmodified upstream texts, including transitive dependencies.
    foreach($manifest in Get-ChildItem node_modules -Recurse -Filter package.json){
        $package=Get-Content -LiteralPath $manifest.FullName -Raw | ConvertFrom-Json
        if(-not $package.name -or -not $package.version){continue}
        $target=Join-Path "$output/licenses/rete" (($package.name -replace '[/@]','_')+'-'+$package.version)
        New-Item -ItemType Directory -Force $target | Out-Null
        Copy-Item -LiteralPath $manifest.FullName -Destination $target -Force
        foreach($notice in Get-ChildItem -LiteralPath $manifest.DirectoryName -File | Where-Object Name -match '^(LICENSE|LICENCE|COPYING|NOTICE)'){
            Copy-Item -LiteralPath $notice.FullName -Destination $target -Force
        }
    }
    Write-Output $output
} finally { Pop-Location }

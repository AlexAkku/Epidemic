[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$JdkRoot,
    [string]$SdkRoot,
    [switch]$DevelopmentPlugin
)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$results=[Collections.Generic.List[object]]::new()
$quiet=Join-Path $root 'Development/AgentsRules/Scripts/Run-Quiet.ps1'
$logs='Temp/U/logs'
function Quote([string]$Value){"'"+$Value.Replace("'","''")+"'"}
function Check([string]$Name,[string]$Command,[int]$Timeout=300){
    & $quiet -Name $Name -Command $Command -LogDir $logs -RuntimeDirectory Temp/U -TimeoutSec $Timeout -ShortOutput
    $code=$LASTEXITCODE
    $results.Add([pscustomobject]@{name=$Name;status=$(if($code -eq 0){'PASS'}else{'FAIL'});exit_code=$code;stdout=$(if(Test-Path "$logs/$Name.stdout.log"){"$logs/$Name.stdout.log"}else{$null});stderr=$(if(Test-Path "$logs/$Name.stderr.log"){"$logs/$Name.stderr.log"}else{$null})})
}
Push-Location $root
try{
    Check 'u8-common-contracts' ("& tools/editor-ui-host/Build-EditorUI.ps1 -JdkRoot "+(Quote $JdkRoot)+" -Test") 900
    Check 'u8-standalone-product' ("& tools/editor-ui-host/Build-Standalone.ps1 -JdkRoot "+(Quote $JdkRoot)+" -Smoke") 1800
    Check 'u8-product-restart' '& tools/editor-ui-host/tests/Test-StandaloneRestart.ps1' 300
    if($DevelopmentPlugin){
        if(-not $SdkRoot){throw '-DevelopmentPlugin requires an explicit -SdkRoot'}
        Check 'u8-development-plugin' ("& tools/editor-ui-host/Build-PlatformUI.ps1 -SdkRoot "+(Quote $SdkRoot)+" -JdkRoot "+(Quote $JdkRoot)+" -Test -Smoke") 420
    }
    Check 'u8-protocol' 'python tools/editor-ui-host/tests/test_protocol.py Temp/b/ui-host/Debug/EpidemicEditorBackend.exe' 60
    Check 'u8-spec-coverage' 'python tools/editor-ui-host/tests/test_catalog.py' 60
    Check 'u8-cpp-only-configure' 'cmake -S tools/editor-ui-host -B Temp/b/ui-cpp-only -G "Visual Studio 18 2026" -A x64 -DEPIDEMIC_UI_NATIVE_VIEWPORT=OFF -DEPIDEMIC_BUILD_MANAGED_BRIDGE=OFF' 120
    Check 'u8-cpp-only-build' 'cmake --build Temp/b/ui-cpp-only --config Debug --target EpidemicEditorBackend --parallel 3' 900
    # This is the required full profile, independently of the supplemental Editor harness.
    Check 'u8-full-profile' '& Development/AgentsRules/Scripts/Invoke-Dev.ps1 -Action Test -Profile full -BuildDirectory Temp/b/ui-validation -RuntimeDirectory Temp/U -BuildJobs 3 -ShortOutput -TimeoutSec 2200' 2260
    Check 'u8-editor-regression' ('ctest --test-dir Temp/b/ui-validation -C Debug -R EpidemicEditor --output-on-failure --no-tests=error --output-junit '+(Quote (Join-Path $root 'Temp/U/u8-editor-regression.xml'))) 420
    Check 'u8-architecture-self-test' 'cmake -P cmake/ArchitectureFreezeSelfTest.cmake' 300
    Check 'u8-public-headers' 'cmake --build Temp/b/ui-validation --config Debug --target EpidemicPublicHeaderSelfContainment --parallel 3' 900
    if(Test-Path 'Temp/U/prepare-host-static.ps1'){
        Check 'u8-static-configure' ("& Temp/U/prepare-host-static.ps1 -JdkRoot "+(Quote $JdkRoot)) 120
        Check 'u8-static' '& Development/AgentsRules/Scripts/Verify-Changed.ps1 -Mode Deep -CompilationDatabaseDirectory Temp/b/ui-host-clangd -RuntimeDirectory Temp/U -ShortOutput' 600
    }else{$results.Add([pscustomobject]@{name='u8-static';status='NOT RUN';reason='Prepare isolated compilation database first (README)'})}
    Check 'u8-diff' 'git diff --check' 60
    if(Test-Path 'Temp/U/check-final-files.py'){Check 'u8-new-files' 'python Temp/U/check-final-files.py' 60}
    $results.Add([pscustomobject]@{name='u8-manual-platform';status='PASS';reason='Owner qualification 2026-11-05: 20/24 PASS; four deviations accepted with revised group-docking, sole-UI and broker-lifecycle requirements. See UI_QA_MATRIX.md; not an automated-input claim.'})
    $results.Add([pscustomobject]@{name='u8-clean-machine';status='NOT RUN';reason='Standalone EXE smoke above checks this workstation; clean-machine deployment, full license review and physical mixed-DPI acceptance require separate actual qualification'})
    $results | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath 'Temp/U/u8-results.json' -Encoding UTF8
    $results | Format-Table name,status -AutoSize
    if(@($results|Where-Object status -ne 'PASS').Count){Write-Output 'Non-PASS diagnostics retained above. U-GATE requires classification against the accepted contract in UI_QA_MATRIX.md; no check failure is silently waived.';exit 1}
}finally{Pop-Location}

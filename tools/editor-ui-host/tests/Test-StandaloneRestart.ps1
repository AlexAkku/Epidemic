[CmdletBinding()]
param([string]$ImageRoot=(Join-Path $PSScriptRoot '../../../Temp/U/distribution/EpidemicEditor'))
$ErrorActionPreference='Stop'
$image=(Resolve-Path -LiteralPath $ImageRoot).Path
$exe=Join-Path $image 'EpidemicEditor.exe'
$state=Join-Path $image 'app/qualification-state'
$log=Join-Path $state 'log/platform-status.log'
$ownerFile=Join-Path $state 'log/owned-main.pid'
$tracked=[Collections.Generic.List[object]]::new()
function Remember([int]$OwnerProcessId,[Diagnostics.Process]$CompletedLaunch=$null){
    $record=Get-CimInstance Win32_Process -Filter "ProcessId=$OwnerProcessId"
    # A successful activation process can finish before CIM observes it. Its
    # original Process handle/exit status still prove completion; never track or
    # terminate a replacement process that happened to reuse that PID.
    if(-not $record -and $CompletedLaunch -and $CompletedLaunch.HasExited -and $CompletedLaunch.ExitCode -eq 0){return $null}
    if(-not $record -or $record.ExecutablePath -ne $exe){throw 'Unexpected regression process owner'}
    $tracked.Add($record)
    return $record
}
function StopOwned($Record){
    $current=Get-CimInstance Win32_Process -Filter "ProcessId=$($Record.ProcessId)"
    if($current -and $current.ExecutablePath -eq $Record.ExecutablePath -and $current.CreationDate -eq $Record.CreationDate){Stop-Process -Id $current.ProcessId -Force}
}
function WaitLauncher($Launch,[DateTime]$Started){
    $deadline=[DateTime]::UtcNow.AddSeconds(75)
    while([DateTime]::UtcNow -lt $deadline){
        if($Launch.HasExited){throw "Launcher exited early: $($Launch.ExitCode)"}
        if((Test-Path $ownerFile) -and (Get-Item $ownerFile).LastWriteTimeUtc -ge $Started){
            try{$text=[IO.File]::ReadAllText($log)}catch{$text=''}
            if($text.Contains('PASS Platform Launcher first')){
                $record=Remember ([int]([IO.File]::ReadAllText($ownerFile)))
                if($record.ParentProcessId -ne $Launch.Id -and $record.ProcessId -ne $Launch.Id){throw 'Unrelated JVM owns test profile'}
                if($text -notmatch 'epidemic\.jar!/com/intellij/platform/ide/bootstrap/DirectoryLock.class'){throw 'Product bootstrap correction not loaded first'}
                return $record
            }
        }
        Start-Sleep -Milliseconds 100
    }
    throw 'Timed out waiting for the isolated Launcher'
}
try{
    $started=[DateTime]::UtcNow
    $first=Start-Process -FilePath $exe -ArgumentList '--qualification' -WindowStyle Hidden -PassThru
    $firstParent=Remember $first.Id
    $firstJvm=WaitLauncher $first $started
    $portFile=Join-Path $state 'system-tcp-v1/.port'
    if([IO.File]::ReadAllText($portFile) -notmatch '^\d+$'){throw 'Windows profile is not using the tested loopback transport'}
    $ownerBefore=[IO.File]::ReadAllText($ownerFile)
    $duplicate=Start-Process -FilePath $exe -ArgumentList '--qualification' -WindowStyle Hidden -PassThru
    [void](Remember $duplicate.Id $duplicate)
    if(-not $duplicate.WaitForExit(30000) -or $duplicate.ExitCode -ne 0){throw 'Second launch failed to activate the live owner'}
    if([IO.File]::ReadAllText($ownerFile) -ne $ownerBefore){throw 'Second launch replaced the live owner diagnostics'}
    Write-Output 'PASS live-owner activation; original profile owner retained'
    StopOwned $firstJvm
    if(-not $first.WaitForExit(5000)){StopOwned $firstParent}
    if(-not (Test-Path $portFile) -or -not (Test-Path (Join-Path $state 'config/.lock'))){throw 'Crash did not retain stale lock/port; regression precondition missing'}
    $restartStarted=[DateTime]::UtcNow
    $restart=Start-Process -FilePath $exe -ArgumentList @('--qualification','--smoke') -WindowStyle Hidden -PassThru
    [void](Remember $restart.Id)
    [void](WaitLauncher $restart $restartStarted)
    if(-not $restart.WaitForExit(180000) -or $restart.ExitCode -ne 0){throw 'Packaged EXE failed stale-profile restart'}
    $text=[IO.File]::ReadAllText($log)
    foreach($required in @('PASS Platform smoke: no fatal Platform diagnostics','PASS Platform native shutdown; surfaces=0','PASS Platform crash/reconnect: same project identity')){
        if(-not $text.Contains($required)){throw "Restart check missing: $required"}
    }
    Write-Output 'PASS actual packaged EXE crash/stale-socket restart, same isolated profile, complete Platform/native smoke'
}finally{
    for($index=$tracked.Count-1;$index -ge 0;$index--){StopOwned $tracked[$index]}
}

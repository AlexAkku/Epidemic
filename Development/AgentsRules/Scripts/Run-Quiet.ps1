param(
    [Parameter(Mandatory = $true)]
    [string]$Name,

    [Parameter(Mandatory = $true)]
    [string]$Command,

    [string]$LogDir = ".ai/logs",

    [ValidateRange(1, 86400)]
    [int]$TimeoutSec = 1800,

    [ValidateRange(4, 128)]
    [double]$MinFreeVirtualGB = 4,

    [ValidateRange(4, 128)]
    [double]$MinFreePhysicalGB = 4,

    [ValidateRange(0.25, 24)]
    [double]$MaxJobMemoryGB = 8
)

$ErrorActionPreference = "Stop"

$root = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../../..'))

$os = Get-CimInstance Win32_OperatingSystem
$freeVirtualGB = [math]::Round(
    (($os.FreeVirtualMemory * 1KB) / 1GB),
    2
)
$freePhysicalGB = ($os.FreePhysicalMemory * 1KB) / 1GB
$jobBudgetBytes = [long]([math]::Min($MaxJobMemoryGB,
    [math]::Min($freeVirtualGB - $MinFreeVirtualGB,
               $freePhysicalGB - $MinFreePhysicalGB)) * 1GB)

if ($jobBudgetBytes -lt 256MB) {
    [ordered]@{
        status = "RESOURCE_BLOCKED"
        name = $Name
        exit_code = 125
        duration_sec = 0
        diagnostics = @(
            "Free virtual/physical memory: $freeVirtualGB/$([math]::Round($freePhysicalGB, 2)) GiB; reserve: $MinFreeVirtualGB/$MinFreePhysicalGB GiB; job needs at least 256 MiB"
        )
        stdout_log = $null
        stderr_log = $null
        report_paths = @()
    } | ConvertTo-Json -Depth 6 -Compress

    exit 125
}

$logRoot = [System.IO.Path]::GetFullPath((Join-Path $root $LogDir))
New-Item -ItemType Directory -Force -Path $logRoot | Out-Null

$safeName = $Name -replace '[^A-Za-z0-9_.-]', '_'
$sha256 = [System.Security.Cryptography.SHA256]::Create()
try {
    $hashBytes = $sha256.ComputeHash([System.Text.Encoding]::UTF8.GetBytes($Name))
}
finally { $sha256.Dispose() }
$tempId = (($hashBytes | ForEach-Object { $_.ToString('x2') }) -join '').Substring(0, 16)
$criticalTempRoot = [IO.Path]::GetFullPath((Join-Path $root '.ai/tmp/jobs'))
$criticalTemp = [System.IO.Path]::GetFullPath((Join-Path $criticalTempRoot "job-$tempId"))
if (-not $criticalTemp.StartsWith($criticalTempRoot + '\', [StringComparison]::OrdinalIgnoreCase)) {
    throw 'temporary target escaped repository .ai/tmp storage'
}
for ($ancestor = [System.IO.DirectoryInfo]::new($criticalTemp); $null -ne $ancestor; $ancestor = $ancestor.Parent) {
    if ($ancestor.Exists -and ($ancestor.Attributes -band [System.IO.FileAttributes]::ReparsePoint)) {
        throw 'temporary storage ancestor is a reparse point'
    }
}
$tempMutex = [System.Threading.Mutex]::new($false, "Local\EpidemicAiQuiet-$tempId")
if (-not $tempMutex.WaitOne(0)) {
    $tempMutex.Dispose()
    throw 'another run owns this command type and temporary directory'
}
New-Item -ItemType Directory -Force -Path $criticalTemp | Out-Null
$runId = "$safeName-$([Guid]::NewGuid().ToString('N').Substring(0, 8))"
$startedAt = Get-Date

$stdout = Join-Path $logRoot "$safeName.stdout.log"
$stderr = Join-Path $logRoot "$safeName.stderr.log"
$script = Join-Path $criticalTemp "command.ps1"
$gate = Join-Path $criticalTemp ".gate"

function Resolve-Pwsh {
    # If run-quiet itself is already executing under PowerShell 7,
    # use exactly that executable.
    if ($PSVersionTable.PSVersion.Major -ge 7) {
        try {
            $current = Get-Process -Id $PID -ErrorAction Stop
            if ($current.Path -and (Test-Path -LiteralPath $current.Path)) {
                return $current.Path
            }
        }
        catch {
        }
    }

    # Normal PATH installation.
    $cmd = Get-Command "pwsh.exe" -ErrorAction SilentlyContinue
    if ($cmd -and $cmd.Source) {
        return $cmd.Source
    }

    # Common system installation.
    if ($env:ProgramFiles) {
        $candidate = Join-Path $env:ProgramFiles "PowerShell\7\pwsh.exe"
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    # Codex bundled PowerShell 7.
    if ($HOME) {
        $candidate = Join-Path $HOME ".cache\codex-runtimes\codex-primary-runtime\dependencies\native\powershell\pwsh.exe"
        if (Test-Path -LiteralPath $candidate) {
            return $candidate
        }
    }

    throw "PowerShell 7 (pwsh.exe) was not found. run-quiet requires PowerShell 7."
}

$pwsh = Resolve-Pwsh

# ---------------------------------------------------------------------------
# Windows Job Object
#
# The launched pwsh waits on a tiny gate before executing the real command.
# This gives us time to attach it to a Job Object first.
#
# Every child process then belongs to the same job:
# cmake -> MSBuild -> cl -> VCTIP etc.
#
# When the root command is finished we close/terminate the job, which removes
# leftover MSBuild node-reuse and telemetry processes without PID hunting.
# ---------------------------------------------------------------------------

if (-not ("AiQuiet.JobObject" -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.ComponentModel;
using System.Runtime.InteropServices;

namespace AiQuiet
{
    public static class JobObject
    {
        private const int JobObjectExtendedLimitInformation = 9;
        private const uint JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE = 0x00002000;
        private const uint JOB_OBJECT_LIMIT_JOB_MEMORY = 0x00000200;

        [StructLayout(LayoutKind.Sequential)]
        private struct MEMORYSTATUSEX
        {
            public uint Length, MemoryLoad;
            public ulong TotalPhysical, AvailablePhysical, TotalPageFile,
                AvailablePageFile, TotalVirtual, AvailableVirtual, AvailableExtendedVirtual;
        }

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool GlobalMemoryStatusEx(ref MEMORYSTATUSEX status);

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool QueryInformationJobObject(IntPtr job, int type,
            out JOBOBJECT_EXTENDED_LIMIT_INFORMATION info, uint length, IntPtr returnedLength);

        public static ulong[] Headroom()
        {
            var status = new MEMORYSTATUSEX();
            status.Length = (uint)Marshal.SizeOf(typeof(MEMORYSTATUSEX));
            if (!GlobalMemoryStatusEx(ref status)) throw new Win32Exception(Marshal.GetLastWin32Error());
            return new ulong[] { status.AvailablePhysical, status.AvailablePageFile };
        }

        public static ulong PeakMemory(IntPtr job)
        {
            JOBOBJECT_EXTENDED_LIMIT_INFORMATION info;
            if (!QueryInformationJobObject(job, JobObjectExtendedLimitInformation, out info,
                (uint)Marshal.SizeOf(typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)), IntPtr.Zero))
                throw new Win32Exception(Marshal.GetLastWin32Error());
            return info.PeakJobMemoryUsed.ToUInt64();
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_BASIC_LIMIT_INFORMATION
        {
            public long PerProcessUserTimeLimit;
            public long PerJobUserTimeLimit;
            public uint LimitFlags;
            public UIntPtr MinimumWorkingSetSize;
            public UIntPtr MaximumWorkingSetSize;
            public uint ActiveProcessLimit;
            public UIntPtr Affinity;
            public uint PriorityClass;
            public uint SchedulingClass;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct IO_COUNTERS
        {
            public ulong ReadOperationCount;
            public ulong WriteOperationCount;
            public ulong OtherOperationCount;
            public ulong ReadTransferCount;
            public ulong WriteTransferCount;
            public ulong OtherTransferCount;
        }

        [StructLayout(LayoutKind.Sequential)]
        private struct JOBOBJECT_EXTENDED_LIMIT_INFORMATION
        {
            public JOBOBJECT_BASIC_LIMIT_INFORMATION BasicLimitInformation;
            public IO_COUNTERS IoInfo;
            public UIntPtr ProcessMemoryLimit;
            public UIntPtr JobMemoryLimit;
            public UIntPtr PeakProcessMemoryUsed;
            public UIntPtr PeakJobMemoryUsed;
        }

        [DllImport("kernel32.dll", CharSet = CharSet.Unicode)]
        private static extern IntPtr CreateJobObject(
            IntPtr lpJobAttributes,
            string lpName
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool SetInformationJobObject(
            IntPtr hJob,
            int JobObjectInfoClass,
            IntPtr lpJobObjectInfo,
            uint cbJobObjectInfoLength
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool AssignProcessToJobObject(
            IntPtr hJob,
            IntPtr hProcess
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool TerminateJobObject(
            IntPtr hJob,
            uint uExitCode
        );

        [DllImport("kernel32.dll", SetLastError = true)]
        private static extern bool CloseHandle(
            IntPtr hObject
        );

        public static IntPtr CreateKillOnClose(long budgetBytes)
        {
            IntPtr job = CreateJobObject(IntPtr.Zero, null);

            if (job == IntPtr.Zero)
            {
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "CreateJobObject failed"
                );
            }

            var info = new JOBOBJECT_EXTENDED_LIMIT_INFORMATION();
            info.BasicLimitInformation.LimitFlags =
                JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE | JOB_OBJECT_LIMIT_JOB_MEMORY;
            info.JobMemoryLimit = new UIntPtr((ulong)budgetBytes);

            int length = Marshal.SizeOf(
                typeof(JOBOBJECT_EXTENDED_LIMIT_INFORMATION)
            );

            IntPtr ptr = Marshal.AllocHGlobal(length);

            try
            {
                Marshal.StructureToPtr(info, ptr, false);

                if (!SetInformationJobObject(
                    job,
                    JobObjectExtendedLimitInformation,
                    ptr,
                    (uint)length))
                {
                    int error = Marshal.GetLastWin32Error();
                    CloseHandle(job);

                    throw new Win32Exception(
                        error,
                        "SetInformationJobObject failed"
                    );
                }
            }
            finally
            {
                Marshal.FreeHGlobal(ptr);
            }

            return job;
        }

        public static void Assign(IntPtr job, IntPtr processHandle)
        {
            if (!AssignProcessToJobObject(job, processHandle))
            {
                throw new Win32Exception(
                    Marshal.GetLastWin32Error(),
                    "AssignProcessToJobObject failed"
                );
            }
        }

        public static void TerminateAndClose(IntPtr job)
        {
            if (job == IntPtr.Zero)
                return;

            // The useful root process has already exited.
            // Anything still in the job is task-owned residue.
            TerminateJobObject(job, 0);
            CloseHandle(job);
        }
    }
}
'@
}

$escapedRoot = $root.Replace("'", "''")
$escapedGate = $gate.Replace("'", "''")
$escapedCriticalTemp = $criticalTemp.Replace("'", "''")

# The child waits for the parent to attach it to the Job Object.
@"
Set-Location -LiteralPath '$escapedRoot'

`$ErrorActionPreference = 'Stop'

# Avoid persistent MSBuild workers and Visual Studio telemetry.
`$env:MSBUILDDISABLENODEREUSE = '1'
`$env:VSCMD_SKIP_SENDTELEMETRY = '1'

# Compilers/filesystem fixtures require real files; transient files stay under disposable .ai/tmp.
# Required filesystem temporaries stay inside disposable repository .ai/tmp.
`$env:TEMP = '$escapedCriticalTemp'
`$env:TMP = '$escapedCriticalTemp'
`$env:PYTHONDONTWRITEBYTECODE = '1'
`$env:PIP_CACHE_DIR = Join-Path '$escapedRoot' '.ai/cache/pip'
`$env:npm_config_cache = Join-Path '$escapedRoot' '.ai/cache/npm'
`$env:HF_HOME = Join-Path '$escapedRoot' '.ai/cache/huggingface'
`$env:TORCH_HOME = Join-Path '$escapedRoot' '.ai/cache/torch'

# run-quiet attaches this process to its Windows Job Object before releasing us.
`$gatePath = '$escapedGate'
`$gateDeadline = (Get-Date).AddSeconds(30)

while (-not (Test-Path -LiteralPath `$gatePath)) {
    if ((Get-Date) -ge `$gateDeadline) {
        Write-Error 'run-quiet process gate timed out'
        exit 125
    }

    Start-Sleep -Milliseconds 25
}

try {
    & {
$Command
    }

    if (-not `$?) {
        exit 1
    }

    if (`$null -eq `$LASTEXITCODE) {
        exit 0
    }

    exit [int]`$LASTEXITCODE
}
catch {
    Write-Error (`$_.Exception.ToString())
    exit 1
}
"@ | Set-Content -LiteralPath $script -Encoding UTF8

$watch = [System.Diagnostics.Stopwatch]::StartNew()

$process = $null
$job = [IntPtr]::Zero
$exitCode = 1
$wrapperFailure = $null
$timedOut = $false
$resourceBlocked = $false
$peakJobBytes = 0
$minimumPhysicalBytes = [ulong]($freePhysicalGB * 1GB)
$minimumVirtualBytes = [ulong]($freeVirtualGB * 1GB)

try {
    $job = [AiQuiet.JobObject]::CreateKillOnClose($jobBudgetBytes)

    $process = Start-Process `
        -FilePath $pwsh `
        -WorkingDirectory $root `
        -ArgumentList @(
            "-NoProfile",
            "-NonInteractive",
            "-ExecutionPolicy", "Bypass",
            "-File", "`"$script`""
        ) `
        -RedirectStandardOutput $stdout `
        -RedirectStandardError $stderr `
        -PassThru `
        -NoNewWindow

    try {
        # Child is still blocked on $gate at this point.
        [AiQuiet.JobObject]::Assign($job, $process.Handle)
    }
    catch {
        try {
            if ($process -and -not $process.HasExited) {
                $process.Kill()
                $process.WaitForExit()
            }
        }
        catch {
        }

        throw
    }

    # Release the child only after it belongs to our Job Object.
    New-Item -ItemType File -Force -Path $gate | Out-Null

    # IMPORTANT:
    # Do not use Start-Process -Wait here.
    #
    # Process.WaitForExit() waits for this pwsh only, not for idle descendants
    # such as MSBuild node-reuse or VCTIP processes.
    while (-not $process.WaitForExit(1000)) {
        $headroom = [AiQuiet.JobObject]::Headroom()
        $minimumPhysicalBytes = [math]::Min($minimumPhysicalBytes, $headroom[0])
        $minimumVirtualBytes = [math]::Min($minimumVirtualBytes, $headroom[1])
        if ($headroom[0] -lt ($MinFreePhysicalGB * 1GB) -or
            $headroom[1] -lt ($MinFreeVirtualGB * 1GB)) {
            $resourceBlocked = $true
            $exitCode = 125
            $wrapperFailure = 'memory reserve breached; owned process job stopped'
            break
        }
        if ($TimeoutSec -gt 0 -and $watch.Elapsed.TotalSeconds -ge $TimeoutSec) {
            $timedOut = $true
            $exitCode = 124
            $wrapperFailure = "command exceeded timeout of $TimeoutSec seconds"
            break
        }
    }
    if (-not $timedOut -and -not $resourceBlocked) {
        $exitCode = $process.ExitCode
    }
}
catch {
    $wrapperFailure = $_.Exception.Message
    $exitCode = 126
}
finally {
    $watch.Stop()

    Remove-Item -LiteralPath $gate -Force -ErrorAction SilentlyContinue

    if ($job -ne [IntPtr]::Zero) {
        try {
            try { $peakJobBytes = [AiQuiet.JobObject]::PeakMemory($job) }
            catch {
                $wrapperFailure = "Job memory accounting failed: $($_.Exception.Message)"
                if ($exitCode -eq 0) { $exitCode = 126 }
            }
            [AiQuiet.JobObject]::TerminateAndClose($job)
        }
        catch {
            if (-not $wrapperFailure) {
                $wrapperFailure = "Job cleanup failed: $($_.Exception.Message)"
            }
            if ($exitCode -eq 0) { $exitCode = 126 }
        }

        $job = [IntPtr]::Zero
    }
    try {
        # Job descendants have exited; only this checked per-type temp slot is removed.
        if (Test-Path -LiteralPath $criticalTemp) {
            if ((Get-Item -LiteralPath $criticalTemp -Force).Attributes -band [System.IO.FileAttributes]::ReparsePoint) {
                throw 'temporary target was replaced by a reparse point'
            }
            Remove-Item -LiteralPath $criticalTemp -Recurse -Force
        }
    }
    catch {
        $wrapperFailure = "Temporary cleanup failed: $($_.Exception.Message)"
        if ($exitCode -eq 0) { $exitCode = 126 }
    }
    finally {
        $tempMutex.ReleaseMutex()
        $tempMutex.Dispose()
    }
}

function Normalize-Line([string]$Line) {
    if ([string]::IsNullOrWhiteSpace($Line)) {
        return $null
    }

    $line = $Line.Trim()

    if ($line -match '^=+$') {
        return $null
    }

    # Avoid useless successful-summary lines being selected because they contain
    # the word "failed".
    if ($line -match '(?i)\b0\s+(tests?\s+)?failed\b') {
        return $null
    }

    if ($line -match '(?i)\bfailed\s*[:=]\s*0\b') {
        return $null
    }

    if ($line.Length -gt 600) {
        $line = $line.Substring(0, 600)
    }

    return $line
}

function Get-StrongDiagnostics([string]$Path) {
    if (-not (Test-Path -LiteralPath $Path)) {
        return @()
    }

    $patterns = @(
        '(?i)\bBLOCKED\b',
        '(?i)\bFAIL(?:ED|URE)?\b',
        '(?i)\bERROR\b',
        '(?i)fatal error',
        '(?i)\bC[0-9]{4}\b',
        '(?i)exception',
        '(?i)access denied',
        '(?i)cannot be evaluated',
        '(?i)\btimed out\b',
        '(?i)\btimeout\b',
        '(?i)assert(?:ion)? failed',
        '(?i)segmentation fault',
        '(?i)stack overflow'
    )

    $found = @()

    foreach ($pattern in $patterns) {
        $found += @(
            Select-String `
                -LiteralPath $Path `
                -Pattern $pattern `
                -ErrorAction SilentlyContinue |
            ForEach-Object {
                Normalize-Line $_.Line
            }
        )
    }

    return @(
        $found |
        Where-Object {
            $_ -and
            $_ -notmatch '^self-check\s+OK'
        } |
        Select-Object -Unique |
        Select-Object -First 10
    )
}


$diagnostics = @()

if ($wrapperFailure) {
    $diagnostics += "run-quiet infrastructure failure: $wrapperFailure"
}

if ($exitCode -ne 0) {
    $diagnostics += Get-StrongDiagnostics $stderr
    $diagnostics += Get-StrongDiagnostics $stdout



    $diagnostics = @(
        $diagnostics |
        Where-Object { $_ } |
        Select-Object -Unique |
        Select-Object -First 12
    )

    # If no known diagnostic pattern matched, return only a small log tail.
    if ($diagnostics.Count -eq 0) {
        foreach ($path in @($stderr, $stdout)) {
            if (-not (Test-Path -LiteralPath $path)) {
                continue
            }

            $diagnostics += @(
                Get-Content `
                    -LiteralPath $path `
                    -Tail 12 `
                    -ErrorAction SilentlyContinue |
                ForEach-Object {
                    Normalize-Line $_
                } |
                Where-Object {
                    $_
                }
            )
        }

        $diagnostics = @(
            $diagnostics |
            Select-Object -Unique |
            Select-Object -First 12
        )
    }
}

[ordered]@{
    status = $(
        if ($resourceBlocked) {
            "RESOURCE_BLOCKED"
        }
        elseif ($timedOut) {
            "TIMEOUT"
        }
        elseif ($exitCode -eq 0) {
            "PASS"
        }
        else {
            "FAIL"
        }
    )
    name = $Name
    exit_code = $exitCode
    duration_sec = [math]::Round($watch.Elapsed.TotalSeconds, 2)
    memory = [ordered]@{
        job_limit_MiB = [math]::Round($jobBudgetBytes / 1MB, 2)
        job_peak_MiB = [math]::Round($peakJobBytes / 1MB, 2)
        minimum_free_physical_GiB = [math]::Round($minimumPhysicalBytes / 1GB, 2)
        minimum_free_virtual_GiB = [math]::Round($minimumVirtualBytes / 1GB, 2)
        reserve_physical_GiB = $MinFreePhysicalGB
        reserve_virtual_GiB = $MinFreeVirtualGB
    }

    diagnostics = @($diagnostics)

    stdout_log = $stdout
    stderr_log = $stderr
} | ConvertTo-Json -Depth 6 -Compress

exit $exitCode

# MS6.3 fixed privileged ETW boundary

The privileged component is `EpidemicAuditorEtwBroker.exe`, a LocalSystem service.
`EpidemicAuditor.exe`, its Python collectors, fixtures, Codex and the IDE remain
non-elevated. This implements the existing real ETW requirements; no requirement,
mandatory fixture, loss check or independent qualification is waived.

The local message pipe admits only the configured non-elevated user SID.
Impersonation is checked, identity/elevation are inspected, and impersonation is
reverted before privileged operations. Remote clients and additional pipe server
instances are denied. Clients verify the pipe server PID against the running SCM
service. Capture control additionally requires the original kernel PID and process
creation time; another process with that SID may inspect status but cannot stop
or cancel its owner's capture. Owner exit or the 180-second recording deadline
cancels only the broker's named WPR instance.

Requests are exactly `START GeneralProfile`, `STATUS`, `STOP <GUID>` and
`CANCEL <GUID>`. No paths, command lines, executable names, profiles, provider
expressions or scripts are accepted from clients. The service invokes the fixed
System32 WPR executable directly, through an owned bounded job with an explicit
inherited-handle list. Every logging command ends with the private instance name.
No default/operator WPR session is controlled.

Installation writes only the fixed ProgramFiles service image and the E: project
`_goal6_handoff/etw-broker` capture root. The image is checked against its qualified SHA-256 under a source
handle that excludes writes/deletion. New output files/directories have SYSTEM
ownership and protected ACLs from creation. SYSTEM/admins can write; the configured
user can read/execute but cannot write, delete, change owner/DACL or redirect paths.
Fresh installation rejects existing paths/reparse points. Repair of an incomplete
installation additionally requires exact old/new image hashes and the same fixed
SCM image path, service type/account and configured SID. It preserves the previous
image and all captures; no user-selected path or service is accepted. This is not
an arbitrary updater. The installer alone enables owner assignment privilege;
the service retains only system profiling, impersonation and traversal privileges.

Captures use generated GUID directories, fixed GeneralProfile and file recording.
Admission requires 6 GiB disk headroom (2 GiB temporary input, 2 GiB merged ETL,
2 GiB reserve); recording is cancelled as incomplete if live output exceeds
2 GiB or free space falls below 2 GiB. Retained storage is bounded to 32 GiB and
128 directories; an accepted trace is nonempty and at most 2 GiB. Retained evidence
is never silently deleted to make a check pass. WPR command results, diagnostics,
QPC receipts and trace bytes are retained. Normal clients hash-copy these files
into existing immutable raw evidence. Missing service/trace/import failures
produce explicit incomplete captures. Successful STOP does not prove zero loss:
independent decoding, loss statistics and actual fixture lifetime matching remain
mandatory before MS6.3 closure. The embedded GeneralProfile is generated from the
SDK's exported File profile: only each collector's buffer count changes from 20
to 512. The owning generator proves the entire restored XML tree equals the
original, preserving providers, keywords, stacks and profile settings.

After verified immutable raw import, the original PID/lifetime owner may use
`CANCEL <GUID>` on its COMPLETE capture to release only the fixed broker-side
`trace.etl` duplicate. A protected release receipt records its SHA-256; clients
check it against the retained raw trace. The broker never accepts an output path
or deletes repository evidence. Normal recording cancellation remains distinct
from this explicit completed-lease release.

Deployment owner: `.ai/ms63-install-broker.ps1`, one authorized UAC native installer
invocation. Automated batch owner: `.ai/ms63-broker-capture.py`, run through the
existing quiet wrapper. `deployment.json` reports installation only, never stage
PASS. Protocol/ACL tests are development evidence, not real ETL qualification.


## Current-workspace integration note (after repository cleanup)

The `_goal6_handoff/etw-broker` store and `.ai/ms63-*` script paths above describe the **historical deployed design**; they are not instructions to restore deleted scripts or assume a fresh clone has the same service installation. Before running a new capture, inspect the actual installed SCM image, ACLs, authorized SID and configured protected store and reconcile them with current source. A privileged installed binary/store must remain OS-protected and must **never** be treated as disposable `.ai/` content. Change a service path only by an explicit reviewed migration with equivalent security tests, fresh deployment and verified end-to-end ETW capture. Normal capture/analysis of large raw traces belongs to the P.4 stage-exit campaign, not every development block.

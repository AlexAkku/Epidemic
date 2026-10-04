# Delta 2: application and local build/test timing

Date: 2026-10-04 (Europe/Ljubljana).
Source: `Deltas/Epidemic_Engine_03-10-26-2_delta`.

## Applied changes

- Persistence discovery now includes name-only Snapshot/Checkpoint DTOs, generic
  nested snapshot identities and relevant forward declarations, while excluding
  template parameters. 24 explicit new semantic reviews were admitted from
  `_goal5_handoff/delta2_persistence_review.json` by the owning
  `tools/record_goal5_delta2_review.py` generator; existing decisions are immutable.
- All 34 owner boundaries now have mandatory non-empty domain-record publication
  fixtures. All 47 participants require nine typed executable obligations,
  including NONEMPTY_DURABLE_ROUNDTRIP. Existing integration and dispatcher/outbox
  proofs remain in place. Matrix records execution only, not semantic decisions.
- Generated C++ and canonical freeze registries were rebuilt from their sources;
  the supplied generated JSON and old execution receipt were not blindly overlaid.
- Current closure: 78 production modules, 497 reviewed candidates, 144 Framework
  durable candidates covered exactly once by 47 participants. UNCLASSIFIED=0,
  UNKNOWN=0; no stage-5.2 gaps. The 291 explicit later-stage debts comprise
  139 in 5.4 and 152 in 5.5. Public surface remains 4374 APIs / 232 headers,
  78/78 LOCAL_READY, with all 51 immutable binary fixtures retained.

## Problems caught and corrected locally

Two early cold-build attempts hit MSVC C1090 in synthetic header consumers.
Reducing parallelism alone did not resolve it. These compile-only probes now use
Embedded debug symbols, eliminating their compiler-PDB service dependency;
production libraries/applications retain ProgramDatabase debug information.
Fresh Matrix builds confirmed the generated MSBuild formats and passed.
Microsoft describes C1090 as a generic PDB-processing error; this record does not
claim a proven unique underlying OS/server cause
([Microsoft C1090 documentation](https://learn.microsoft.com/en-us/cpp/error-messages/compiler-errors-1/fatal-error-c1090?view=msvc-170)).

The new Facts fixture committed a fact but left fact-change events pending, then
accessed an unsuccessful capture Result. Production correctly requires a quiescent
boundary. The fixture now checks Dispatch before capture and uses persistent-fact
count diagnostics. The focused MSVC regression passed, followed by the complete
109/109 Debug profile. Generated participant tests now emit participant/phase
markers so failures identify their last operation instead of producing an empty log.
Failed attempts and their diagnostic logs remain separate in `_goal5_handoff`;
none of their time is included in the successful benchmark.

## Successful cold Matrix timing

Machine: AMD Ryzen 5 2600, 6 cores / 12 logical processors, approximately 32 GiB RAM.
Toolchain: MSVC 19.51.36248.0, Visual Studio 18 2026 x64 hosted compiler.
Build parallelism: `CMAKE_BUILD_PARALLEL_LEVEL=6`.
Command: `./run.ps1 -Mode Matrix -NoCache -TimeoutSec 3600 -HeartbeatSec 20`.

Every profile started in a fresh build directory: no reused CMake cache, objects,
libraries or executables. Windows filesystem cache was not flushed; this is one
observed cold-artifact rebuild, not a reboot-level or averaged performance benchmark.

Matrix: **314/314 PASS**, **36:18** total runner wall time.
UTC interval: 2026-10-04T10:55:44.7534131Z to 2026-10-04T11:32:02.2786067Z.
The two Full profiles alone took **22:39** for configure/build/header checks and
test stages combined, excluding common Validators. Their build stages took
**18:25**; test stages **4:14**. Pure CTest execution was **3:55**
(Debug 216.25 s + Release 18.92 s).

| Profile | Configure/build/headers | Test stage | Combined |
| --- | ---: | ---: | ---: |
| base-debug | 1:39 | 0:10 | 1:49 |
| base-release | 1:52 | 0:09 | 2:01 |
| runtime-debug | 2:57 | 0:31 | 3:28 |
| runtime-release | 3:30 | 0:16 | 3:46 |
| full-debug | 7:44 | 3:44 | 11:27 |
| full-release | 10:41 | 0:31 | 11:11 |

Durations are minutes:seconds, rounded. Build stages include configuration and
public-header self-containment. Test stages include manifest checks, test discovery,
execution and reporting. The common Validators stage took **2:32**. All six build
stages sum to **28:23** and test stages to **5:20**; Matrix also qualifies independent
Base/Runtime builds, so it is not equivalent to a single Full run.

Cold receipt: `_goal5_handoff/delta2_cold_matrix_ci_report.json`.
Latest Matrix receipt: `_goal5_handoff/matrix_ci_report.json`.
Fresh exact-tree qualification is PASS for all 47 participants and 745 fingerprinted
source/evidence files; the qualification check after recording passed.

## Final verification

The subsequent required Full NoCache passed **218/218** (109 Debug + 109 Release)
on the already-built tree. Total runner time was **9:17**, including uncached
Validators (**2:21**), reconfiguration/build/header checks (**2:56**) and test
stages (**3:59**). Pure CTest execution was 224.63 s (209.48 s + 15.15 s).
UTC interval: 2026-10-04T11:32:48.618649Z to 2026-10-04T11:42:05.3425601Z.
Receipt: `_goal5_handoff/delta2_warm_full_ci_report.json`; the latest Full report
is also `_goal5_handoff/full_ci_report.json`. Read-only qualification and closure
checks after Full passed. No qualified code changed between Matrix and Full.
Existing formatter/line-limit/coverage-collector/SAST/archive SKIP policies remain
explicit; no instrumented coverage or remote CI is claimed.

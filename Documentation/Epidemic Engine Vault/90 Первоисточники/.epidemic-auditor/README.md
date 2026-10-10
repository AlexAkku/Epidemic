# Epidemic Auditor

Private, external audit harness for Epidemic Engine. The auditor does not link into the engine and treats the engine process as untrusted.

## Current implementation

This archive is the first usable vertical slice of the architecture:

- Tk GUI with Configure, Build, Discover Tests, Run All Tests, Run Selected, Build + Run, Start Game, Stop.
- CMake and CTest orchestration.
- CTest JSON discovery with labels/prefix grouping.
- Every selected test can be executed as an independent process and captured separately.
- Process-tree CPU, RSS, thread and I/O sampling using an independent clock.
- stdout/stderr capture with timestamps.
- Windows WPR raw ETW capture in Deep mode when `wpr.exe` is available.
- Best-effort `xperf` post-processing of Deep ETL into agent-readable CPU/function and sampled stack text reports when Windows Performance Toolkit is installed.
- Run folders with immutable raw evidence and normalized JSON/JSONL/Markdown reports.
- SQLite run index.
- Lightweight static source scan for file/line counts, TODO/FIXME/HACK/XXX markers and include edges.
- Agent pack: INDEX.md, RUN_SUMMARY.md, FAILURES.md, HOT_PATHS.md, STATIC_SCAN.md, manifest.json.
- Baseline/current run comparison.
- CPU and memory charts in the GUI from externally sampled process data.

Exact function-entry/exit instrumentation and lock ownership reconstruction are available in their supported scopes. Audit-only allocation/causal probes emit declared engine observations; they are not independent external evidence. Deep mode stores WPR ETL and supported native CPU/stack observations. MS6.5/6.6 qualification remains separate from passing individual fixtures.

### Diagnostic profiles and limits

- `audit-memory` enables MSVC x64 AddressSanitizer and allocation probes. CMake removes incompatible `/RTC` and incremental-link options only for that profile and deploys the installed ASan DLL beside executables in `build/`. No download is performed. Allocation stacks/peak/lifetimes describe the observed allocator scope, not the entire heap; live allocations remain leak candidates.
- Build physical controls with `cmake --build build/auditor-native --config Release --target EpidemicAuditorObservation EpidemicAuditorShadowFixture EpidemicAuditorMemoryFixture --parallel 2`. The ASan fixture uses Release and accepts `memory-good`, `uaf`, `double-free`, `oob`, `heap-corruption`, `leak`. Windows ASan does not provide LeakSanitizer; `leak` is a probe-only candidate.
- `EpidemicAuditorObservation.exe --exceptions <binary> [arguments...]` launches only its own child under Windows Debug API. The observation window is bounded to 15 seconds, 4096 exceptions and 8 MiB per record family; overflow, unavailable stacks and timeout produce incomplete capture. It retains first/second chance, code/address/thread/QPC, stack and target exit. Helper exit 0 means capture complete; a failed target cannot finalize PASS. CPU intervals from notification to continuation are measured on that thread and include intervening work; they are not isolated handler cost. Recovery markers are target declarations; the controlled fixture validates its handler path.
- Application Verifier is an explicit scoped Heaps/Leak profile. `ep_auditor.collectors.sanitizer.capture_verifier_fixture` accepts only `EpidemicAuditorShadowFixture.exe` and `verifier-good`, `verifier-bad`, `leak-good`, `leak-bad`; it refuses existing settings, waits for appverif activation/export, captures PID-bound XML and debugger activation, and removes only settings it created. Leak controls unload their allocating DLL. Manual GUI-executable commands must use `Start-Process -Wait -PassThru -WindowStyle Hidden` and the returned `ExitCode`; `$LASTEXITCODE` does not prove completion. Required registry rights are a deployment prerequisite, not permission to elevate the Auditor UI.
- Native API graphs retain module load clocks so address reuse across DLL lifetimes does not merge nodes. IO caller totals describe entry attempts/requested bytes; API outcomes remain UNKNOWN. ETW disk latency uses explicit IRP init/completion pairs, process lifetimes and raw CSV ordinals, never temporal proximity.
- Verifier exports are PID-bound by `parse_verifier_xml`. Empty XML is UNKNOWN: page-heap AV may appear only in the debugger capture. Native reports include actually loaded Verifier module paths. Neither arbitrary sanitizer stderr nor XML alone establishes tool activation or whole-process safety.
- Probe files are bounded, run-owned and sealed before derivation. Windows producer files use exclusive creation; the importer rejects symlinks/junctions, malformed health, reversed clocks, duplicate events and capture loss. Missing optional telemetry never means the corresponding behavior was observed.

### Causal Actions and rolling capture

`audit-deep` and `audit-memory` add optional engine probes; ordinary builds have no probe dependency. Explicit job, EventBus, dispatcher/coroutine, IO, persistence, entity, physics and render handoffs retain IDs. Compiler entry ordinals resolve CallId/FunctionId/ModuleId against sealed traces; unresolved links prevent complete analysis. Entered dispatch is distinct from successful mutation or GPU completion.

Use `python -B -m ep_auditor.cli event-log <sealed-run> --offset 0 --limit 50` to browse observed events and recorded Actions, then `python -B -m ep_auditor.cli action <sealed-run> <ActionId>` for the separate causal graph, call tree, function profile and diagnostic categories. Pages support `--action-id`. Missing categories are UNKNOWN. These commands replay existing captured bytes and do not launch a target.

The desktop **Actions / Event Log** tab opens a sealed capture or a selected Run. Choose a recorded Action, page events, select an event for its raw reference, and browse paged profile/call-tree/DAG/diagnostic details. Queries run off the UI thread through the same verifying backend. Corrupt evidence clears the previous result; INCOMPLETE and UNKNOWN remain visible. COMPLETE capture is not a memory-safety verdict.

`capture.arm_next_input` accepts `action_id` and `name`; the first valid published key/button press starts that Action, and the measured frame boundary closes it. `capture.rolling` accepts finite `pre_seconds`/`post_seconds` in 0–60 and `capacity_bytes` in 64 KiB–8 MiB. The existing runtime exposes a live 65,536-event compiler ring per producer; the collector keeps bounded original ENTER/EXIT bytes and probe records around explicit or external triggers. Overwrite, torn final writes, dropped records and insufficient pre/post coverage forbid complete rolling capture. A process ending at a crash cannot supply a requested post-crash window. Partial ENTER/EXIT slices do not establish a balanced call tree.

Standalone captures select existing fixture binaries explicitly: Core `--auditor-causal`, `--auditor-continuations`, `--auditor-coroutine`, `--auditor-rolling-exact`; Input `--auditor-input`; owner fixtures `--auditor-command`, `--auditor-persistence`, `--auditor-entity`, `--auditor-physics`, `--auditor-render`. Pickup, combat, save/load and streaming use their existing service/wire/runtime implementations. `menu` exercises a native Win32 fixture menu, not an engine UI implementation. ScenarioId and RunId remain independent of CTest registration. None of these focused controls alone qualifies the entire stage.

The `EpidemicExtendedGameplayIntegrationSaveParticipantTests --auditor-save-load` scenario saves and restores a live ItemsInventoryService container through its production save participant and wire codec. `--auditor-save-load-corrupt` separately verifies corrupt-package rejection without changing live state. The decoder's internal C++ exception has no paired unwind EXIT under `/Gh /GH`: that negative capture must remain INCOMPLETE, rather than inventing a complete call tree. Core's causal scenario also exercises the real DefaultAllocator with compiler-bound allocation/deallocation facts.

## Install into the engine tree

Place the folder as:

```text
Epidemic Engine/
  EngineBase/
  EngineRuntime/
  EngineFramework/
  ...
  .epidemic-auditor/
```

Rename this folder to `.epidemic-auditor` if desired. Exclude it locally from the public engine repository with `.git/info/exclude`:

```text
/.epidemic-auditor/
```

Do not add it to the public `.gitignore` if you do not want the public repository to acknowledge the tool.

## Run on Windows 11

PowerShell:

```powershell
Set-ExecutionPolicy -Scope CurrentUser RemoteSigned
.\.epidemic-auditor\Start-Auditor.ps1
```

The launcher creates the environment under repository-root `.ai/venv/auditor` and installs only `psutil`. Python bytecode and pip caches do not go into the source tree.

`Start-Auditor.ps1 -SetupOnly` restores that environment without starting the application; the agent-tools setup reuses it. Native CMake builds belong in `build/auditor-native/`, with compiler executables/PDBs in its `bin/<Configuration>/`. The existing post-build generator copies the executables/helpers into `.ai/tools/auditor/` and creates the private runtime bundle/configuration there. After deleting `.ai/`, setup-only also redeploys the bundle from preserved native outputs, without relinking or reconstructing configuration manually. On a first checkout, build the native target to create those outputs.

## Configuration

Edit `auditor.config.json` or use the Settings tab. Default values assume the auditor folder is directly inside the engine root.

The game/application command is intentionally empty by default. Set `run.command`, for example:

```json
"run": {
  "command": ["../build/audit/bin/Debug/Epidemic.exe"],
  "working_dir": "..",
  "environment": {}
}
```

## Capture modes

- `Fast`: process tree, CPU, RSS, threads, I/O, console, exit code.
- `Full`: currently the same external sampling plus static scan and richer report generation.
- `Deep`: Full plus WPR `GeneralProfile` capture when available.

The capture mode is persisted in every run and never inferred later.

### Instrumented build profiles

Root CMake accepts `EP_AUDITOR_PROFILE=audit` or `audit-deep` on MSVC x64.
These profiles apply `/Gh /GH /Ob0` to C++ targets and link the existing native
exact runtime; ordinary Debug/Release builds have no hooks. The runtime emits
bounded per-process/module streams only when the managed runner supplies
`EP_AUDITOR_TRACE_DIRECTORY`. Missing required streams, dropped events, unmatched
entries/exits, ambiguous cross-module ordering, invalid PDB identity and interrupted
output remain INCOMPLETE. Process pairing health is captured before the raw seal
and carried through derived outputs and independently replayed test batches.
QPC intervals include instrumentation overhead; thread CPU counters have limited
granularity. Function history is scoped to binary-bound identities and verified
complete captures; group projections do not count child executions twice.

`EP_AUDITOR_PROFILE=audit-coverage` uses ClangCL source coverage. The managed test
runner owns `%p-%m` profile files; LLVM tools adjacent to the recorded compiler
merge and export them, including HTML heatmaps. Coverage binds profiles and
binaries, while source hashes describe files at decoding time. Unchanged compiled
source identity must be established separately before interpreting a heatmap as
current-source proof. Zero coverage and one-call results are bounded candidates,
not proof of dead code.

Static scan uses `build/clangd/compile_commands.json` for bounded compiler AST
analysis of owned translation units and declared namespace scopes. It caches
only the current input key under `.ai/auditor/cache`; failed or partial parses
cannot supply a complete compiler graph. Compiler-resolved calls distinguish
calling a function from taking its address. Lexical include/complexity/ownership
and duplicate candidates retain their heuristic provenance. Runtime architecture
uses observed PDB-bound calls; absence outside the capture is not verified.

The six product rule files under `.epidemic-auditor/rules/` use the YAML JSON
subset, schemaVersion 1, with `forbidden_edge` and `max_metric` rules. Unknown or
incomplete facts produce UNKNOWN, never an inferred zero or PASS. Rule config
hashes bind findings to the evaluated configuration. Other domains gain observed
facts only when their corresponding collectors supply complete measurements.

## Evidence layout

Each operation creates a unique run folder under repository-root `.ai/auditor/runs`; the SQLite database is `.ai/auditor/auditor.sqlite3`. C++ builds remain under `build/`.

The ETW broker is a separately installed Windows service. Its native protocol and Python client use the fixed protected capture store `E:\EpidemicAuditorEtwBroker\captures`, outside the disposable workspace. The existing native installer creates its SYSTEM-owned protected ACLs and updates only the fixed SCM image using exact old/new binary hashes and the same authorized user SID. An older installed broker is not relocated by a workspace move: rebuild, review and explicitly deploy the matching broker before capture. Never move privileged storage into `.ai/`, change only the client path, or weaken its ACLs. Non-elevated clients copy verified captures into their Run directories.

```text
20261004_143812_001/
  run.json
  console.jsonl
  process_samples.jsonl
  children.jsonl
  raw/
    trace.etl
  metrics/
    summary.json
    static_scan.json
  tests/
    results.jsonl
  agent/
    INDEX.md
    RUN_SUMMARY.md
    FAILURES.md
    HOT_PATHS.md
    STATIC_SCAN.md
    manifest.json
```

All timestamps in normalized evidence use UTC ISO-8601 plus an independent monotonic offset for duration calculations.

## Trust model

Every normalized record carries provenance. Current external records use values such as:

- `external_process`
- `external_console`
- `external_ctest`
- `external_static_scan`
- `external_wpr_raw`

Engine semantic probes retain separate declared provenance, so an agent can distinguish externally observed evidence from engine-declared facts.

## Current development strategy and MS6.5–MS6.6 backlog

Milestone 6 is **PARTIALLY COMPLETE** as of 2026-10-10. Full closure is deferred
by the user until MS8 is fully complete and the first test game exists in `Game`.
Finish current fixes and bounded regressions, then stop. The comprehensive
manual qualification and independent audit remain pending; neither CI nor
fixture success replaces mandatory physical or Game acceptance.

As directed on 2026-10-10, implement MS6.7–MS6.9 as one integrated capability
package after agreement on the interfaces below. Use compilation, grouped
focused regressions and targeted integration smoke tests during development.
Do not repeat the four-profile engine matrix or the full Auditor Self-Test at
each intermediate milestone. Comprehensive coverage/integration qualification
and an independent agent audit follow the MS6.9 implementation, on explicit
request. This cadence does not permit fabricated PASS/COMPLETE or weaker evidence.

The 2026-10-09 SRC-171–298 matrix was an observation of that version, not a
current acceptance certificate. Its remaining gaps are development backlog:

| SRC | Backlog / acceptance work |
|---|---|
| 186–187 | PARTIAL: physical production engine assert/contract paths, beyond controlled exception-code fixtures. |
| 196 | PARTIAL: allocations in concrete production pickup/combat Actions. |
| 197 | PARTIAL: whole-process peak heap; observed allocator peak is insufficient. |
| 216 | PARTIAL: current physical disk-latency capture; API-entry timing is insufficient. |
| 221–222 | PARTIAL: dedicated observed synchronization/thread API calls; async IO does not call SetEvent/CreateThread. |
| 234 | PARTIAL: physical source-bound FindingId linked into a production causal graph. |
| 249, 276, 294 | PARTIAL: exception-unwind call trees lose EXIT; preserve INCOMPLETE, never manufacture durations. |
| 263 | PARTIAL: current physical static-analysis provenance record. |
| 269, 292, 296 | DEFERRED/BLOCKED by the user until a Game prototype exists: whole-Game scope and production menu; fixtures are not substitutes. |
| 270 | Previous PARTIAL from Tcl callback errors: lifecycle regression and widget/backend smoke PASS on 2026-10-10; broader integrated GUI coverage remains for final acceptance. |
| 198, 204, 220 | Previous FAIL dependency on Verifier ordering and misplaced Virtual API assertion: all four Heaps/Leak controls plus a dedicated VirtualAlloc/VirtualFree physical test PASS on 2026-10-10; integrate into final coverage review. |
| 205–215, 245 | Previous FAIL dependency on an incorrect async IO assertion: physical async/cancel IO, authoritative status and causal terminals PASS on 2026-10-10; integrate with final IO coverage, without treating those old test failures as independent product defects. |

Previously failing broad Verify-Changed Fast/Deep consumers also remain backlog;
successful focused native checks do not certify a full static-analysis gate.
Record actual focused results separately from this backlog. Do not silently
turn deferred or partially observed requirements into qualified requirements.

## Integrated MS6.7–MS6.9 implementation

The 2026-10-10 decision preserves the existing backend, collectors, analyzers,
GUI and agent boundaries. The entries below describe extensions of existing
operations, not new architectural layers. Implement the three stages together;
perform the comprehensive qualification only after the integrated functionality.

Reuse `RunStorage`, `RawEvidence`, `EvidenceLineage`, `authoritative_run`,
`core.operations` and `RunManager`; keep GUI, CLI and agent as consumers of the
same backend. Keep native observation/ETW privilege boundaries unchanged.
No parallel Run database, development receipts or additional acceptance framework.

### Shared interfaces

- `analyzers.summary.query_run_view(run_directory, view, scope, cursor, limit)`: verified read-only projections
  for dashboard and required tabs/graphs. Resolve stable Finding/Test/Scenario/
  Action/Trace/Function/Module IDs through existing run storage and evidence.
  Responses carry RunId, schema, provenance, completeness/availability,
  raw/source references, `truncated` and a snapshot/scope-bound continuation token.
  Missing telemetry is UNKNOWN, not zero. Missing/corrupt mandatory evidence is
  rejected, not displayed as a successful empty result. Reuse existing Action
  and event-log queries; bounded views must not materialize whole traces.
- `analyzers.summary.publish_findings(run_directory)` and the scoped Findings view: one normalized
  finding representation shared by UI/headless/agent. Fields: FindingId, RunId,
  rule/category, severity, confidence, provenance, summary, evidence/source refs,
  reproduction inputs, first/last seen and baseline state. Stable issue identity
  is separate from its Run occurrence. Reproduction is emitted only when verified
  rerun inputs exist; heuristic findings do not become correctness failures.
- `analyzers.compare.compare_runs(baseline_directory, current_directory, cursor, limit)`: verify both runs and compatibility
  of capture profile, target/build configuration, schemas, clocks/units and
  symbol identity mapping. Preserve both source/binary identities; different
  revisions alone are not silently treated as equivalent builds. Incompatible
  observations return INCOMPATIBLE; missing metrics return UNKNOWN. Compare
  functions/edges, CPU/RAM/allocations, p95/p99, exceptions/waits/locks and coverage.
  Missing and nonfinite metrics stay UNKNOWN; measured candidates remain heuristic.
- `export_investigation(run_id, kind, stable_id, limits, cursor)`: extend the
  existing agent/export modules. Defaults: 200 events, 50 frames, 20 related
  findings and 20 graph neighbors. Truncation is explicit and resumable. Generate
  the source-required named Markdown/JSON/JSONL/CSV product outputs from these
  same verified projections, with no alternate findings or status calculation.
- `RunManager.execute('stress', mode)`, `RunManager.stop()` and
  `query_run_view(run_directory, 'Stress')`: existing admitted operations and
  verified views, with an explicit owned target/workspace,
  injector parameters, finite duration/iterations and resource budget. Injection
  records retain InjectorId/type/scope, requested/observed intervals, measured
  effect, evidence refs and cleanup result. ACK is not proof of effect.
  CPU/memory/worker pressure helpers belong to the owned process tree. Disk delay
  and allocation-failure adapters are explicitly scoped; no global storage
  filters or silent changes to ordinary engine behavior. Unavailable observation
  or adapter means BLOCKED/UNKNOWN. Keep existing memory reserves/job limits.
- MS6.9 exercises the existing build/test/run/stress operations and the same
  query/compare/export functions through integration tests. There is no separate
  acceptance operation or qualification framework. The integrated checks include
  production persistence, physical regression controls, stable-ID GUI/agent
  parity, corrupt evidence rejection and deceptive target PASS/nonzero exit.
  One overall stabilization, full matrix and independent audit follow completion
  of the functional package. Future milestone campaigns and Game remain deferred.

### Integrated implementation blocks

1. Verified projections, finding identity, compatibility-aware comparison and
   bounded agent exports; GUI/CLI parity with the same IDs and missing-data rules.
2. Owned injection/campaign lifecycle and real measured good/bad controls;
   save/restore, cancellation and degradation use existing production components.
3. Integrated smoke flow through existing operations: workload/capture -> finding
   -> compare -> GUI/agent navigation -> export -> cancellation/cleanup. Reject
   false internal PASS, incomplete captures and incompatible comparisons.

Each block gets only its changed-source compilation and focused smoke/regression
checks. Product raw/manifests remain under `.ai/`; compiler products stay under
`build/`. Do not implement Game or launch an independent audit in these blocks.

Current integrated checks (2026-10-10) are focused regressions, not milestone
qualification. Shared views/compare/GUI/export tests pass; ETW decoder replay
rejects forged projections, including view reads after incomplete telemetry.
Dashboard panels reuse bounded verified views and label page counts as lower
bounds; missing detectors/baselines remain UNKNOWN. Findings expose verified
captured launch inputs separately from an unexecuted reproduction.

Owned rolling capture under CPU pressure passed with retained original probe
bytes; absent compiler instrumentation remains UNKNOWN. The authorized narrow
broker repair completed, and the real WPR smoke passed with sealed raw evidence,
independent zero-loss verification and a truncated-ETL negative control. The
streamed event dump uses the existing 2 GiB raw bound; analytical text remains
bounded to 512 MiB, and overflow remains an explicit failure. Physical Verifier
Heaps/Leak controls also passed. These are focused results, not full milestone
qualification. No disabled guard or synthetic ETL substitution is accepted.
Full stabilization and independent audit remain pending; Game remains DEFERRED.

The existing native module snapshot now supplies the Modules view and bounded
ModuleId investigation. Its original stdout/helper binding is verified; IDs
identify captured disk binaries, not hashes of loaded memory. Physical lookup,
unknown-ID and raw-corruption controls passed. Existing function history is
available in GUI and the headless `history FunctionId --limit 200` command;
continuations bind the scope and capture snapshot, and function facts replay
their original trace even after a disposable-index update. Compatible measured
history and a reindexed forged metric were tested with native captures.

Agent pack publication rejects a changing run snapshot. Failed stress attempts
retain owned child references, including post-capture quota failures; an absent
observer produces parent FAIL with a retained INCOMPLETE child and an incomplete
view. These focused controls passed. Persistent pressure has an explicit Memory
capture scope: sanitizer stdio, external resources and retained owned files;
exact/diagnostic/causal streams are NOT_CAPTURED, not silently marked complete.
The default capture policy is unchanged. The small physical scenario and three
1000-cycle production Save/Restore pressure controls passed (439.42 seconds),
including retained file count/bytes, structured state checks and raw validation.
This scope does not qualify omitted exact/diagnostic/causal telemetry.
Focused native static checks passed after local fixes; a passing broad Deep gate
has not been established. GitHub CI now builds the native Auditor in Debug and
Release and runs bounded backend/evidence/GUI regressions. Privileged WPR/Verifier,
long stress and whole-Game acceptance are outside that CI subset.

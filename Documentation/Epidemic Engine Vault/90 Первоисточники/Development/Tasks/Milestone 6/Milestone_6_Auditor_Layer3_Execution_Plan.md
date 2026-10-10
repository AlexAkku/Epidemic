# Milestone 6 — Full External Auditor
## Layer 3 — Execution plan

Revision: 2026-10-06  
Authoritative source count: 460  
Source SHA-256: `08ea103776d7e29489fde4e4c1d0d227bc847022d3073026e2b0512eee5fa222`

# P. Execution contract

## P.1 Requirement fidelity

The original460 numbered conversation obligations are immutable input.
Record origin_source_sha256 separately from canonical_requirements_sha256 of
the authoritative repository copy Milestone_6_Auditor_Requirement_Traceability.md.
Future drift is checked against that canonical copy; its byte SHA need not equal
the conversation-source SHA.

A source requirement may be split across implementation tasks, but it may not disappear.
The authoritative mapping is `Milestone_6_Auditor_Requirement_Traceability.md`.

Allowed source-requirement status:

- `NOT_STARTED`;
- `IMPLEMENTED_UNQUALIFIED`;
- `PASS`;
- `BLOCKED`;
- `CONDITIONAL_PASS` only when the original wording is explicitly conditional and the supported condition is recorded.

`DEFERRED` is not a valid final MS6 status.

## P.2 Coarse implementation blocks (development, not qualification)

- Work **one stage at a time**. Inside a stage group related capabilities into a small number of **large, end-to-end implementation blocks** (normally 2–4; MS6.7 may require more). A block owns a usable path across producer/collector, schema, analyzer, public backend and consumer, as applicable; it is not an arbitrary batch of files or single SRC IDs. A block can span many production, test and build files. Preserve every mapped SRC ID.
- At block start, inspect only the relevant source, data contracts and test surfaces. Implement all related production code **and good/bad tests** before running the block's focused validation. Do not build/test after each small edit, method or fixture. Prefer one compile/build plus the targeted tests for that completed block. Use exceptional immediate runs only to prove a serious defect's before/after behavior or unblock compiler errors.
- Avoid a complete ETW capture, full importer replay, entire CTest matrix, full static analysis, fresh builds across every profile or final evidence regeneration during implementation. Prefer compact deterministic input and existing stable real-source captures. Re-run a heavy layer only if its input/consumer contract changed; preserve the actual sensor-loss/integrity checks for final qualification.
- Before qualification, perform **one comprehensive semantic review of the stage's final diff**, dependencies, changed tests/fixtures and relevant trust boundaries. Resolve issues; then **freeze** all production/CMake/test/fixture/analyzer/validator inputs for the final campaign. Do not reread unchanged subsystems reflexively; do review every changed surface and all affected invariants.

## P.3 Stage transitions and historical results

- Finish stage implementation and review before final qualification. No next-stage work while the current stage has a real correctness blocker. A housekeeping-only item (e.g., protected obsolete installer files) cannot block MS6 unless it actually compromises build/runtime correctness, required evidence or security.
- MS6.0/6.1/6.2 historical execution records remain descriptions of prior results, **not substitutes for current-source checks**. Legacy `_goal6_handoff` receipts/generators removed by cleanup must not be recreated solely to revalidate history. Preserve actual permanent regression tests and source-contract behavior.
- On failure after freeze, correct the defect, identify the **affected dependency scope**, and rerun every affected check plus the consolidated stage-exit decision. A changed capture/decoder/analyzer does not automatically invalidate unrelated independent checks. Never claim final PASS from stale artifacts.
- Store development build output under `build/` and agent-generated temporary work, analysis and test logs under `.ai/`. The finished Auditor's **product** RunId/raw/manifest/reanalysis semantics are separate contractual requirements and must not be silently deleted or weakened to enforce development housekeeping. Installed privileged broker binaries/storage live at an explicitly secured, separately qualified OS-managed location, not a disposable workspace path.

## P.4 Strong consolidated stage-exit qualification (once per stage)

After semantic review and freeze, run **one ordered consolidated qualification** using the existing build/test/Auditor capabilities, not one new receipt/validator per requirement or micro-fix. Minimum gate:

1. **Requirement completeness:** all stage-owned original SRC IDs map to real implementation and at least one concrete executable/static/observable acceptance check. Reject absent coverage, unclassified or `IMPLEMENTED_UNQUALIFIED` IDs, silent skips, unsupported claims masquerading as PASS, and any source-text changes without explicit user authorization. Respect source-conditional requirements as written.
2. **Clean integrated build:** rebuild current source with the relevant Debug and Release targets; additionally Audit/Deep/Memory/Coverage configurations where this stage changes those contracts. Assert normal Debug/Release do not acquire Auditor instrumentation and Audit paths include what they claim. Correct CMake, linker flags, binaries/PDB and runtime process ownership are checked, not inferred from docs. One clean build campaign per stage; avoid unrelated matrix rebuilds without impacted edges.
3. **Complete behavior & adversarial controls:** run the stage-owned focused suite as a **single grouped campaign** and any impacted integration tests. For each newly implemented critical detector or security boundary, verify known-bad detection, known-good silence, missing/incomplete/corrupt evidence rejection and a deceptive or negative control relevant to its semantics; verify failures are for the intended reason. Reuse tests/fixtures—do not create proof-of-proof systems.
4. **Real-world observation:** at least one suitable actual Windows/Epidemic fixture for capabilities that synthetic data cannot qualify (e.g. ETW process lifetimes, loss counters, symbolization, native process tree, native thread/lock ownership, exact instrumentation). Check the physical evidence independently of internal target PASS. Record partial/missing/dropped/unsupported states honestly. Use **one canonical reusable raw input** where its producer is unchanged; no repeated giant replay just to refresh paperwork.
5. **Traceability, provenance and diff:** verify observed data, origin/hash/binary/schema identity and replay linkage for claims that depend on them. Audit the final diff of all changed production, build, probe, test and validator files; confirm no fixtures, assertions, warnings or checks were removed/relaxed without equivalent replacement. Check all affected earlier-stage invariants and no new privilege/process/storage regressions.
6. **Final verdict:** record once in a compact stage summary the source revision/commit, tested profiles, commands, test identities/results, representative raw references/hashes as appropriate, full missing/loss/limitations, regressions and remaining blockers. A bare PASS string or checklist alone never closes a stage. `PASS` only if all mandatory checks are complete and green; otherwise `BLOCKED` with precise reasons. Do not generate per-change acceptance packets or extra closure frameworks.

## P.5 Full Milestone-6 final acceptance (additional MS6.9 gate)

Beyond individual stage gates, MS6.9 repeats the **cross-stage product workflow** on a fresh clean build and production-like Windows machine/session: ordinary non-admin user starts the finished GUI EXE; selects engine root; configures/builds/discovers/runs/stops tests; uses supported profiles; collects independent observations; navigates findings through stable IDs; exports and reanalyzes immutable real raw; demonstrates known-bad/good detector and loss/incomplete cases; checks GUI/headless consistency, permissions/IPC and no production dependency pollution. Exercise actual performance/overhead and controlled fault/stress behavior with limits, without granting silent exemptions for expensive operations.

The milestone may emit `GOAL6_AUDITOR_COMPLETE` only if all **460/460** original requirements are demonstrably satisfied under their original wording, every stage gate and relevant integrated suite passes against the final tree, there are no mandatory deferred/unknown/false-PASS cases, and reproducible final inputs/results remain identifiable. The final report may be concise; **the underlying product observations and executable tests must be real**.

# Stage map

| Stage | Source requirements | Gate |
|---|---:|---|
| MS6.0 | governance over 1–460 | requirement/baseline admission |
| MS6.1 | 372–394 + cross-cutting evidence | immutable evidence/replay |
| MS6.2 | 1–42 | orchestration/build/test |
| MS6.3 | 43–92 | process/thread/ETW/sampling |
| MS6.4 | 93–170 | exact trace/coverage/architecture |
| MS6.5 | 171–227 | exceptions/memory/IO/API |
| MS6.6 | 228–298 | causal/probes/actions/scenarios |
| MS6.7 | 299–420, with 372–394 already provided by MS6.1 | findings/UI/compare/agent |
| MS6.8 | 421–436 | fault/stress injectors |
| MS6.9 | 437–460 + 460/460 closure | self-qualification/acceptance |

# MS6.0 — Admission

## M6.0.1 Freeze source requirements
Parse canonical IDs1..460, reject missing/duplicate IDs, store origin and canonical SHA-256 separately.

## M6.0.2 Build traceability matrix
Every source ID must point to one primary stage, one evidence family and one acceptance owner.

## M6.0.3 Inventory actual workspace Auditor
For each actual baseline capability classify at exit:
`VERIFIED_REUSE`, `REPAIR`, `REPLACE`, or `MISSING`.
Known baseline failures remain REPAIR with their original executable evidence.

## M6.0.4 Freeze trust/provenance/confidence enums
No collector may invent a fifth provenance without schema revision.

## M6.0.5 Freeze ID and clock contracts
Implement test fixtures for ID stability and cross-collector timestamp correlation before consumers depend on them.

## M6.0.6 Freeze mode/build isolation
Prove Debug and Release compile/link isolation, positive Audit integration,
and Auditor/raw storage outside production dependencies. Negative injection
must fail; a compile database alone does not qualify link isolation.

## M6.0.7 Prepare fixture registry
Every critical future detector has a named bad/good fixture placeholder before detector work begins.

### Exit
`460/460 mapped`, no source ambiguity that affects implementation, existing baseline classified, no capability assumed PASS.

# MS6.1 — Evidence core

## M6.1.1 Create versioned Run manifest
Includes hashes, producer versions, instrumentation profile, completeness and binary/source fingerprints.

## M6.1.2 Make raw immutable
Analyzer reruns must never mutate files in `raw/`.

## M6.1.3 Implement normalized-event schema
Unknown/unsupported/dropped states are explicit.

## M6.1.4 Implement derived graph/metric/finding lineage
Every derived object stores input evidence references.

## M6.1.5 Implement SQLite index/history
Database loss must not destroy raw capture.

## M6.1.6 Implement JSON/JSONL/CSV/Markdown export
Exports derive from structured evidence.

## M6.1.7 Implement old-raw reanalysis
A new analyzer version can regenerate derived output from a fixture raw capture.

## M6.1.8 Implement canonical run directory contract
All source-required run folders/files exist where applicable.

## M6.1.9 Implement capture health
Lossy collectors expose dropped counters and completeness.

### Exit
Corrupted manifest fixture rejected; raw mutation test rejected; replay fixture reproduces normalized facts; source requirements 372–394 qualified.

# MS6.2 — Orchestration/build/test

## M6.2.1 GUI actions
Configure, Build, Build+Run, Run All, Run One, Run Selected, Run Group, Start Game, Stop.

## M6.2.2 Backend parity
Every GUI action invokes the same backend operation used by headless execution.

## M6.2.3 CTest discovery/catalog
Store required metadata and stable TestId.

## M6.2.4 Test history/stability
Persist results and durations by compatible TestId/build identity.

## M6.2.5 Audit modes/build trees
Fast, Full, Deep, Memory, Coverage, Stress are selectable; required build isolation is enforced.

## M6.2.6 CMake project discovery
Detect project root and configure inputs; record exact commands.

## M6.2.7 Build receipt
Capture flags, defines, compiler/toolchain/optimization/instrumentation and configure/build/link timings.

## M6.2.8 Structured diagnostics
Normalize compiler/linker warnings/errors and emit first-root build finding.

## M6.2.9 Parallel reproduction matrix
Run controlled build parallelism variants and keep each attempt as a separate evidence run.

### Exit
Positive GUI/headless runs PASS; compile/link/PDB-class negative fixtures classified; source requirements 1–42 qualified.

# MS6.3 — Windows observation

## M6.3.1 Managed process ownership
Record PID/PPID/cmd/cwd/env/exit/start/end and complete child tree.

## M6.3.2 Module inventory/symbolization
Loaded EXE/DLL identities; PDB address->function->file:line resolution.

## M6.3.3 External time/process metrics
Wall/CPU/QPC, process-tree CPU, RSS/peak, thread count/history.

## M6.3.4 Thread timeline
Creation/destruction, RUN/WAIT, context switches, scheduler activity.

## M6.3.5 Concurrency diagnostics
Starvation, hung workers, orphan jobs, owner-after-destruction work, cancellation latency, late completion.

## M6.3.6 Wait/lock diagnostics
Waits, mutex/lock contention, owner, hold time, acquisition stack.

## M6.3.7 ETW/WPR/xperf
Capture raw ETL, allow WPA opening, normalize selected events, derive xperf reports.

## M6.3.8 Sampling profiler/flame graph
CPU samples and stacks are correlated with modules/functions.

### Exit
Controlled process/thread/lock fixtures produce expected evidence and no forbidden findings in good variants; source requirements 43–92 qualified.

# MS6.4 — Exact trace/coverage/architecture

## M6.4.1 Exact instrumentation
Deep MSVC path uses `/Gh` `/GH` and Auditor-owned `_penter/_pexit`.

## M6.4.2 Exact raw health
Sequence, QPC, PID/TID/address, drop count and unmatched events are recorded.

## M6.4.3 Exact analysis
Call count/tree/edges/self/inclusive/min/max/mean/p50/p95/p99/thread distribution.

## M6.4.4 Function history
Observed-run count, test usage, historical calls/performance, regressions, never/rarely executed candidates.

## M6.4.5 Coverage
Function/line/branch; per test/group/suite; merged heatmap.

## M6.4.6 Static analysis graphs
Include/module/namespace/API/class graphs, cycles, forbidden deps, size/complexity/ownership/unused/duplicate analysis.

## M6.4.7 Runtime architecture graph
Observed subsystem calls, counts and costs.

## M6.4.8 Static/runtime comparison
Unexpected runtime edges and unobserved/rare APIs are explicit.

## M6.4.9 Rules engine
Load and validate architecture/concurrency/persistence/performance/memory/io YAML rules.

### Exit
Known call-tree fixture exactly matches expected call counts/edges; overflow fixture marks trace incomplete; coverage fixture has known branch result; architecture bad/good fixtures distinguish violations; source requirements 93–170 qualified.

# MS6.5 — Exceptions/memory/IO/API

## M6.5.1 Exception debugger
First/second chance, code/address/thread/stack, handled/unhandled where provable.

## M6.5.2 Crash/assert/contract/recovery
Capture trigger and post-error recovery evidence.

## M6.5.3 Exception metrics
Repeated exceptions, counts and CPU-cost estimate.

## M6.5.4 Allocation tracing
Alloc/free/size/stack/lifetime/rate/function/Action/peak.

## M6.5.5 Safety profiles
Leak/UAF/double-free/OOB/heap corruption with ASan and Application Verifier integration.

## M6.5.6 File IO
Open/close/read/write/seek/mmap/directory enumeration with stack/latency.

## M6.5.7 IO findings
Main-thread synchronous IO and per-function IO statistics.

## M6.5.8 Network/API observation
Network plus selected File/Memory/Sync/Thread/Socket/Window/GPU API families.

## M6.5.9 Import and Deep system-API graph
Import inventory is kept distinct from observed execution.

### Exit
Controlled exception/crash/memory/IO fixtures produce exact expected findings; good variants remain clean; source requirements 171–227 qualified.

# MS6.6 — Causal tracing/probes/actions

## M6.6.1 Causal IDs
Trace/Span/Parent/Event/Call/Finding/Function/Module/Entity/Job/Command IDs implemented.

## M6.6.2 Async causal edges
Job, EventBus, command, coroutine, async IO, physics and render boundaries use explicit evidence.

## M6.6.3 Causal DAG vs call tree
Two separate query/data models.

## M6.6.4 Shadow probes
Job/Event/Command/async/persistence/entity/state-change probes only in Audit build.

## M6.6.5 Provenance/confidence enforcement
Probe facts cannot masquerade as external observations.

## M6.6.6 Interactive event log
Action Start/End and Action query surfaces.

## M6.6.7 Arm next input
Next configured input creates an Action boundary.

## M6.6.8 Rolling/autocapture
Pre/post windows and crash/assert/contract/long-frame/deadlock/memory triggers.

## M6.6.9 Scenario capture
Pickup, save/load, combat, menu, streaming are independent ScenarioIds.

### Exit
Synthetic cross-thread causality fixture reconstructs expected DAG; no edge is created when correlation is intentionally absent; Action query returns all required evidence families; source requirements 228–298 qualified.

# MS6.7 — Findings/UI/compare/agent

## M6.7.1 Findings schema/engine
All required categories/severity/reproduction fields and evidence links.

## M6.7.2 Dashboard/tabs
All source-required tabs and summary values available from backend queries.

## M6.7.3 Visualizations
All required charts/graphs render from structured metrics.

## M6.7.4 Compare/history
Baseline/current deltas for functions, runtime edges, CPU/RAM/allocations/timings/exceptions/waits/locks/coverage.

## M6.7.5 Human exports
Generate every required Markdown/JSON/JSONL/CSV artifact.

## M6.7.6 Agent stable-ID API
Queries by Finding/Test/Scenario/Action/Trace/Function/Module.

## M6.7.7 Bounded packages
Failed-test, trace, Action and Finding packages obey limits and continuation contract.

## M6.7.8 Navigation chain
Finding -> Run -> Scenario/Test/Action -> Trace -> causal -> call tree -> threads/locks -> allocations -> IO -> source.

### Exit
UI navigation test and agent-only root-cause fixture PASS; oversized evidence fixture proves bounded/truncated response behavior; source requirements 299–420 qualified with 372–394 backed by MS6.1.

# MS6.8 — Fault/stress

## M6.8.1 Injector framework
Every injection has InjectorId/config/timing/observed effect/evidence.

## M6.8.2 Disk slowdown
Controlled fixture proves higher IO latency only within scope.

## M6.8.3 Memory/CPU pressure
Measured pressure must be visible in external metrics.

## M6.8.4 Allocation failure
Failure count/locations recorded; non-injected allocations remain distinguishable.

## M6.8.5 Scheduler pressure/cancellation storms
Injected work pressure is traceable through causal/thread evidence.

## M6.8.6 Save/load under load
Repeated and loaded persistence runs produce independent evidence.

## M6.8.7 IO cancellation
Cancellation request/completion outcome recorded.

## M6.8.8 Long stress/controlled degradation/bad environment
Run remains observable even when target degrades/fails.

### Exit
Every injector has proof-of-effect and no-scope-leak fixture; source requirements 421–436 qualified.

# MS6.9 — Final self-qualification

## M6.9.1 Qualification reuse mapping
Original requirements 437–442 are relabeled to the new roadmap without losing capability.

## M6.9.2 Retrospective MS5 verification
Use Auditor to independently re-check representative persistence/determinism claims. A mismatch becomes an MS7 blocker and is not hidden.

## M6.9.3 End-to-end Deep Action
Open one action from input through causal/API/function/thread/allocation/exception/IO evidence.

## M6.9.4 Architecture/performance/dead-code/API-use capability
Demonstrate each on controlled fixtures and then on Epidemic inventory.

## M6.9.5 Independent test-claim verification
At least one engine-internal claim is checked against external evidence rather than accepted from the test output.

## M6.9.6 Full detector matrix
Every critical detector has known-bad and known-good result.

## M6.9.7 Reanalysis compatibility
Old fixture raw is reprocessed by final analyzer.

## M6.9.8 Final 460 matrix
No UNMAPPED, NOT_STARTED, IMPLEMENTED_UNQUALIFIED or mandatory BLOCKED requirements.

## M6.9.9 Epidemic acceptance run
Run Fast, Full, targeted Deep, Memory smoke, Coverage smoke and Stress smoke against the same recorded source baseline.

## M6.9.10 Final verdict
Only machine closure may emit `GOAL6_AUDITOR_COMPLETE`.

### Exit
`460/460 PASS or explicitly source-conditional PASS`, no hidden deferral, self-qualification green, acceptance run recorded.

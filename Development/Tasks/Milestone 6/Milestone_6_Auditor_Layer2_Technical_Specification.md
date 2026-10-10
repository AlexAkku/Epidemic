# Milestone 6 — Full External Auditor
## Layer 2 — Technical specification

Revision: 2026-10-06  
Source requirement count: 460  

# 1. Scope and trust boundary

The Auditor is a separate engineering system that controls experiments against Epidemic Engine and records evidence externally.

The trust boundary is:

```text
Epidemic source / binaries / tests
          |
          | observed or explicitly audit-instrumented
          v
External Auditor
```

The following are prohibited as final proof by themselves:

- an engine test returning exit code 0;
- a log line that says PASS;
- a Markdown checklist;
- an engine-provided timing without an external clock;
- an engine-provided causal edge without provenance;
- a generated report whose raw source is absent.

# 2. Mandatory logical subsystems

The physical implementation may use different folders/classes, but the following contracts must exist.

## 2.1 Run Orchestrator

Owns `RunId` lifecycle.

Required operations:

- configure;
- build;
- build + run;
- discover tests;
- run one test;
- run selected tests;
- run label/subsystem group;
- run all tests;
- run scenario;
- start game;
- stop/cancel managed run;
- compare runs;
- reanalyze an existing raw capture.

Every operation emits a structured receipt with:

- `run_id`;
- operation type;
- start/end timestamps;
- command;
- cwd;
- sanitized environment snapshot;
- build/test/scenario identifiers;
- exit status;
- evidence paths;
- instrumentation profile.

## 2.2 Stable IDs

Minimum identifiers:

- `RunId`;
- `BuildId`;
- `TestId`;
- `ScenarioId`;
- `ActionId`;
- `TraceId`;
- `SpanId`;
- `ParentSpanId`;
- `EventId`;
- `CallId`;
- `FindingId`;
- `FunctionId`;
- `ModuleId`;
- `ThreadId`;
- `JobId`;
- `CommandId`;
- `EntityId`.

Rules:

- `RunId` is unique per execution.
- `TestId` is stable for the same discovered CTest identity.
- `ModuleId` includes binary identity/fingerprint.
- `FunctionId` is stable only within a compatible binary identity unless source identity makes stronger stability provable.
- cross-run comparisons store both stable semantic identity and exact binary identity.
- no UI-only transient integer may be the canonical ID.

## 2.3 Clock model

Auditor distinguishes:

- wall time;
- monotonic trace time;
- CPU time;
- game time;
- simulation time.

External native collectors use one documented monotonic base, preferably QPC on Windows.

Each run stores clock anchors sufficient to correlate:

- Auditor process samples;
- debugger events;
- exact function trace;
- ETW/WPR;
- Action markers;
- shadow-probe events.

If clocks cannot be exactly correlated, the relation is stored with uncertainty rather than treated as exact.

## 2.4 Evidence provenance and confidence

Every normalized event/derived fact supports:

```text
provenance:
  external
  compiler_instrumentation
  static_analysis
  engine_probe

confidence:
  observed
  derived
  declared
  heuristic
```

A derived finding must retain links to the lower evidence used to produce it.

## 2.5 Immutable raw evidence

Raw files are append-only for one Run.

At Run finalization, `manifest.json` contains:

- raw file path;
- size;
- SHA-256;
- collector/version;
- capture start/end;
- completeness state;
- dropped-event count where applicable;
- binary/source fingerprints;
- analyzer version.

Analyzers may be rerun, but raw files are not rewritten by analyzers.

## 2.6 Evidence layers

Canonical layer order:

```text
raw/
normalized/
graphs/
metrics/
findings/
agent/
```

`normalized` may be regenerated from `raw`.
`graphs`, `metrics`, `findings`, and `agent` may be regenerated from lower layers.

Each derived artifact records:

- schema version;
- producer version;
- input hashes;
- generated_at;
- completeness.

## 2.7 SQLite history

SQLite is the durable query/index layer for:

- runs;
- builds;
- tests;
- functions;
- modules;
- traces;
- findings;
- performance history;
- coverage history;
- exceptions;
- waits/locks;
- API usage.

SQLite is not the only copy of primary raw evidence.

# 3. Build and test control

## 3.1 Build profiles

Required logical profiles:

- normal external audit;
- deep exact trace;
- memory;
- coverage.

Each profile has its own build directory.

Auditor records:

- configure command;
- build command;
- generator;
- architecture;
- compiler path/version;
- toolchain;
- defines;
- compile flags;
- linker flags;
- optimization;
- instrumentation flags;
- parallelism;
- configure duration;
- compile duration;
- link duration where derivable.

## 3.2 Build diagnostics

Compiler/linker output is normalized into records:

```text
diagnostic_id
severity
tool
code
message
file
line
column
target
phase
raw_reference
```

The build analyzer must identify the first actionable root diagnostic without discarding subsequent diagnostics.

A controlled fixture must cover at least:

- compiler error;
- linker error;
- missing file/include;
- PDB/C1090-like class;
- parallel-build reproduction matrix.

## 3.3 CTest catalog

Discovery stores:

- `TestId`;
- name;
- labels;
- executable;
- arguments;
- working directory;
- source location if derivable;
- timeout;
- discovered build profile.

History stores pass/fail/duration/stability by TestId and compatible build identity.

# 4. Process and symbol model

Every managed process record contains:

- Auditor process identity;
- OS PID/PPID;
- command line;
- cwd;
- environment snapshot/hash;
- creation/exit time;
- exit code;
- process tree relation.

Loaded module record contains:

- module path;
- base address;
- image size;
- file version;
- binary hash;
- PDB identity where available.

Symbolization must resolve:

```text
address -> ModuleId -> FunctionId -> function -> source file:line
```

Symbolization failure is explicit and does not delete the raw address.

# 5. Windows/native observation

Required external collection families:

- process metrics;
- native debugger;
- ETW/WPR;
- xperf-derived analysis where available;
- sampling profiler;
- module/import inventory.

The native debugger records:

- process start/exit;
- thread start/exit;
- module load/unload;
- first-chance exceptions;
- second-chance exceptions;
- crash exception;
- stack where technically available.

# 6. Thread, wait and lock model

Normalized thread events support:

- thread creation/destruction;
- RUN/WAIT intervals;
- context switches;
- scheduler activity;
- worker active time;
- waits;
- mutex/lock contention;
- lock owner;
- hold duration;
- acquisition stack.

Required analyzers:

- starvation candidate;
- hung worker candidate;
- orphan job candidate;
- work-after-owner-destruction candidate;
- cancellation latency;
- late completion;
- long wait;
- lock contention.

Each heuristic finding includes its detection rule and confidence.

# 7. Exact function tracing

Deep mode uses external Auditor-owned instrumentation support.

For MSVC the required path includes:

- `/Gh`;
- `/GH`;
- Auditor `_penter`;
- Auditor `_pexit`.

Canonical raw event fields:

```text
sequence
monotonic_timestamp
event_kind ENTER|EXIT
PID
TID
instruction/function address
trace completeness metadata
```

Mandatory health metadata:

- total records;
- dropped records;
- buffer capacity;
- first sequence;
- last sequence;
- unmatched ENTER count;
- unmatched EXIT count.

An exact trace is `complete` only if the health contract passes.

Offline analysis computes:

- exact observed call count;
- per-thread call tree;
- caller->callee edges;
- inclusive time;
- self time;
- min/max/mean;
- p50/p95/p99;
- caller count;
- callee count;
- thread distribution;
- hot functions;
- hot paths;
- rare timing spikes.

The trace runtime itself must not recursively instrument its own hooks.

# 8. Coverage

Coverage supports:

- function;
- line;
- branch;
- one test;
- selected group;
- merged qualification suite;
- source heatmap.

Coverage evidence is never interpreted as semantic correctness.

The system can compare:

```text
static source inventory vs observed/covered code
```

and distinguish:

- not covered;
- not observed in runtime traces;
- unreachable/unused candidate;
- excluded by build/profile.

# 9. Static and runtime architecture

Static graphs:

- include;
- module dependency;
- namespace dependency;
- public API;
- class dependency.

Static analyzers:

- cycles;
- forbidden dependencies;
- large functions;
- complexity;
- suspicious ownership;
- unused code;
- duplicate code.

Runtime graph records:

- caller module;
- callee module;
- call count;
- CPU/time cost;
- evidence profile/run.

Static/runtime comparison identifies:

- runtime edge absent from allowed architecture;
- declared edge never observed;
- API with zero/rare observed usage;
- runtime dependency inconsistent with rules.

Rule files:

- `architecture.yaml`;
- `concurrency.yaml`;
- `persistence.yaml`;
- `performance.yaml`;
- `memory.yaml`;
- `io.yaml`.

A rule violation always references the exact rule ID and evidence.

# 10. Exception/crash/assert model

Required events:

- first chance;
- second chance;
- exception code/address;
- thread;
- stack;
- handled/unhandled classification when provable;
- crash;
- assert;
- contract violation;
- recovery continuation/path.

Required metrics/findings:

- exception frequency;
- repeated handled exception pattern;
- estimated CPU cost of repeated exceptions;
- crash root stack;
- recovery outcome.

# 11. Memory and safety

Allocation event supports:

- allocation ID;
- address;
- size;
- timestamp;
- thread;
- stack;
- FunctionId;
- TraceId/ActionId when correlated.

Deallocation closes allocation lifetime.

Metrics:

- allocation rate;
- live bytes;
- peak heap;
- allocations by function;
- allocations by Action;
- lifetime distribution.

Detection/profiles cover:

- leak;
- use-after-free;
- double free;
- out-of-bounds;
- heap corruption;
- AddressSanitizer;
- Application Verifier.

The evidence must identify whether a defect comes from Auditor instrumentation, ASan, Application Verifier or another external source.

# 12. IO, network and OS APIs

File operation schema covers:

- open;
- close;
- read;
- write;
- seek;
- memory-map;
- directory enumeration.

Each event stores:

- path/resource;
- timing/latency if available;
- bytes where available;
- thread;
- stack;
- TraceId/ActionId where correlated.

Required analysis:

- synchronous IO on main thread;
- per-function file IO;
- disk latency;
- network activity.

Selected API families:

- file;
- memory;
- synchronization;
- thread;
- socket;
- window;
- current graphics API when enabled.

Deep mode provides a runtime system-API call graph where capture technology can observe the call; import-table presence alone is not presented as an executed call.

# 13. Causal trace model

The causal DAG is separate from the function call tree.

Required explicit edges include:

- Job submit -> execute;
- EventBus publish -> handler;
- command dispatch -> execution;
- coroutine suspend/schedule -> continuation;
- async IO submit -> completion;
- physics callback;
- render command where correlation exists.

No cross-thread edge is inferred solely from temporal proximity.

# 14. Shadow Audit probes

Allowed probe families:

- JobSystem;
- EventBus;
- CommandBus;
- async/coroutines;
- persistence transactions;
- entity lifecycle;
- semantic state-change probe for Action state changes.

Rules:

- probes compile only in shadow Audit build;
- ordinary production builds contain no Auditor probe dependency;
- every probe event uses `provenance=engine_probe`;
- probe output cannot overwrite external observation;
- probe schema is versioned.

State-change probe fields:

```text
subject_type
subject_id
semantic_path
old_value_hash
new_value_hash
semantic_label
TraceId
provenance
```

# 15. Action and rolling capture

Action:

```text
ActionId
name
start_trace_time
end_trace_time
RunId
```

For an Action, the backend can query:

- functions;
- call tree;
- causal graph;
- exceptions;
- allocations;
- threads;
- waits/locks;
- IO;
- state changes.

`Arm next input` creates the next Action boundary according to the configured input trigger.

Rolling capture records configurable pre-event and post-event windows.

Automatic capture triggers:

- crash;
- assert;
- contract violation;
- long frame threshold configured for the run;
- suspected deadlock;
- memory error.

# 16. Scenario capture

Scenarios are independent from CTest.

Minimum named scenario types:

- pickup item;
- save/load;
- combat;
- menu;
- streaming.

Each Scenario has a ScenarioId and owns its captures/findings.

# 17. Findings

Finding fields:

```text
FindingId
RunId
category
severity
confidence
provenance
title
summary
rule_id
evidence_refs[]
source_refs[]
reproduction
first_seen
last_seen
baseline_state
```

Categories:

- build;
- performance;
- concurrency;
- memory;
- architecture;
- exception;
- IO;
- regression.

A recommendation may be generated only if the evidence links needed to reproduce the finding are present.

# 18. GUI

Required tabs:

- Dashboard;
- Build;
- Tests;
- Timeline;
- Actions;
- Call Tree;
- Causal Graph;
- Functions;
- Threads;
- Locks & Waits;
- Memory;
- Allocations;
- IO;
- Exceptions;
- Coverage;
- Architecture;
- Findings;
- Compare;
- Agent.

Required visualizations:

- CPU;
- memory;
- flame graph;
- thread timeline;
- causal graph;
- module/runtime dependency graph;
- allocation histogram;
- lock contention;
- IO timeline;
- test duration history;
- performance history;
- exception frequency;
- function usage.

GUI uses backend queries and canonical IDs. It does not maintain a second evidence model.

# 19. Run comparison

Comparable runs can report:

- new/removed functions;
- new/removed runtime edges;
- CPU delta;
- RAM delta;
- allocation delta;
- p95/p99 timing delta;
- new exceptions;
- new waits;
- new locks;
- coverage delta.

If profiles/build identities are incompatible, comparison is explicitly marked incompatible or partial.

# 20. Agent interface

Required query keys:

- FindingId;
- TestId;
- ScenarioId;
- ActionId;
- TraceId;
- FunctionId;
- ModuleId.

Agent packages exist for:

- failed test;
- trace;
- Action;
- Finding.

A package contains only bounded relevant evidence and includes continuation/expansion links.

Default bounded response contract:

- at most 200 normalized events;
- at most 50 stack frames total;
- at most 20 related findings;
- at most 20 call/causal neighbors per expansion;
- source snippets are references, not full-source dumps;
- if truncated, `truncated=true` and a continuation token are mandatory.

Named human-readable exports include:

- `INDEX.md`;
- `RUN_SUMMARY.md`;
- `BUILD_DIAGNOSTICS.md`;
- `FAILURES.md`;
- `REGRESSIONS.md`;
- `ACTIONS.md`;
- `EXCEPTIONS.md`;
- `FUNCTION_PROFILE.md`;
- `CALL_GRAPH.md`;
- `THREADS.md`;
- `LOCKS.md`;
- `MEMORY.md`;
- `IO.md`;
- `COVERAGE.md`;
- `ARCHITECTURE.md`;
- `HOT_PATHS.md`;
- `IMPORTED_APIS.md`;
- `STATIC_SCAN.md`;
- `RAW_EVIDENCE.md`.

# 21. Fault/stress injection

Each injector has:

```text
InjectorId
type
scope
parameters
start/end
observed_effect
evidence_refs
```

Required injectors/campaign primitives:

- disk IO slowdown;
- memory pressure;
- CPU pressure;
- allocation failure;
- worker scheduling pressure;
- cancellation storm;
- save under load;
- IO cancellation;
- long-running stress;
- repeated save/restore;
- persistent-state load;
- async-cancellation load;
- controlled degradation;
- causal tracing under load;
- bad-environment execution.

An injector is not PASS because it was requested; a fixture must prove the intended effect occurred.

# 22. Self-qualification

Critical detector families require positive and negative fixtures.

Minimum fixture families:

- build compiler error;
- linker error;
- PDB/C1090-class failure;
- process tree child/orphan;
- thread starvation;
- long lock contention;
- deadlock/suspected deadlock;
- cancellation latency;
- exact known call tree;
- exact call count;
- trace overflow/drop;
- exception storm;
- crash;
- assert/contract;
- leak;
- UAF;
- double free;
- OOB/heap corruption;
- sync main-thread IO;
- forbidden dependency;
- runtime unexpected dependency;
- unused/dead API candidate;
- performance regression;
- cross-thread Job/Event/Command causality;
- rolling pre/post capture;
- stale/broken raw manifest;
- reanalysis of old raw;
- agent bounded package;
- each fault injector.

For each fixture:

```text
fixture_id
bad_variant
good_variant
expected_findings
forbidden_findings
evidence_required
```

## Windows application contract

Основной пользовательский артефакт MS6:

`EpidemicAuditor.exe`

`EpidemicAuditor.exe` является владельцем GUI и верхнеуровневого Run Orchestrator.

Допустимы дочерние helper processes:
- symbolizer;
- native debugger;
- trace collector;
- ETW/WPR helpers;
- coverage tools;
- fault injectors;
- analyzer workers.

Но пользователь не должен вручную запускать их для штатной работы.

CLI/headless интерфейс может существовать для automation и Agent workflows,
но он является дополнительным интерфейсом того же backend, а не заменой desktop application.

# 23. Final MS6 closure

PASS requires:

- source requirements: 460;
- mapped: 460;
- unmapped: 0;
- silently dropped/deferred mandatory requirements: 0;
- all stage gates PASS;
- all final artifacts generated from structured evidence;
- exact raw hashes recorded;
- final Auditor self-tests PASS;
- final Epidemic acceptance audit completed.

Final status:

```text
GOAL6_AUDITOR_COMPLETE
```

This status does not imply `SYSTEM_READY`, `FREEZE_READY` or `FROZEN`.

**Current-source qualification boundary:** apply Layer 3 P.4 at each stage exit and P.5 at final closure. Final qualification must independently challenge good/bad detectors, corrupt/missing/lost observations and actual GUI/headless/native Windows end-to-end behavior. Complete the stage's implementation in large coherent blocks before testing, but do not weaken mandatory product-level provenance, immutable raw, privacy/privilege or replay contracts. Historical generated receipts are not present-tree proof.

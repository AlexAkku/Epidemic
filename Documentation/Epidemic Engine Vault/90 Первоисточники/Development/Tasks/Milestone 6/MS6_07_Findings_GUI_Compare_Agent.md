# MS6.7 — Findings, GUI analytics, compare/history and agent interface

Primary source requirements: 299–420  
Requirements 372–394 consume the storage implementation already qualified in MS6.1.

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Findings

### M6.7.1 Categories
Performance, concurrency, memory, architecture, exception, IO, build and regression.

### M6.7.2 Finding fields
FindingId, RunId, severity, confidence, provenance, summary, rule ID, evidence refs, source refs, reproduction, first/last seen, baseline state.

### M6.7.3 Reproduction recommendation
Only generated if exact evidence and rerun inputs exist.

## Dashboard and views

### M6.7.4 Dashboard
Build/test/crashes/exceptions/warnings/CPU/memory/threads/waits/locks/functions/coverage/new findings/regressions.

### M6.7.5 Required tabs
Build, Tests, Timeline, Actions, Call Tree, Causal Graph, Functions, Threads, Locks & Waits, Memory, Allocations, IO, Exceptions, Coverage, Architecture, Findings, Compare, Agent.

### M6.7.6 Required visualizations
CPU, memory, flame graph, thread timeline, causal graph, module/runtime dependencies, allocation histogram, lock contention, IO timeline, test duration, performance history, exception frequency, function usage.

## Compare/history

### M6.7.7 Two-run compare
New/removed functions and runtime edges; CPU/RAM/allocation/p95/p99 deltas; new exceptions/waits/locks; coverage delta.

### M6.7.8 Compatibility guard
Incompatible instrumentation/build identities cannot silently produce ordinary regression verdicts.

## Agent interface

### M6.7.9 Query IDs
Finding, Test, Scenario, Action, Trace, Function and Module.

### M6.7.10 Investigation packages
Separate packages for failed tests, traces, Actions and Findings.

### M6.7.11 Bounded output
Defaults:
- 200 events;
- 50 stack frames;
- 20 related findings;
- 20 neighbors per graph expansion.
Any truncation requires `truncated=true` plus continuation token.

### M6.7.12 Named exports
Generate all source-required Markdown files plus JSON/JSONL/CSV where applicable.

### M6.7.13 Navigation
Finding -> Run -> Scenario/Test/Action -> Trace -> causal -> call tree -> threads/locks -> allocations -> IO -> source.

## Qualification fixtures

- controlled finding with all navigation links;
- two runs with known CPU/function/edge/coverage differences;
- incompatible-profile comparison;
- oversized trace package proving bounds/continuation;
- agent-only investigation fixture where root cause can be reached without raw dump.

## Final-stage adversarial focus

Final controls: known finding and clean control, GUI/headless/agent identical IDs; incompatible comparison rejected, compatible difference detected; missing raw or unknown provenance not reported as definitive; package truncation is explicit and resumable; navigates real root cause without huge model output.

## Exit gate

Source requirements 299–420 qualified.
GUI and agent both reference canonical backend IDs/evidence.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-299` — Findings Engine.
- [ ] `SRC-300` — Performance findings.
- [ ] `SRC-301` — Concurrency findings.
- [ ] `SRC-302` — Memory findings.
- [ ] `SRC-303` — Architecture findings.
- [ ] `SRC-304` — Exception findings.
- [ ] `SRC-305` — IO findings.
- [ ] `SRC-306` — Build findings.
- [ ] `SRC-307` — Regression findings.
- [ ] `SRC-308` — Severity findings.
- [ ] `SRC-309` — Автоматические рекомендации по воспроизведению найденной проблемы.
- [ ] `SRC-310` — Dashboard после каждого Run.
- [ ] `SRC-311` — Build result.
- [ ] `SRC-312` — Test result.
- [ ] `SRC-313` — Crashes.
- [ ] `SRC-314` — Exceptions.
- [ ] `SRC-315` — Warnings.
- [ ] `SRC-316` — CPU.
- [ ] `SRC-317` — Memory.
- [ ] `SRC-318` — Threads.
- [ ] `SRC-319` — Long waits.
- [ ] `SRC-320` — Lock contention.
- [ ] `SRC-321` — Functions observed.
- [ ] `SRC-322` — Coverage.
- [ ] `SRC-323` — New findings.
- [ ] `SRC-324` — Regressions.
- [ ] `SRC-325` — Вкладка Build.
- [ ] `SRC-326` — Вкладка Tests.
- [ ] `SRC-327` — Вкладка Timeline.
- [ ] `SRC-328` — Вкладка Actions.
- [ ] `SRC-329` — Вкладка Call Tree.
- [ ] `SRC-330` — Вкладка Causal Graph.
- [ ] `SRC-331` — Вкладка Functions.
- [ ] `SRC-332` — Вкладка Threads.
- [ ] `SRC-333` — Вкладка Locks & Waits.
- [ ] `SRC-334` — Вкладка Memory.
- [ ] `SRC-335` — Вкладка Allocations.
- [ ] `SRC-336` — Вкладка IO.
- [ ] `SRC-337` — Вкладка Exceptions.
- [ ] `SRC-338` — Вкладка Coverage.
- [ ] `SRC-339` — Вкладка Architecture.
- [ ] `SRC-340` — Вкладка Findings.
- [ ] `SRC-341` — Вкладка Compare.
- [ ] `SRC-342` — Вкладка Agent.
- [ ] `SRC-343` — CPU charts.
- [ ] `SRC-344` — Memory charts.
- [ ] `SRC-345` — Function flame graph.
- [ ] `SRC-346` — Thread timeline visualization.
- [ ] `SRC-347` — Causal graph visualization.
- [ ] `SRC-348` — Module dependency visualization.
- [ ] `SRC-349` — Runtime dependency visualization.
- [ ] `SRC-350` — Allocation histogram.
- [ ] `SRC-351` — Lock contention charts.
- [ ] `SRC-352` — IO timeline.
- [ ] `SRC-353` — Test duration history.
- [ ] `SRC-354` — Performance history.
- [ ] `SRC-355` — Exception frequency charts.
- [ ] `SRC-356` — Function usage charts.
- [ ] `SRC-357` — Compare двух runs.
- [ ] `SRC-358` — Baseline vs current.
- [ ] `SRC-359` — Новые функции.
- [ ] `SRC-360` — Исчезнувшие функции.
- [ ] `SRC-361` — Новые runtime dependency edges.
- [ ] `SRC-362` — Исчезнувшие dependency edges.
- [ ] `SRC-363` — Изменение CPU.
- [ ] `SRC-364` — Изменение RAM.
- [ ] `SRC-365` — Изменение allocations.
- [ ] `SRC-366` — Изменение p95/p99 timings.
- [ ] `SRC-367` — Новые exceptions.
- [ ] `SRC-368` — Новые waits.
- [ ] `SRC-369` — Новые locks.
- [ ] `SRC-370` — Изменение coverage.
- [ ] `SRC-371` — Regression history.
- [ ] `SRC-395` — Agent readable export.
- [ ] `SRC-396` — `INDEX.md`.
- [ ] `SRC-397` — `RUN_SUMMARY.md`.
- [ ] `SRC-398` — `BUILD_DIAGNOSTICS.md`.
- [ ] `SRC-399` — `FAILURES.md`.
- [ ] `SRC-400` — `REGRESSIONS.md`.
- [ ] `SRC-401` — `ACTIONS.md`.
- [ ] `SRC-402` — `EXCEPTIONS.md`.
- [ ] `SRC-403` — `FUNCTION_PROFILE.md`.
- [ ] `SRC-404` — `CALL_GRAPH.md`.
- [ ] `SRC-405` — `THREADS.md`.
- [ ] `SRC-406` — `LOCKS.md`.
- [ ] `SRC-407` — `MEMORY.md`.
- [ ] `SRC-408` — `IO.md`.
- [ ] `SRC-409` — `COVERAGE.md`.
- [ ] `SRC-410` — `ARCHITECTURE.md`.
- [ ] `SRC-411` — `HOT_PATHS.md`.
- [ ] `SRC-412` — `IMPORTED_APIS.md`.
- [ ] `SRC-413` — `STATIC_SCAN.md`.
- [ ] `SRC-414` — `RAW_EVIDENCE.md`.
- [ ] `SRC-415` — `manifest.json`.
- [ ] `SRC-416` — Отдельные agent пакеты для конкретных failed tests.
- [ ] `SRC-417` — Отдельные agent пакеты для конкретных traces.
- [ ] `SRC-418` — Отдельные agent пакеты для конкретных Actions.
- [ ] `SRC-419` — Возможность агенту расследовать проблему по стабильным ID, не читая гигабайты raw trace.
- [ ] `SRC-420` — Автоматическое ограничение agent context только релевантными событиями.

Source obligations owned by this stage: **99**.

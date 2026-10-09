# Milestone 6 — Full External Auditor
## Layer 1 — Roadmap

Revision: 2026-10-06  
Authoritative source obligations: 460 numbered Auditor requirements.  

## 1. Milestone result

MS6 produces one finished external Auditor for Epidemic Engine.

`GOAL6_AUDITOR_COMPLETE` means:

- all 460 source requirements are mapped;
- every mandatory capability is implemented and independently exercised;
- every sensor family has known-bad and known-good qualification fixtures;
- raw captures are immutable and can be reprocessed by a newer analyzer;
- GUI and headless/backend paths use the same run model;
- normal Epidemic Debug/Release builds do not depend on Auditor;
- shadow probes exist only in Audit builds;
- a real Epidemic acceptance run has been completed;
- no Auditor correctness blocker remains.

MS6 does **not** mean that Epidemic itself is fully hardened or frozen.

## 2. Zero-law

Epidemic is an untrusted system under observation.

Internal engine logs, tests and declarations are evidence inputs, not truth.
Where practical, Auditor must confirm behavior using Windows/process/debugger/ETW/compiler/static evidence that is independent of Epidemic.

Every fact carries provenance:

- `external`;
- `compiler_instrumentation`;
- `static_analysis`;
- `engine_probe`.

Every interpretation carries confidence:

- `observed`;
- `derived`;
- `declared`;
- `heuristic`.

## 3. Build boundary

Required build trees:

- normal engine build: existing project build trees;
- `audit`: external/sampling observation without exact function instrumentation;
- `audit-deep`: exact instrumentation and optional shadow probes;
- `audit-memory`: ASan/Application Verifier oriented profile where applicable;
- `audit-coverage`: coverage profile.

Auditor must not inject `/Gh`, `/GH`, probes or fault hooks into ordinary production builds.

## 4. Evidence pipeline

```text
immutable raw
    -> normalized events
        -> derived graphs
            -> metrics
                -> findings
                    -> human/agent views
```

A Markdown report is never the canonical source of truth.

## 5. Execution order

1. `MS6.0` — freeze460 obligations and classify the actual workspace Auditor baseline.
2. `MS6.1` — implement/version evidence store, immutable raw, replay and manifests.
3. `MS6.2` — GUI/backend orchestration, CMake/build, CTest and managed runs.
4. `MS6.3` — process/module/symbol/thread/lock/ETW/sampling observation.
5. `MS6.4` — exact `/Gh` `/GH` tracing, coverage, static/runtime architecture.
6. `MS6.5` — exceptions/crashes, memory safety, IO/network/Win32 API.
7. `MS6.6` — causal DAG, async correlation, shadow probes, Actions, scenarios.
8. `MS6.7` — findings, dashboards, comparisons, history and bounded agent queries.
9. `MS6.8` — fault/stress injectors and controlled degradation instrumentation.
10. `MS6.9` — full self-qualification and acceptance audit against Epidemic.

No later stage may treat an earlier red gate as PASS.

Implementation cadence: finish substantial coherent feature blocks before grouped focused tests. Review the full final stage diff, freeze it, then perform the strict P.4 consolidated stage gate once. The broad, independent P.5 cross-stage acceptance is mandatory at MS6.9. This changes execution efficiency, **not** any of the 460 required capabilities or product evidence. Historical stage receipts do not establish current-source PASS.

## 6. Actual workspace Auditor baseline

The source tree present at MS6 start is the authoritative implementation candidate
(currently0.1.0), not accepted evidence. Historical0.2 source is absent; its documents
are non-authoritative design reference and must not be fabricated into a baseline.

Claims such as:

- build/test GUI;
- Full and Deep modes;
- native debugger;
- WPR/ETW;
- `/Gh` `/GH` trace runtime;
- PDB symbolization;
- SQLite/history;
- agent exports;

are untrusted claims until exercised against the actual source. At admission exit,
each actual capability is VERIFIED_REUSE, REPAIR, REPLACE or MISSING.

They become `PASS` only after the new acceptance contract exercises them.

The original460 requirements came from a conversation. Preserve their
origin_source_sha25608ea103776d7e29489fde4e4c1d0d227bc847022d3073026e2b0512eee5fa222.
The repository canonical copy is Milestone_6_Auditor_Requirement_Traceability.md;
its actual canonical_requirements_sha256 separately governs future drift.

## 7. Required modes

The finished Auditor exposes:

- `Fast`;
- `Full`;
- `Deep`;
- `Targeted Exact`;
- `Memory`;
- `Coverage`;
- `Stress`.

Every run records its instrumentation profile.
Performance comparisons across incompatible profiles must be marked invalid or explicitly normalized; they may not silently appear as ordinary regressions.

## 8. Primary navigation invariant

The user and agent must be able to navigate:

```text
FindingId
 -> RunId
 -> TestId / ScenarioId / ActionId
 -> TraceId
 -> causal nodes
 -> call tree
 -> thread / waits / locks
 -> allocations
 -> IO
 -> source file:line
```

Missing evidence must be represented explicitly as `not_observed`, `dropped`, `unsupported` or `not_applicable`, never as an invented edge.

## 9. Self-qualification invariant

For each critical detector family:

```text
known bad fixture -> detector must fire
known good fixture -> detector must stay silent
```

For lossy collectors, the run records dropped-event counters and completeness flags.
A truncated trace can never be reported as complete.

## 10. Final application deliverable

Итоговый Auditor должен поставляться как самостоятельное Windows desktop application:

`EpidemicAuditor.exe`

Требования:

- обычный пользователь запускает Auditor через `.exe`;
- PowerShell/Python/внутренние helper executables могут использоваться внутри реализации, но не являются основным пользовательским entry point;
- для обычного запуска не требуется вручную открывать терминал;
- GUI доступен непосредственно из `EpidemicAuditor.exe`;
- приложение должно уметь найти/выбрать Epidemic project root и работать с ним как с внешним проектом;
- Auditor может находиться в отдельной папке или отдельном Git repository;
- production Epidemic не зависит от `EpidemicAuditor.exe`;
- native helper tools допускаются, но запускаются и управляются Auditor автоматически.

## 11. Handoff after MS6

The new order is:

```text
MS5  persistence + determinism
MS6  Full External Auditor
MS7  engine hardening using Auditor
MS8  integration qualification + test truthfulness audit
MS9  destructive/stress/fault campaign
Freeze
```

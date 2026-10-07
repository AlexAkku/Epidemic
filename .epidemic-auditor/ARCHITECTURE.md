# Epidemic Auditor architecture

## Trust boundary

Epidemic is the observed system. Auditor controls process launch and owns timestamps, capture files, normalization and reports. Engine output is evidence only as `external_console`; future engine semantic probes must be explicitly tagged `engine_probe` and must never be promoted to externally observed facts.

## Main pipeline

```text
GUI / CLI
   |
RunManager
   +-- BuildController ---- cmake
   +-- TestCatalog -------- ctest JSON discovery
   +-- TestExecutor ------- one isolated ctest process per selected test
   +-- GameRunner -------- configured executable
   |
ManagedProcessRunner
   +-- stdout/stderr timestamp capture
   +-- ProcessStatsCollector (process tree CPU/RSS/threads/I/O)
   +-- WprCollector (Deep, Windows, optional)
   +-- XperfPostProcessor (Deep, Windows Performance Toolkit, optional)
   |
RunStorage
   +-- immutable-ish per-run evidence folders
   +-- SQLite run/test history index
   |
Analyzers
   +-- process sample summary
   +-- test aggregation
   +-- source static scan
   |
AgentExporter
   +-- compact Markdown index
   +-- machine-readable JSON/JSONL
   +-- direct pointers to raw evidence
```

## Provenance

Current evidence sources:

- `external_process`: OS process-tree measurements through psutil/Windows process APIs.
- `external_console`: stdout/stderr captured by the parent auditor process.
- `external_ctest`: CTest discovery/execution controlled by Auditor.
- `external_environment`: host, toolchain and Git state captured outside Epidemic.
- `external_static_scan`: read-only scan of source files.
- `external_wpr_raw`: WPR ETL trace.
- `external_xperf_analysis`: text analysis derived from ETL using xperf.

Future source:

- `engine_probe`: semantic correlation declared by an Audit-only engine probe. It must remain distinguishable from external observation.

## Run identity

Every operation creates a Run. Test suites create one parent Run and an independent child capture directory for every test. This prevents one crashing/noisy test from contaminating another test's evidence.

## Capture modes

Fast is intended for normal work. Full adds source scan and expanded reports. Deep adds WPR/ETW and xperf post-processing if those Windows tools are available.

Exact function enter/exit instrumentation is intentionally not implemented by modifying Epidemic. The next stage should add an external/native instrumentation adapter or Audit-build-only compiler instrumentation while preserving the same evidence/provenance model.

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

Exact function-entry/exit instrumentation, allocation stacks, lock ownership reconstruction and semantic causal probes are intentionally adapters for the next stages. Deep mode already stores WPR ETL and, when `xperf` is available, exports sampled native CPU/function and call-stack reports without modifying Epidemic.

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

Future semantic probes must use a different provenance, for example `engine_probe`, so an agent can distinguish externally observed evidence from engine-declared facts.

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

The launcher creates a local `.venv` and installs only `psutil`.

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

## Evidence layout

Each operation creates a unique run folder under `data/runs`:

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

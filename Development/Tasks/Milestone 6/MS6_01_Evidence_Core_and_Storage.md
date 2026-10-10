# MS6.1 — Evidence core, immutable storage and replay

Primary source requirements: 372–394  
Cross-cutting support for every later source requirement.

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Required run layout

```text
data/runs/<RunId>/
    run.json
    environment.json
    build/
    tests/
    raw/
    normalized/
    graphs/
    metrics/
    findings/
    agent/
    manifest.json
```

## Required contracts

### M6.1.1 Immutable raw
After run finalization, analyzers open raw files read-only.
A self-test snapshots hashes before and after reanalysis and requires equality.

### M6.1.2 Manifest
For every raw file store path, size, SHA-256, collector/version, time range, completeness and dropped count.

### M6.1.3 Environment fingerprint
Store OS, CPU, RAM, compiler/toolchain, Auditor version, engine source revision/fingerprint and relevant build profile.

### M6.1.4 Versioned normalized schema
Each normalized record has schema version and stable event type.
Unknown fields are forward-tolerant; unknown required record types fail analysis explicitly.

### M6.1.5 Derived lineage
Graph/metric/finding records store producer version and input evidence references/hashes.

### M6.1.6 SQLite history
SQLite indexes structured facts but raw remains independently usable if DB is deleted.

### M6.1.7 Human exports
JSON, JSONL, CSV and Markdown are generated from structured evidence.

### M6.1.8 Reanalysis
A fixture raw capture is processed by analyzer V1 and V2 without recapture.
Raw hashes remain identical; derived producer versions differ.

### M6.1.9 Capture health
Lossy collectors expose:
- capacity;
- observed;
- dropped;
- incomplete reason.

### M6.1.10 Atomic finalization
A run interrupted before finalization is marked `INCOMPLETE`; it may not appear as a clean PASS run.

## Required named files

The storage layer must support generation of the source-required files/directories:
`run.json`, `environment.json`, `build/`, `tests/`, `raw/`, `normalized/`, `metrics/`, `graphs/`, `findings/`, `agent/`, `manifest.json`.

## Negative fixtures

- mutate one raw byte after finalization -> manifest verification FAIL;
- delete SQLite -> raw replay still succeeds;
- delete required raw file -> run completeness FAIL;
- collector reports dropped events -> trace cannot be `complete`;
- crash during run finalization -> run status `INCOMPLETE`.

## Final-stage adversarial focus

Final controls: one-byte raw mutation and raw deletion rejected; sealed raw cannot be rewritten; drop/unknown-type/incomplete finalization fail honestly; schema-version reanalysis preserves input hash; SQLite cache deletion/forgery cannot invent raw; product RunId/history/export consistency.

## Exit gate

Requirements 372–394 have direct artifact/evidence proof.
Replay/immutability/corruption/incomplete-run fixtures PASS.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-372` — Persistent SQLite database для GUI и истории.
- [ ] `SRC-373` — Одновременно human-readable exports.
- [ ] `SRC-374` — JSON.
- [ ] `SRC-375` — JSONL.
- [ ] `SRC-376` — CSV.
- [ ] `SRC-377` — Markdown.
- [ ] `SRC-378` — Immutable raw captures.
- [ ] `SRC-379` — Отдельный слой normalized events.
- [ ] `SRC-380` — Отдельный слой derived graphs.
- [ ] `SRC-381` — Отдельный слой metrics.
- [ ] `SRC-382` — Отдельный слой findings.
- [ ] `SRC-383` — Возможность повторно прогнать новый Analyzer по старому raw capture.
- [ ] `SRC-384` — Отдельная папка каждого Run.
- [ ] `SRC-385` — `run.json`.
- [ ] `SRC-386` — `environment.json`.
- [ ] `SRC-387` — `build/`.
- [ ] `SRC-388` — `tests/`.
- [ ] `SRC-389` — `raw/`.
- [ ] `SRC-390` — `normalized/`.
- [ ] `SRC-391` — `metrics/`.
- [ ] `SRC-392` — `graphs/`.
- [ ] `SRC-393` — `findings/`.
- [ ] `SRC-394` — `agent/`.

Source obligations owned by this stage: **23**.

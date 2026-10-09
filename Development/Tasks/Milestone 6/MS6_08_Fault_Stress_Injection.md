# MS6.8 — Fault injection and stress capability

Primary source requirements: 421–436

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Injector contract

Every injection record contains:
- InjectorId;
- type;
- target/scope;
- parameters;
- requested start/end;
- observed start/end;
- measured effect;
- evidence refs;
- cleanup result.

An injector is not qualified because its API returned success.
The measured target condition must change in the controlled fixture.

## Required injectors/campaign primitives

### M6.8.1 Disk IO slowdown
Scope-limited latency injection; external IO metrics confirm effect.

### M6.8.2 Memory pressure
Pressure amount/time recorded; external memory metrics confirm effect.

### M6.8.3 CPU pressure
Load profile recorded; CPU metrics confirm effect.

### M6.8.4 Allocation failure
Deterministic/configurable failure policy with exact failure evidence.

### M6.8.5 Worker scheduling pressure
Pressure/configuration recorded and visible in thread/scheduler evidence.

### M6.8.6 Cancellation storm
Cancellation request volume and completion outcomes correlated.

### M6.8.7 Save under load
Persistence operation correlated with load window.

### M6.8.8 IO cancellation
Request/cancel/completion result linked.

### M6.8.9 Long stress
Long run keeps rolling evidence and bounded storage/health metadata.

### M6.8.10 Thousands of save/restore
Iteration count, failures and persistent-state checks are structured metrics.

### M6.8.11 Persistent state under load
State qualification evidence is correlated to load windows.

### M6.8.12 Async cancellation under load
Late completion/orphan evidence available.

### M6.8.13 Controlled degradation
Expected degraded result is distinguishable from silent corruption/hang.

### M6.8.14 Causal chains under load
Trace loss/completeness reported; no invented edges.

### M6.8.15 Bad environment
At least one controlled missing/slow/unavailable dependency/environment case.

## Negative qualification

For every injector:
- disabled control run;
- enabled run;
- scope leak check;
- cleanup check.

## Final-stage adversarial focus

Final controls: for each source-required injection family run disabled control, injected case, measured physical effect, scope non-leak and teardown; injector ACK alone never qualifies; protect machine/resource limits and catch hung/orphan process; at least one integrated save/load/cancellation under load.

## Exit gate

Source requirements 421–436 qualified.
All injectors have proof-of-effect, scope and cleanup evidence.

# Authoritative source-requirement checklist

These source obligations remain authoritative. Checking a box requires the stage proof described above; the checklist itself is not evidence.

- [ ] `SRC-421` — Fault injection.
- [ ] `SRC-422` — Искусственное замедление disk IO.
- [ ] `SRC-423` — Memory pressure.
- [ ] `SRC-424` — CPU pressure.
- [ ] `SRC-425` — Allocation failure injection.
- [ ] `SRC-426` — Worker scheduling pressure.
- [ ] `SRC-427` — Cancellation storms.
- [ ] `SRC-428` — Save во время интенсивной нагрузки.
- [ ] `SRC-429` — IO cancellation.
- [ ] `SRC-430` — Long-running stress tests.
- [ ] `SRC-431` — Тысячи повторов save/restore.
- [ ] `SRC-432` — Проверка persistent state под нагрузкой.
- [ ] `SRC-433` — Проверка async cancellation под нагрузкой.
- [ ] `SRC-434` — Проверка controlled degradation.
- [ ] `SRC-435` — Проверка causal chains под реальной нагрузкой.
- [ ] `SRC-436` — Проверка поведения движка при плохом окружении, а не только happy path.

Source obligations owned by this stage: **16**.

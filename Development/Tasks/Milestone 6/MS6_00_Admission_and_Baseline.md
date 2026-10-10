# MS6.0 — Admission, requirement fidelity and workspace Auditor baseline

Source obligations: all 1–460  
Source SHA-256: `08ea103776d7e29489fde4e4c1d0d227bc847022d3073026e2b0512eee5fa222`

Authoritative clarification (2026-10-06): the origin was a conversation input.
The SHA above is `origin_source_sha256`, not the canonical repository file hash.
`Milestone_6_Auditor_Requirement_Traceability.md` is the authoritative canonical
copy; record its actual `canonical_requirements_sha256` separately and reject
future drift. The implementation baseline is the actual workspace source tree
at MS6 start (currently0.1.0). Historical0.2 is non-authoritative design reference.

## Development cadence and stage-exit authority

Follow Layer 3 P.2–P.4 (plus P.5 for MS6.9): implement **large, coherent, end-to-end capability blocks** with their good/bad fixtures; run grouped focused tests **after each complete block**, not after each small edit. At stage end review the complete changed code/test/build surface and affected invariants, fix findings, freeze inputs, then run **one strong consolidated qualification**. Historical PASS/receipts do not establish current-source acceptance. Preserve required **Auditor product** raw/manifest/provenance semantics, but do not create development proof-of-proof infrastructure.

## Forbidden shortcuts

- Do not use README claims as qualification.
- Do not use a PASS string as sole proof.
- Do not hand-edit generated evidence to make a validator green.
- Do not remove a failing detector/fixture to close the stage.
- Do not treat missing telemetry as zero activity.
- Do not change production engine behavior merely to make Auditor observation easier.


## Inputs

- the canonical repository copy of the exact460 conversation-source requirements;
- actual workspace Auditor source tree at MS6 start;
- that implementation's README/architecture as untrusted claims;
- current Epidemic root/build conventions;
- current Windows/MSVC/ClangCL toolchain availability.

## Required verified contracts (not a legacy file-layout mandate)

Independently establish the unchanged canonical 460 source requirements, complete ownership/traceability, real source baseline classification, fixture coverage, stable identifiers, clock correlation, provenance/confidence, mode names and compiler/link isolation. An existing source implementation or one concise machine-readable current-stage result may satisfy several contracts. The old `requirements/*.json`, `contracts/*.json`, `baseline/*.json` and `stage_closure/ms6_0.json` file inventory was a historical execution layout; **do not regenerate it solely for MS6.0 closure**. An actual source requirement for an Auditor product artifact, however, remains mandatory as written.

## Exact requirements

### M6.0.1 Source integrity
The parser must produce exactly 460 records with IDs 1..460.
Duplicate/missing/out-of-range IDs fail admission.

### M6.0.2 Source hash
Record origin_source_sha256 for the conversation input and independently hash the
actual canonical repository Markdown as canonical_requirements_sha256. Fail
traceability generation if the canonical copy changes without an explicit revision.

### M6.0.3 Traceability completeness
Every source ID has:
- primary stage;
- normalized capability family;
- implementation owner;
- validator;
- fixture family;
- status.

### M6.0.4 Existing-code inventory
Every actual workspace baseline capability is classified at admission exit:
- `VERIFIED_REUSE`;
- `REPAIR`;
- `REPLACE`;
- `MISSING`.

No capability begins as `PASS`.

### M6.0.5 Trust boundary
A validator proves:
- normal Debug and normal Release builds do not link Auditor trace/probe runtime;
- Audit build may link it;
- Auditor and raw storage live outside the production dependency/state boundary.

### M6.0.6 Mode contract
Freeze exact names and intended instrumentation for:
`Fast`, `Full`, `Deep`, `Targeted Exact`, `Memory`, `Coverage`, `Stress`.

### M6.0.7 Provenance/confidence contract
Only defined enum values are accepted. Unknown values fail schema validation.

### M6.0.8 ID contract
Fixture proves stable TestId/ModuleId/FunctionId semantics across equivalent discovery/binary identities and proves incompatibility when binary identity changes.

### M6.0.9 Clock contract
Fixture emits markers through at least two collectors and proves correlation or records a bounded uncertainty.

### M6.0.10 Fixture registry
Before stage MS6.2 begins, each critical detector family has a named fixture ID and expected bad/good result.

## Negative fixtures

- requirement 17 removed from copied source -> admission FAIL;
- duplicate source ID -> FAIL;
- unknown provenance enum -> FAIL;
- same transient UI row assigned two canonical IDs -> FAIL;
- Deep instrumentation present in normal build -> FAIL.

## Final-stage adversarial focus

Final controls: exact 460 source IDs and unchanged text/origin vs canonical hash; duplicate, missing and mutated IDs rejected; actual baseline capability verification, bad link/probe isolation rejected for normal Debug/Release, positive Audit proof; no false baseline PASS.

## Exit gate

- parsed source = 460;
- mapped = 460;
- unmapped = 0;
- actual workspace baseline inventory complete, without fabricated historical0.2 source;
- trust/ID/clock/mode contracts PASS;
- fixture registry complete enough for all later critical detector families.

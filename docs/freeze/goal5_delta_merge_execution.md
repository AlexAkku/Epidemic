# M5.1 / M5.2 selective parallel-delta merge

Archive: `Deltas/Epidemic_Engine_03-10-26-1_MS5_5.1_5.2_complete_delta.zip`.
SHA256: `b8ab91d1f03943c48b99320f1b546b5da95420b1b4e96ba33943a2b0978ae142`.

## Reviewed decisions

- Adopted the read-only 5.1 mapping, private participant implementation header,
  and explicit changed-state fixtures for all eleven newly added integration
  participants. Preserved current production wire/storage and Runtime durable
  paths; the archive does not include the current Runtime qualification suite.
- Kept the native Base Platform file commit (owned CREATE_NEW temporary file,
  explicit OS flush and atomic replacement). The alternate store uses a fixed
  `.next` pathname and stream flush/rename; it would discard current native fault,
  interruption and repeated-generation durability coverage.
- Preserved all 51 immutable binary fixtures, including 47 actual participant
  schema-v1 payloads. No historical bytes or initial schema policies regenerated.
- Strengthened publication and one-use tests: a committed old stage cannot
  overwrite a newer live state. Added identity, staging and one-use assertions
  for the two existing dispatcher/outbox participants.
- Added eight typed, distinctly anchored executable obligations per participant.
  Mandatory obligations cannot use generic N/A exemptions. Matrix qualification
  records execution only; it never assigns IMPLEMENTED or changes semantic reviews.
- Added public/internal positive controls and seven negative external-TU compile fixtures. Supported
  public consumers cannot instantiate private participants, access restore ports,
  codecs/fault seams, bind workspace internals, copy a workspace, or include the
  private implementation header through exported include directories.
  Valid owner definitions are identical across both privilege-negative fixtures
  and the compiling internal control, so unrelated owner errors cannot pass them.
- Preparation-failure injection runs with no existing stage, so a duplicate-stage
  rejection cannot masquerade as an allocation-failure test. Typed identity/schema/
  dependency evidence carries distinct assertions; shared proof must actually be
  invoked by the concrete participant's own generated regression function.
- Connected all Goal 5 admission/inventory/semantic/schema/storage validators,
  self-tests, generation/restore audits and private-surface checks to local
  Validators and architecture-freeze. The CI contract rejects missing gates.
- Matrix snapshots source identities before execution; receipt recording rejects
  changes during or after the run. Semantic evidence and compiler fixtures are
  included in qualification identities.

## Inventory and public surface

78 production modules, 473 reviewed candidates, UNCLASSIFIED=0, UNKNOWN=0.
139 Framework durable candidates map exactly once to 47 participants.
28 technical public callables were privatized and one workspace constructor
reviewed: 4374 current callables, preserving all 4311 historical admission APIs.
232 public headers and 78 LOCAL_READY modules remain. Raw handoff sources were
reviewed and owning generators rebuilt canonical freeze outputs.
The 282 M5.4/M5.5 debts remain explicit; there is no stage-5.2 debt.

## Verification

The changed-state/dispatcher/outbox/mechanics/composition Debug selection passed
12/12. MSVC passed both public/internal positive controls and rejected all seven
negative fixtures. Local Validators and all generation/self-test gates passed.

Final Windows/MSVC Matrix NoCache passed 314/314 tests across six profiles
(Base 14, Runtime 34, Full 109, each Debug/Release), finishing at
2026-10-03T21:25:26Z. The subsequent Full NoCache passed 218/218
(109 Debug + 109 Release), finishing at 2026-10-03T21:35:47Z.
Both runs passed /W4 /WX builds and public-header self-containment.
The refreshed exact-tree qualification is PASS for all 47 reviewed participants,
with 743 source/evidence files fingerprinted; a read-only qualification check
after Full confirmed the receipt still matches. The 5.1/5.2 closure gate passed.

Receipts: `_goal5_handoff/participant_qualification.json`,
`_goal5_handoff/matrix_ci_report.json`, `_goal5_handoff/full_ci_report.json`.
Existing formatter/line-limit/coverage-collector/SAST/archive SKIP policies remain
explicit; this record does not claim instrumented coverage or remote CI.

Semantic verification: `python docs/freeze/goal5_participant_contract.py --check --implementation`.
Exact-tree execution verification: `python docs/freeze/goal5_participant_contract.py --check --qualification`.
After successful complete Matrix: `python tools/record_goal5_participant_qualification.py --record`.

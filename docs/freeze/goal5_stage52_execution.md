# M5.1 / M5.2.A-E execution record

Scope: complete ownership review, participant coverage/bindings, initial supported
schemas, compatibility fixtures, whole-image wire and durable Framework/Runtime
package storage. This is not whole-engine consistent-cut/restore/determinism qualification.

## Ownership recheck

78 production modules and 497 candidates are explicitly reviewed. Discovery includes
name-only and nested Snapshot/Checkpoint DTOs and relevant forward declarations;
24 additional decisions are explicit source handoff reviews admitted by a generator.
Unknown new candidates remain fail-closed.
The re-review distinguishes Runtime interfaces, open transactions and private
prepare copies from concrete TimeRuntime/InMemoryPersistenceStore authority.
Runtime TimeSnapshot is frame-facing, not a continuation checkpoint; TimeCheckpoint
contains the fractional accumulator/revision/configuration identity.
Module placeholders and missing reviewed contract fields are rejected explicitly.

144 Framework durable candidates have exactly one of 47 participants, with 45 new
bindings in 12 existing integration modules. Private Stage and audited noexcept
one-use Commit retain bootstrap/provider registrations outside durable DTOs.
The actual shared-owner composition regression closes the exact registry before Freeze.

## Schema and wire

All 47 production participants currently support only schema 1, with explicit
NO_PRIOR_SUPPORTED_VERSION and no invented historical v0. Five compatibility
profiles are tested; only ExactRequired/CompatibleVersion/MigrationRequired are
strict qualification. Synthetic v2 migrations test the machinery without claiming
v2 support for production v1 owners. All 47 immutable participant v1 fixtures are
validated against actual factories; two whole-wire binary goldens also decode and
canonically re-encode. SHA256 manifests fail on changed/missing fixtures. Normal
CI never writes them; explicit first-admission commands refuse overwrites.

EPISAVE1 wire v1 stores explicit LE fixed-width integers, raw-u64 TypeIds, signed
gameplay time, manifest metadata/dependencies and sections. Participant/dependency
order is canonical. Whole/section FNV1a64 checksums detect accidental corruption,
not authentication. Bounds/hard caps are checked before allocation. Bad magic,
version, enums, IDs, graph, lengths, duplicate sections, hashes, ordering and trailing
bytes are rejected. Codec has no OS or gameplay-major dependency.

## Storage

SaveGame defines ISavePackageStore, CommitSaveGameImage and LoadSaveGameImage.
Integration FileSavePackageStore validates packages and delegates native byte I/O
to Base Platform ReadFilePackage/CommitFilePackage. Win32 remains confined to
Platform/src/Windows; architecture allowlists are not relaxed and production
inventory/public-header count remain 78/232.

Single writer per path is externally synchronized. The destination is never
opened for truncating writes: CREATE_NEW same-directory temp, bounded writes,
FlushFileBuffers, close, MoveFileExW(REPLACE_EXISTING | WRITE_THROUGH). Handled
pre-publication failure cleans only the temp created by that operation and preserves
the old save. Errors carry stable code, phase, path and OS detail. Native path
validation rejects embedded NUL before access. The durability promise is file-data
flush plus requested Windows write-through replacement, not a hardware/power-loss
guarantee. Microsoft documents the flag semantics in
[MoveFileExW](https://learn.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-movefileexw).

Deterministic in-memory failures cover open/write/replace, corrupt/truncated reads
and callback exceptions. Production tests cover flush/replace failures, cleanup,
old generation preservation, reopen, successive generations, corruption and
oversize. A separate process writes real Framework Time state and exits; another
process reopens, decodes and restores it into a fresh composition. A child exits
after writing/flushing its temp but before replace: the old save remains readable,
and the orphan temp is never treated as an authoritative save.

Runtime Support now also supplies independent EPIRTIM1 and EPIRPER1 version-1
packages through the same Base atomic byte port. Runtime Time persists complete
configuration identity and fractional remainder; the production IPersistenceBackend
persists all object payload/schema metadata, tombstones, lazy rules and zone overrides.
Service composition reuses the existing ValidatePersistenceSnapshot semantics.
Both formats use bounded explicit LE fields, canonical record ordering and whole
FNV1a64 corruption checks; strict future-version rejection, two immutable v1 goldens,
every truncated prefix, native failure preservation and durable create/delete cycles
are tested. Missing files require explicit initialization, not silent empty restore.
The file backend meets the strongest flush/replace policy even for weaker requests;
the default MemoryOnly store remains explicitly non-durable. The process smoke
reopens Runtime Time and Persistence in separate fresh clock/store services.
This closes the broader plan's all-save-planes durable-path criterion as well as
the execution plan's Framework-specific gate. Unified causal capture is still M5.3.

## Qualification

The figures below describe the final tree after both parallel deltas.
Merge decisions and additional proof hardening are tracked in
`goal5_delta_merge_execution.md` and `goal5_delta2_execution.md`; fresh Windows Matrix and subsequent Full runs
qualified the merged sources.

Exact-tree verification completed on 2026-10-04: cold Matrix NoCache passed 314/314
tests across all six profiles (Base 14, Runtime 34, Full 109, each Debug/Release).
The subsequent required Full NoCache passed 218/218 (109 Debug + 109 Release).
All freeze/evidence/adversarial validators, /W4 /WX builds and public-header
self-containment passed; 78/78 modules remain LOCAL_READY, 4374 APIs reviewed,
232 public headers unchanged. All 47 boundaries have reviewed IMPLEMENTED semantic
status and separate PASS execution qualification with source/graph fingerprints
covering 745 files, including Runtime package code and all 51 binary
fixtures. Reports: `_goal5_handoff/matrix_ci_report.json`,
`_goal5_handoff/full_ci_report.json`, `_goal5_handoff/participant_qualification.json`.
The common local runner includes all
5.1/5.2 validators/self-tests/generator audits. Persistence suites carry schema,
wire and storage labels. No planning Markdown is a validator input.

Formatter/line-limit/coverage-collector/SAST/archive stages retain their documented
baseline SKIP policies; this record does not claim instrumented coverage or remote CI.

The generated gap report retains 291 later-stage debts: 139 in M5.4
(reconciliation/runtime identity), 152 in M5.5
(deterministic digests). M5.2 debt, missing Framework participant/schema/storage
paths and orphan participants are zero in all save planes. Runtime Persistence keeps
its application-owned payload-schema contract; MemoryOnly is not disk durability.
No unified cross-plane generation, fresh whole-engine restore or deterministic
continuation is claimed before those later stages.

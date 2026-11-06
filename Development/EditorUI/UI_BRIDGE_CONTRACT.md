# UI bridge contract v2

Private presentation protocol, not a public Engine/Framework ABI or an archive
schema. F-I baseline: `32eef79`; the owner accepted the first document slice
`719970a` before this F0 completion batch. Current verification/acceptance:
[FI_FEATURE_MATRIX.md](FI_FEATURE_MATRIX.md).

## Ownership and transport

One C++ companion owns one EditorWorkspace, its frozen game Schema, selection,
transactions, Play and persistence. Java has immutable projections, view keys
and input drafts. Detached tools/documents reuse the same PlatformSession and
companion. JNI/JAWT owns only renderer surface lifetimes on EDT, never authoring.
Managed-off/C++-only remains independent of JVM.

Private stdin/stdout, unsigned big-endian length, at most 16 MiB per frame.
UTF-8 strings have u32 byte lengths, at most 1 MiB; malformed UTF-8 and NUL are
rejected. The Java encoder reports invalid UTF-16 rather than silently replacing
surrogates; local pre-send failures keep the valid stream Ready. There are at
most 8 arguments and 32 queued requests; started operations finish atomically,
queued tasks can be cancelled, backend timeout is 15 seconds.

Request: magic `EPUI` (0x45505549), u16 major **2**, u16 minor 0, u64 correlation,
u8 opcode, u64 generation, u64 expected project revision, u16 argument count,
then strings. A process nonce seeds generation; workspace resets advance it.
Previous-process references cannot pass optimistic concurrency checks.
Response: same magic/version/correlation, u8 result, u64 generation, capability
CSV, error, snapshot-present byte. Failed writes publish authoritative state
and an error, never success. Versions 1/3 are explicitly rejected.

## Operations

| Opcode | Operation / authoritative path |
|---|---|
| 1 / 4 | Hello / Snapshot |
| 2 / 3 | Open descriptor/legacy archive / NewFolder |
| 5 / 6 | RenameDocument / atomic whole-project Save (also Save All) |
| 7 / 8 | Workspace Undo / Redo, including mixed folder history |
| 9 | Select: kind, document, element; optionally component and part. All-zero document selection clears to project scope. |
| 10 / 11 | Close (explicit discard optional) / Quit; reject dirty implicit close |
| 12 | CloneFolder into a fresh, non-nested destination; preserve source and IDs |
| 13 / 14 | Existing isolated Play / idempotent Stop |
| 15 | CreateDocument: kind 0 Scene / 1 Prefab / 2 Graph, UTF-8 name. Optional `registered-components` for Prefab uses real host Health/Movement recipes, no demo behavior/Graph dependency. |
| 16 | Scalar commit: expected selection revision, field key, textual typed value. Rebuild the current native Inspector entry, validate with InspectorCommand, Execute once. |
| 17 | DiscardDocument: stable document ID, through existing Workspace/Folder transaction/history boundary. |

Writes require an open project, current generation and nonzero project revision.
Create/Rename/scalar/Discard are read-only during Play. Unsupported capabilities
remain disabled. Save during Play serializes authoring, never runtime values.
Names are nonblank single-line UTF-8, at most 256 bytes. Native validators decide
integer/range/type/string/finite-real validity. Java does not serialize game data.

## Immutable snapshot and drafts

Snapshot: project/asset/selection revisions; dirty/undo/redo/playing flags;
storage, project/entry IDs, path/title; typed selection including component/part;
documents with native per-document dirty; hierarchy nodes with component/part;
source assets; native Inspector descriptors/values/source/read-only reason/ref.
Property keys contain typed owner/property/override IDs. Read-only rows may use
snapshot-local ordinal suffixes, never persisted identities or writable handles.
Integers/reals cross as lossless native text, not Java floating-point conversions.
Unknown/unavailable defaults are explicitly read-only, not invented values.

Document files hold only generation/project/kind/ID and view ID. Titles, dirty
and values resolve from native snapshots. Same-ID open reuses its file; missing
views stay safe; reconnect confirms IDs and rebinds to the new generation.
Selection is not an authoring edit and does not dirty the project. UI drafts
capture generation/revision/selection at first text change; Enter/focus loss
commits once, Escape restores the latest native value. Stale/invalid drafts
show inline errors. Snapshot refresh does not overwrite an unfinished draft.

IO stays off EDT. Platform publication and success callbacks use one ordered
IntelliJ queue. Save flushes input commits and coalesces pending saves; close
waits for actual Save completion. Disconnect retains read-only state. Reconnect
restores only typed selection still present in the committed native snapshot.

## Dirty/close lifecycle

EditorProject retains an immutable durable save-point snapshot. DiscardDocument
restores only that saved document (or removes a new unsaved one), validates the
candidate and publishes one undoable transaction. It cannot accept replacement
payloads or arbitrary retired IDs. Other documents, source files, assets, visual
metadata and their dirty/history remain owned by their existing services.

Tabs use Platform VirtualFilePreCloseCheck. Platform 253 floating DockWindow
close bypasses that hook: the host gates its two existing DockWindow/FrameWrapper
closing callbacks only for its own document windows. Cancel forwards neither;
Save All / Discard awaits native success, closes the captured EditorWindow view,
then forwards the original lifecycle callbacks. Peer views keep their identity.
No Platform JAR or signed module is patched, and no docking/window owner is
introduced. The pinned adapter is covered by real AWT close-event smoke on
independent HWNDs; physical mouse/focus acceptance remains the owner's task.

U group docking/layout remains accepted. Entity picking/gizmos/composition are
F2; Graph Canvas and continuous Game View are later phases. F0 covers document,
typed hierarchy/component selection, real stored scalars and native lifecycle.

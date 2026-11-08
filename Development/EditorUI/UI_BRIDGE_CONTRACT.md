# UI bridge contract v4

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

Request: magic `EPUI` (0x45505549), u16 major **4**, u16 minor 0, u64 correlation,
u8 opcode, u64 generation, u64 expected project revision, u16 argument count,
then strings. A process nonce seeds generation; workspace resets advance it.
Previous-process references cannot pass optimistic concurrency checks.
Response: same magic/version/correlation, u8 result, u64 generation, capability
CSV, error, snapshot-present byte. Failed writes publish authoritative state
and an error, never success. Versions 1/2/4 are explicitly rejected.

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
| 18 | QueueImportPng (historical native API name, now typed supported media): asset revision, external source, Content-relative destination. |
| 19 | Reimport selected: asset revision, AssetId; native worker/publication only. |
| 20 / 21 | Move / queued Duplicate: asset revision, AssetId, new Content-relative path. Preserve media extension. |
| 22 / 23 | Stage Delete (asset revision, AssetId, explicit allow-broken 0/1) / Restore (revision, AssetId). |
| 24 | Assign native typed texture binding: asset revision, Prefab ID, texture AssetId. |
| 25 / 26 / 27 | Cancel native jobs / Poll native content / enable canonical Content watch (0/1). |
| 28 | Bounded native asset query (UTF-8 search, max 1024 bytes); presentation only, no authoring revision. |

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

## F1 asset projection and media

Asset selection is kind 6, document 0, element AssetId. One native selection
feeds Explorer, Asset Details/References and detached readers. Asset writes also
check expected asset revision; no silent retry after Conflict. Poll can publish
new asset generations while the project document revision stays unchanged.
Flags 16/32 describe native job busy/content watch. Pure query/poll does not
simulate import or create another asset model.

Asset snapshots expose native type/ID/generation/fingerprint/cooked SHA-256,
canonical Content path, trash original path, source presence and selected cooked
metadata. Browser queries return at most 1000 sorted matches plus the selected
asset, with the native total count; rows virtualize in List/Grid. Existing native
ASCII-fold search semantics remain. Service-root entry is read-only.

The bounded native event stream (64 recent events) exposes importer job IDs,
source/destination, phase and terminal outcome. Reimport/watch events carry
AssetId; there is no invented percentage. Cancel includes ready but unpublished
workers. Partial batch request failures go to Problems/Console per file.
The existing 32-stats-per-poll watcher / two-observation debounce / five-second
hash audit stays native. JVM polls at 500 ms and never owns a WatchService.
Watch observes project Content copies, not the external originals chosen for
import; clone remains independent of their paths.

Supported native import subset: PNG; static WebP (bundled static libwebp 1.6.0);
integer PCM WAV, 1/2 channels, 8/16/24/32-bit, 8–192 kHz; SFNT TrueType TTF and
CFF OTF validated for registration/metadata. Ogg, animated WebP, playback,
glyph atlas generation and new font/audio component slots remain unavailable.
The file chooser only offers the supported subset. Media uses the existing
private staging, source lease, content digest, immutable cooked archive and
atomic ProjectFolder publication; source/cooked files and IDs survive Save/Clone.
The historical PreparedCookedPng API name is retained, not a parallel importer.
PNG recipe/archive compatibility is unchanged; other media has distinct recipes
and a cooked_media document type. No public Framework ABI changed.

128px native thumbnails retain aspect/alpha and use existing bounded cache
policy. Scene/Prefab viewport receives only native SceneView world projections
and typed bindings/cooked keys. Existing ImageResourceLoader/ResourceServices
prepare immutable preview payloads on a worker; Renderer uploads before Begin,
then draws with retained leases/textures. Replaced generations release after
frame use; closing/reparenting surfaces releases the view resources. Preview
cache is 128 entries/16 MiB per live view, using reduced pixels and original
source dimensions for geometry. Continuous Game View remains F-II/F8.

## F2 authoring operations

| Opcode | Native operation |
|---|---|
| 29 | Create, rename, delete, reparent, reorder and assign Prefab on a Scene node. Logical Prefab reparent and nested link edits use the same workspace transaction boundary. |
| 30 | Selection revision plus native Add/Remove Component, attach a Graph with an eligible Event Entry, or add/delete a logical Prefab child. |
| 31 | Selection revision plus reset local TRS, or revert the addressed override field by key. |
| 32 | SceneDrag begin/move/commit/cancel. Pointer and camera values are presentation coordinates. Move publishes an ephemeral snapshot. Commit is one native transform edit. |
| 33 | Place a registered Prefab at the native viewport world coordinate, or assign an imported texture to the addressed Prefab definition with the expected asset revision. |
| 34 | Add, remove, enable or change Graph on a persisted Scene/World system. Play uses Framework SystemInstance and the existing scheduler. |

The v4 snapshot adds Prefab IDs to Scene rows, registered component type
IDs/names, native Scene item bounds and pivots, and system assignments.
Mutations carry the project generation and expected revision. Inspector edits
also carry selection revision and keep using operation 16, including native
local transform fields. Native viewport input maps client coordinates through
SceneView and returns stable SceneNodeIds. No runtime handles cross IPC.

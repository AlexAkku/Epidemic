# F-I implementation

Baseline: branch `v2`, `32eef79ba28519f3b62f0576a38fe039d80e112e`.
F0 continuation: `719970a`, first document slice accepted by the owner.
Task: `Temp/!Tasks/F-I_Implementation_Spec.md`, revision 1.0, 2026-11-06.
No Computer Use. Owner accepted complete F0 visual checks on 2026-11-06 (195e0c7).

## Ownership

| Path | Authority / binding |
|---|---|
| Project | One PlatformSession → private Bridge → EditorWorkspace / ProjectFolder; immutable snapshots, no Java authoring serialization. |
| Documents | Native AssetId/kind/revision/name and per-document dirty; FileEditorProvider owns views only. All peer copies close with the project. |
| Selection | Operation 9 → EditorWorkspace::Select; kind/document/element/component/part plus selection revision. Project root clears selection. Hierarchy, Inspector and document focus share native selection; Scene displays its projection. |
| Inspector | Native InspectComponents descriptors → ScalarInspector; operation 16 validates project/selection revisions and invokes InspectorCommand in one native transaction. Unavailable defaults/references stay read-only. |
| Discard | Operation 17 → DiscardDocument; restore only the target from the native durable savepoint, retaining other documents/assets/history. No second mutable model. |
| Assets / viewport | Existing native catalog/cook/leases and Renderer/JNI retained. Asset mutation wiring is F1; picking, gizmos and scene authoring are F2. |

Private IPC is version 3 after F1, with no archive schema/public Framework ABI change.
Process-specific generations reject previous-companion tokens; malformed UTF-16
never becomes silent replacement UTF-8. Bridge publication and success callbacks
use the same Platform queue. Drafts stay local until Enter/focus commit, checkbox
change or Save flush; Escape restores the current native value.

## Tracking

| Stage | State | Evidence / remaining work |
|---|---|---|
| FI-0 | PASS | Owners mapped; protected legacy/P04 fixtures exercised through disposable copies; C++-only build and architecture checks passed. |
| F0.1 | PASS automatic | Lifecycle, Clone isolation, writer/generation checks, native save progress/title and shortcuts preserved. |
| F0.2 | PASS automatic | Native Create/Open/Rename, same-ID reuse, headers, dirty/close, missing IDs and detached peer views. |
| F0.3 | PASS automatic | Typed selection/readers, component/part, clear/missing/stale targets, no navigation dirty. Owner visual acceptance PASS. |
| F0.4 | PASS automatic | Registered Health/Movement bool/i64/double/string; validation, stale edits, Undo/Redo, Save/reopen, two readers. Unsupported references read-only. |
| F0.5 | PASS automatic | Flush-before-Save; native tab/floating Save/Discard/Cancel, in-flight edit/Save ordering, isolated discard, all-window project close. |
| F0.6 | PASS automatic | Native Play/Stop, authoring isolation, editing disabled while running, repeated Stop/close. Live Game View belongs to F-II/F8. |
| F0.7 / F0-GATE | PASS | Owner confirmed the complete F0 visual scenario on 2026-11-06. Automatic checks passed; historical analyzer findings remain separately classified. |
| F1 / F1-GATE | PARTIAL | Implementation complete; final functional/Release/full-engine checks PASS on 2026-11-07; own analyzer findings fixed, historical/external raw failures classified; only owner visual acceptance NOT RUN. |
| F2 / F-I-GATE | NOT RUN | F2 starts after owner acceptance of F1. |

## Verification

2026-11-06: common native/UI contracts, protocol adversarial tests (14 cases),
catalog and C++-only checks passed. Coverage includes exact scalar types/ranges,
UTF-8/malformed UTF-16, readonly/missing/stale targets, second unfinished draft
surviving refresh, Escape/commit/Save, dirty isolation, native discard/Undo/Redo,
old-process generations, protected legacy/P04 data and Play/Stop isolation.

Full engine CTest: **161/161, zero failed/skipped**, `Temp/U/f0-engine-tests.xml`.
The earlier raw 160/161 run is retained separately: the new discard fixture used
a relative path from the CTest build directory. Its absolute fixture path was
fixed, focused Core passed, then the full suite passed. No test weakened.
Architecture self-test and public-header self-containment passed.

Standalone tests exercise real Platform ToolWindowManager/FileEditorManager,
independent Windows HWND close events, native Save/Discard/Cancel, queued close
after scalar commit, immediate Save→close, reconnect and multiple Scene peer views;
shutdown has **zero native surfaces / zero fatal diagnostics**. Close-dialog
choices are supplied directly by the test: application-API checks do not replace
physical mouse/visual acceptance. Release packaging and actual EXE activation/
restart are checked separately. Current outputs/logs are under `Temp/U/`.

Deep changed-header consumer analysis retains raw failures under
`Temp/U/logs/f0-deep.*`. Previously classified U/L0 findings remain open.
The expanded sweep also reports unchanged filesystem-role parameter pairs and
an MSVC filesystem enum diagnostic. No analyzer policy/suppression was changed.
See `UI_QA_MATRIX.md` for final classification.

## Owner verification: complete F0

Launch `Temp/U/distribution/EpidemicEditor/EpidemicEditor.exe`, maximized.
Use a disposable project in a Cyrillic path, never the committed demo/P04 backup.
Owner confirmed these F0 steps PASS on 2026-11-06. No Computer Use by the agent.

1. Create Scene `Арена`, Prefab `Герой`, Graph `Логика`. Keep the Prefab
   dialog's registered Health/Movement option checked. Unchecked creates an empty
   Prefab with no invented fields. Open each twice: the existing native-ID view
   is selected. Rename: tree/tab/header agree.
2. Expand the Prefab in Scene Hierarchy; select root and actual component rows.
   Inspector shows native `points`, `speed`, `enabled`, `label` in the
   appropriate scope. Switch document/tool tabs and a detached view; check shared
   selection and retained expansion/scroll. Project root clears selection.
3. Enter `123`, `1.25`, toggle the checkbox, enter a Cyrillic label. Enter/focus
   loss commits once. After one Enter start a second unfinished edit and allow
   refresh: it survives. Escape restores native value. Try integer overflow,
   a negative out-of-range value and `NaN`: visible error, no false write.
   Correct or Escape before continuing.
4. Focus a noneditable heading before global Ctrl+Z/Ctrl+Y (text fields retain
   text-editing shortcuts). One edit undoes/redoes once; two Inspector views agree.
   Save All also flushes the active valid draft.
5. Modify two documents. Close one tab or floating window: Cancel retains it;
   Discard restores only that document, leaving the other dirty; Save All
   completes before closing the requested view. Try immediate Save→close and
   closing just after a field commit. Check dirty marks and window titles.
6. Save All (Ctrl+Shift+S), restart/reopen: names, IDs and scalar values persist;
   revision tokens may be rebuilt. Close Project with multiple Scene peer windows:
   document views close and the launcher is clean.
7. F8 Play/Stop: actual Running/Stopped, scalar/New/Rename disabled while running;
   Stop restores editing without authoring changes.

Picking/gizmos, asset import/mutation, editable Graph Canvas and continuous
Game View are outside this F0 batch. F1 continuation starts from `195e0c7`.


## F1 implementation

Accepted F0 baseline: `195e0c7`; F1 entry HEAD: `098d605` (README update).
F0-GATE PASS is recorded from the owner's
2026-11-06 confirmation. No Computer Use, no new panel IDs, no push.

| F1 area | Existing authority → current binding |
|---|---|
| F1.0 ownership | ProjectFolder source/cooked manifests, AssetCatalog, ContentImportQueue and ContentHotReload, typed PrefabSpriteBinding, BuildContentDependencyIndex, ImageResourceLoader, ResourceServices and ThumbnailCache retained. No Java filesystem catalog/importer. |
| F1.1 browser | Existing Project Explorer Tree/List/Grid; native BrowseContent query max 1000 matches plus selection, native total, document double-open, typed Asset selection and preserved tree/list scrolling. Scripts and service-root entries remain presentation, service data read-only. |
| F1.2–F1.3 import/jobs | Parented Platform multi-select chooser → native bounded queue (128 waiting plus worker), validated private staging/cook/atomic publication, native job IDs and 64-event outcome stream, explicit cancel incl. completed-unpublished worker, per-file rejected requests in Problems. |
| F1.4 details/refs | Actual source/cooked identities, generation/fingerprint, recipe/dimensions or typed WAV/SFNT metadata; selected native 128px alpha/aspect thumbnail; stable incoming/outgoing IDs and document navigation. Unchanged refresh preserves readonly text selection/caret and references. |
| F1.5–F1.6 lifecycle | Native Move/Rename, Duplicate, reference-aware staged Delete/Restore and mixed history. Extension changes rejected; new ID only for duplicate; native project asset dirty, Save/reopen and portable Clone. No permanent purge. |
| F1.7 watch | Native canonical Content polling/debounce/hash-audit; manual Reimport shares the owner. Stable AssetId, only successful publication changes generation/key. Broken source retains last-good, recovery clears current error. External original is not watched after import copies it. |
| F1.8 bounds | Existing source 128MiB copy limit, decoded image 4096px/axis and cooked document bound, PCM/SFNT probes, traversal/reparse/path/collision checks, native queue/backpressure/cache policies preserved. UTF-8 import/trash/clone and >260 Windows path regression added; companion embeds longPathAware. |
| F1.8a formats | PNG, static WebP via SHA-256-pinned static libwebp1.6.0, integer PCM WAV 1/2ch 8/16/24/32-bit 8–192kHz, SFNT TrueType TTF / CFF OTF registration/metadata. Real source/cooked publication, reimport/invalid/last-good. Ogg/animated WebP/playback/glyph authoring remain unavailable. |
| F1.9 viewport | Native SceneView world projection / Prefab sprite binding → immutable cooked keys → existing ResourceServices/ImageResourceLoader/Renderer. Async 128px previews, original dimensions/PPU/pivot, retained leases, generation replacement and surface shutdown; no authoring/camera model in Java. Full-resolution authoring, picking/gizmos and placements remain F2. |

Asset edits are project-level folder history/dirty (including sprite bindings),
not a second document file or Java save path. Watch polls at 500ms; native audit
uses 32 stats per poll, two stable observations, five-second hash policy. Large
catalog latency follows that bounded native sweep, not an instantaneous promise.

Windows long paths require the OS long-path policy; it was read as enabled in
this qualification, never changed. The companion now opts in through its own
manifest ([Windows contract](https://learn.microsoft.com/windows/win32/fileio/maximum-file-path-limitation)).
Media/font probes are bounded registration/metadata validation, not a font engine
or audio playback implementation. Texture assignment accepts PNG/WebP only.
Resource previews use 128 entries/16MiB per live view; actual fixture residency,
responsiveness and final checks are reported in UI_QA_MATRIX.md. Existing
ProjectFolder threat-model limits against adversarial concurrent filesystem
replacement are not promoted into a new security-boundary guarantee.

Final qualification, 2026-11-07: `f1-final2` PASS 1081.50s, peak owned memory
1674.70MiB; minimum free physical/virtual 18.81/17.06GiB. Current-source common
F0/P04/F1 contracts (all five formats, UTF-8, long paths, lifecycle, cancellation,
query, Save/Open/Clone), 15 IPC adversarial cases, packaged Release/GPU/actual
EXE activation and crash/stale-profile restart passed. Full engine build and
CTest **161/161, zero failed/skipped**, 334.01s; `Temp/U/f1-engine-tests.xml`.
Native polling 84 samples: p50 4.832ms, p95 8.163ms; EDT heartbeat p95 0.145ms.
Actual red→blue GPU pixels prove published preview replacement and Resource
leases; shutdown surfaces=0/fatal=0. Architecture/public-header checks passed.
Analyzer raw failures and the packaging CRT fix are classified in UI_QA_MATRIX.md;
these are not represented as analyzer PASS. No Computer Use was performed.

## Owner verification: F1

Run `Temp/U/distribution/EpidemicEditor/EpidemicEditor.exe` maximized, with a new
disposable Cyrillic-path project. These new F1 visual checks are **NOT RUN**.

1. Create a Prefab. Import multiple PNG/static WebP files (include non-square
   alpha), a supported PCM WAV and a TTF/OTF. Empty destination means Content
   root; a nested destination parent must already exist. Cancel picker/destination:
   no imported row or dirty change. Repeat an existing destination: explicit
   per-file failure in Problems, no silent duplicate.
2. Compare Explorer Tree/List/Grid; Enter-to-search, clear filter, select assets.
   Check real ID/type/generation/source/cooked metadata, long/Cyrillic paths,
   aspect/alpha/Fit preview, retained selection/scroll and metadata clipboard copy.
   WAV/font have metadata/IDs without pretending image preview or playback.
3. Assign a PNG/WebP to the Prefab, open its canvas: native sprite appears. Move /
   Rename within Content and Duplicate: original ID/refs stay, duplicate ID differs.
   Undo/Redo and Save All. Asset References navigates to the actual Prefab document.
4. Open Asset Details and the document in independent windows. Edit the canonical
   imported file under the project's **Content**, not only its external original.
   Reimport or enable Watch: actual generation/key/preview/sprite change. Corrupt
   the source: error and last-good. Restore valid bytes: current error clears.
5. Delete a referenced texture: dialog lists incoming documents/IDs, staged missing
   state keeps identity. Restore, Undo/Redo, Save/restart/reopen: refs/values remain.
   No permanent-delete action is enabled. Start several imports and Cancel in
   Background Jobs: actual terminal outcomes, no half-published asset.
6. Test resize, switching Scene/Graph/Prefab, detached Explorer file chooser,
   reconnect and closing detached/native views. Clone a saved project into a new
   directory; open the clone, verify independent source/cooked data and IDs.

F1-GATE stays PARTIAL until this owner's visual acceptance; F2 has not started.

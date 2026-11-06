# F-I implementation

Baseline: branch `v2`, `32eef79ba28519f3b62f0576a38fe039d80e112e`.
F0 continuation: `719970a`, first document slice accepted by the owner.
Task: `Temp/!Tasks/F-I_Implementation_Spec.md`, revision 1.0, 2026-11-06.
No Computer Use. New visual acceptance remains with the owner.

## Ownership

| Path | Authority / binding |
|---|---|
| Project | One PlatformSession → private Bridge → EditorWorkspace / ProjectFolder; immutable snapshots, no Java authoring serialization. |
| Documents | Native AssetId/kind/revision/name and per-document dirty; FileEditorProvider owns views only. All peer copies close with the project. |
| Selection | Operation 9 → EditorWorkspace::Select; kind/document/element/component/part plus selection revision. Project root clears selection. Hierarchy, Inspector and document focus share native selection; Scene displays its projection. |
| Inspector | Native InspectComponents descriptors → ScalarInspector; operation 16 validates project/selection revisions and invokes InspectorCommand in one native transaction. Unavailable defaults/references stay read-only. |
| Discard | Operation 17 → DiscardDocument; restore only the target from the native durable savepoint, retaining other documents/assets/history. No second mutable model. |
| Assets / viewport | Existing native catalog/cook/leases and Renderer/JNI retained. Asset mutation wiring is F1; picking, gizmos and scene authoring are F2. |

Private IPC is version 2, with no archive schema/public Framework ABI change.
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
| F0.3 | PASS automatic | Typed selection/readers, component/part, clear/missing/stale targets, no navigation dirty. Physical mouse acceptance NOT RUN. |
| F0.4 | PASS automatic | Registered Health/Movement bool/i64/double/string; validation, stale edits, Undo/Redo, Save/reopen, two readers. Unsupported references read-only. |
| F0.5 | PASS automatic | Flush-before-Save; native tab/floating Save/Discard/Cancel, in-flight edit/Save ordering, isolated discard, all-window project close. |
| F0.6 | PASS automatic | Native Play/Stop, authoring isolation, editing disabled while running, repeated Stop/close. Live Game View belongs to F-II/F8. |
| F0.7 / F0-GATE | PARTIAL | Implementation and automatic functional checks complete; owner visual scenario below NOT RUN. Raw analyzer failures classified separately. |
| F1 / F2 / F-I-GATE | NOT RUN | F1 begins after owner acceptance of F0. |

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
These steps are **NOT RUN** by the agent.

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
Game View are outside this F0 batch. F1 has not started.

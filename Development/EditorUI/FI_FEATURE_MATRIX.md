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

Private IPC is version 4 in F2. Scene names, nested Prefab links and system
assignments are optional native archive fields. The editor reads older project data.
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
| F1 implementation and owner acceptance | PASS | Implementation at `695bd40`. Functional/Release/full-engine checks passed on 2026-11-07. Owner confirmed manual acceptance on 2026-11-08. |
| F1-GATE | PASS | Owner acceptance and functional qualification complete. On 2026-11-08 the owner accepted disk-full and Windows reboot during import as nonblocking NOT RUN backlog items. Historical/external analyzer findings remain classified. |
| F2.0 native contract | PASS | Scene/Prefab ownership, persistence and IPC gaps audited. Nine existing targeted CTest cases passed. |
| F2.1–F2.10 | Implementation complete | Targeted native parity, standalone Release, IPC framing and the packaged companion scenario passed. |
| F2.11 / F2-GATE / F-I-GATE | PARTIAL | Full engine CTest passed on 2026-11-10. Owner visual qualification and remaining F-I gate checks are tracked separately. |

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
5. Save All, then rename a temporary Scene and change an Enemy Prefab field.
   Use X on the tab of the renamed Scene itself. Check its header for Unsaved
   before closing. Cancel retains the tab. Click X again and choose Discard
   Document: reopen the Scene and check its saved name, then check Enemy is
   still dirty. Repeat with Save All and in a floating window. A clean document
   or a generic Scene panel can close without prompting. Asset edits are saved
   at project scope and do not dirty individual document tabs.
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

The owner confirmed the F1 manual verification on 2026-11-08. The checklist
below is the procedure supplied with implementation commit `695bd40`. Run the
standalone EXE maximized with a disposable Cyrillic-path project to reproduce it.

1. Create a Prefab. Import multiple PNG/static WebP files (include non-square
   alpha), a supported PCM WAV and a TTF/OTF. Empty destination means Content
   root; a nested destination parent must already exist. Cancel picker/destination:
   no imported row or dirty change. Repeat the same destination for one file:
   Problems opens with that file's destination and the rejection. To update an
   existing indexed file, use Reimport.
2. Compare Explorer Tree/List/Grid; Enter-to-search, clear filter, select assets.
   Check real ID/type/generation/source/cooked metadata, long/Cyrillic paths,
   aspect/alpha/Fit preview, retained selection/scroll and metadata clipboard copy.
   WAV/font have metadata/IDs without pretending image preview or playback.
3. Select the imported file under Content, then Assign to Prefab and open its
   canvas. Use Rename / Move and Duplicate on the same imported file. Original
   ID/refs stay and Duplicate proposes a separate Copy filename. Document
   Rename is a separate command for Scene, Prefab and Graph.
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

## F1 completion audit

Reviewed the specification against `695bd40`, the existing native regression
sources, the common UI/IPC contracts, and the packaged Release evidence. Product
code was unchanged during this review.

| Requirement | Status and evidence |
|---|---|
| F1.0 native ownership | PASS. Owner and query/command mapping above and in UI_BRIDGE_CONTRACT.md. Protected P04 fixtures passed. |
| F1.1–F1.2 Explorer and import | PASS. Common catalog/selection/import tests plus owner acceptance of Explorer, File Picker, Unicode and Cancel. FI-013–015 and FI-017. |
| F1.3 queue and cancellation | PASS. Native job outcomes, cancellation before publication and explicit 128-waiting-job backpressure. FI-016 and closure audit. |
| F1.4 metadata, previews and references | PASS. Asset Details capture, alpha and aspect tests, current native metadata and stable reference navigation. FI-018. |
| F1.5–F1.6 lifecycle | PASS. Stable IDs, reference-aware delete/restore, history, Save/Open and Clone. FI-019–021 and FI-026. |
| F1.7 watch and last-good | PASS for canonical project Content. Native debounce, same-size/same-mtime replacement, stable ID, last-good and recovery covered by AssetTests and P04 async tests. FI-022–024. |
| F1.8 safety and performance | PASS under the owner-approved gate exception. Traversal, case collision, Unicode, long paths, junction escape, bounded queue, denied source sharing, destination ACL denial and companion restart after interrupted import verified. Disk-full and Windows reboot during import remain NOT RUN in the backlog. FI-025 path coverage passed. |
| F1.8a additional formats | PASS. PNG, static WebP, PCM WAV, TrueType TTF and CFF OTF imported, reopened and reimported with invalid-source recovery. Each format rejected mismatched bytes and a 128 MiB + 1 byte source, and cancelled during a confirmed partial staging copy without publishing or leaving staging files. FI-026a. |
| F1.9 gate evidence | PASS. Owner workflow, native stable ID, Asset Details capture, Prefab and Scene GPU before/after, source and cooked SHA-256 changes, cancellation and persisted P04 model verified. Scene proof used the current Scene panel, packaged viewport DLL/runtime and native snapshots from a P04 copy with three Prefab placements. |

Closure audit on 2026-11-08 used the packaged Release companion on a disposable
project. It rejected a destination junction with zero writes outside Content,
reported a locked source without publishing it, rejected the 129th waiting
job and cancelled the queue, and reopened the saved project after the companion
was terminated before import publication. The interrupted destination was
absent and retry succeeded. See `Temp/U/logs/f1-closure-audit.stdout.log`.

The follow-up `f1-remaining-native` run verified Windows `ERROR_ACCESS_DENIED`
on private staging, unchanged catalog on failure and successful retry after
ACL restoration. `f1-scene-proof` captured three unclipped sprites before and
after reimport, each 89–90×45 pixels, plus Asset Details. AssetId
`4152778353207564332` remained stable while the native fingerprint, generation,
cooked key and both file SHA-256 digests changed. Native resource invariants and
surface teardown passed. Logs are in `.ai/logs/`; captures are in
`Temp/U/f1-scene-proof-adc146b518194109b1b8574221134414/captures/`.

The Windows token has a filtered Administrators group. Hyper-V denied both
`Get-VM` and creation of a small test VHD. A separate bounded Windows volume
and a rebootable Windows guest are needed for the two remaining F1.8 scenarios.

## F2.0 Scene and Prefab contract

Entry commit `aa5b336` closed F1. The audit uses the existing C++ authoring
owners, archive and viewport. The next change starts with hierarchy wiring.

| Surface | Native ownership and persisted identity |
|---|---|
| Scene hierarchy | `SceneCompositionDefinition` owns `SceneSnapshot` and placements. The archive persists the Scene AssetId and each SceneNodeId, parent, local TRS, mobility, visibility and optional local bounds. `CreateSceneComposition` maps authoring IDs to runtime nodes, attaches parents with KeepLocal and derives world transforms/bounds. A Scene node is a spatial record. A placement on that node references the Prefab AssetId and carries overrides. |
| Prefab definition | `PrefabDefinition` owns a logical tree of PrefabNodeIds and an archetype per node. The archive persists children, parameters, component type/part, property IDs and behavior bindings. `UiSelection` addresses a component by document, node, type and part. Logical children have no persisted spatial TRS or reference to an independent nested Prefab definition. |
| Placement overrides | `SetParameterOverride` targets a placement and parameter ID. `SetComponentOverride` targets its Prefab node, component type, part and property ID. Both verify the expected Prefab and patch only the addressed override. Reset erases that entry so the definition value applies. |
| Runtime instances | `PlaySession` takes the native snapshot, registers archetypes and freezes EntityService, builds Scene spatial services, registers graphs and Prefabs, then calls `PrefabComposer::Spawn` for each placement with its overrides. The composer plans and validates through ComponentRegistry, creates fresh EntityService entities, attaches prepared components and behaviors, and rolls back failed instantiation. Play binds the spawned root entity to the runtime node for that placement. Runtime handles stay inside Play. |
| Systems | Framework `SystemInstance` supports Scene and World scopes through GraphBehaviorService and the existing scheduler. `Game/Composition/tests/system_instance_tests.cpp` covers separate lifetimes and event routing. The current project archive and `PlaySession` have no persisted Scene/World system assignment path. F2.9 needs that native owner contract before enabling edits. |
| Scene rendering and input | The companion derives world items from native `SceneView`. `UiPanels.scene` feeds the projected sprite DTOs to `NativeViewport` and the same `EpidemicUiViewport` JNI/JAWT DLL and D3D11/D2D Renderer qualified in U and F1. `SceneView` already owns DIP/world conversion, fit, pan/zoom and hit testing. `SceneDrag` owns local/world gizmo preview, staleness checks and commit/cancel. F1 feeds a sprite preview into the renderer. F2 must connect editor camera and gestures to these existing contracts. |

Native `EditorWorkspace::Execute` and `EditorProject` validate the expected
revision, apply the whole command batch to a candidate, validate graph and
persistence invariants, then publish one history step. Use one batch for child
creation and placement, one validated commit for a completed drag, and zero edits
for a cancelled or stale drag. Undo/Redo restore IDs and links. Reparenting uses
KeepLocal, preserves the node and placement IDs, and rejects invalid parents,
cycles and cross-document input. DeleteSceneNode removes the subtree and its
placements as one edit.

The current P03 path supports one typed selection and existing PlacePrefab
transactions. It has no dedicated subtree duplication or Scene node rename
command. Scene nodes store visibility and local TRS, with no authoring name or
enabled field. Prefab preview draws the bound root sprite. Spatial Prefab
children and nested asset instances are limited by the archive contract above.
The native placement-to-runtime path is present and covered by CTest. Live
runtime presentation is a separate integration.

Verification on 2026-11-08: **9/9 targeted CTest cases passed** across SceneView
and SceneDrag, Workspace and Inspector, Core, Demo Edit/ReadPlay, GameComposition
and Folder Create/Reopen/Recovery. The F1.9 packaged Scene capture already verifies
the retained Renderer path with three persisted placements. The remaining IPC
work is recorded in UI_TECH_ADR.md.

## F2 implementation

Entry commit `46199a9`. The focused SceneView/SceneDrag target passed with
three persisted Health overrides, nested link cycle rejection and Save/Open
with an inherited default update and a retained nested override. The Release
EXE was packaged and IPC v4 passed all 16 framing cases. A short packaged
companion run verified hierarchy edits, one-edit SceneDrag and Undo, registered
component defaults and Prefab placement. Computer Use was not used. Full
engine and owner visual qualification are reserved for the combined F-I run.

| Stage | Wiring and remaining work |
|---|---|
| F2.1 | Scene Hierarchy uses native Add/Delete/Rename, KeepLocal reparent, unparent, sibling reorder and Prefab assignment. Context actions and scoped drop submit a single transaction. Tree refresh preserves typed selection and uses existing IDs. Subtree duplicate remains unavailable. |
| F2.2 | Component Registry IDs and real factory defaults drive Add/Remove. Inspector keeps native scalar validation. Graph links use a compatible native Event Entry and AssetId. |
| F2.3–F2.4 | The existing native viewport draws bounds, axes, grid, selection and resource sprites. Native SceneView supplies pan/zoom/pick. Numeric local TRS, reset and SceneDrag implement Move, Rotate, Scale and move snapping. Cancel and preview are transient. UI/DPI/capture behavior remains for owner qualification. |
| F2.5 | Drag a Prefab from Explorer to place it at the native world point. Drop a texture on a placed Prefab or choose it in Inspector. A texture already bound to a Prefab can place that Prefab from an empty-space drop. The existing Sprite binding is definition-scoped and the UI confirms that scope. Creating a standalone sprite from an unbound texture remains unavailable. |
| F2.6–F2.7 | Prefab child/component authoring stays definition-scoped. Placement fields show definition, override or runtime values. Instance edits and Revert use the existing native override commands. Preview shows the bound root sprite. |
| F2.8 | Scene parenting and logical reparent of a Prefab's own child nodes preserve identity. Native archive links retain target AssetId and stable link ID. The editor materializes linked definitions for the existing PrefabComposer and Inspector. Per-placement nested overrides persist on deterministic expanded node IDs. A native transaction rejects cycles. Nested Prefab content is logical and uses the root sprite preview. |
| F2.9 | Scene and World Graph assignments persist with enabled state and stable IDs. Play creates existing Framework SystemInstances for enabled assignments and registers their Graph behaviors. Graph routing uses an existing compatible Event Entry. |
| F2.10 | Existing native SceneView test extended for three persisted placements, Health 100/150/200, repeated Play/Stop and linked-definition update with a preserved nested instance override. |
| F2.11 | Full engine CTest and packaged smoke passed. See the updated owner retest below and UI_QA_MATRIX.md. |

For the owner run, create a disposable Scene and Prefab, add a registered Health
component, and choose an imported PNG/WebP. Place three copies from Explorer.
Use both hierarchy and viewport to select them, then set independent Health
overrides and local transforms. Drag a gizmo and undo it once. Cancel a second
gesture with Escape. Reorder and reparent Scene rows, try an invalid cycle, then
Save and reopen. In Prefab Hierarchy, link a second definition, change its
default, and check that an explicit nested placement override survives. Exercise
Scene/World system add, enable and remove with a Graph that has an Event Entry.
Finish with Play/Stop, Clone, resize and a detached viewport.

## Owner retest after the 2026-11-10 fixes

Use the updated standalone image. Close the previous editor window before
opening this build with the normal profile.

1. Open a Scene, then select an imported file under Content. Try Rename / Move,
   Duplicate, Reimport and Delete. Restore the deleted row. Check that each
   command applies to the row and that returning from an asset dialog keeps
   that asset selected.
2. Import the same file into the same destination. Problems should open and
   name the destination. Importing a second copy into another folder is valid.
3. Save All, rename an open Scene document, and change a field in Enemy
   (a Prefab). Click X on the renamed Scene's own tab while it says Unsaved.
   Cancel keeps it open. Discard Document closes it and restores the saved name.
   Enemy's edit remains. Reopen the Scene and repeat with Save All.
4. With several central tabs open, scroll over the Scene grid, then over the
   tab strip. The grid gesture zooms Scene. Tab-strip scrolling stays available
   over the tabs. Save, Undo, Redo and Play remain in the merged main toolbar
   when switching documents.

The registered example Health points range is 0–10000 inclusive.
`component.value_rejected` for -1 or 112322 is expected. A rejected draft
blocks Save until it is corrected or cancelled with Escape.

## Повторная приёмка после замечаний к 5503770

Владелец подтвердил операции в Content, конфликт импорта, закрытие изменённой
вкладки, независимую прокрутку Scene и вкладок, а также пункты 6–9, 11–12, 14 и 16
предыдущего прогона. Обновление ресурса из пункта 15 наблюдалось на Scene.
Ручная проверка переключателя Watch, уточнение пункта 13 и пункты 17–20 ожидают
повторной приёмки.

Автоматические проверки включили 8 адресных CTest, общий Swing/IPC контракт
и smoke упакованного Release. Проверены сохранение спрайта при дублировании
Prefab, восстановление через Undo, размеры и alpha cooked preview, разделение
Problems и Console, отмена оставшихся файлов из File Picker и запись
Health override через поле Inspector. Ручная квалификация продолжается.

### Исправления

1. **Documents.** На тестовой сцене, которая не является стартовой, или на
   неиспользуемом Prefab откройте **Project Explorer → Document Actions…**.
   Переименуйте документ, сделайте копию и сверяйте разные Document ID.
   Если у исходного Prefab был спрайт, проверьте спрайт в копии. Удалите
   открытый дубликат, убедитесь, что закрылась его вкладка, и верните его через
   Undo. В Documents имя задаётся через Rename, а восстановление после удаления
   идёт через Undo. Перенос по пути и Restore из Trash относятся к Content.
2. **Problems.** Импортируйте новый PNG, затем повторите импорт под тем же
   именем. Успех должен быть в Console, конфликт с путём назначения в Problems.
   Очередь и успешная публикация не должны появляться в Problems.
3. **Asset Details.** Выберите PNG с прозрачностью, у которого ширина больше
   128 px. Переключите Full, Half и Quarter. Сверьте размер под превью и
   прозрачность на шахматном фоне. С выключенным Fit preview изображение
   показывается в масштабе 1:1 и обрезается границами панели, если не влезает.
4. **Отмена импорта.** Выберите сразу несколько десятков новых PNG. Сразу после
   подтверждения пути нажмите **Assets → Cancel native imports** или одноимённую
   кнопку в Jobs. Уже опубликованные файлы останутся. Очередь должна перейти
   в idle, а следующие файлы из этого выбора перестать появляться в Content.
   Jobs показывает последние результаты, включая Cancelled, если файлы успели
   попасть в нативную очередь.
5. **Пункт 13.** Откройте Prefab с Health, поместите его на сцену три раза и
   выберите экземпляры по одному. Справа в Inspector найдите **Health / points**.
   Оставьте у первого значение по умолчанию 100, второму задайте 150, третьему
   200, подтверждая Enter. Проверьте значения повторным выбором. У второго
   нажмите Revert Override и убедитесь, что он вернулся к 100, а третий остался
   на 200. Для отдельной проверки Add/Remove создайте Prefab без стартовых
   компонентов, выберите его корневую строку в Hierarchy, добавьте Health,
   выберите строку Health и нажмите Remove Component.
6. **Watch.** В **Assets → Watch Content…** нажмите Enable. В Jobs должно быть
   **Content watch: on**. Замените уже импортированный PNG внутри папки Content
   проекта. Дождитесь изменения картинки на сцене. Повторите замену с Watch
   выключенным и примените её вручную через Reimport.

### Оставшиеся пункты

17. **Вложенный Prefab.** В Scene Hierarchy щёлкните правой кнопкой по корню
    Prefab A и выполните Link Nested Prefab… → B. На экземпляре A переопределите
    вложенный Health из B. Измените значение Health в определении B и проверьте,
    что второй экземпляр A наследует новое значение, а первый хранит override.
    Попытка связать B обратно с A должна быть отклонена.
18. **Save, перезапуск и Play.** Сохраните проект с разными Health и Transform,
    закройте редактор и откройте проект снова. Сверьте значения и ID трёх
    экземпляров. Запустите Play, проверьте runtime-значения выбранных
    экземпляров в Inspector, затем Stop. Сверьте значения authoring после Stop.
19. **Плавающее окно и Clone.** Вынесите Scene или группу Inspector в отдельное
    окно, измените размер и проверьте выбор одного и того же объекта.
    При сохранённом проекте используйте File → Clone Saved Project, откройте
    клон и измените его. Оригинал должен сохранить свои данные.
20. **Scene/World Systems.** Для валидного Graph возьмите рабочую копию
    `Game/Editor/example/EditorDemo/EditorDemo.epidemic` и откройте её в новой
    оболочке. В Scene Systems добавьте Graph с Event Entry, выключите и включите
    систему, сохраните и проверьте её после открытия. Повторите с World scope.
    Попытка назначить новый пустой Graph должна дать `editor.graph_event_entry`.
    В конце удалите тестовые назначения.

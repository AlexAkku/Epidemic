# F-I implementation

Starting point: branch `v2`, commit `32eef79ba28519f3b62f0576a38fe039d80e112e`
(`Replace legacy editor UI with standalone IntelliJ Platform host`), clean tree,
2026-11-06. Task: `Temp/!Tasks/F-I_Implementation_Spec.md` revision 1.0.
Computer Use is excluded; visible acceptance belongs to the owner.

## Ownership and compatibility

| Path | Existing authority / data / verification |
|---|---|
| Project lifecycle | `PlatformSession.installActions/connect/close` → `Bridge.call` → backend operations 2/3/6/7/8/10/12/13/14 → `EditorWorkspace` Open/NewFolder/Save/Undo/Redo/CloneFolder/Play/Stop; immutable `Bridge.Snapshot`, `Project.epidemicproj` or retained legacy archive; `UiTests.lifecycle/compatibility/main` |
| Documents | `EditorProject::Snapshot`, `ProjectSnapshot::Find`, `EditorDocument` (`AssetId`, kind, revision, name); `CreateDocument`/`RenameDocument`/`DeleteDocument`/`SetEntryScene` through `EditorWorkspace::Execute` and `ProjectFolder::Execute`; one project transaction/history. There is no native open-tab owner: opening/closing a view is presentation state. |
| Selection | backend operation 9 → `EditorWorkspace::Select`, `UiSelection`/`UiElement`; snapshot selection kind/document/element. No serialization or authoring revision change. |
| Inspector | `InspectComponents`, `InspectorCommand`, `EditorWorkspace::ParseValue` → `SetProperty`/parameter/override commands; current Platform forms are still Planned, not a second scalar model. |
| Assets (F1, not yet wired) | `EditorWorkspace` QueueImportPng/ReimportPng/CancelAssetJobs/EnableContentWatch/PollContent/QueueDuplicate/MoveAsset/DeleteAsset/RestoreAsset/AssignPrefabSprite → native ProjectFolder/catalog/cook/leases/history. Current snapshot exposes source IDs, generations, fingerprints, cooked keys and trash state. P04 tests retained. |
| Viewport (F2) | `NativeViewport` → JNI `native_viewport.cpp`, EDT/JAWT lifetime → retained Renderer. Current shell does not perform document-specific authoring draw/picking. `SceneView`/`SceneDrag` remain native projections; no second scene/camera model is introduced. |

Current persisted kinds are SceneCompositionDefinition, PrefabDefinition and
GraphDefinition. New UI fields without schema support stay disabled. No archive
schema or public Framework ABI change is planned for the document binding.

## Tracking

| ID | State | Scope / evidence |
|---|---|---|
| FI-0 | PARTIAL | Source ownership mapped above; current common/native baseline PASS, 87.40 s (`Run-Quiet fi0-baseline`; the wrapper discards successful logs). Historical full CTest is 161/161 from U/L0, not a new run. |
| F0.1 | PARTIAL | Existing lifecycle contracts pass at entry. Shared command/availability review and broader UI status remain to finish. |
| F0.2 | PARTIAL | First vertical slice: native Create/Open/Rename/Save bindings, scoped document tabs, `UiTests.documents/documentReaders`; final common/native contracts PASS, 53.89 s. Per-document dirty/close policy and manual detached-tab qualification remain pending. |
| F0.3–F0.7 | NOT RUN | Full selection/Inspector/dirty document qualification and F0 gate pending. |
| F1 | NOT RUN | Asset mutation/import/watch integration starts after F0. |
| F2 / F-I-GATE | NOT RUN | Scene authoring/runtime parity and owner acceptance pending. |

Current-slice verification (2026-11-06): common/native contracts PASS 53.89 s;
IPC adversarial and catalog checks PASS; native Editor CTest **27/27**, zero
failed/skipped (82.87 s wall time, `Temp/U/f02-editor-tests.xml`); managed-off
C++-only build PASS; packaged Release smoke PASS (including the document path,
Play/Stop and crash/reconnect, zero surfaces/fatal diagnostics). The combined
check command took 274.75 s and ended FAIL solely because its final restart
driver evaluated `$PSScriptRoot` in a parameter default on Windows PowerShell
5.1. Path resolution was moved into the script body; the unchanged real
activation/crash/restart assertions then **PASS, 52.07 s** under PowerShell 5.1.
Successful wrapper logs are intentionally discarded; product diagnostics are
under `Temp/U/distribution/EpidemicEditor/app/{smoke,qualification}-state/log`.
The full 161-case engine profile is not rerun for this first binding slice;
the new task requires it at stabilized F0/F1/F2 closure. It remains NOT RUN for
the current F0 phase, as do physical/manual acceptance and F0-GATE.

Changed-file Deep analysis ran against the existing host compilation database:
one translation unit (`backend.cpp`), no changed headers, two checks, 25.90 s.
Raw result **FAIL**, retained under `Temp/U/logs/f02-static.*` and
`{tidy,cppcheck}-tools_editor-ui-host_backend.cpp.*`: six clang-tidy diagnostics
at unchanged `scene_state.h:23–26` and `scene_composition.h:38/40`, plus the
unchanged cppcheck `runtime_bridge.h:504` passedByValue diagnostic. These seven
are a subset of the nine already classified U/L0 findings; no finding points
to changed source. No warnings, tests or analyzer policies were suppressed.

Fixtures used at entry: generated empty Unicode project; protected committed
`Game/Editor/example/EditorDemo/EditorDemo.epidemic`; current P04 backup under
`Temp/U/fixtures/P04-backup` copied to disposable test folders including immutable
cooked images. Original fixtures must remain unchanged. Corrupt/missing cases
must be explicitly generated in disposable outputs when their tests are added.

Existing U acceptance and its nine classified analyzer diagnostics are unchanged.
Continuous simulation/live Game View belong to F-II/F8 under the new task;
they are not prerequisites for F-I Play/Stop acceptance and are not implemented.

## Owner verification: first document slice

Use the rebuilt `Temp/U/distribution/EpidemicEditor/EpidemicEditor.exe`, maximized.
Use a disposable new project, never the committed demo or P04 backup directly.
These steps are NOT RUN by the agent; no Computer Use is involved.

1. Create a project in a Unicode path. In File or Explorer → New Document,
   create Scene `Арена`, Prefab `Герой`, Graph `Логика`. Verify three new rows
   with distinct IDs and named tabs with their own ID/revision headers.
2. Double-click each document twice. Verify the existing tab is selected,
   rather than a duplicate tab or independent native project being created.
3. Select a row and use Edit / Explorer → Rename Document. Verify the row,
   tab name and header change together; folder expansion and scroll are kept.
4. Put focus outside text inputs. Undo/Redo a rename and a creation. The
   native values/rows must follow; an undone open document safely shows missing.
5. Detach a document tab using the already accepted Platform docking. Rename
   its selected native document; the detached header/title must update as well.
6. Save All, Close Project, reopen (also after an EXE restart). Verify all
   document IDs, kinds and names are preserved; revisions may be rebuilt.
7. Cancel New/Rename: no new row/change. Submit an empty name: a native error
   dialog, no authoring change. During Play, New/Rename are disabled; Stop
   enables them again and leaves saved authoring intact.

Scene/Prefab/Graph canvases remain the existing shells. This batch adds no
entity/component authoring, scalar fields, editable Graph Canvas, import or
gizmos. Closing a tab currently releases its presentation only; authoring is
retained by Workspace. A separate per-document Save/Discard/Cancel policy and
dirty indicator are still pending, so this is **not F0-GATE PASS**.

Next implementation: F0.3 typed selection and context synchronization, then
F0.4 native scalar Inspector, F0.5 document dirty/close ordering, F0.6–F0.7
phase qualification. F1 starts only after the F0 gate.

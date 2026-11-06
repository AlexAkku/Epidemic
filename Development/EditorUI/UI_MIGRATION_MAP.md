# UI migration map

## L0 dependency inventory — owner contract 2026-11-05

U-GATE is accepted in UI_QA_MATRIX.md. Owner retires the old UI compatibility requirement and authorizes L0 before complete F parity. Source graph plus exhaustive include/CMake/script references establish the following removal boundary.

| Category | Sources / targets | Decision |
|---|---|---|
| A: retired native shell | `ui/windows_editor.cpp`, `windows_content_browser.cpp/.h`, `windows_scene_view.cpp/.h`, `windows_editor_style.h`; `EpidemicEditorDesktop`, `EpidemicUiCompatibility` | REMOVED: Win32 entry point, controls, native dialogs/child-window wrapper and launch/copy paths. |
| A: obsolete shell design | `ui/DESIGN.md`, `ui/PRODUCT.md`, `ui/.impeccable/design.json` | REMOVED: retired Win32 presentation specifications; superseded by the accepted Platform Roadmap. |
| A: diagnostic prototype owners | `EditorApp`, `DockSpace`, `DockLayout`, `IslandsTabsUI`; old U1 HostProbe/plugin launch scripts | REMOVED; shared name validation/tests retained, live JNI adapter relocated. |
| B: reused editor session/projections | `ui/src/editor_workspace.cpp`, `editor_inspector.cpp`, `scene_view.cpp`, `scene_drag.cpp`, their public headers and `EpidemicEditorUi` static library | Keep: native backend and semantic tests depend on them. This library creates no user-facing editor window. |
| C: shared engine/data infrastructure | Editor Core, ProjectFolder, source/cooked cache services, Resources/leases, Graph VM, PlaySession, Renderer, EngineRuntime, GameFramework | Keep; no backend API/model/serializer/data removal. |
| D: tests/data | Core/folder/content/watch/thumbnail/SceneView/Inspector/Workspace/stress/headless tests, demo archives and fixtures | Keep all semantic regressions. Retire only tests of the deleted Win32 window/handlers. Preserve PNG GPU/lease smoke as an independent hidden-surface resource test. |

Window-specific smoke targets: ContentDesktopSmoke, DesktopSmoke, DemoDesktopSmoke, its dependent UI-produced-file reader, DesktopStress. Core command/history/Play/Scene/Inspector/stress and cross-process demo tests remain; Platform EXE smoke covers the replacement shell. The resource preview test keeps prepared PNG upload, repaint/resize, release/registration cleanup and repeated acquisition without retaining the Content Browser UI.

The adapter files `native_viewport.cpp` and `NativeViewport.java` were moved unchanged into `tools/editor-ui-host`; the obsolete spike directory is removed. Their JNI names are preserved. Cache cleanup is restricted to verified generated/obsolete builds and reports, preserving current dependencies, game projects, fixtures and source data.

## Existing functional owners for F0 and later

| Former UI trigger / source | Existing backend call | Target presentation / phase |
|---|---|---|
| File Open, `Desktop::Load/Action/Choose` | `EditorWorkspace::Open`, extension routes to `OpenFolder` | Launcher/Recent/File, U3/F0 |
| New legacy/demo archive | `New`, demo template imported through command/persistence path | Explicit template/legacy path, F0 |
| New project folder | `NewFolder(root,title,definition)` | Launcher name + parent produces `parent/name`, U3/F0 |
| Clone project folder | `CloneFolder(destination)` | File/Clone, F0/F1 |
| Save | `Workspace::Save` routes to active owner | File/toolbar/router, F0 |
| Undo / Redo | `Workspace::Undo/Redo`, `CanUndo/CanRedo` | Edit/toolbar/shortcut, F0; preserve folder-wide ordering |
| New/Delete/Rename document, default Scene | `CreateDocument`, `DeleteDocument`, `RenameDocument`, `SetEntryScene` in `Execute` | Project Explorer, F0/F1 |
| Add/Delete Entity | `AddEntity`, `DeleteEntity` | Prefab hierarchy/Inspector, F2 |
| Add/Delete Component | `SetComponent`, `DeleteComponent` | Schema picker/Inspector, F2/F4 |
| Apply typed field | `SetProperty` (definition); Inspector builder | Universal Inspector, F2 |
| Assign/Remove behavior | `SetBehavior`, `DeleteBehavior` | Inspector typed graph references, F3 |
| Add/Delete Graph node | `AddGraphNode`, `DeleteGraphNode` | Graph Canvas/palette, F3 |
| Connect/disconnect ports | `ConnectPorts`, `DisconnectPorts` | Graph Canvas, F3 |
| Add/Delete Scene node | `AddSceneNode`, `DeleteSceneNode` | Scene Hierarchy, F2 |
| Scene parent/TRS/gizmo commit | `SetSceneParent`, `SetSceneTransform` | Hierarchy/Scene toolbar/Inspector, F2 |
| Assign Prefab | `PlacePrefab` | Scene/Prefab chooser, F2 |
| Expose/modify parameter | `SetPrefabParameter` plus `SetProperty` transaction | Prefab Editor, F2 |
| Instance override/reset | `SetComponentOverride`, `SetParameterOverride` | Prefab context Inspector, F2; preserve unrelated overrides |
| Open definition | `PrefabDefinitionSelection`, `Select` by stable IDs | Prefab Editor first anchor right context wall, U5/F2 |
| View/Content Browser | `RunContentBrowser(owner, workspace)` modal dialog | Project Explorer presentation, U6/F1; keep backend |
| Import external PNG | `QueueImportPng`/`ImportPng` | Native picker U6; existing async cook wired F1 |
| Reimport/Cancel/watch | `ReimportPng`, `CancelAssetJobs`, `EnableContentWatch`, `PollContent` | Explorer/Jobs/Preferences, F1 |
| Duplicate asset | `QueueDuplicate` | Explorer/Assets, F1; no cross-project remap promise |
| Rename/Move, file history | `MoveAsset`, workspace unified Undo/Redo | Explorer/Assets, F1 |
| Trash/Restore | `DeleteAsset(asset,keep_broken_references)`, `RestoreAsset` | Explorer/references policy, F1 |
| Sprite assignment | `AssignPrefabSprite`, typed `Execute(AssetEditRequest)` | Asset/reference Inspector, F1/F2 |
| Sprite/sheet metadata | `AssetEditRequest` existing typed variants | Sprite/Sheet editor fields, F1/F7 |
| Play/Stop/Tick/Send event | `Workspace::Play/Stop/Tick/Send` | Toolbar/Game/session tools, F0/F8 |
| Runtime property read | `PlaySession::ReadComponentProperty`, `ReadSprite`, activations | Read-only Runtime Inspector, F8 |
| Native viewport draw/resize/input | JNI NativeViewport + retained SceneView/SceneDrag projections → existing Renderer | Scoped viewport surface adapter after U1, U2/F2 |

No direct project-file writes from Swing/Compose. All authoring requests carry expected revision and typed stable IDs. JNI handles, native addresses, HTREEITEM and HWND cannot become Entity/Asset identities. Snapshot read adapters must copy only values with bounded ownership; no second Undo, Scene, Graph VM or Project per window.

Future panels that lack current calls are `planned` in [UI_FEATURE_MATRIX.md](UI_FEATURE_MATRIX.md), not renamed legacy actions. Current runtime/resource/archive/fixture files are shared functionality; their browser/editor names are not grounds for L0 deletion.

## First F0 bindings after L0

CloneFolder is exposed by File → Clone Saved Project; it requires a clean folder,
keeps the current Workspace, rejects overwrite/nesting and preserves identities.
Save All aliases the existing atomic whole-project Save. Play/Stop use existing
isolated native session lifecycle (toolbar/Project menu/F8), with snapshot flag 8
reflecting actual IsPlaying. Tests pass on empty, legacy demo and P04/cooked data.
Continuous stepping/live Game View, full scalar Inspector/document workflows and
asset mutation remain unwired; complete F0/F1 parity is not claimed. No engine
public signatures, archive fields, registries or UI panels were added.

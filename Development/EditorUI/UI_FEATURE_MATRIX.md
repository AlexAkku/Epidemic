# UI feature matrix

Sources: `Temp/!Tasks/UI_Roadmap.md` §§7,10–12, `Temp/!Tasks/U_Phase.md`. Current-source inventory at `v2/eab3200`, 2026-11-04. Current host: actual IntelliJ Platform; selected wiring and evidence below supersede the original U0 inventory. U-GATE PASS under the owner contract dated 2026-11-05. `backend-only` means preserved C++ API without a new-UI binding, not complete feature acceptance. `planned` means disabled in a future shell; no runtime registration is implied.

| Feature ID | Capability and current evidence | Route now | Integration phase |
|---|---|---|---|
| project.lifecycle | Workspace Open/New/OpenFolder/NewFolder/Save/CloneFolder; folder committed revisions, writer lock and legacy reads | wired: Open/NewFolder/Save/Save All/Clone/Undo/Redo/Select; native F0 lifecycle contracts passed | F0 |
| project.launcher | Launcher-first startup, 10 Recent, external preferences, name/parent validation | wired Platform launcher/Recent; owner manual acceptance PASS | U3/F0 |
| history.transactions | Execute with expected revision; unified folder Undo/Redo, stable IDs; 64-step history | wired RenameDocument/Undo/Redo; remaining edit commands backend-only | F0 |
| scene.authoring | Add/Delete Scene node, TRS, parents, PlacePrefab, picking/gizmos; SceneViewTests | backend-only | F2 |
| prefab.authoring | Definition tree/components/parameters, targeted instance overrides, binding/reset; workspace/Inspector tests | backend-only | F2 |
| inspector.schema | Bool/i64/double/string/ref values supported through existing schema; future Enum/Flags/Vector/Color/Array/Optional widgets are not current runtime types | backend-only / planned fields | U4/F2/F4 |
| assets.png | WIC PNG/cook, stable AssetId, leases, async import/reimport, last-good; ContentTests/ContentReopen | backend-only | F1 |
| assets.lifecycle | Move/rename, queued duplicate, staged delete/restore, reference diagnostics, global history; SpriteHotTests | backend-only | F1 |
| assets.watch | Workspace-level watcher, bounded jobs, cancel, stale epoch filtering, audit; AsyncPngTests | backend-only | F1 |
| assets.sheet | Typed visual/sheet metadata, stable regions, thumbnail cache helpers; SheetCore/ThumbnailCache tests | backend-only / limited | F1/F7 |
| assets.explorer | Folder tree, breadcrumb, rich filters/grid, multi-select, typed drag/drop, full Project Explorer | planned | U6/F1 |
| assets.native-picker | Platform FileChooser/JBR Windows native selection; selected source performs no import | picker wired; import/cook backend-only | U6/F1 |
| graph.authoring | Existing command-based nodes/ports/connections, validation and native command services; editor tests | backend-only | F3 |
| graph.canvas | Canvas zoom/pan, typed visual ports, palette, variables, node Inspector, comments | planned | U6/F3 |
| graph.scene-drag | Drawn affordance and helper text; reference creation must be disabled until contract | planned | U6/F3 |
| play.session | Existing isolated Play runtime and pinned Sprite generation; actual lifecycle/data-isolation tests pass | wired Play/Stop lifecycle; Tick/Send/live Game View still backend-only/planned | F0/F2/F8 |
| play.game-view | Full game input focus, Pause/Step/Restart/time scale, runtime renderer window | planned (Scene View is authoring) | U6/F8 |
| types.registry | C++ borrowed frozen ProjectSchema exists; user-authored Type Registry UI/C# workflow unqualified | planned | U6/F4 |
| code.space | Code tabs, source tree, symbols, search, diagnostics, compile, Code↔Graph usages | planned | U6/F4 |
| components.rendering | Transform and typed Sprite bindings exist; camera/tilemap/animator/particles/mesh/lights UI fields are planned | limited backend-only / planned | U6/F2/F7/F13 |
| components.gameplay | Example Health/Movement registered in host; damage/projectile/weapon/waves/timer/inventory/AI fields are UI targets | limited backend-only / planned | U6/F5 |
| components.physics | Rigidbody/collider 2D/3D, masks/materials, constraints and navigation tools require real authoring contracts | planned | U6/F5/F13 |
| components.behavior | Existing GraphBehavior assignment; script, complete system editing, UI binding and custom component UI planned | limited backend-only / planned | U6/F3/F4/F6 |
| ui.canvas | Canvas/widget hierarchy, layout, anchors, bindings and event fields | planned | U6/F6 |
| ui.widgets | Panel/Text/Image/Button/Slider/Progress/Checkbox/Input/Dropdown/Scroll and layout containers | planned | U6/F6 |
| ui.flow | Screen transitions, conditions, overlays/back/focus policy | planned | U6/F6 |
| animation.tools | Clip/controller, timeline, tracks/keyframes/markers/states/transitions | planned | U6/F7 |
| art.tools | Sprite/atlas/tilemap editor tools, packing, palette/brushes; metadata helpers are not full tools | planned / limited helpers | U6/F7 |
| materials.tools | 2D texture/color/blend/effects and 3D metallic/roughness/normal slots | planned | U6/F7/F13 |
| audio.tools | Clip properties/waveform/preview, mixer master/music/SFX/UI routes, listener/emitter | planned; MediaProbe is helper coverage only | U6/F7 |
| debug.console | Current diagnostic/activation text exists; complete Console/Problems/Debugger UI needs routing | limited backend-only / planned | U6/F8 |
| profile.metrics | Frame/CPU/GPU/resources/allocations/entities/graphs/UI panels | planned | U6/F8 |
| build.export | Settings/targets/scenes/cook/script compile/Windows and mandatory Web export shells | planned | U6/F9/Q1 |
| settings.project | General/render/input/physics/scripts/assets/build/security | planned | U6/F0–F9 |
| settings.editor | Theme/layout/keys/navigation/font/performance/accessibility | planned | U4–U6 |
| agent.workspace | Prompt/context/proposed edits/diff/review/validate/logs, no fake execution | planned | U6/F10 |
| docking.multi-window | Standalone Platform ToolWindow/FileEditor owner; independent HWND and programmatic redock verified; owner whole-group docking/central-document acceptance PASS | wired shell / accepted revised contract | U1/U5 |
| theme.islands | Actual Islands Dark in the standalone host with shared form tokens; Platform owns look-and-feel | wired shell / owner resize/scaling acceptance PASS | U4 |

## Availability contract for subsequent implementation

Separate backend state (`Unavailable/Loading/Ready/Error`) from feature maturity (`wired/backend-only/planned/blocked`). Every planned control needs stable feature ID, reason and phase. Planned shell navigation can work, while its mutation action must be rejected centrally before Execute or filesystem publication. `Available/Limited` may route only explicitly supported requests. Do not store target fields in fake components/archive extensions.

The complete field lists in Roadmap §§7,10,11 remain the acceptance checklist. The 46-panel/77-form shell is qualified under the revised U contract. Its 570 metadata fields do not imply functional F parity. P04 V2/V3 remain F1/F2 work; shell acceptance does not close them.

## Current executable feature classification

| Route | Features / evidence |
|---|---|
| wired | Platform launcher, native New/Open/snapshot/Save (clean no-op), RenameDocument/Undo/Redo/typed Select/Close, Clone Saved Project, Save All, native Play/Stop; bounded bridge and queue cancel/reconnect; menus/panel lookup/gallery; native ToolWindow Window mode and FileEditor documents. UiTests and Platform status record actual checks. |
| backend-only | P03 authoring, actual Scene/Prefab editing/gizmos, PNG import/reimport/cook/watch/last-good, References/history, Graph editing, runtime Tick/Send and advanced Play/view controls. Preserved C++ Core/session services and semantic regressions; the retired Desktop UI is removed in L0. |
| planned | All 570 future UI metadata fields and F0–F13 actions without completed native contracts; source picker selects only. They do not register runtime types or write archives. |
| blocked / incomplete | Complete F parity/P03-P04 in the new UI, classified historical analyzer findings, clean-machine deployment and full transitive-license review. Owner visual U acceptance and standalone product smoke pass. See QA for current post-L0 test counts. |

Shell reachability is not feature parity. See UI_QA_MATRIX.md for PASS/PARTIAL boundaries and exact artifacts.

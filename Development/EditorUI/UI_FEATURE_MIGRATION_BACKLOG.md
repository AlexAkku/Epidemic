# Feature integration backlog (after U-GATE and L0)

## Current U implementation and remaining integration

The sole target UI is the standalone IntelliJ Platform product. U-GATE is PASS under the owner contract of 2026-11-05. L0 removes the retired Win32/custom Swing shells while preserving backend APIs, engine services, semantic regressions and game data. Old UI compatibility is no longer a prerequisite for F progression. F0 remains a functional integration stage, not a consequence of shell acceptance.

## U0 audit baseline and source map

Sources: `Temp/!Tasks/UI_Roadmap.md` §13.3 and `Temp/!Tasks/U_Phase.md` §13. This is a wiring index, not a competing roadmap. These are functional integration stages; U acceptance alone completes none of them. F0 starts after L0 regression acceptance. Feature IDs refer to UI_FEATURE_MATRIX; current command mappings are in UI_MIGRATION_MAP.

| Phase | Ordered integration and acceptance |
|---|---|
| F0 | Launcher/New/Open/Save/Recent and command availability; single Workspace, folder-wide Undo/Redo, existing Scene/Prefab/Play parity |
| F1 | Project Explorer → existing asset IDs/index/import/cook/watch/leases/history/refs; retain P04 V2/V3 and visible visual/UX acceptance, folder/navigation/filters/drag and security debt |
| F2 | Scene/Prefab/Inspector/gizmos; definition/instance/override/runtime distinction, parameters and Scene transforms; verified P03 visual and Play parity |
| F3 | Visual Graph Canvas, typed ports/variables/palette/debugging; actual Graph↔Scene binding via commands, no fake reference creation |
| F4 | Schema/user components and optional C#/Code Space; type registration without rebuilding Editor UI; pure C++ remains independently buildable |
| F5 | Gameplay components/physics/navigation and playable 2D combat slice; UI fields enabled only after real contracts |
| F6 | Game UI Canvas/widgets/Flow, Graph/C# event bindings and live HUD/menu Play |
| F7 | Sprite/atlas/tilemap, animation/controller, audio/mixer and materials; real animated/audio level |
| F8 | Full Game View/Play controls, runtime Inspector, debugger and profiler; preserve session lifetime and no authoring dirtying |
| F9 | Windows compile/cook/export and independent game package; real errors/progress/cancel |
| F10 | Agent/import automation through existing bounded transactional commands and review workflow |
| F11 | Complete Cats vs Zombies GUI/gameplay migration with actual acceptance |
| F12 | Ascent compatibility migration on backup, not destructive format rewrite |
| F13 | Real 3D runtime/render/physics/editor/export proof, otherwise label release honestly as 2D |

Then Q0 reliability/performance/security, Q1 mandatory independently runnable WebAssembly/Web package, Q2 clean-machine/SDK/install/release gates. No panel screenshot counts as any F/Q backend acceptance.

Known P04 debt must follow F1/F2, not disappear during U: configured mounts, Unicode/collision/TOCTOU policy, rich Explorer UI, true mixed DPI, native dialogs/focus, large-scale assets/security, visible V2/V3 and pilot. Preserve stable IDs, archive readers, committed fixtures, source recovery, Resource leases, last-good and Play generation isolation throughout.

## F0 current batch

**F0 PARTIAL, first batch verified after L0 PASS:** Clone Saved Project, Save All alias,
and native Play/Stop lifecycle through existing C++ APIs. No new panels, runtime
models, asset registries or serialization fields are introduced. Tests cover
identity/source/cooked preservation, no overwrite, independent-process clone,
dirty/history isolation, stale requests and close while playing. Native contracts PASS (8.08 s); Release/restart/IPC/spec PASS (183.82 s);
final-source standalone smoke PASS (78.18 s). These are automated/native checks,
not a new owner manual qualification of every F control.

Remaining F0: complete Scene/Prefab/scalar Inspector command bindings, document
creation/default-scene workflows and continuous game interaction/view routing.
Asset mutation/import/watch/P04 visual debt remains F1; advanced runtime/debug
presentation remains F8. Retired UI compatibility is not a prerequisite.

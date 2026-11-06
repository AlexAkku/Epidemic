# UI Panel Registry — metadata schema v1, U acceptance PASS

## Executable registry and Platform owners

Executable registry: tools/editor-ui-host/resources/epidemic/editor/panels.tsv (38 canonical IDs + 8 on-demand views), fields.tsv (77 schemas/570 UI metadata fields). tests/test_catalog.py verifies all 38 spec IDs/anchors and 325 tabulated fields exactly. PlatformSession maps left navigation to one native ContentManager with two tabs; central documents to FileEditorProvider; right primary/secondary contexts to ToolWindows; output to bottom. Prefab first opens on the right. Window mode, tab movement and detailed docking belong to IntelliJ, without a custom dock owner. Bridge v1 and metadata schema v1 are pinned; native HWND is never a persistent model ID. Owner qualification dated 2026-11-05 accepts whole-group Tool Window movement, central document detachment and saved layout/scaling. UI_QA_MATRIX records the revised contract and authoritative results.

## U0 audit baseline and source map

Sources: `Temp/!Tasks/UI_Roadmap.md` §3.2 (canonical UI Spec v2 supplied under this local name), `Temp/!Tasks/U_Phase.md` U0/U5.

U0 historical status: specification inventory preceded host registration. The current Platform registration and v1 metadata contract are described above; U-GATE is PASS under the owner-revised contract; complete F feature parity remains separate. IDs below are copied verbatim from the supplied specification.

### 3.2. Матрица исходных привязок

| Panel/Document ID | Начальная точка докинга / таб-группа | Показ при старте | Содержимое/контекст |
|---|---|---|---|
| `project.explorer` | `dock.left.navigation`, tab 1 | Да | Project + Assets |
| `scene.hierarchy` | `dock.left.navigation`, tab 2 | Да | Hierarchy выбранной Scene/Prefab |
| `game.view` | `dock.center.documents`, tab 1 | Да | Play либо Empty/Planned |
| `scene.editor` | `dock.center.documents`, tab 2 | Да, активна | Scene 2D/3D |
| `graph.editor` | `dock.center.documents`, tab 3 | Да | Graph Canvas |
| `code.space` | `dock.center.documents`, tab 4 | Да | Scripts / Code |
| `inspector` | `dock.center.right.primary`, tab 1 | Да | Схема выбранного элемента |
| `prefab.editor` | `dock.center.right.primary`, tab 2; пристыкован к правой стенке | Да, в Scene-контексте | Prefab definition, instance, overrides |
| `scene.tools` | `dock.center.right.primary`, tab 3 | В Scene | Gizmos, Scene Settings |
| `scene.systems` | `dock.center.right.secondary`, tab 1 | В Scene | World/Scene Systems |
| `scene.layers` | `dock.center.right.secondary`, tab 2 | В Scene | Layers / Filters |
| `graph.palette` | `dock.center.right.primary`, tab 1 | В Graph | Поиск и создание узлов |
| `graph.inspector` | `dock.center.right.primary`, tab 2 | В Graph | Свойства узла/порта |
| `graph.variables` | `dock.center.right.secondary`, tab 1 | В Graph | Variables / Parameters |
| `graph.references` | `dock.center.right.secondary`, tab 2 | В Graph | Entity/Asset bindings |
| `game.controls` | `dock.center.right.primary`, tab 1 | В Game | Play settings, camera, resolution |
| `game.runtime.inspector` | `dock.center.right.primary`, tab 2 | В Game | Runtime values read-only |
| `game.session` | `dock.center.right.secondary`, tab 1 | В Game | Activations / runtime status |
| `code.symbols` | `dock.center.right.primary`, tab 1 | В Code | Symbols, classes, methods |
| `code.properties` | `dock.center.right.primary`, tab 2 | В Code | Type/schema/compilation |
| `code.references` | `dock.center.right.secondary`, tab 1 | В Code | Usages / Graph calls |
| `ui.palette` | `dock.center.right.primary`, tab 1 | В UI Canvas | Widgets, layout tools |
| `ui.inspector` | `dock.center.right.primary`, tab 2 | В UI Canvas | Selected UI element |
| `animation.properties` | `dock.center.right.primary`, tab 1 | В Animation | Clip/Track/Keyframe |
| `asset.details` | `dock.center.right.primary`, tab 1 | При выборе Asset | Thumbnail/metadata/import recipe |
| `asset.references` | `dock.center.right.secondary`, tab 1 | При выборе Asset | Incoming/outgoing refs |
| `console` | `dock.bottom.global`, tab 1 | Скрыта до открытия | Логи |
| `problems` | `dock.bottom.global`, tab 2 | Скрыта до появления ошибок | Diagnostics |
| `background.jobs` | `dock.bottom.global`, tab 3 | Скрыта до job | Import/compile/progress/cancel |
| `graph.debugger` | `dock.bottom.global`, tab 4 | В Debug/Play | Execution trace |
| `profiler` | `dock.bottom.global`, tab 5 | В Debug | CPU/GPU/memory |
| `animation.timeline` | `dock.bottom.global`, tab 6 | В Animation | Keyframe timeline |
| `build.output` | `dock.bottom.global`, tab 7 | Во время Build | Build output |
| `agent.workspace` | `dock.center.documents`, новый tab | По запросу | Agent UI, Planned |
| `settings.project` | `dock.center.documents`, новый tab | По запросу | Project Settings |
| `settings.editor` | `dock.center.documents`, новый tab | По запросу | Editor Preferences |
| `ui.flow` | `dock.center.documents`, новый tab | По запросу | Screens transitions |
| `type.registry` | `dock.center.right.secondary`, дополнительная вкладка | В Code | Types/operations |

**Пользовательский принцип:** таб-группы являются докируемыми группами, но **каждая вкладка также индивидуально перетаскивается** из группы. Расположение правого Context Dock допускает docking других инструментов с любым split; фабричное размещение воспроизводимо.


## Shared policies

All listed panels are detachable in the target UI. Kind is document for center.documents, tool for all other anchors. Initial size follows the sketch: navigation 22%, center 60%, context right 18%; context primary/secondary 65/35. The values are initial fractions, subject to measured minimum sizes.

Allowed targets: tab merge, reorder, split edge, own native top-level host; context scope follows the table. Resolution: explicit user target > valid saved layout > factory anchor > visible main-host factory group. Never spawn a floating host on first open. Reset restores the factory anchor. Prefab Editor first-open anchor is the right wall inside Scene context.

Bottom global groups start collapsed, except when explicitly opened or required by diagnostics/jobs. Scene is the initially selected center tab. Selection belongs to EditorWorkspace; document focus belongs to each window. Closing a detached host returns surviving tabs to the main host without constructing another workspace. Removed monitors require bounds clamping to a visible work area.

## Additional document IDs explicitly listed in §2.4

These are center.documents, on demand, initially hidden: `prefab.canvas`, `ui.canvas`, `animation.editor`, `sprite.editor`, `tilemap.editor`, `material.editor`, `audio.editor`, `build.editor`. Existing `ui.flow` is already in the canonical matrix. Dynamic instances append a stable AssetId: `scene.editor:<SceneAssetId>`, `graph.editor:<GraphAssetId>`.

## Coverage and unresolved details

The canonical table supplies 38 static panel IDs; §2.4 adds eight on-demand document kinds. It does not supply fixed order for on-demand documents or measured pixel minima. Those remain U5 qualification work; no generated UUIDs or guessed runtime types replace them. Atlas, mixer, font, mesh and controller tools from §10.4 need schema-driven views and explicit stable document-kind decisions in U6. Their omission from §3.2 is a specification detail to resolve in code review before freezing the registry.

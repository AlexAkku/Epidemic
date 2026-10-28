# EditorDemo

A permanent authoring project for the minimal Windows Editor. Use **New demo**
in EpidemicEditorDesktop and choose a working file under Temp (or another project
folder). This imports EditorDemo.epidemic through EditorProject and saves with the
existing archive/Platform path. Tests never save into this source archive. Open
works on a copied .epidemic file without editing or recompiling engine C++.

Arena contains three placements of one Enemy Prefab and its shared
Event -> Damage -> Wait -> Damage graph. Health is bound to the parameter
editor.demo.health (default 100); each placement has independent runtime state.

| Placement ID | Role (Movement.label) | Local X | XY scale | Health | Speed |
| --- | --- | --- | --- | --- | --- |
| 42 | Scout | -6 | 1 | 120 (parameter override) | 2.5 |
| 43 | Guard | 0 | 1.3 | 75 (property override) | 4 |
| 44 | Veteran | 6 | 1.7 | 150 (property override) | 1 |

Scene View remains an authoring view during Play; runtime values are inspected
with Refresh values and activation diagnostics. It uses transformed unit-box
placeholders. Viewport/tree labels show the
Prefab name and stable ID (viewport uses base 36: 16/17/18); role strings appear
in Inspector. SceneSnapshot has no persisted object-name field. No bitmap assets
are included: this authoring path has no Sprite bindings yet. Prefab ownership
children have no spatial transforms and are not rendered as spatial descendants.

## Manual end-to-end

1. New demo -> Temp/M7/Manual.epidemic. Select Arena and Fit scene: three distinct
   boxes must be visible. Select Scout by viewport and Guard by tree; both paths
   must update the same selection/Inspector.
2. Choose Scout, Move / World XY, move snap 1. Drag X from -6 to -5. Inspector
   must show -5; one Undo restores -6 and Redo restores -5.
3. Choose Edit Instance Override, set Health.points=110 and Movement.label to
   Edited Scout. Guard/Veteran retain 75/150 and their labels. Undo/Redo must
   affect only the selected transaction. A property reset would reveal Scout's
   parameter override 120; reset the parameter row separately to inherit 100.
4. Try Health.points=wrong or -1. Apply must show an error and keep the previous
   authoring state; Revert clears the draft. Missing references/type mismatches
   are also covered by API tests; the asset picker offers existing documents.
5. Select Veteran, Delete, Undo: placement ID 44, its overrides and geometry must
   return. Select Arena's Document property and rename to Arena edited.
6. Save, close the editor and reopen the working file in a new process. Check
   IDs 42/43/44, Scout X=-5, Health 110/75/150 and the edited label.
7. Select Scout, Play, payload 1, Send hit. Refresh values after about one second:
   runtime Health is 108, Guard/Veteran remain 75/150; authoring Scout stays 110.
   Diagnostics must show the completed activation. Payload 999 must produce
   example.damage_range with graph context, without dirtying authoring.
8. Stop, Play again: runtime Health returns to 110/75/150. Stop before closing.
   Check resize, 100/150% DPI, focus, pan/zoom and native text Undo manually.

## Regression and reproduction

Headless --edit-demo <Temp/file.epidemic> imports this committed archive and
performs the fixed edit scenario above; --read-demo reads that result in a new
process and verifies identity, independent components, Wait/resume, failure
reporting and cancellation. Native --demo-smoke drives the editor's own hidden
controls/viewport, saves a separate Temp file, then CTest reopens it with the same
headless checks. This is not a visible desktop/UX qualification.

The developer-only --create-demo <new-file.epidemic> recipe reproduces the initial
baseline through Command API and rejects an existing destination. Regeneration is
explicit; builds/tests do not replace this source asset. New demo resolves this
file through the dev build's source path; packaging a relocatable editor is outside
this stage. Archive schema remains 1.0.0 and the host uses its existing fixed schema.

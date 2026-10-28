# Editor core and minimal desktop host

`EditorProject` is the authoring and automation boundary. `Open`, `Save`,
`Execute(EditRequest)`, `Undo`, `Redo`, `Snapshot`, and `Play` are the same C++ API
for future UI, tests, and automation. No mutable authoring handles are exposed.
Schemas are frozen and borrowed: their EntityService metadata and ComponentRegistry
must outlive projects and Play sessions. Calls are externally synchronized on one thread.

A request publishes one validated transaction. `expected_revision` rejects stale
clients (zero opts out). Undo/redo restores immutable snapshots, including IDs and
references; revisions remain monotonic within a session. History retains at most 64 transactions.
Document IDs and Scene/Prefab node and Graph node/port/connection IDs remain
reserved across history branches and disk reloads. Prefab parameter IDs are
scoped to their Prefab, and GraphBehavior IDs to their Entity or Component
behavior list; these IDs are currently reusable after undo or deletion. They
are not covered by the retired-ID guarantee. A save point
controls project and per-document dirty state; history is session-local.
Create/Execute preflight the actual ProjectArchive encoder before publishing.
All persisted authoring doubles must be finite (including parameters/overrides);
runtime component value rules are unchanged. Archive size/value-count limits also
apply before publication. Preflight moves the candidate, without another full
project copy; its temporary wire buffer is discarded.

Documents compose existing SceneSnapshot, PrefabDefinition and GraphDefinition.
Assets owns the sealed catalog. Serialization owns the versioned archive encoding;
Platform owns bounded, atomic disk publication. The project archive schema is 1.0.0.
Scene world transforms/bounds and runtime revisions are derived on instantiation.
Structurally valid incomplete graphs may be edited/saved. Play eagerly compiles
the entry Scene's transitive dependency manifest from Assets. Unused drafts stay
registered without IR compilation; all Prefab definitions remain available for
dynamic Spawn, and their graphs compile when bound. Used invalid graphs fail
Play/Attach/Spawn. Binding factories are still called for all Graph definitions;
schema/reference checks apply to the whole project. Thrown binding-factory
errors and mismatched declared operations still block Play; other IR/callable
completeness checks occur when a deferred graph is bound. Structurally invalid
documents remain rejected.

Play creates fresh EntityService, components, SceneServices, RuntimeBridge, Time,
Simulation and GraphScheduler instances. Callables are bound to that session's
`PlayContext`. Authoring edits affect subsequent sessions. Runtime pointers are
borrowed until Stop; Stop invalidates scopes and cancels active graphs.
Cancellation covers Entity/Part, Scene and World system behaviors, including queued
work. The host cancels and finalizes all activations before Simulation teardown;
Stop requires no external Tick and invokes no native graph effects.
Send/Tick/Stop reject reentry during a Tick or shutdown/lifetime callback.

`Tick` reports Simulation infrastructure failures. Game results are independent:
after `Send`, use `Activations()` and keep its read-only `PlayActivation` handles.
`Report()` is empty until the first execution report; `IsFinished()` identifies
Completed/Failed/Cancelled. Original error/asset/node/instruction diagnostics are
copied by value. Reports update after Send/Tick/Stop (Context-started work is first
captured at Tick/Stop). The session retains all live handles (host cap: 1024) and
the last 64 completed handles, ordered by completion for eviction. IDs increase
within a session; the query returns IDs in ascending order. Caller-owned handles
survive eviction, Stop and session destruction and retain no VM/native context.

Run `EpidemicEditorHeadless --write <path>` followed in a new process by
`EpidemicEditorHeadless --read-play <path>`. The example creates Entity/Health and
GraphBehavior through commands, connects ports, undoes/redoes, saves, reloads,
executes an event with Wait/resume, and stops without dirtying authoring data.

## Windows desktop

Scene/Prefab authoring uses the existing `SetSceneTransform`, `SetSceneParent`
and `PlacePrefab` commands. Archive 1.0.0 already stores local position/rotation/
scale, spatial parents and placement overrides; SceneServices derives world
transforms at instantiation. Prefab children express ownership, not spatial
parenting. Placement overrides replace only the selected instance's assignments;
an editor changing one assignment must preserve the others. Human-readable object
names are not persisted by SceneSnapshot/PrefabNode; display labels are derived.

The 2D viewport is a child surface of the Platform-owned editor
window, driven by its existing event pump and invalidation. Presentation's Canvas
currently requires its own Host and cannot simply be attached to this HWND.
The selected integration point is a child HWND plus the existing
`EpidemicRuntimePresentation` Renderer, whose public constructor accepts a native
window. The Game/Editor surface adapter owns resize/paint/input/lifetime and
leaves the Platform event pump in charge, without a second renderer, Scene or
Framework API change. The M3 native smoke exercises
embedded drawing/resize at 100/150% DPI; actual monitor-DPI checks are separate.

Build/run `EpidemicEditorDesktop [project.epidemic]` on Windows (full profile).
The portable `EpidemicEditorUi` workspace owns the Command API client, stable-ID
selection and PlaySession. The desktop uses the existing Platform window and
standard Windows controls; Core, Runtime and Framework have no UI dependency.
The executable shares the headless example's frozen game schema and native bindings:
Enemy, Health, Movement and an event-driven Damage/Wait graph. Other game schemas
must supply their registrations and operations in the host; unknown types are rejected.

Example chooses a project path; Open loads the existing Serialization archive.
The document tree opens Scene/Prefab/Graph authoring data. Add/Delete and typed
properties use atomic commands. Scene placements can assign a Prefab; Prefab
entities expose registered properties and can assign a graph's first event input
to the example `hit` event. Graphs use node/port/connection lists, with Event entry,
Native damage and Wait templates. Core validates connections and references.
Node/port labels include stable IDs to distinguish repeated templates; connection
rows and properties show their endpoints. Drop-down lists expand for long labels.
Hierarchy labels use complete base-36 IDs (no truncation collisions); properties
retain full decimal IDs. Scene parents form the spatial tree; Prefab children
remain ownership relationships. Components have their own selection using the
document/node/type/part key and a filtered Inspector. Delete falls back to its
owner or current document; Undo restores identities without resurrecting stale
selection. Open/reload resets selection. A placement's Open Prefab definition
action resolves its current definition through the same workspace selection API.
Creation selects the new item; refresh preserves collapsed branches. The header
shows document/selection context, while Project, history and Play actions are
grouped separately. Actions for other document kinds are hidden. Native message
fonts and layout scale with window DPI; native smoke covers 100/150% layout and
font metrics, separately from an actual monitor-DPI GUI check.
The drafting layout places hierarchy left, the Scene viewport in the center,
and a property/value Inspector right. Scene object operations live in the
Inspector **Actions** tab; **Properties** retains the explicit override mode.
Apply or Revert an unfinished value before switching tabs or selection.
Selected-property metadata is scrollable and copyable below the value editor.
Dark panels use the system message font (11 pt body); Windows high-contrast
colors override the palette. ComboBox rows/closed faces and tabs use dark owner drawing;
keyboard commands and native control semantics remain. DWM caption colors apply on
supported Windows versions; system dialogs and scrollbar chrome follow Windows.
Parameter defaults are editable; bound properties show the parameter link and
can be detached. Scene component/parameter fields use explicit View Definition
or Edit Instance Override modes; reassigning the same Prefab preserves overrides.

Spatial fields edit **local** Scene TRS: XYZ position/scale in Scene units and
Z rotation in degrees for XY-plane orientations. Angles wrap modulo 360; tilted
3D quaternions remain read-only. Finite float range, non-zero scale and existing
Scene validity rules apply before publication; negative positions/scales are
supported. Spatial parent selection commits `SetSceneParent` with **KeepLocal**
semantics, so world placement changes; missing/cyclic parents are rejected
atomically. World transforms belong to SceneServices, including its documented
simplified-TRS approximation for shear-producing hierarchies. Inspector fields
remain local TRS. These fields use archive 1.0.0 unchanged;
the headless write/read-play example verifies parented TRS in a new process.

Scene documents show an XY **authoring** viewport hosted in a child HWND using
the existing Runtime Presentation renderer. Runtime SceneServices derives world
TRS/bounds once per changed immutable snapshot; the viewport retains only IDs,
labels, XY bounds and a transient camera. It does not tick, serialize or rebuild
the Scene on idle paints. During Play it still displays authoring, not simulation.
Visible nodes draw world AABBs, or transformed unit-box placeholders without
bounds. Sprite bindings are not yet present in these authoring documents; missing
or nonresident visual assets never block placeholders. Hidden nodes remain in
the hierarchy but are not drawn or picked. Labels include full base-36 IDs.
Greater SceneNodeId draws last and wins overlapping picks, independently of
selection. Thin/subpixel bounds use the same minimum 14-DIP box for display/picking.
Left click selects through the shared workspace selection; an unapplied field
blocks selection changes. Middle drag or left drag on empty space pans; wheel
zooms around the cursor. Frame selection (`F` with viewport focus) and Fit scene
(`Home`) restore the camera. Navigation never changes dirty state or Undo history.
The camera is session-local and resets on Open/document change. Native paint and
resize messages drive the surface; there is no second event loop or idle timer.

The viewport tool selector offers Move, Rotate Z and Scale XY. Move supports
World XY or Local XY axes; object-body/center drag is free World XY translation.
Rotate uses the ring around the Scene node pivot, changing local Z rotation;
Scale uses local X/Y endpoints, or the center square for proportional XY scale
(drag up/right to increase). Z position/scale remain unchanged. All ancestors
must have XY-plane orientations for gizmos; tilted 3D hierarchies remain editable
through the existing Inspector. World/local conversion uses Runtime Scene's
existing representability checks, including its simplified TRS contract.

One gesture owns one transient Runtime Scene, an immutable source reference and
the source revision/selection. Mouse movement updates bounded drawing geometry,
including descendants, without EditRequest or archive encoding. Inspector values
remain authoring until release. A release beyond the 3-DIP threshold publishes
one `SetSceneTransform`; a click/no-op publishes nothing. Esc, right click,
capture/focus loss, resize/DPI changes, camera/tool changes and project actions
cancel preview without changing authoring/history. Stale revision/selection,
nonfinite transforms, zero scale or nonfinite derived bounds reject the gesture;
the last valid preview is never silently committed after an invalid endpoint.
Negative final scales remain supported by the existing Scene contract.

Move snap is a finite nonnegative Scene-unit value, applied with Set snap or
Enter; zero disables it. Free/World movement snaps absolute world coordinates;
Local axis movement snaps distance along that axis. Rotation/scale are not
snapped. With viewport focus, arrows move World XY by the snap step or 0.1 units
(Shift multiplies by 10); key repeats/chords form one transaction until all arrow
keys are released. Focus loss/Esc cancels that transaction. Text edits retain
native text Undo. Changes during Play affect authoring and the next Play only.

Apply (Enter) publishes the current field; Revert (Escape) discards its draft.
Invalid input leaves source, history and save points unchanged. Apply/Revert is
required before changing selection. Save applies a valid field first; close and
project replacement offer Save/Discard/Cancel. Ctrl+S/O/N and F5/Shift+F5 operate
on the workspace. Ctrl+Z/Y control project history outside value/payload edits;
inside those edits native text undo is retained. Workspace shortcuts
also work when the main window itself has focus; Alt+F4 uses normal close handling.
Tab/Shift+Tab enter the native control traversal even from initial host focus.
Play uses the project's entry Scene snapshot (there is no entry-Scene selector
in this host); authoring changes become visible on the next Play. Select
a Scene placement and Send hit to run the example with the supplied i64 payload
(also its wait duration). The host advances one game tick per 33 ms UI timer;
this is a development preview, not a real-time scheduling policy. Activation
status and original native error/asset/node/instruction details stay visible after Stop.
Instruction/native-call counters describe the last execution step, including after
Wait resumes. Command errors are labelled as the last failed command.
Outside Play the host waits for Windows messages without polling the workspace;
the 33 ms game timer runs only during Play and is removed on Stop/close.
The window supports Windows DPI scaling and a minimum size of 1000 x 800 logical pixels.

`EpidemicEditorDesktop --ui-smoke <Temp path>` runs real native controls hidden,
including property rejection, Undo/Redo, typed connections, disk reload and Wait
cancellation. `EpidemicEditorWorkspaceTests` checks the portable command client.
Close owned preview windows before rebuilding their executable. If a user session
must stay open, use another normal build directory (for example
`Temp/b/ui-validation`); no executable renaming is required.

### Schema Inspector

Inspector fields come from the frozen ComponentRegistry descriptors: i64/double
use checked numeric entry, bool uses a true/false selector, and string uses native
single-line text editing. Apply (Enter) publishes one command; Revert (Escape)
discards the draft. Native text Undo remains local until publication. Invalid
numbers, non-finite doubles and component validators reject publication; a pending
field blocks selection changes and other authoring commands until Apply/Revert.

The component template picker supports Add on a Prefab Entity, Replace on a
selected Component, and Delete through the existing hierarchy action. Replace
atomically deletes the old slot and installs template defaults in the same part;
it does not migrate old properties/behaviors. Occupied target slots are rejected.
Undo restores the entire old component. Templates supply authoring defaults in
the host; metadata/validation remain in ComponentRegistry. The example registers
Health, Movement and Tuning (count/weight/active/title). Other frozen host schemas
use the same Inspector without UI-specific field code; user DLL loading is outside
this stage. A missing authored property shows its factory default as unavailable:
Inspector never instantiates a runtime component to discover defaults.

Prefab definition values are editable unless parameter-bound. Scene placements
show effective definition/parameter/override values, editable in Instance Override
mode. Runtime remains read-only. IDs, graph routes and GraphBehavior references are
read-only fields; existing Graph/Prefab pickers assign valid project references.
GraphBehavior may belong to an Entity or a Component; the example assignment maps
hit to the selected Graph's first event input. Open reference navigates existing
project documents, using their asset IDs rather than a separate catalog.

During Play, Scene Inspector can also show detached read-only runtime component
values, clearly marked runtime snapshot. Refresh values explicitly rereads them;
Tick does not rebuild Inspector fields. Authoring edits affect the next Play.
PlaySession::ReadComponentProperty returns an owned value and fails for inactive,
busy, missing or destroyed runtime objects; it exposes no mutable service/handle.
Single-line editing preserves Unicode/whitespace. Existing multiline/NUL strings
are escaped and read-only; invalid UTF-8 is read-only in the Windows host.

### Prefab definition and instance editing

Prefab documents expose the existing ownership tree, components, GraphBehavior
and parameter rows. Prefab nodes carry no spatial transforms or render geometry;
there is no spatial Prefab preview, and ownership never becomes Scene parenting.
The Scene viewport continues to show placement geometry/placeholders.

Expose parameter publishes SetPrefabParameter + SetProperty as one transaction.
The generated parameter ID is shown on the bound field and its default row.
Defaults retain their original value kind; all bound-property validators and
archive constraints apply before publication. Detach binding keeps the current
definition value as a literal. It retains the parameter itself, including other
bindings and instance assignments; it does not delete authoring data implicitly.

Scene's View Definition mode keeps component values read-only; Edit Prefab
Definition navigates by asset ID. Edit Instance Override writes only the selected
placement's component or parameter assignment, using SetComponentOverride or
SetParameterOverride. These commands patch the current placement and reject a
mismatched expected Prefab ID, preserving unrelated component/parameter/behavior
overrides. Reset selected override removes just that assignment in one Undo step.
Effective priority is component override, then bound parameter override, then
definition default. Resetting a component assignment can therefore reveal an
existing parameter override; reset its parameter row separately to inherit the
definition default. A mode change cannot discard an unapplied draft.

Definition edits affect subsequent Play sessions and effective authoring views;
running sessions keep their independent snapshot. Rename, Scene reparenting and
project-file relocation preserve asset/node/property IDs. No Apply All Overrides,
Prefab inheritance, nested asset references or live runtime propagation is added.
The archive remains 1.0.0. Headless --write-overrides / --read-overrides exercise
two placements, parameter binding, independent Health/Movement and event/Wait in
separate processes. Temporary projects remain under Temp/editor-tests.

### EditorDemo

The permanent source project is example/EditorDemo/EditorDemo.epidemic; its README
documents stable IDs, role/override values and the manual end-to-end scenario.
New demo imports that archive into a user-chosen working file and immediately
saves it through the existing Command/Persistence path. Selecting the shipped
source as the destination is rejected. Other authoring files remain editable;
the dev build uses its source path to locate the template.

The demo uses three spatially separated, differently scaled unit-box placeholders
for a shared Enemy definition. Role strings live in Movement.label; viewport/tree
labels use Prefab name and stable ID, matching SceneSnapshot's existing contract.
There are no new Sprite bindings, asset formats, runtime services or UI frameworks.
Headless --edit-demo / --read-demo and native --demo-smoke operate on Temp copies
of this same archive. The native result is also reloaded by a new headless process.
Hidden native smoke verifies the owned controls and viewport; real desktop UX/DPI
and visible end-to-end acceptance remain manual when computer use is excluded.

### Load and lifecycle qualification

EpidemicEditorStressTests creates a 200-placement working project from the same
EditorDemo definition. It checks branching Undo/Redo, the 64-transaction history
cap, save points, stale IDs/revisions, atomic invalid edits, graph/reference
rejections and repeated Play/Stop. A counted native operation proves that Stop,
repeated Stop and later Tick invoke no additional graph effects.

The Windows --stress-smoke <Temp/project.epidemic> path checks the owned hidden
window, hierarchy, render/picking/drag, long Unicode names, native text Undo,
100/150% layout, zero-size restoration and closing during a waiting activation.
It measures only its own process CPU, private/working-set bytes and resource
counts. UI refresh exceptions restore notification/redraw state and expose the
exception reason; unsupported metadata text can be recovered with Undo/Open,
without silently changing persisted bytes or imposing a new Core string policy.

--large on the portable test, and Windows --stress-diagnostic, use 1000 placements
as separate diagnostics, not a supported industrial capacity/performance claim.
Measurements are observations without hardware-dependent pass thresholds. Drag
preview timing excludes rendering, which is measured separately. Idle samples
are short, hidden-process samples and include possible driver/OS transients;
visible desktop CPU, readability, actual monitor DPI, focus and native dialogs
still require manual checks. Copy/encode/preflight, commit, history, Save/Open
and Play/Stop are timed separately; no alternate history/serialization path is
introduced. Full archive preflight remains part of each published edit and can
dominate large-project Debug latency. Current results/logs belong only in Temp.

There is no duplicate command or custom hierarchy context menu. Native edit
controls retain their standard context menus; Save/Discard/Cancel dialogs and
visible UX are not claimed as tested by the hidden smoke.

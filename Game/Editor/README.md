# Editor core and minimal desktop host

`EditorProject` is the authoring and automation boundary. `Open`, `Save`,
`Execute(EditRequest)`, `Undo`, `Redo`, `Snapshot`, and `Play` are the same C++ API
for future UI, tests, and automation. No mutable authoring handles are exposed.
Schemas are frozen and borrowed: their EntityService metadata and ComponentRegistry
must outlive projects and Play sessions. Calls are externally synchronized on one thread.

A request publishes one validated transaction. `expected_revision` rejects stale
clients (zero opts out). Undo/redo restores immutable snapshots, including IDs and
references; revisions remain monotonic within a session. History retains at most 64 transactions.
Deleted IDs remain reserved across history branches and disk reloads. A save point
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

The future 2D viewport must be a child surface of the Platform-owned editor
window, driven by its existing event pump and invalidation. Presentation's Canvas
currently requires its own Host and cannot simply be attached to this HWND.
The selected integration point is a child HWND plus the existing
`EpidemicRuntimePresentation` Renderer, whose public constructor accepts a native
window. A Game/Editor surface adapter must own resize/paint/input/lifetime and
leave the Platform event pump in charge. Embedded-surface behavior still needs
its M3 spike; no second renderer, scene or Framework API change is required.
No viewport or gizmo is supplied by the document-navigation milestone.

Build/run `EpidemicEditorDesktop [project.epidemic]` on Windows (full profile).
The portable `EpidemicEditorUi` workspace owns the Command API client, stable-ID
selection and PlaySession. The desktop uses the existing Platform window and
standard Windows controls; Core, Runtime and Framework have no UI dependency.
The executable shares the headless example's frozen game schema and native bindings:
Enemy, Health, Movement and an event-driven Damage/Wait graph. Other game schemas
must supply their registrations and operations in the host; unknown types are rejected.

New example chooses a project path; Open loads the existing Serialization archive.
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
Parameters and parameter-bound properties are read-only. Prefab overrides have
no editing UI; reassigning the same Prefab preserves its existing overrides.

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

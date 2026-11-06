# Editor Core and native session backend

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

## Editor UI and native session adapters

The only target Editor UI is the standalone IntelliJ Platform product in
`tools/editor-ui-host`; build/run instructions are in its README. The retired
Win32 desktop executable and its controls/dialogs are removed by L0 under the
owner contract of 2026-11-05. Existing project/archive readers and engine
functionality remain supported independently of that UI removal.

`EpidemicEditorUi` is a reusable static session/projection library, not another
window host. It retains EditorWorkspace, stable-ID selection, SceneView/SceneDrag
and schema Inspector builders. The new native companion uses the same Core,
ProjectFolder, commands/history and isolated PlaySession. Core, EngineRuntime and
GameFramework have no Java/IntelliJ dependency.

Scene authoring uses existing SetSceneTransform, SetSceneParent and PlacePrefab
commands. Spatial parents and ownership parents remain distinct. Typed Inspector
commands validate scalar values and component recipes against the frozen registry;
definition values, instance overrides, parameter bindings and runtime values are
separate. These APIs survive L0; controls are enabled only as their F-phase
bindings and regressions pass in the new UI. Shell acceptance does not establish
complete Scene/Prefab/Graph/asset editing parity.

The headless demo and tests remain available. `EpidemicEditorHeadless --edit-demo`
and `--read-demo` exercise cross-process authoring/Play on a copied demo archive.
The supplied Enemy/Health/Movement graph schema and native operations remain in
`example/demo_game.h`; arbitrary game schemas must supply their real registrations.

## Regression coverage

Workspace/Inspector/SceneView tests retain command, selection, transform, override,
validator and Play isolation coverage. EpidemicEditorStressTests retains the
200-placement scenario, history/savepoints/stale IDs, graph rejection and repeated
Play/Stop; its optional --large scenario remains a diagnostic, not a performance
budget. Project/folder/content/async PNG/hot reload/thumbnail/media and separate
process persistence tests remain unchanged.

Retired Win32 control/window smokes are replaced by the standalone Platform smoke.
EpidemicEditorPreviewResources separately preserves the old Content Browser's
prepared PNG resource lease → GPU upload/draw/resize → release/reacquire checks
using a hidden test surface, with no browser/editor window implementation.

### Portable project folders / PNG preview

ProjectFolder provides NewFolder/OpenFolder/CloneTo, committed revisions, stable
IDs and portable source/cooked identities. Native command clients retain PNG
copy/import/reimport, rename/move, staged delete/restore, references, queued jobs,
unified Undo/Redo, resource leases and thumbnails. These are backend APIs for
Project Explorer/F0–F1; a standalone Content Browser UI is not retained.
See [folder contract](PROJECT_FOLDER.md), [PNG/query limits](CONTENT_BROWSER_AND_PNG.md)
and [pipeline contract](ASSET_PIPELINE.md).

Async reimport publication arrives through Workspace PollContent. Existing Play
keeps its own generation while authoring resources update. Stop preserves the
watch preference. Watch services are independent of a browser; Cancel/Reset do
not wait, while final native service destruction drains its worker.

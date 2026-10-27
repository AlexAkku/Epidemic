# Headless editor core

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
schema/reference checks apply to the whole project. This does not defer missing
native factory/configuration errors or permit structurally invalid documents.

Play creates fresh EntityService, components, SceneServices, RuntimeBridge, Time,
Simulation and GraphScheduler instances. Callables are bound to that session's
`PlayContext`. Authoring edits affect subsequent sessions. Runtime pointers are
borrowed until Stop; Stop invalidates scopes and cancels active graphs.
Cancellation covers Entity/Part, Scene and World system behaviors, including queued
work. The host cancels and finalizes all activations before Simulation teardown;
Stop requires no external Tick and invokes no native graph effects.

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

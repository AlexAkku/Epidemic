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

Documents compose existing SceneSnapshot, PrefabDefinition and GraphDefinition.
Assets owns the sealed catalog. Serialization owns the versioned archive encoding;
Platform owns bounded, atomic disk publication. The project archive schema is 1.0.0.
Scene world transforms/bounds and runtime revisions are derived on instantiation.
Incomplete graphs may be edited/saved; Play requires successful Runtime compilation.

Play creates fresh EntityService, components, SceneServices, RuntimeBridge, Time,
Simulation and GraphScheduler instances. Callables are bound to that session's
`PlayContext`. Authoring edits affect subsequent sessions. Runtime pointers are
borrowed until Stop; Stop invalidates scopes and cancels active graphs.

Run `EpidemicEditorHeadless --write <path>` followed in a new process by
`EpidemicEditorHeadless --read-play <path>`. The example creates Entity/Health and
GraphBehavior through commands, connects ports, undoes/redoes, saves, reloads,
executes an event with Wait/resume, and stops without dirtying authoring data.

# Game composition

EntityService remains the sole Entity/Parts/lifecycle owner. GraphScope is a
capability for a generation-checked EntityHandle and optional existing Part;
it neither allocates Entity identities nor restores or duplicates Entity state.
PendingDestroy, removal, slot reuse and explicit invalidation expire the scope.
Scopes and EntityService are externally synchronized on the gameplay thread;
EntityService outlives its scopes.

Framework exposes object context and activation lifetime interfaces without
including Runtime Graph, Schema or Support headers. Game/Composition is the
composition root above Framework and Runtime: GraphExecutionHost binds native
operations to that context and delegates compilation/execution/waits to Runtime.
This follows the existing layer direction; Architecture Freeze allowlists remain
unchanged. No compiler, VM or scheduler is implemented in Framework.

Compile creates a BoundGraph for one scope. Start derives its owner from that
binding, preventing execution with another Entity's context. Native invocation
state remains per activation. OperationContext exposes the existing RuntimeBridge
service; runtime representations continue to use its normal projection lifecycle.
The bridge, host scheduler and domain services outlive the host and its programs.

Before the existing Simulation Tick, call GraphExecutionHost::Prepare. This is
the lifecycle barrier for Entity changes made outside graph code, and delegates
timer preparation to GraphScheduler. Domain calls validate their scope before
execution and synchronize it on return/unwind. Destruction inside a native call
therefore cancels all owned activations before the VM can run their suffixes.
Cancellation does not roll back an already accepted domain operation.

An authoring GraphDefinition may be shared, while its BoundGraph carries one
object's context. Framework does not persist IR or active VM continuations.

ComponentRegistry extends the Framework StableTypeRegistry with typed member
bindings (i64, double, bool, string) and validation. Freeze the registry before
attaching components. ComponentStore attaches fresh user objects to existing
EntityHandles/Parts; it allocates component generations, never Entity identities.
Factories must return uniquely owned state. Definitions contain property values,
not live component objects. Get returns a borrowed const pointer; it expires on
removal/lifecycle reconciliation. Remove immediately invalidates the component
scope, and Synchronize releases components whose Entity or Part expired.

GraphBehaviorService binds registered GraphDefinitions through IGraphBehaviorHost.
Each behavior has a child lifetime scope: detaching it cancels its activations,
without disabling its Entity or sibling components. Removing a component also
invalidates child behaviors. Event routes and input values are validated on bind;
the service snapshots definitions and assigns generation-checked behavior handles.
Send targets one behavior; SendTo explicitly targets an Entity/Part in stable
attachment order. There is no implicit bus or polling of unrelated Facts. Runtime
continues to own every compiled program, activation, budget and continuation.

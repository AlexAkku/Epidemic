# Graph execution — i64 profile

`CompileGraph` links a GraphDefinition to explicit node bindings and emits immutable
process-local register IR. The first profile supports i64 Input, checked Add and
Multiply, unary NativeCall, Wait and Output. It requires exact node shapes, connected
data inputs and a data DAG. Control/event ports and other scalar profiles fail
explicitly; no execution semantics are guessed for unsupported graphs. Ordering
is topological, with stable node IDs breaking ties, including native effects.

One `shared_ptr<const CompiledGraph>` serves many one-shot GraphInstances. Each
instance owns registers, cursor, terminal failure and native invocation state.
SetInput addresses an Input node's output port before Run seals inputs. ReadValue
addresses node output ports or an Output node's input port. Compiled snapshots
retain authoring asset/node IDs and are independent of later definition edits.

Run budgets bound attempted instructions and native invocations per call. A budget
yield preserves the next unexecuted instruction; resuming never replays a prefix.
Completed/failed/cancelled instances are terminal. Create a fresh instance for a new run.
Diagnostics retain asset, node, IR index and original native error code. Callback
exceptions and factory failures are contained; reentrant Run is rejected. Native
failure can leave its own state changed and the completed prefix intact: there is
no promise to roll back arbitrary external effects. Budgets are cooperative and
cannot preempt one synchronous native call or state factory by elapsed time.
Native operations remain responsible for their own domain/range validation.

Graph depends only on RuntimeFoundation. Support's CreateSchemaGraphCallable
checks the existing native Schema signature without execution and copies the
initial Owner per call site per instance; registry lifetime is shared safely.
Owner copy semantics must isolate mutable data. External shared objects and all
concurrent access remain caller responsibilities. Binding catalogs and native
pointers are runtime link inputs, not a new registry or serialized ABI.

GraphDefinition save/load remains the existing Serialization path. IR, instance
state and native bindings are not persisted. Registers/call sites are bounded by
the existing graph limits; local allocations retain ordinary C++ semantics.
Discard an instance if a local allocation exception escapes Run; that is not a resumable outcome.

## Scheduler/Wait 2.5

Wait has two i64 inputs (value, non-negative game ticks) and one value output.
It suspends even for zero ticks. Run cannot skip a pending Wait; ResumeWait is
the host's explicit acknowledgement that it is due, and then publishes its value.
Cancel seals inputs, stops a suspended invocation or the suffix after an active
native callback, and never replays/rolls back the accepted prefix. A synchronous
callback itself cannot be preempted. A completed/failed invocation keeps its result.

Runtime Support's GraphScheduler submits GraphTasks to the existing Simulation
scheduler on its MainThread lane in a caller-supplied valid Simulation zone.
It owns no clock, loop, worker pool or timer queue.
The host advances Runtime Time, calls GraphScheduler::Prepare, checks its result,
then calls its existing Simulation::Tick. Prepare arms continuations through
IScheduledSimulationTasks; Simulation does not invoke waiting jobs before they
are due. Prepare inspects job metadata without running VM code.
A timer-publication failure keeps the request for retry without replaying effects.
Tick without the required Prepare fails the task rather than running an unarmed
continuation. Dependencies must use the same authoritative Time clock/tick domain;
Runtime Support's existing Simulation clock adapter provides that composition.
Submit transfers a fresh instance with its inputs configured; already started
instances are rejected, including those previously yielded with a zero budget.

Each task has per-step instruction/native budgets, further capped by Simulation's
work budget. Effects follow compiled order; independently submitted tasks follow
Simulation ordering. Tasks and the facade have stable identities and require
external synchronization. Graph failures are retained in GraphTask::LastReport
and its Simulation job becomes Failed; spent instructions still consume the
Simulation work budget. Simulation Tick failures describe host scheduler errors.
Cancel through GraphScheduler to remove Simulation's timer and pending job
together. Self-cancellation from a native publishes through
the active step. Destroying the facade does not cancel jobs owned by Simulation;
cancel active tasks explicitly before tearing down their composition.

A zero fixed_step resumes on the next host Tick. A positive interval anchors a
fixed game-tick grid at submission: budget yields advance one step; waits round
up to a grid boundary, at least the next step. Late host frames retain the logical
phase and catch up at most one bounded step per host Tick, without dropping debt
or advancing the authority. This schedules a one-shot graph's continuations;
periodic behavior systems and physics stepping remain their existing owners.
Checked deadlines fail with an authoring-node diagnostic on overflow. Time rewind
requires cancelling/rebinding the session; Prepare rejects a detected rewind.

Application composition uses Framework's existing RuntimeTimeAdapter to synchronize
GameplayTime and maps GameplayDuration to Runtime GameDuration in the same game
ticks. It then prepares GraphScheduler and ticks the existing Simulation runtime.
Pause and scale come from Runtime Time; synchronization never advances either clock.
Synchronize only on a new nonzero Runtime revision; the existing adapter rejects
zero/stale revisions. Repeated Prepare needs no additional synchronization when
the authority has not changed.
Framework does not acquire a Graph/Support dependency. Existing Time, Simulation
and Framework signatures are unchanged.

Control/event execution, persisted suspended continuations, Editor UI, CLR and
code generation remain outside this profile.

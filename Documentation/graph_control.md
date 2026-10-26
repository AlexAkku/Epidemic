# Structured i64 graph execution

This extends CompiledGraph/GraphInstance without changing the linear data DAG
profile. Entry/Sequence/Branch select the control profile; unused control bindings
in a catalog do not change an existing data graph. IR and authoring serialization
remain separate. Graph still depends only on RuntimeFoundation.

Port order within each kind is significant. Data ports use the existing i64 type;
control ports have no payload. Shapes (data in/out, control in/out):

| Operation | Data | Control | Meaning |
|---|---|---|---|
| Input | 0/1 | 0/0 | Activation input, configured before Run |
| Add / Multiply | 2/1 | 0/0 | Checked pure expression |
| Entry | 0/0 | 0/1 | Immediate one-shot entry |
| EventEntry | 0/1 | 0/1 | External i64 event input plus immutable payload output |
| Sequence | 0/0 | 1/N | N positive; execute each output subtree in port order |
| Branch | 1/0 | 1/2 | Nonzero selects first output; zero selects second |
| NativeCall | 1/1 | 1/1 | Ordered action bound to an explicit native factory |
| Output | 1/0 | 1/0 | Publish an output sink when reached |
| Repeat | 1/1 | 1/2 | Captured count, current index; body then done |

An unconnected control output ends that subtree; a control output has at most
one connection. Use Sequence for explicit ordering. Raw control/data cycles,
bad shapes, disconnected inputs and unreachable actions are rejected. Merged
paths execute whenever reached; this does not imply synchronization or parallelism.

Pure data dependencies are evaluated on demand with a per-control-step memo.
Evaluation never invokes an effect or chooses an execution path. A value from an
unexecuted action fails with value_unavailable instead of implicitly running that
action. ReadValue observes materialized values; it does not execute the graph.
Native state is constructed per call site/instance, as in the data profile; state
factories must not perform world effects. World effects belong to Invoke.

Run budgets count control dispatch, expression evaluation and application work;
native calls have their existing additional budget. The explicit continuation
stack survives budget yields. Resuming never repeats an accepted action. Overflow,
native errors, reentrancy and cancellation keep asset/node/static IR diagnostics.
InstructionPointer is the current static node index for this profile. Exceptions
from local allocations retain the existing discard-instance policy. Native calls
are synchronous and cannot be preempted or rolled back.

CreateForEvent selects an EventEntry by stable event-input port ID and supplies
an i64 payload. Every delivery creates a fresh activation; program/native factory
snapshots are shared, mutable state is not. A program can contain several event
entries and at most one immediate Entry. Create requires that immediate Entry;
an event-only program requires CreateForEvent. Event inputs are external delivery
boundaries, not control transitions or an implicit global event bus.

Repeat captures a count in [0, kMaxRepeatIterations] once. Its first control output
is a subtree invoked for indices 0..count-1, its second is the continuation after
the loop. Return from the body is implicit; explicit cyclic edges remain invalid.
The index is available only inside the active loop, including nested loops.
Each iteration invalidates body action results before execution, so a skipped
branch cannot consume a previous iteration's effect. Pure expressions are freshly
memoized at each control step. Iteration, invalidation and expression work consume
the same instruction budget; even an empty/large loop yields. ReadValue observes
the last materialized pure value, never evaluates an expression by itself.

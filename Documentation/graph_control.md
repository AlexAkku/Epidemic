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
| Sequence | 0/0 | 1/N | N positive; execute each output subtree in port order |
| Branch | 1/0 | 1/2 | Nonzero selects first output; zero selects second |
| NativeCall | 1/1 | 1/1 | Ordered action bound to an explicit native factory |
| Output | 1/0 | 1/0 | Publish an output sink when reached |

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

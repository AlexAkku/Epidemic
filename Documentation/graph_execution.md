# Graph Compiler/VM 2.4 — i64 profile

`CompileGraph` links a GraphDefinition to explicit node bindings and emits immutable
process-local register IR. The first profile supports i64 Input, checked Add and
Multiply, unary NativeCall and Output. It requires exact node shapes, connected
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
Completed/failed instances are terminal. Create a fresh instance for a new run.
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
No clock, game loop, Wait, scheduler, Editor UI, CLR or code generator is added.

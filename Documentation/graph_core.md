# Graph Core 2.3

`EpidemicRuntimeGraph` depends only on RuntimeFoundation. It describes owned
`GraphDefinition`, typed nodes, data/control/event ports, and directed connections.
It executes no node and owns no world, clock, scheduler, VM or instance state.

An asset owns one namespace of nonzero node/port/connection StringIds. The authoring
owner preserves these opaque IDs through rename/move/reload; labels are metadata.
Existing `AssetElementReference {asset_id, element_id}` can identify graph elements.

`ValidateGraph` accepts a borrowed schema type-ID inventory. Node types and
data/event payload types must be known; control ports carry only a signal and
have no payload type. Connections require Output -> Input, identical kind and
exact payload type. Data inputs accept one connection; outputs can fan out.
Control/event inputs can have multiple sources. Duplicate IDs/edges, dangling
ports, malformed enums and excessive counts/labels fail explicitly.

This is structural validation, not proof of executability. Cycles are representable;
node-specific contracts, evaluation order, coercion and model-specific cycle rules
belong to later compilation. Known type IDs are supplied from existing Schema;
Graph creates no competing type registry and has no peer-major dependencies.

Runtime Support provides `GraphDefinitionSerializer` using existing Serialization,
type `epidemic.graph_definition`, schema 1.0.0. It owns a caller-provided type-ID
snapshot, validates before saving and after reading, bounds counts before building
vectors, and publishes a loaded candidate only on success. IDs are uint64 fields;
names, node types, port kinds/directions/payload types and all edges round-trip.

The current backend is SerializedDocument in memory, without a disk wire format.
The separate [i64 Compiler/VM profile](graph_execution.md) consumes these definitions;
Graph Core itself performs no execution. Scheduler, Editor UI and C# bindings remain separate.
Mutating definitions is externally synchronized; validation allocates local indexes
and retains ordinary C++ allocation semantics. Serialization's existing typed
boundary handles archive errors and allocation exceptions.

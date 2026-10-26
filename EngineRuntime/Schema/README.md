# Native Schema probe

`EpidemicRuntimeSchema` depends only on RuntimeFoundation. Framework's
`StableTypeRegistry` cannot be imported into a lower layer; it registers names,
not native property/operation bindings. This module reuses Foundation `StringId`
and `Result` and adds checked native member bindings without copying Framework
registries, changing entity identity, or adding a serializer.

Register a C++ owner type, a pointer-to-member property (optional validator), and
a callable returning `Result`. `Read<Value>`, `Write` and `Invoke<Return>` check
the owner/value/argument/return types before accessing a binding. Unknown IDs,
duplicate registration and rejected values return explicit errors. A rejected
validator runs before assignment. Callback exceptions/allocation failure retain
ordinary C++ semantics; this is not a transactional object store.

Member IDs are scoped by owner type and property/operation kind. Names are
nonempty ASCII letters/digits/dot/underscore/hyphen and use existing StringId
hashing; collisions are rejected without replacing accepted registrations.
Objects are borrowed only for the call; registry mutation is externally
synchronized. RTTI and callable/member pointers are process-local and must not
be serialized or treated as a stable native/managed ABI. No CLR, UI, codegen,
graph VM or hot reload is introduced.

`EpidemicRuntimeSchemaTests` exercises one owner type, one property and one
operation, including type/ID/validation failures and duplicate registrations.

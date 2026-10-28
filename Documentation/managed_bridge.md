# Optional managed bridge — ABI v1

The Windows x64 adapter belongs to Runtime Support. Enable
`EPIDEMIC_BUILD_MANAGED_BRIDGE` explicitly; its default is OFF. C++ Graph, Time,
Simulation, Framework and the ordinary Support target have no CLR linkage.
The disabled build neither discovers dotnet nor builds managed assemblies.
The existing Python managed-architecture preflight still validates source ownership
and project references, including when the runtime adapter is disabled.

The enabled build uses an installed .NET 10 SDK and existing native host headers
(`EPIDEMIC_DOTNET_HOST_PACK`). Tests additionally receive an absolute
`EPIDEMIC_HOSTFXR_LIBRARY` path. No installer/downloader runs. Build into Temp and
keep .NET output/intermediate artifacts there. The managed assembly is a separate
deployment artifact; this stage does not add an SDK installer or hot reload.

`ManagedHost::Load` receives explicit absolute hostfxr/runtimeconfig/assembly paths
and an assembly-qualified type exporting static UnmanagedCallersOnly/Cdecl GetApi.
The component uses BridgeExports.WriteApi and BridgeRuntime.Open with its explicit
callable factory; this is not another type/Schema registry. The versioned, sized C
tables contain only fixed-width values, transient pointers and Cdecl functions.
ABI v1 is independent of the existing native SDK ABI. Mismatches fail explicitly.
Raw ABI callers must supply valid memory; unsafe components are trusted process code.

Each Open creates an opaque, non-reused process-local session, independent managed
state and an explicitly bound native callback. Buffers are borrowed synchronously,
up to 4096 i64 items, disjoint or exactly in place. A confirmed completed prefix
survives errors; an item which fails may already have effects. There is no rollback
or automatic replay. Status, completed count and original native error codes are
retained; managed exceptions produce status diagnostics without crossing the ABI.
Derived managed batch callables must update the completed prefix after publishing
outputs, including when a native batch reports a partial failure.

Invoke stays on the opening thread. NativeContext is a lease for one active batch:
retained contexts, later batches and worker-thread callbacks cannot use its native
pointer. Calls root the session across GC and callback-driven release of the last
external native reference. Recursive Invoke/Close returns Busy. Idle Close may run
on another thread; component Dispose and native-state destruction must support the
closing thread. Close invalidates the token and callback lease and releases callable
roots even if Dispose throws. Explicit Close reports cleanup errors; RAII performs
best-effort cleanup. CLR, loaded code and pinned hostfxr remain process resident.
Closing host contexts or sessions does not imply runtime/assembly unload.

CreateManagedGraphCallable adapts this boundary to the existing Graph native
callable contract. Every call site/instance gets a managed session and a fresh
native callback state, including the existing typed Schema factory. Create and
execute those instances on the native-effect thread. No managed pointer/token is
saved in GraphDefinition or IR serialization. Control/event execution is the
separate priority stage 2.6a, before components and Editor Core.

C# ownership and project references use the existing managed validator; `.cs` and
`.csproj` are routed separately from native include scans. Dependency allowlists
are unchanged. WebAssembly builds are a mandatory release goal after the remaining functionality is complete; this hostfxr
backend does not implement a browser CLR host.

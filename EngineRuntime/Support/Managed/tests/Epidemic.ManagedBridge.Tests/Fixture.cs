using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Epidemic.ManagedBridge;

namespace Epidemic.ManagedBridge.Tests;

internal class FixtureCallable : ManagedCallable
{
    private long calls;
    private static NativeContext? retained;
    private static int disposed;
    private static WeakReference<FixtureCallable>? lastDisposed;
    internal static ulong TestHandle;

    protected override long Invoke(long value, NativeContext native)
    {
        if (value == -1) throw new InvalidOperationException("fixture managed exception");
        if (value == -7) throw new OverflowException();
        if (value == -2) { GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect(); value = 1; }
        if (value == -3)
        {
            try { _ = retained!.Invoke(1); return -99; }
            catch (BridgeCallException error) { return (long)error.Status; }
        }
        if (value == -4) return Task.Run(() =>
        {
            try { _ = native.Invoke(1); return -99L; }
            catch (BridgeCallException error) { return (long)error.Status; }
        }).GetAwaiter().GetResult();
        if (value == -5) return disposed;
        if (value == -6)
        {
            GC.Collect(); GC.WaitForPendingFinalizers(); GC.Collect();
            return lastDisposed != null && !lastDisposed.TryGetTarget(out _) ? 1 : 0;
        }
        if (value == -10) return Task.Run(() => BridgeRuntime.Close(TestHandle)).GetAwaiter().GetResult();
        retained = native;
        return checked(native.Invoke(value) * 2 + ++calls);
    }
    public override void Dispose()
    {
        lastDisposed = new WeakReference<FixtureCallable>(this);
        Interlocked.Increment(ref disposed);
    }
}

internal sealed class ThrowDispose : FixtureCallable
{
    public override void Dispose() { base.Dispose(); throw new InvalidOperationException("fixture Dispose exception"); }
}
internal sealed class BatchCallable : ManagedCallable
{
    protected override long Invoke(long value, NativeContext native) => throw new NotSupportedException();
    public override void InvokeBatch(ReadOnlySpan<long> input, Span<long> output, NativeContext native, ref uint completed)
    {
        try { completed = native.InvokeBatch(input, output); }
        catch (BridgeCallException error) { completed = error.Completed; throw; }
    }
}

public static unsafe class Entry
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int GetApi(uint version, uint capacity, ManagedApi* api) => BridgeExports.WriteApi(version, capacity, api, &Open);
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int Open(NativeApi* native, ulong* handle) => BridgeRuntime.Open(native, static () => new FixtureCallable(), handle);
}
public static unsafe class BatchEntry
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int GetApi(uint version, uint capacity, ManagedApi* api) => BridgeExports.WriteApi(version, capacity, api, &Open);
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int Open(NativeApi* native, ulong* handle) => BridgeRuntime.Open(native, static () => new BatchCallable(), handle);
}
public static unsafe class ThrowDisposeEntry
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int GetApi(uint version, uint capacity, ManagedApi* api) => BridgeExports.WriteApi(version, capacity, api, &Open);
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int Open(NativeApi* native, ulong* handle) => BridgeRuntime.Open(native, static () => new ThrowDispose(), handle);
}
public static unsafe class BadAbiEntry
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int GetApi(uint version, uint capacity, ManagedApi* api)
    {
        var get = (delegate* unmanaged[Cdecl]<uint, uint, ManagedApi*, int>)&Entry.GetApi;
        int status = get(version, capacity, api);
        if (status == 0) api->Version = 2;
        return status;
    }
}
public static unsafe class BadResponseEntry
{
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    public static int GetApi(uint version, uint capacity, ManagedApi* api)
    {
        var get = (delegate* unmanaged[Cdecl]<uint, uint, ManagedApi*, int>)&Entry.GetApi;
        int status = get(version, capacity, api);
        if (status == 0) api->InvokeBatch = &Malformed;
        return status;
    }
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int Malformed(ulong session, long* input, long* output, uint count, uint* completed)
    { *completed = count + 1; return 0; }
}

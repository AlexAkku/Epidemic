using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;
using Epidemic.ManagedBridge;
using Epidemic.ManagedBridge.Tests;

internal static unsafe class Program
{
    private static int batches;
    private static void Check(bool condition, string message) { if (!condition) throw new InvalidOperationException(message); }
    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int Native(void* context, long* input, long* output, uint count, uint* completed)
    {
        ++batches; *completed = 0;
        for (uint index = 0; index < count; ++index)
        { output[index] = input[index] + 1; ++*completed; }
        return 0;
    }
    private static (int Status, long Value) One(ulong session, long value)
    {
        long output = -99; uint completed = 0;
        int status = BridgeRuntime.InvokeBatch(session, &value, &output, 1, &completed);
        return (status, output);
    }
    private static void Run()
    {
        Check(sizeof(NativeApi) == 24 && sizeof(ManagedApi) == 32, "ABI layout");
        NativeApi native = new() { Size = (uint)sizeof(NativeApi), Version = 1, InvokeBatch = &Native };
        ManagedApi api = default;
        var get = (delegate* unmanaged[Cdecl]<uint, uint, ManagedApi*, int>)&Entry.GetApi;
        Check(get(2, (uint)sizeof(ManagedApi), &api) == (int)BridgeStatus.AbiMismatch, "ABI version mismatch");
        Check(get(1, 1, &api) == (int)BridgeStatus.AbiMismatch, "short API table");
        Check(get(1, (uint)sizeof(ManagedApi), &api) == 0 && api.Version == 1 && api.Open != null, "exported API");
        ulong handle = 99;
        native.Version = 2;
        Check(BridgeRuntime.Open(&native, static () => new FixtureCallable(), &handle) == (int)BridgeStatus.AbiMismatch && handle == 0, "callback ABI mismatch");
        native.Version = 1;
        Check(BridgeRuntime.Open(&native, static () => throw new InvalidOperationException(), &handle) == (int)BridgeStatus.ManagedException && handle == 0, "factory exception contained");
        Check(BridgeRuntime.Open(&native, static () => new FixtureCallable(), &handle) == 0, "open");
        ulong first = handle;
        long* input = stackalloc long[] { 1, -1, 3 };
        long* output = stackalloc long[] { -99, -99, -99 };
        uint completed = 99;
        Check(BridgeRuntime.InvokeBatch(handle, input, output, 3, &completed) == (int)BridgeStatus.ManagedException &&
              completed == 1 && output[0] == 5 && output[1] == -99 && output[2] == -99, "exception keeps confirmed prefix");
        Check(One(handle, -3).Value == (long)BridgeStatus.InvalidState, "old batch callback lease rejected");
        Check(One(handle, -4).Value == (long)BridgeStatus.WrongThread, "worker callback rejected without blocking owner");
        FixtureCallable.TestHandle = handle;
        Check(One(handle, -10).Value == (long)BridgeStatus.Busy, "concurrent Close never blocks active invocation");
        Check(Task.Run(() => One(first, 1).Status).GetAwaiter().GetResult() == (int)BridgeStatus.WrongThread, "wrong thread invoke");
        Check(BridgeRuntime.InvokeBatch(handle, input, input + 1, 2, &completed) == (int)BridgeStatus.InvalidArgument, "overlap rejected");
        Check(BridgeRuntime.InvokeBatch(handle, input, output, BridgeExports.MaxBatch + 1, &completed) == (int)BridgeStatus.InvalidArgument, "batch bound");
        Check(BridgeRuntime.Close(handle) == 0 && One(handle, 1).Status == (int)BridgeStatus.InvalidHandle &&
              BridgeRuntime.Close(handle) == (int)BridgeStatus.InvalidHandle, "stale handles invalidated");
        Check(BridgeRuntime.Open(&native, static () => new FixtureCallable(), &handle) == 0 && handle != first, "handles not reused");
        Check(One(handle, -3).Value == (long)BridgeStatus.InvalidHandle, "captured context after close safe");
        Check(One(handle, -6).Value == 1 && One(handle, -2).Value == 5, "closed callable collectable, live session survives GC");
        ulong second = handle;
        Check(Task.Run(() => BridgeRuntime.Close(second)).GetAwaiter().GetResult() == 0, "idle Close allowed on other thread");
        Check(BridgeRuntime.Open(&native, static () => new BatchCallable(), &handle) == 0, "batch callable");
        batches = 0; input[0] = 1; input[1] = 2; input[2] = 3;
        Check(BridgeRuntime.InvokeBatch(handle, input, output, 3, &completed) == 0 && completed == 3 &&
              output[0] == 2 && output[2] == 4 && batches == 1, "one native callback for a whole batch");
        Check(BridgeRuntime.Close(handle) == 0, "batch close");
        Check(BridgeRuntime.Open(&native, static () => new ThrowDispose(), &handle) == 0 &&
              BridgeRuntime.Close(handle) == (int)BridgeStatus.ManagedException &&
              One(handle, 1).Status == (int)BridgeStatus.InvalidHandle, "Dispose exception cannot leave a live session");
    }
    private static int Main()
    {
        try { Run(); Console.WriteLine("Managed bridge contracts PASS"); return 0; }
        catch (Exception error) { Console.Error.WriteLine(error); return 1; }
    }
}

using System.Runtime.CompilerServices;
using System.Runtime.InteropServices;

namespace Epidemic.ManagedBridge;

public enum BridgeStatus : int
{
    Ok, InvalidArgument, AbiMismatch, InvalidHandle, Busy, WrongThread,
    ManagedException, NativeFailure, NativeException, Overflow, Capacity, InvalidState, InvalidResponse
}

[StructLayout(LayoutKind.Sequential)]
public unsafe struct NativeApi
{
    public uint Size;
    public uint Version;
    public void* Context;
    public delegate* unmanaged[Cdecl]<void*, long*, long*, uint, uint*, int> InvokeBatch;
}

[StructLayout(LayoutKind.Sequential)]
public unsafe struct ManagedApi
{
    public uint Size;
    public uint Version;
    public delegate* unmanaged[Cdecl]<NativeApi*, ulong*, int> Open;
    public delegate* unmanaged[Cdecl]<ulong, long*, long*, uint, uint*, int> InvokeBatch;
    public delegate* unmanaged[Cdecl]<ulong, int> Close;
}

public sealed class BridgeCallException(BridgeStatus status, uint completed = 0) : Exception(status.ToString())
{
    public BridgeStatus Status { get; } = status;
    public uint Completed { get; } = completed;
}

public interface IManagedCallable : IDisposable
{
    // Publish only a confirmed output prefix. Exceptions preserve that prefix;
    // effects in the failing item are not rolled back and must not be replayed.
    void InvokeBatch(ReadOnlySpan<long> input, Span<long> output, NativeContext native, ref uint completed);
}

public abstract class ManagedCallable : IManagedCallable
{
    protected abstract long Invoke(long value, NativeContext native);
    public virtual void InvokeBatch(ReadOnlySpan<long> input, Span<long> output, NativeContext native, ref uint completed)
    {
        for (int index = 0; index < input.Length; ++index)
        {
            output[index] = Invoke(input[index], native);
            ++completed;
        }
    }
    public virtual void Dispose() { }
}

public static unsafe class BridgeExports
{
    public const uint Version = 1;
    public const uint MaxBatch = 4096;

    public static int WriteApi(uint version, uint capacity, ManagedApi* api,
        delegate* unmanaged[Cdecl]<NativeApi*, ulong*, int> open)
    {
        if (api == null || open == null) return (int)BridgeStatus.InvalidArgument;
        if (version != Version || capacity < sizeof(ManagedApi)) return (int)BridgeStatus.AbiMismatch;
        *api = new ManagedApi { Size = (uint)sizeof(ManagedApi), Version = Version,
            Open = open, InvokeBatch = &InvokeBatch, Close = &Close };
        return (int)BridgeStatus.Ok;
    }

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int InvokeBatch(ulong session, long* input, long* output, uint count, uint* completed)
        => BridgeRuntime.InvokeBatch(session, input, output, count, completed);

    [UnmanagedCallersOnly(CallConvs = [typeof(CallConvCdecl)])]
    private static int Close(ulong session) => BridgeRuntime.Close(session);
}

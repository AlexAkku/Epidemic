namespace Epidemic.ManagedBridge;

internal sealed unsafe class Session(NativeApi native, IManagedCallable callable)
{
    internal readonly object Gate = new();
    internal readonly int Thread = Environment.CurrentManagedThreadId;
    internal NativeApi Native = native;
    internal IManagedCallable? Callable = callable;
    internal bool Closed;
    internal bool Executing;
    internal ulong Call;
}

// A callback lease belongs to exactly one synchronous managed batch. Retained
// contexts cannot call native code in a later batch or after session disposal.
public sealed unsafe class NativeContext
{
    private readonly Session session;
    private readonly ulong call;
    internal NativeContext(Session session, ulong call) { this.session = session; this.call = call; }

    public long Invoke(long value)
    {
        long result = 0;
        InvokeBatch(new ReadOnlySpan<long>(&value, 1), new Span<long>(&result, 1));
        return result;
    }
    public uint InvokeBatch(ReadOnlySpan<long> input, Span<long> output)
    {
        if (Environment.CurrentManagedThreadId != session.Thread) throw new BridgeCallException(BridgeStatus.WrongThread);
        lock (session.Gate)
        {
            if (session.Closed) throw new BridgeCallException(BridgeStatus.InvalidHandle);
            if (!session.Executing || session.Call != call) throw new BridgeCallException(BridgeStatus.InvalidState);
            if (input.Length != output.Length || input.Length > BridgeExports.MaxBatch ||
                (input.Overlaps(output, out int offset) && offset != 0))
                throw new BridgeCallException(BridgeStatus.InvalidArgument);
            uint completed = 0;
            fixed (long* source = input)
            fixed (long* destination = output)
            {
                int status = session.Native.InvokeBatch(session.Native.Context, source, destination, (uint)input.Length, &completed);
                if (completed > input.Length || (status == 0 && completed != input.Length) || !Enum.IsDefined((BridgeStatus)status))
                    throw new BridgeCallException(BridgeStatus.InvalidResponse);
                if (status != 0) throw new BridgeCallException((BridgeStatus)status, completed);
            }
            return completed;
        }
    }
}

public static unsafe class BridgeRuntime
{
    private static readonly object RegistryGate = new();
    private static readonly Dictionary<ulong, Session> Sessions = [];
    private static ulong next = 1;
    private const int MaxSessions = 4096;

    public static int Open(NativeApi* native, Func<IManagedCallable> factory, ulong* handle)
    {
        if (handle == null) return (int)BridgeStatus.InvalidArgument;
        *handle = 0;
        IManagedCallable? callable = null;
        try
        {
            if (native == null || factory == null || native->InvokeBatch == null) return (int)BridgeStatus.InvalidArgument;
            if (native->Version != BridgeExports.Version || native->Size < sizeof(NativeApi)) return (int)BridgeStatus.AbiMismatch;
            callable = factory();
            if (callable == null) return (int)BridgeStatus.InvalidArgument;
            var state = new Session(*native, callable);
            lock (RegistryGate)
            {
                if (Sessions.Count == MaxSessions || next == ulong.MaxValue) return (int)BridgeStatus.Capacity;
                ulong candidate = next;
                Sessions.Add(candidate, state);
                ++next;
                *handle = candidate;
            }
            callable = null; // Ownership transferred to Session.
            return (int)BridgeStatus.Ok;
        }
        catch { return (int)BridgeStatus.ManagedException; }
        finally { if (callable != null) { try { callable.Dispose(); } catch { /* No published session remains. */ } } }
    }

    private static Session? Find(ulong handle)
    {
        lock (RegistryGate) return Sessions.GetValueOrDefault(handle);
    }

    public static int InvokeBatch(ulong handle, long* input, long* output, uint count, uint* completed)
    {
        if (completed == null) return (int)BridgeStatus.InvalidArgument;
        *completed = 0;
        try
        {
            if (count > BridgeExports.MaxBatch || (count != 0 && (input == null || output == null))) return (int)BridgeStatus.InvalidArgument;
            var state = Find(handle);
            if (state == null) return (int)BridgeStatus.InvalidHandle;
            if (Environment.CurrentManagedThreadId != state.Thread) return (int)BridgeStatus.WrongThread;
            lock (state.Gate)
            {
                if (state.Closed || state.Callable == null) return (int)BridgeStatus.InvalidHandle;
                if (state.Executing) return (int)BridgeStatus.Busy;
                if (state.Call == ulong.MaxValue) return (int)BridgeStatus.Capacity;
                var source = new ReadOnlySpan<long>(input, (int)count);
                var destination = new Span<long>(output, (int)count);
                if (source.Overlaps(destination, out int offset) && offset != 0) return (int)BridgeStatus.InvalidArgument;
                state.Executing = true;
                uint progress = 0;
                BridgeStatus status;
                try
                {
                    var context = new NativeContext(state, ++state.Call);
                    state.Callable.InvokeBatch(source, destination, context, ref progress);
                    status = progress == count ? BridgeStatus.Ok : BridgeStatus.InvalidResponse;
                }
                catch (BridgeCallException error) { status = error.Status; }
                catch (OverflowException) { status = BridgeStatus.Overflow; }
                catch { status = BridgeStatus.ManagedException; }
                finally { state.Executing = false; }
                if (progress > count || !Enum.IsDefined(status) || (status == BridgeStatus.Ok && progress != count))
                    return (int)BridgeStatus.InvalidResponse;
                *completed = progress;
                return (int)status;
            }
        }
        catch { return (int)BridgeStatus.ManagedException; }
    }

    public static int Close(ulong handle)
    {
        try
        {
            var state = Find(handle);
            if (state == null) return (int)BridgeStatus.InvalidHandle;
            if (!Monitor.TryEnter(state.Gate)) return (int)BridgeStatus.Busy;
            IManagedCallable? callable;
            try
            {
                if (state.Closed) return (int)BridgeStatus.InvalidHandle;
                if (state.Executing) return (int)BridgeStatus.Busy;
                lock (RegistryGate) Sessions.Remove(handle);
                state.Closed = true;
                state.Native = default;
                callable = state.Callable;
                state.Callable = null;
            }
            finally { Monitor.Exit(state.Gate); }
            // Close is thread-independent. Dispose cannot invoke native callbacks.
            callable?.Dispose();
            return (int)BridgeStatus.Ok;
        }
        catch { return (int)BridgeStatus.ManagedException; }
    }
}

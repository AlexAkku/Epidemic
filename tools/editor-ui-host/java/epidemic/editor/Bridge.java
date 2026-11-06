package epidemic.editor;

import java.io.*;
import java.nio.ByteBuffer;
import java.nio.charset.*;
import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.function.Consumer;
import javax.swing.SwingUtilities;

/** One private companion/pipe and one bounded IO dispatcher per application session. */
public final class Bridge implements AutoCloseable {
    public enum Service { Unavailable, Loading, Ready, Error }
    public record Selection(int kind, long document, long element) { }
    public record Document(long id, long revision, int kind, String name) { }
    public record Node(long document, long id, long parent, int kind, String label) { }
    public record Asset(long id, long type, long generation, long fingerprint, boolean trashed,
                        String path, String cookedKey) { }
    public record Snapshot(long revision, long assetRevision, long selectionRevision, int flags,
        int storage, long project, long entry, String path, String title, Selection selection,
        List<Document> documents, List<Node> nodes, List<Asset> assets) {
        public Snapshot {
            documents = List.copyOf(documents); nodes = List.copyOf(nodes); assets = List.copyOf(assets);
        }
        public boolean dirty() { return (flags & 1) != 0; }
        public boolean canUndo() { return (flags & 2) != 0; }
        public boolean canRedo() { return (flags & 4) != 0; }
        public boolean playing() { return (flags & 8) != 0; }
    }
    public record Reply(long generation, String error, Set<String> capabilities, Snapshot snapshot,
                        long latencyNanos) { }
    public record State(Service service, Reply reply, String diagnostic) { }
    private static final int MAGIC = 0x45505549, LIMIT = 16 * 1024 * 1024;
    private final ThreadPoolExecutor worker = new ThreadPoolExecutor(1, 1, 0, TimeUnit.SECONDS,
        new ArrayBlockingQueue<>(32), runnable -> {
            Thread thread = new Thread(runnable, "Editor bridge IO"); thread.setDaemon(true); return thread;
        }, new ThreadPoolExecutor.AbortPolicy());
    private final ScheduledExecutorService timer = Executors.newSingleThreadScheduledExecutor(r -> {
        Thread thread = new Thread(r, "Editor bridge timeout"); thread.setDaemon(true); return thread;
    });
    private final Path executable, logs;
    private final Consumer<State> listener;
    private volatile Process child;
    private DataInputStream input;
    private DataOutputStream output;
    private volatile long generation, connection;
    private long correlation;
    private volatile boolean closed, stopping;
    private volatile Reply latest;
    public Bridge(Path executable, Path logs, Consumer<State> listener) {
        this.executable = executable; this.logs = logs; this.listener = listener;
    }
    public Reply latest() { return latest; }
    private void publish(Service state, String diagnostic, long epoch) {
        State value = new State(state, latest, diagnostic);
        SwingUtilities.invokeLater(() -> { if (!closed && epoch == connection) listener.accept(value); });
    }
    public CompletableFuture<Reply> connect() {
        return enqueue(() -> {
            if (child != null) throw new IOException("ui.already_connected");
            Files.createDirectories(logs);
            child = new ProcessBuilder(executable.toString())
                .redirectError(ProcessBuilder.Redirect.appendTo(logs.resolve("backend.log").toFile())).start();
            if (closed) { child.destroy(); throw new IOException("ui.cancelled"); }
            input = new DataInputStream(child.getInputStream());
            output = new DataOutputStream(child.getOutputStream());
            Process owned = child;
            long epoch = connection;
            owned.onExit().thenRun(() -> {
                if (!closed && !stopping && child == owned && epoch == connection)
                    publish(Service.Error, "Native backend disconnected", epoch);
            });
            return exchange(1, 0, 0, List.of());
        });
    }
    public CompletableFuture<Reply> call(int operation, String... args) {
        long expectedGeneration = generation;
        long expectedRevision = latest == null || latest.snapshot() == null ? 0 : latest.snapshot().revision();
        return callAt(expectedGeneration, expectedRevision, operation, args);
    }
    public CompletableFuture<Reply> callAt(long expectedGeneration, long expectedRevision, int operation, String... args) {
        return enqueue(() -> exchange(operation, expectedGeneration, expectedRevision, List.of(args)));
    }
    private interface Request { Reply run() throws Exception; }
    private final class Task implements Runnable {
        boolean started;
        final CompletableFuture<Reply> future = new CompletableFuture<>() {
            @Override public boolean cancel(boolean interrupt) {
                synchronized (Task.this) { return !started && super.cancel(false); }
            }
        };
        final Request request;
        final long epoch = connection;
        Task(Request request) { this.request = request; }
        public void run() {
            synchronized (this) { if (future.isCancelled()) return; started = true; }
            if (closed || epoch != connection) { future.completeExceptionally(new IOException("ui.cancelled")); return; }
            publish(Service.Loading, "", epoch);
            ScheduledFuture<?> timeout;
            try { timeout = timer.schedule(() -> {
                if (future.completeExceptionally(new IOException("ui.backend_timeout"))) {
                    Process owned = child;
                    if (owned != null) owned.destroyForcibly();
                }
            }, 15, TimeUnit.SECONDS); }
            catch (RejectedExecutionException cancelled) { future.completeExceptionally(new IOException("ui.cancelled")); return; }
            try {
                Reply reply = request.run();
                if (!closed && epoch == connection && !future.isDone()) {
                    latest = reply; generation = reply.generation();
                    publish(Service.Ready, reply.error(), epoch);
                    if (reply.error().isEmpty()) future.complete(reply);
                    else future.completeExceptionally(new IOException(reply.error()));
                } else future.completeExceptionally(new IOException("ui.cancelled"));
            } catch (Exception error) {
                future.completeExceptionally(error);
                publish(Service.Error, error.toString(), epoch);
            } finally { timeout.cancel(false); }
        }
    }
    private CompletableFuture<Reply> enqueue(Request request) {
        Task task = new Task(request);
        if (closed) task.future.completeExceptionally(new IOException("ui.closed"));
        else try { worker.execute(task); }
        catch (RejectedExecutionException busy) { task.future.completeExceptionally(new IOException("ui.queue_full")); }
        return task.future;
    }
    public int cancelQueued() {
        int cancelled = 0;
        for (Runnable pending : worker.getQueue().toArray(Runnable[]::new))
            if (pending instanceof Bridge.Task task && task.future.cancel(false)) { worker.remove(task); ++cancelled; }
        return cancelled;
    }
    private static void text(DataOutputStream out, String value) throws IOException {
        byte[] bytes = value.getBytes(StandardCharsets.UTF_8);
        if (bytes.length > 1024 * 1024) throw new IOException("ui.string_limit");
        out.writeInt(bytes.length); out.write(bytes);
    }
    private static String text(DataInputStream in) throws IOException {
        int size = in.readInt();
        if (size < 0 || size > 1024 * 1024 || size > in.available()) throw new IOException("ui.string_limit");
        return StandardCharsets.UTF_8.newDecoder().onMalformedInput(CodingErrorAction.REPORT)
            .onUnmappableCharacter(CodingErrorAction.REPORT).decode(ByteBuffer.wrap(in.readNBytes(size))).toString();
    }
    private static int count(DataInputStream in) throws IOException {
        int count = in.readInt();
        if (count < 0 || count > 262144) throw new IOException("ui.count_limit");
        return count;
    }
    private Reply exchange(int operation, long expectedGeneration, long revision, List<String> args) throws IOException {
        if (SwingUtilities.isEventDispatchThread()) throw new IOException("ui.blocking_on_EDT");
        if (output == null || child == null || !child.isAlive()) throw new IOException("ui.disconnected");
        ByteArrayOutputStream bytes = new ByteArrayOutputStream();
        DataOutputStream request = new DataOutputStream(bytes);
        long id = ++correlation, start = System.nanoTime();
        request.writeInt(MAGIC); request.writeShort(1); request.writeShort(0); request.writeLong(id);
        request.writeByte(operation); request.writeLong(expectedGeneration); request.writeLong(revision);
        if (args.size() > 8) throw new IOException("ui.argument_limit");
        request.writeShort(args.size()); for (String arg : args) text(request, arg);
        if (bytes.size() > LIMIT) throw new IOException("ui.frame_limit");
        output.writeInt(bytes.size()); bytes.writeTo(output); output.flush();
        int size = input.readInt();
        if (size <= 0 || size > LIMIT) throw new IOException("ui.frame_limit");
        byte[] body = input.readNBytes(size);
        if (body.length != size) throw new EOFException("ui.truncated_response");
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(body));
        if (in.readInt() != MAGIC || in.readUnsignedShort() != 1 || in.readUnsignedShort() != 0 || in.readLong() != id)
            throw new IOException("ui.protocol_or_correlation");
        int status = in.readUnsignedByte(); long serverGeneration = in.readLong();
        Set<String> capabilities = Set.copyOf(Arrays.asList(text(in).split(",")));
        String error = text(in);
        if ((status == 0) != error.isEmpty()) throw new IOException("ui.invalid_result");
        Snapshot snapshot = null;
        if (in.readBoolean()) {
            long rev = in.readLong(), assetsRev = in.readLong(), selectionRev = in.readLong();
            int flags = in.readUnsignedByte(), storage = in.readUnsignedByte();
            long project = in.readLong(), entry = in.readLong();
            String path = text(in), title = text(in);
            Selection selection = new Selection(in.readUnsignedByte(), in.readLong(), in.readLong());
            List<Document> documents = new ArrayList<>();
            for (int n = count(in); n > 0; --n)
                documents.add(new Document(in.readLong(), in.readLong(), in.readUnsignedByte(), text(in)));
            List<Node> nodes = new ArrayList<>();
            for (int n = count(in); n > 0; --n)
                nodes.add(new Node(in.readLong(), in.readLong(), in.readLong(), in.readUnsignedByte(), text(in)));
            List<Asset> assets = new ArrayList<>();
            for (int n = count(in); n > 0; --n)
                assets.add(new Asset(in.readLong(), in.readLong(), in.readLong(), in.readLong(),
                                     in.readBoolean(), text(in), text(in)));
            snapshot = new Snapshot(rev, assetsRev, selectionRev, flags, storage, project, entry, path,
                                    title, selection, documents, nodes, assets);
        }
        if (in.available() != 0) throw new IOException("ui.trailing_response");
        return new Reply(serverGeneration, error, capabilities, snapshot, System.nanoTime() - start);
    }
    public CompletableFuture<Void> shutdown() {
        stopping = true;
        return call(11).thenAcceptAsync(reply -> {
            try {
                Process owned = child;
                if (owned != null && !owned.waitFor(3, TimeUnit.SECONDS)) owned.destroyForcibly();
                close();
            } catch (InterruptedException interrupted) {
                Thread.currentThread().interrupt(); close(); throw new CompletionException(interrupted);
            }
        }, worker).whenComplete((value, error) -> { if (error != null) stopping = false; });
    }
    @Override public void close() {
        closed = true; ++connection;
        Process owned = child;
        if (owned != null && owned.isAlive()) owned.destroy();
        for (Runnable pending : worker.shutdownNow())
            if (pending instanceof Bridge.Task task)
                task.future.completeExceptionally(new IOException("ui.cancelled"));
        timer.shutdownNow();
    }
}

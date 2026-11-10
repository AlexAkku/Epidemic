package epidemic.editor;

import java.awt.image.BufferedImage;
import java.awt.image.DataBufferInt;
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
    public record Selection(int kind, long document, long element, long component, long part) {
        public Selection(int kind,long document,long element){this(kind,document,element,0,0);}
    }
    public record Document(long id, long revision, int kind, String name, boolean dirty) {
        public Document(long id,long revision,int kind,String name){this(id,revision,kind,name,false);}
    }
    public record Field(String key,String name,int kind,String value,boolean editable,String source,String reason,long reference) { }
    /** Presentation identity only; resolves against the current native snapshot. */
    public record DocumentKey(long generation, long project, int kind, long id) {
        public Optional<Document> resolve(long currentGeneration, Snapshot snapshot) {
            if(generation!=currentGeneration||snapshot==null||snapshot.project()!=project)return Optional.empty();
            return snapshot.documents().stream().filter(d->d.id()==id&&d.kind()==kind).findFirst();
        }
        public String panel() { return switch(kind) {case 0->"scene.editor";case 1->"prefab.canvas";case 2->"graph.editor";default->throw new IllegalArgumentException("Document kind");}; }
    }
    public record Node(long document, long id, long parent, int kind, String label,long component,long part,long prefab) {
        public Node(long document,long id,long parent,int kind,String label,long component,long part){this(document,id,parent,kind,label,component,part,0);}
        public Selection selection(){return new Selection(kind,document,id,component,part);}
    }
    public record Asset(long id, long type, long generation, long fingerprint, boolean trashed,
                        String path, String cookedKey,String originalPath,String metadata,boolean present) {
        public Asset(long id,long type,long generation,long fingerprint,boolean trashed,String path,String cookedKey){this(id,type,generation,fingerprint,trashed,path,cookedKey,"","",true);}
    }
    public record AssetEvent(long id,int kind,String diagnostic,long job,String source) { }
    public record Reference(long source,long dependent,String origin,boolean required) { }
    public record Sprite(long document,String key,float x,float y,float sx,float sy,float rotation,float ppu,float pivotX,float pivotY) { }
    public record AssetState(List<Sprite> sprites,int totalAssets,long revision,List<AssetEvent> events,List<Reference> references,int width,int height,String pixels,String previewError) {
        public AssetState {sprites=List.copyOf(sprites);events=List.copyOf(events);references=List.copyOf(references);}
        public static AssetState empty(){return new AssetState(List.of(),0,0,List.of(),List.of(),0,0,"","");}
    }
    public record ComponentType(long id,String name) {}
    public record SceneItem(long document,long id,double left,double top,double right,double bottom,double x,double y,double rotation) {}
    public record SystemItem(long id,long scene,long graph,boolean enabled) {}
    public record Snapshot(long revision, long assetRevision, long selectionRevision, int flags,
        int storage, long project, long entry, String path, String title, Selection selection,
        List<Document> documents, List<Node> nodes, List<Asset> assets, List<Field> fields,AssetState assetState,List<ComponentType> componentTypes,List<SceneItem> sceneItems,List<SystemItem> systems,List<String> contentFolders) {
        public Snapshot {
            documents = List.copyOf(documents); nodes = List.copyOf(nodes); assets = List.copyOf(assets);
            fields=List.copyOf(fields); contentFolders=List.copyOf(contentFolders);
        }
        public Snapshot(long revision,long assetRevision,long selectionRevision,int flags,int storage,long project,long entry,String path,String title,Selection selection,List<Document> documents,List<Node> nodes,List<Asset> assets,List<Field> fields){this(revision,assetRevision,selectionRevision,flags,storage,project,entry,path,title,selection,documents,nodes,assets,fields,AssetState.empty(),List.of(),List.of(),List.of(),List.of());}
        public Snapshot(long revision,long assetRevision,long selectionRevision,int flags,int storage,long project,long entry,String path,String title,Selection selection,List<Document> documents,List<Node> nodes,List<Asset> assets){this(revision,assetRevision,selectionRevision,flags,storage,project,entry,path,title,selection,documents,nodes,assets,List.of());}
        public boolean dirty() { return (flags & 1) != 0; }
        public boolean canUndo() { return (flags & 2) != 0; }
        public boolean canRedo() { return (flags & 4) != 0; }
        public boolean playing() { return (flags & 8) != 0; }
    }
    public record Reply(long generation, String error, Set<String> capabilities, Snapshot snapshot,
                        long latencyNanos, BufferedImage preview) { }
    public record State(Service service, Reply reply, String diagnostic) { }
    private static final int MAGIC = 0x45505549, LIMIT = 16 * 1024 * 1024;
    private static final int PREVIEW_EDGE = 4096;
    private final ThreadPoolExecutor worker = new ThreadPoolExecutor(1, 1, 0, TimeUnit.SECONDS,
        new ArrayBlockingQueue<>(32), runnable -> {
            Thread thread = new Thread(runnable, "Editor bridge IO"); thread.setDaemon(true); return thread;
        }, new ThreadPoolExecutor.AbortPolicy());
    private final ScheduledExecutorService timer = Executors.newSingleThreadScheduledExecutor(r -> {
        Thread thread = new Thread(r, "Editor bridge timeout"); thread.setDaemon(true); return thread;
    });
    private final Path executable, logs;
    private final Consumer<State> listener;
    private final Consumer<Runnable> uiDispatch;
    private volatile Process child;
    private DataInputStream input;
    private DataOutputStream output;
    private volatile long generation, connection;
    private long correlation;
    private volatile boolean closed, stopping;
    private volatile Reply latest;
    public Bridge(Path executable, Path logs, Consumer<State> listener) {
        this(executable,logs,listener,SwingUtilities::invokeLater);
    }
    public Bridge(Path executable,Path logs,Consumer<State> listener,Consumer<Runnable> uiDispatch){
        this.executable = executable; this.logs = logs; this.listener = listener;this.uiDispatch=uiDispatch;
    }
    public Reply latest() { return latest; }
    private void publish(Service state, String diagnostic, long epoch) {
        State value = new State(state, latest, diagnostic);
        uiDispatch.accept(() -> { if (!closed && epoch == connection) listener.accept(value); });
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
        return enqueue(() -> exchange(operation, expectedGeneration, expectedRevision, List.of(args)),
            operation==26||operation==32&&args.length>0&&args[0].equals("move"),
            false, operation==18&&args.length==3?args[2]:null);
    }
    public CompletableFuture<BufferedImage> assetPreview(long expectedGeneration, long asset, String key, int divisor) {
        return enqueue(() -> exchange(36, expectedGeneration, 0,
            List.of(Long.toUnsignedString(asset), key, Integer.toString(divisor))), true, true, null)
            .thenApply(Reply::preview);
    }
    private interface Request { Reply run() throws Exception; }
    private static final class RequestRejected extends IOException {
        private static final long serialVersionUID=1L;
        RequestRejected(String message){super(message);}
    }
    private final class Task implements Runnable {
        boolean started;
        final CompletableFuture<Reply> future = new CompletableFuture<>() {
            @Override public boolean cancel(boolean interrupt) {
                synchronized (Task.this) { return !started && super.cancel(false); }
            }
        };
        final Request request;
        final boolean quiet, preview;
        final String importDestination;
        final long epoch = connection;
        Task(Request request, boolean quiet, boolean preview, String importDestination) {
            this.request=request;this.quiet=quiet;this.preview=preview;this.importDestination=importDestination;
        }
        void reject(String message) {
            String diagnostic=message;
            if(importDestination!=null){
                diagnostic="Import rejected: "+importDestination+" · "+message;
                if(message.equals("editor.import_destination")||message.equals("editor.import_destination_pending"))
                    diagnostic+=" · The destination exists, is invalid or is already queued. Use Reimport for an existing asset.";
            }
            if(!preview)publish(Service.Ready,diagnostic,epoch);
            future.completeExceptionally(new IOException(message));
        }
        public void run() {
            synchronized (this) { if (future.isCancelled()) return; started = true; }
            if (closed || epoch != connection) { future.completeExceptionally(new IOException("ui.cancelled")); return; }
            if(!quiet)publish(Service.Loading, "", epoch);
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
                    if(!preview){latest=reply;generation=reply.generation();}
                    if (reply.error().isEmpty()) {
                        if(!preview)publish(Service.Ready,"",epoch);
                        future.complete(reply);
                    } else reject(reply.error());
                } else future.completeExceptionally(new IOException("ui.cancelled"));
            } catch (RequestRejected rejected) {
                reject(rejected.getMessage());
            } catch (Exception error) {
                future.completeExceptionally(error);
                publish(Service.Error, error.toString(), epoch);
            } finally { timeout.cancel(false); }
        }
    }
    private CompletableFuture<Reply> enqueue(Request request) {
        return enqueue(request,false,false,null);
    }
    private CompletableFuture<Reply> enqueue(Request request, boolean quiet, boolean preview, String importDestination) {
        Task task = new Task(request,quiet,preview,importDestination);
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
        if(value.length()>1024*1024)throw new RequestRejected("ui.string_limit");
        ByteBuffer encoded;
        try{encoded=StandardCharsets.UTF_8.newEncoder().onMalformedInput(CodingErrorAction.REPORT).onUnmappableCharacter(CodingErrorAction.REPORT).encode(java.nio.CharBuffer.wrap(value));}
        catch(CharacterCodingException invalid){throw new RequestRejected("ui.invalid_utf8_text");}
        byte[] bytes=new byte[encoded.remaining()];encoded.get(bytes);
        if (bytes.length > 1024 * 1024) throw new RequestRejected("ui.string_limit");
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
        request.writeInt(MAGIC); request.writeShort(4); request.writeShort(0); request.writeLong(id);
        request.writeByte(operation); request.writeLong(expectedGeneration); request.writeLong(revision);
        if (args.size() > 8) throw new RequestRejected("ui.argument_limit");
        request.writeShort(args.size()); for (String arg : args) text(request, arg);
        if (bytes.size() > LIMIT) throw new RequestRejected("ui.frame_limit");
        output.writeInt(bytes.size()); bytes.writeTo(output); output.flush();
        int size = input.readInt();
        if (size <= 0 || size > LIMIT) throw new IOException("ui.frame_limit");
        byte[] body = input.readNBytes(size);
        if (body.length != size) throw new EOFException("ui.truncated_response");
        DataInputStream in = new DataInputStream(new ByteArrayInputStream(body));
        if (in.readInt() != MAGIC || in.readUnsignedShort() != 4 || in.readUnsignedShort() != 0 || in.readLong() != id)
            throw new IOException("ui.protocol_or_correlation");
        int status = in.readUnsignedByte(); long serverGeneration = in.readLong();
        Set<String> capabilities = Set.copyOf(Arrays.asList(text(in).split(",")));
        String error = text(in);
        if ((status == 0) != error.isEmpty()) throw new IOException("ui.invalid_result");
        if(operation==36){
            int width=in.readInt(),height=in.readInt();
            if(width<0||height<0||width>PREVIEW_EDGE||height>PREVIEW_EDGE||
                in.available()!=width*height*4||(status==0)!=(width>0&&height>0))
                throw new IOException("ui.preview_bounds");
            BufferedImage preview=null;
            if(status==0){
                preview=new BufferedImage(width,height,BufferedImage.TYPE_INT_ARGB_PRE);
                int[] pixels=((DataBufferInt)preview.getRaster().getDataBuffer()).getData();
                for(int i=0;i<pixels.length;i++)pixels[i]=Integer.reverseBytes(in.readInt());
            }
            return new Reply(serverGeneration,error,capabilities,null,System.nanoTime()-start,preview);
        }
        Snapshot snapshot = null;
        if (in.readBoolean()) {
            long rev = in.readLong(), assetsRev = in.readLong(), selectionRev = in.readLong();
            int flags = in.readUnsignedByte(), storage = in.readUnsignedByte();
            long project = in.readLong(), entry = in.readLong();
            String path = text(in), title = text(in);
            Selection selection = new Selection(in.readUnsignedByte(), in.readLong(), in.readLong(),in.readLong(),in.readLong());
            List<Document> documents = new ArrayList<>();
            for (int n = count(in); n > 0; --n)
                documents.add(new Document(in.readLong(), in.readLong(), in.readUnsignedByte(), text(in),in.readBoolean()));
            List<Node> nodes = new ArrayList<>();
            for (int n = count(in); n > 0; --n)
                nodes.add(new Node(in.readLong(), in.readLong(), in.readLong(), in.readUnsignedByte(), text(in),in.readLong(),in.readLong(),in.readLong()));
            List<Asset> assets = new ArrayList<>();
            for (int n = count(in); n > 0; --n)
                assets.add(new Asset(in.readLong(), in.readLong(), in.readLong(), in.readLong(),
                                     in.readBoolean(), text(in), text(in),text(in),text(in),in.readBoolean()));
            List<Field> fields=new ArrayList<>();
            for(int n=count(in);n>0;--n)fields.add(new Field(text(in),text(in),in.readUnsignedByte(),text(in),in.readBoolean(),text(in),text(in),in.readLong()));
            List<Sprite> sprites=new ArrayList<>();
            for(int n=count(in);n>0;--n){long document=in.readLong();String key=text(in);float[] values=new float[8];for(int i=0;i<8;i++){values[i]=(float)in.readDouble();if(!Float.isFinite(values[i]))throw new IOException("ui.sprite_finite");}sprites.add(new Sprite(document,key,values[0],values[1],values[2],values[3],values[4],values[5],values[6],values[7]));}
            int totalAssets=count(in);long contentRevision=in.readLong();
            List<AssetEvent> events=new ArrayList<>();
            for(int n=count(in);n>0;--n)events.add(new AssetEvent(in.readLong(),in.readUnsignedByte(),text(in),in.readLong(),text(in)));
            List<Reference> references=new ArrayList<>();
            for(int n=count(in);n>0;--n)references.add(new Reference(in.readLong(),in.readLong(),text(in),in.readBoolean()));
            int width=in.readInt(),height=in.readInt();String pixels=text(in),previewError=text(in);
            if(width<0||height<0||width>128||height>128||pixels.length()!=width*height*8)throw new IOException("ui.preview_bounds");
            for(int at=0;at<pixels.length();at++){char hex=pixels.charAt(at);if(!((hex>='0'&&hex<='9')||(hex>='a'&&hex<='f')))throw new IOException("ui.preview_encoding");}
            AssetState assetState=new AssetState(sprites,totalAssets,contentRevision,events,references,width,height,pixels,previewError);
            List<ComponentType> types=new ArrayList<>();
            for(int n=count(in);n>0;--n)types.add(new ComponentType(in.readLong(),text(in)));
            List<SceneItem> items=new ArrayList<>();
            for(int n=count(in);n>0;--n){
                long document=in.readLong(),node=in.readLong();
                double[] bounds=new double[7];
                for(int i=0;i<bounds.length;i++){
                    bounds[i]=in.readDouble();
                    if(!Double.isFinite(bounds[i]))throw new IOException("ui.scene_bounds");
                }
                items.add(new SceneItem(document,node,bounds[0],bounds[1],bounds[2],bounds[3],bounds[4],bounds[5],bounds[6]));
            }
            List<SystemItem> systems=new ArrayList<>();
            for(int n=count(in);n>0;--n)systems.add(new SystemItem(in.readLong(),in.readLong(),in.readLong(),in.readBoolean()));
            List<String> folders=new ArrayList<>();
            if(capabilities.contains("content-folders"))
                for(int n=count(in);n>0;--n)folders.add(text(in));
            snapshot = new Snapshot(rev, assetsRev, selectionRev, flags, storage, project, entry, path,
                                    title, selection, documents, nodes, assets,fields,assetState,List.copyOf(types),List.copyOf(items),List.copyOf(systems),folders);
        }
        if (in.available() != 0) throw new IOException("ui.trailing_response");
        return new Reply(serverGeneration, error, capabilities, snapshot, System.nanoTime() - start, null);
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

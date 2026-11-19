package epidemic.editor.platform;

import com.google.gson.*;
import com.intellij.openapi.Disposable;
import com.intellij.openapi.application.ApplicationManager;
import com.intellij.openapi.util.Disposer;
import com.intellij.ui.jcef.*;
import epidemic.editor.Bridge;
import java.awt.BorderLayout;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.util.*;
import java.util.concurrent.CompletableFuture;
import java.util.function.Supplier;
import javax.swing.*;
import org.cef.browser.*;
import org.cef.handler.*;
import org.cef.misc.BoolRef;
import org.cef.network.CefRequest;

/** One browser/query per FileEditor; no filesystem or general-purpose RPC is exposed. */
final class GraphEditor implements Disposable {
    interface Commands {
        CompletableFuture<Bridge.Reply> execute(long generation, long revision, int operation, String... args);
    }
    private static final Gson JSON = new Gson();
    private static final com.intellij.openapi.diagnostic.Logger LOG = com.intellij.openapi.diagnostic.Logger.getInstance(GraphEditor.class);
    private final String nonce = UUID.randomUUID().toString();
    private final String origin = "file:///epidemic-graph/" + nonce + "/index.html";
    private final Bridge.DocumentKey key;
    private final Supplier<Bridge.Reply> latest;
    private final Commands commands;
    private final epidemic.editor.GraphSceneDragCoordinator referenceDrag;
    private java.awt.dnd.DropTarget referenceDrop;
    private CompletableFuture<epidemic.editor.GraphSceneDragCoordinator.GraphHit> referenceHit;
    private boolean referenceAllowed;
    private CompletableFuture<Void> inlineFlush;
    private final JPanel component = new JPanel(new BorderLayout());
    private JBCefBrowser browser;
    private JBCefJSQuery query;
    private javax.swing.Timer timer;
    private boolean disposed, pending;
    private long rateStart; private int rateCount;
    private long published = -1;
    private int publishedFlags = -1;
    private String publishedDiagnostic = "";
    private String diagnostic = "";
    private long diagnosticRevision=-1;
    private CompletableFuture<Bridge.Reply> smokeResult;
    private CompletableFuture<String> smokeReport;
    private final CompletableFuture<Void> readiness = new CompletableFuture<>();

    GraphEditor(Bridge.DocumentKey key, Supplier<Bridge.Reply> latest, Commands commands,epidemic.editor.GraphSceneDragCoordinator referenceDrag) {
        this.key = key; this.latest = latest; this.commands = commands;this.referenceDrag=referenceDrag;
        try {
            if (!JBCefApp.isSupported()) throw new IllegalStateException("JBR with JCEF is required");
            if (!epidemic.launcher.NativeLauncher.available())
                throw new IllegalStateException("Start Graph with the native sandbox launcher");
            // Initialize the real Platform proxy service outside JBCefApp Holder's clinit.
            com.intellij.util.net.HttpConfigurable.getInstance();
            if (JBCefApp.getInstance().getCefSettings().no_sandbox || org.cef.CefApp.isRemoteEnabled())
                throw new IllegalStateException("JCEF sandbox configuration rejected");
            browser = new JBCefBrowser();
            Disposer.register(this, browser);
            query = JBCefJSQuery.create((JBCefBrowserBase) browser);
            Disposer.register(browser, query);
            browser.getJBCefClient().addRequestHandler(new CefRequestHandlerAdapter() {
                @Override public boolean onBeforeBrowse(CefBrowser b, CefFrame frame, CefRequest request,
                                                          boolean gesture, boolean redirect) {
                    if (Boolean.getBoolean("epidemic.ui.platform.smoke")) LOG.info("Graph browse " + request.getURL());
                    return !origin.equals(request.getURL());
                }
                @Override public boolean onOpenURLFromTab(CefBrowser b, CefFrame frame, String url, boolean gesture) {
                    return true;
                }
                @Override public CefResourceRequestHandler getResourceRequestHandler(CefBrowser b, CefFrame frame,
                    CefRequest request, boolean navigation, boolean download, String initiator, BoolRef disable) {
                    if (origin.equals(request.getURL()) && !download) return null;
                    disable.set(true);
                    return new CefResourceRequestHandlerAdapter() {
                        @Override public boolean onBeforeResourceLoad(CefBrowser b, CefFrame f, CefRequest r) { return true; }
                        @Override public void onProtocolExecution(CefBrowser b, CefFrame f, CefRequest r, BoolRef allow) { allow.set(false); }
                    };
                }
            }, browser.getCefBrowser());
            browser.getJBCefClient().addLifeSpanHandler(new CefLifeSpanHandlerAdapter() {
                @Override public boolean onBeforePopup(CefBrowser b, CefFrame f, String url, String target) { return true; }
            }, browser.getCefBrowser());
            browser.getJBCefClient().addContextMenuHandler(new CefContextMenuHandlerAdapter() {
                @Override public void onBeforeContextMenu(CefBrowser b, CefFrame f,
                    org.cef.callback.CefContextMenuParams params, org.cef.callback.CefMenuModel model) { model.clear(); }
            }, browser.getCefBrowser());
            browser.getJBCefClient().addDisplayHandler(new CefDisplayHandlerAdapter() {
                @Override public boolean onConsoleMessage(CefBrowser b, org.cef.CefSettings.LogSeverity level,
                    String message, String source, int line) {
                    LOG.info("Graph console " + message); return false;
                }
            }, browser.getCefBrowser());
            query.addHandler(payload -> {
                if (disposed) return null;
                // Do not retain oversized text on the EDT; still publish a typed rejection.
                String bounded=payload.length()>65536?null:payload;
                ApplicationManager.getApplication().invokeLater(() -> request(bounded));
                return null;
            });
            String script;
            try (var stream = Objects.requireNonNull(GraphEditor.class.getResourceAsStream("/graph/canvas.js"))) {
                script = new String(stream.readAllBytes(), StandardCharsets.UTF_8);
            }
            String bootstrap = "window.graphNonce=" + JSON.toJson(nonce) + ";window.nativeGraph=function(payload){"
                + query.inject("payload") + "};";
            script = bootstrap + script;
            String sha = Base64.getEncoder().encodeToString(MessageDigest.getInstance("SHA-256").digest(script.getBytes(StandardCharsets.UTF_8)));
            String html, style;
            try (var stream = Objects.requireNonNull(GraphEditor.class.getResourceAsStream("/graph/graph.html"))) {
                html = new String(stream.readAllBytes(),StandardCharsets.UTF_8);
            }
            try (var stream = Objects.requireNonNull(GraphEditor.class.getResourceAsStream("/graph/graph.css"))) {
                style = new String(stream.readAllBytes(),StandardCharsets.UTF_8);
            }
            html = html.replace("%HASH%",sha).replace("%STYLE%",style).replace("%SCRIPT%",script);
            component.add(browser.getComponent());
            final Runnable clearReference=()->{referenceAllowed=false;referenceFeedback("");};
            referenceDrop=new java.awt.dnd.DropTarget(browser.getCefBrowser().getUIComponent(),java.awt.dnd.DnDConstants.ACTION_COPY,new java.awt.dnd.DropTargetAdapter(){
                @Override public void dragEnter(java.awt.dnd.DropTargetDragEvent e){dragOver(e);}
                @Override public void dragOver(java.awt.dnd.DropTargetDragEvent e){
                    if(e.isDataFlavorSupported(epidemic.editor.GraphTools.PALETTE_FLAVOR)){e.acceptDrag(java.awt.dnd.DnDConstants.ACTION_COPY);return;}
                    if(!referenceDrag.fromScene(e.getTransferable())){e.rejectDrag();return;}
                    if(referenceHit==null||referenceHit.isDone()) {
                        var transfer=e.getTransferable();
                        referenceHit(e.getLocation()).thenAccept(hit->referenceDrag.hoverGraph(transfer,hit,(valid,reason)->{referenceAllowed=valid;referenceFeedback(reason);},clearReference));
                    }
                    if(referenceAllowed)e.acceptDrag(java.awt.dnd.DnDConstants.ACTION_COPY);else e.rejectDrag();
                }
                @Override public void dragExit(java.awt.dnd.DropTargetEvent e){referenceAllowed=false;referenceFeedback("");}
                @Override public void drop(java.awt.dnd.DropTargetDropEvent e){
                    if(e.isDataFlavorSupported(epidemic.editor.GraphTools.PALETTE_FLAVOR)){
                        try{var recipe=(epidemic.editor.GraphTools.PaletteDrag)e.getTransferable().getTransferData(epidemic.editor.GraphTools.PALETTE_FLAVOR);var current=latest.get();
                            if(current==null||current.generation()!=recipe.generation()||current.snapshot().project()!=recipe.project()||current.snapshot().revision()!=recipe.revision()){e.rejectDrop();return;}
                            e.acceptDrop(java.awt.dnd.DnDConstants.ACTION_COPY);
                            referenceHit(e.getLocation()).thenAccept(hit->{var graph=current.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();int index=-1;for(int i=0;i<graph.catalogue().size();++i)if(graph.catalogue().get(i).type()==recipe.type())index=i;
                                if(index>=0)commands.execute(recipe.generation(),recipe.revision(),38,"create",Long.toUnsignedString(key.id()),""+index,""+hit.x(),""+hit.y());
                            });e.dropComplete(true);
                        }catch(Exception error){e.rejectDrop();}return;
                    }
                    if(!referenceDrag.fromScene(e.getTransferable())){e.rejectDrop();return;}
                    e.acceptDrop(java.awt.dnd.DnDConstants.ACTION_COPY);
                    referenceDrag.dropGraph(e.getTransferable(),referenceHit(e.getLocation()),browser.getCefBrowser().getUIComponent());
                    e.dropComplete(true);referenceFeedback("");
                }
            });
            browser.loadHTML(html, origin);
            timer = new javax.swing.Timer(300, event -> refresh());
            timer.start();
        } catch (Exception error) {
            if (browser != null) Disposer.dispose(browser);
            component.removeAll(); component.add(new JLabel("Graph unavailable: " + error.getMessage()));
        }
    }
    CompletableFuture<Void> flushLiterals(){
        if(disposed||browser==null||!readiness.isDone()||!component.isShowing())return CompletableFuture.completedFuture(null);
        if(inlineFlush!=null&&!inlineFlush.isDone())return inlineFlush;
        inlineFlush=new CompletableFuture<>();browser.getCefBrowser().executeJavaScript("window.graphFlushInline?.();",origin,0);return inlineFlush.orTimeout(5,java.util.concurrent.TimeUnit.SECONDS);
    }
    JComponent component() { return component; }
    void shortcut(String action){if(!disposed&&browser!=null&&!browser.isDisposed())browser.getCefBrowser().executeJavaScript("window.graphAction?.("+JSON.toJson(action)+");",origin,0);}

    private void referenceFeedback(String value) {
        if(!disposed&&browser!=null&&!browser.isDisposed())browser.getCefBrowser().executeJavaScript("window.graphReferenceFeedback?.("+JSON.toJson(value)+");",origin,0);
    }
    private CompletableFuture<epidemic.editor.GraphSceneDragCoordinator.GraphHit> referenceHit(java.awt.Point point) {
        if(referenceHit!=null&&!referenceHit.isDone())referenceHit.completeExceptionally(new java.util.concurrent.CancellationException());
        referenceHit=new CompletableFuture<>();
        var ui=browser.getCefBrowser().getUIComponent();
        browser.getCefBrowser().executeJavaScript("window.graphReferenceHit("+point.x+","+point.y+","+ui.getWidth()+","+ui.getHeight()+");",origin,0);
        return referenceHit.orTimeout(3,java.util.concurrent.TimeUnit.SECONDS);
    }
    JComponent preferredFocus() { return browser == null || browser.isDisposed() ? component : browser.getComponent(); }
    Bridge.DocumentKey identity() { return key; }
    private long currentRevision(){var reply=latest.get();return reply==null||reply.snapshot()==null?-1:reply.snapshot().revision();}
    private static void checkJson(String payload) {
        if (payload == null || payload.length()>65536) throw new IllegalArgumentException("ui.graph_message_limit");
        int depth=0;boolean string=false,escape=false;
        for(int i=0;i<payload.length();++i){char c=payload.charAt(i);
            if(string){if(escape)escape=false;else if(c=='\\')escape=true;else if(c=='"')string=false;}
            else if(c=='"')string=true;
            else if(c=='{'||c=='['){if(++depth>6)throw new IllegalArgumentException("ui.graph_depth_limit");}
            else if(c=='}'||c==']')--depth;
        }
    }
    private void request(String payload) {
        if (Boolean.getBoolean("epidemic.ui.platform.smoke")) LOG.info("Graph query url=" + (browser == null ? "null" : browser.getCefBrowser().getURL()) + " pending=" + pending);
        if (disposed || browser == null || !origin.equals(browser.getCefBrowser().getURL())) return;
        if(pending&&(inlineFlush==null||inlineFlush.isDone()))return;
        try {
            checkJson(payload);
            long now=System.nanoTime();if(now-rateStart>1_000_000_000L){rateStart=now;rateCount=0;}
            if(++rateCount>60)throw new IllegalArgumentException("ui.graph_rate_limit");
            JsonObject message = JsonParser.parseString(payload).getAsJsonObject();
            if(Boolean.getBoolean("epidemic.ui.platform.smoke"))LOG.info("Graph action="+message.get("action").getAsString());
            if (!nonce.equals(message.get("nonce").getAsString())) throw new IllegalArgumentException("ui.graph_nonce");
            if(pending&&!"flush-inline".equals(message.get("action").getAsString()))return;
            if ("__ready".equals(message.get("action").getAsString())) {
                readiness.complete(null); published = -1; runtimeVersion="";refresh(); return;
            }
            if (Boolean.getBoolean("epidemic.ui.platform.smoke") && "__report".equals(message.get("action").getAsString()) && smokeReport != null) {
                smokeReport.complete(message.get("result").getAsString()); return;
            }
            if("flush-inline".equals(message.get("action").getAsString())) {
                if(inlineFlush==null||inlineFlush.isDone())throw new IllegalArgumentException("ui.graph_flush");
                var items=message.getAsJsonArray("args");if(items.size()>64)throw new IllegalArgumentException("ui.graph_flush_limit");
                CompletableFuture<Bridge.Reply> chain=CompletableFuture.completedFuture(latest.get());
                for(var item:items){var parts=item.getAsJsonArray();if(parts.size()!=4)throw new IllegalArgumentException("ui.graph_flush");if(!parts.get(3).getAsString().equals(Long.toUnsignedString(latest.get().snapshot().revision())))throw new IllegalArgumentException("ui.stale_revision");String action=parts.get(0).getAsString(),id=parts.get(1).getAsString(),value=parts.get(2).getAsString();if(!action.equals("literal")&&!action.equals("inputvalue"))throw new IllegalArgumentException("ui.graph_action");
                    chain=chain.thenComposeAsync(previous->commands.execute(key.generation(),previous.snapshot().revision(),38,action,Long.toUnsignedString(key.id()),id,value),task->ApplicationManager.getApplication().invokeLater(task));
                }
                var completion=inlineFlush;chain.whenComplete((r,error)->{if(error!=null)completion.completeExceptionally(error);else completion.complete(null);});return;
            }
            Bridge.Reply reply = latest.get();
            if (reply == null || key.resolve(reply.generation(), reply.snapshot()).isEmpty())
                throw new IllegalArgumentException("ui.graph_document_stale");
            long revision = Long.parseUnsignedLong(message.get("revision").getAsString());
            if (revision != reply.snapshot().revision()) throw new IllegalArgumentException("ui.stale_revision");
            if (reply.snapshot().playing()) throw new IllegalArgumentException("ui.playing_readonly");
            String action = message.get("action").getAsString();
            var values = message.getAsJsonArray("args");
            if(action.equals("text-shortcut")) {
                if(values.size()!=1)throw new IllegalArgumentException("ui.graph_arguments");
                var frame=browser.getCefBrowser().getFocusedFrame();
                switch(values.get(0).getAsString()){case "copy"->frame.copy();case "cut"->frame.cut();case "paste"->frame.paste();default->throw new IllegalArgumentException("ui.graph_action");}return;
            }
            if(action.equals("reference-hit")) {
                if(referenceHit==null||referenceHit.isDone()||values.size()!=3)throw new IllegalArgumentException("ui.reference_session");
                long port=Long.parseUnsignedLong(values.get(0).getAsString());
                double x=Double.parseDouble(values.get(1).getAsString()),y=Double.parseDouble(values.get(2).getAsString());
                if(!Double.isFinite(x)||!Double.isFinite(y)||Math.abs(x)>1000000||Math.abs(y)>1000000)throw new IllegalArgumentException("ui.reference_position");
                referenceHit.complete(new epidemic.editor.GraphSceneDragCoordinator.GraphHit(key.id(),port,x,y));return;
            }
            if(action.equals("reference-start")) {
                if(values.size()!=3)throw new IllegalArgumentException("ui.reference_arguments");
                long port=Long.parseUnsignedLong(values.get(0).getAsString());
                var graph=reply.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();
                var node=graph.nodes().stream().filter(n->n.ports().stream().anyMatch(p->p.id()==port&&p.kind()==0&&p.direction()==0)).findFirst().orElseThrow();
                referenceDrag.startGraph(browser.getCefBrowser().getUIComponent(),new epidemic.editor.GraphSceneDragCoordinator.GraphHit(key.id(),port,node.x()-260,node.y()));return;
            }
            int expected = switch (action) { case "create", "move", "reconnect" -> 3; case "connect", "literal", "inputvalue" -> 2;
                case "delete", "disconnect", "moves", "deletebulk", "copy", "cut", "duplicate", "select" -> 1;
                case "paste", "group" -> 2; case "settings" -> 6;
                case "save", "undo", "redo", "compile", "run", "uses" -> 0;
                default -> throw new IllegalArgumentException("ui.graph_action"); };
            if (values.size() != expected) throw new IllegalArgumentException("ui.graph_arguments");
            int operation = switch (action) { case "save" -> 6; case "undo" -> 7; case "redo" -> 8; default -> 38; };
            List<String> args = new ArrayList<>();
            if (operation == 38) { args.add(action); args.add(Long.toUnsignedString(key.id())); }
            for (JsonElement value : values) {
                if (!value.isJsonPrimitive() || !value.getAsJsonPrimitive().isString() || value.getAsString().length() > 16384)
                    throw new IllegalArgumentException("ui.graph_argument");
                args.add(value.getAsString());
            }
            pending = true;
            commands.execute(key.generation(), revision, operation, args.toArray(String[]::new))
                .whenComplete((result, error) -> ApplicationManager.getApplication().invokeLater(() -> {
                    pending = false;
                    if (smokeResult != null && !smokeResult.isDone()) {
                        if (error != null) smokeResult.completeExceptionally(error);
                        else smokeResult.complete(result);
                    }
                    diagnostic = error == null ? result.error() : error.toString();
                    diagnosticRevision=currentRevision();
                    published = -1; refresh();
                }));
        } catch (RuntimeException error) {
            if (smokeResult != null && !smokeResult.isDone()) smokeResult.completeExceptionally(error);
            if(inlineFlush!=null&&!inlineFlush.isDone())inlineFlush.completeExceptionally(error);
            diagnostic = error.getMessage();diagnosticRevision=currentRevision(); published = -1; refresh();
        }
    }
    void refresh() {
        if (disposed || browser == null || browser.isDisposed() || !readiness.isDone()) return;
        var reply = latest.get();
        if (reply == null || key.resolve(reply.generation(), reply.snapshot()).isEmpty()) return;
        var snapshot = reply.snapshot();
        if(diagnosticRevision!=snapshot.revision())diagnostic="";
        var graph = snapshot.graphs().stream().filter(g -> g.document() == key.id()).findFirst().orElse(null);
        if (graph == null) return;
        refreshRuntime(reply.runtime());
        String version = diagnostic+graph.report()+snapshot.selectionRevision();
        if (published==snapshot.revision() && publishedFlags==snapshot.flags() && publishedDiagnostic.equals(version)) return;
        JsonObject dto = new JsonObject(); dto.addProperty("revision", Long.toUnsignedString(snapshot.revision()));
        dto.addProperty("readonly", snapshot.playing()); dto.addProperty("error", diagnostic);
        dto.addProperty("schemaRevision","1");dto.addProperty("report",graph.report());
        dto.addProperty("selected",snapshot.selection().document()==key.id() && snapshot.selection().kind()==3 ? Long.toUnsignedString(snapshot.selection().element()) : "0");
        JsonArray catalogue=new JsonArray();
        for(var recipe:graph.catalogue()){
            JsonObject r=new JsonObject();r.addProperty("name",recipe.name());r.addProperty("category",recipe.category());
            r.addProperty("type",Long.toUnsignedString(recipe.type()));r.addProperty("operation",recipe.operation());catalogue.add(r);
        }
        dto.add("catalogue",catalogue);
        JsonArray nodes = new JsonArray(), edges = new JsonArray();
        for (var node : graph.nodes()) {
            JsonObject n = new JsonObject(); n.addProperty("id", Long.toUnsignedString(node.id()));
            n.addProperty("name", node.name()); n.addProperty("x", node.x()); n.addProperty("y", node.y());
            n.addProperty("type",Long.toUnsignedString(node.type()));n.addProperty("previewDefault",Bridge.inputText(node));
            n.addProperty("comment",node.comment());n.addProperty("group",node.group());n.addProperty("markers",node.markers());
            JsonArray ports = new JsonArray();
            for (var port : node.ports()) {
                JsonObject p = new JsonObject(); p.addProperty("id", Long.toUnsignedString(port.id()));
                p.addProperty("name", port.name()); p.addProperty("kind", port.kind()); p.addProperty("direction", port.direction());
                p.addProperty("type", Long.toUnsignedString(port.type()));if(port.defaultValue()!=null)p.addProperty("defaultValue",Bridge.numericText(port.type(),port.defaultValue())); ports.add(p);
            }
            n.addProperty("reference",node.reference()!=null);n.addProperty("referenceAvailable",node.referenceAvailable());
            n.addProperty("referenceLabel",node.referenceLabel());n.addProperty("referenceReason",node.referenceReason());
            n.add("ports", ports); nodes.add(n);
        }
        for (var edge : graph.edges()) {
            JsonObject e = new JsonObject(); e.addProperty("id", Long.toUnsignedString(edge.id()));
            e.addProperty("output", Long.toUnsignedString(edge.output())); e.addProperty("input", Long.toUnsignedString(edge.input())); edges.add(e);
        }
        dto.add("nodes", nodes); dto.add("edges", edges);
        browser.getCefBrowser().executeJavaScript("window.graphSnapshot("+JSON.toJson(dto)+");",origin,0);
        published=snapshot.revision();publishedFlags=snapshot.flags();publishedDiagnostic=version;
    }

    private String runtimeVersion="";
    private void refreshRuntime(Bridge.RuntimeSession runtime){
        String version=runtime.connection()+":"+runtime.session()+":"+runtime.tick()+":"+runtime.graphSteps()+":"+runtime.phase()+":"+runtime.breakpoints().hashCode();
        if(version.equals(runtimeVersion))return;runtimeVersion=version;
        JsonObject dto=new JsonObject();dto.addProperty("session",Long.toUnsignedString(runtime.session()));dto.addProperty("connection",Long.toUnsignedString(runtime.connection()));dto.addProperty("generation",Long.toUnsignedString(runtime.generation()));dto.addProperty("tick",Long.toUnsignedString(runtime.tick()));dto.addProperty("phase",runtime.state());
        JsonArray nodes=new JsonArray();for(var activation:runtime.activations())if(activation.graph()==key.id()){
            JsonObject node=new JsonObject();node.addProperty("node",Long.toUnsignedString(activation.node()));node.addProperty("activation",Long.toUnsignedString(activation.id()));node.addProperty("state",activation.state());node.addProperty("instructions",activation.instructions());node.addProperty("nativeCalls",activation.nativeCalls());
            JsonArray values=new JsonArray();for(var value:activation.values()){JsonObject port=new JsonObject();port.addProperty("port",Long.toUnsignedString(value.port()));port.addProperty("type",value.type());port.addProperty("value",value.value());values.add(port);}node.add("values",values);nodes.add(node);
        }dto.add("nodes",nodes);JsonArray breakpoints=new JsonArray();for(var point:runtime.breakpoints())if(point.graph()==key.id())breakpoints.add(Long.toUnsignedString(point.node()));dto.add("breakpoints",breakpoints);
        browser.getCefBrowser().executeJavaScript("window.graphRuntime?.("+JSON.toJson(dto)+");",origin,0);
    }
    CompletableFuture<JsonObject> smokeRuntimeReport(){return smokeDom("return {session:document.getElementById('runtime-state').dataset.session||'',tick:document.getElementById('runtime-state').dataset.tick||'',active:document.querySelectorAll('.runtime-active').length,readonly:document.querySelector('[data-command=compile]').disabled};");}
    // Programmatic qualification uses the same browser JSQuery and command guard.
    CompletableFuture<Bridge.Reply> smokeCommand(String action, String... args) {
        if (!Boolean.getBoolean("epidemic.ui.platform.smoke")) throw new IllegalStateException("Qualification only");
        if (!readiness.isDone())
            return readiness.orTimeout(20, java.util.concurrent.TimeUnit.SECONDS).thenCompose(ignored -> smokeCommand(action, args));
        smokeResult = new CompletableFuture<>();
        JsonObject value = new JsonObject(); value.addProperty("nonce", nonce);
        value.addProperty("revision", Long.toUnsignedString(latest.get().snapshot().revision()));
        value.addProperty("action", action); value.add("args", JSON.toJsonTree(args));
        browser.getCefBrowser().executeJavaScript("console.info(\"Graph qualification ready=\"+window.graphCanvasReady+\" url=\"+location.href);window.nativeGraph(" + JSON.toJson(JSON.toJson(value)) + ");", origin, 0);
        return smokeResult.orTimeout(15, java.util.concurrent.TimeUnit.SECONDS);
    }
    CompletableFuture<String> smokeSecurity() {
        if (!Boolean.getBoolean("epidemic.ui.platform.smoke")) throw new IllegalStateException("Qualification only");
        smokeReport = new CompletableFuture<>();
        String js = "(async()=>{const urls=['file:///C:/Windows/win.ini','https://example.invalid/graph','http://127.0.0.1:9/graph'];"
            + "const denied=await Promise.all(urls.map(url=>fetch(url).then(()=>false,()=>true)));"
            + "let evalDenied=false;try{eval('1+1')}catch(e){evalDenied=true}"
            + "window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({denied,evalDenied})}));})()";
        browser.getCefBrowser().executeJavaScript(js, origin, 0);
        return smokeReport.orTimeout(10, java.util.concurrent.TimeUnit.SECONDS);
    }
    private void smokeEscapeKeys(){
        var input=browser.getCefBrowser().getUIComponent();
        for(int kind:new int[]{java.awt.event.KeyEvent.KEY_PRESSED,java.awt.event.KeyEvent.KEY_RELEASED}){
            var event=new java.awt.event.KeyEvent(input,kind,System.currentTimeMillis(),0,java.awt.event.KeyEvent.VK_ESCAPE,(char)27);
            try{var raw=java.awt.event.KeyEvent.class.getDeclaredField("rawCode");raw.setAccessible(true);raw.setLong(event,27);
                var scan=java.awt.event.KeyEvent.class.getDeclaredField("scancode");scan.setAccessible(true);scan.setLong(event,1);
            }catch(ReflectiveOperationException error){throw new java.util.concurrent.CompletionException(error);}
            browser.getCefBrowser().sendKeyEvent(event);
        }
    }
    private CompletableFuture<JsonObject> smokeSocket(String port){
        smokeReport=new CompletableFuture<>();
        browser.getCefBrowser().executeJavaScript("{const e=document.querySelector('[data-port-id=\""+port+"\"]');const r=e?.getBoundingClientRect();window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({x:r?.left+r?.width/2,y:r?.top+r?.height/2,gesture:window.graphGestureState(),node:e?.closest('[data-node-id]')?.dataset.nodeId,root:e?.closest('[data-node-id]')?.getBoundingClientRect().toJSON(),canvas:document.getElementById('graph').getBoundingClientRect().toJSON()})}));}",origin,0);
        return smokeReport.orTimeout(10,java.util.concurrent.TimeUnit.SECONDS).thenApply(value->JsonParser.parseString(value).getAsJsonObject());
    }
    CompletableFuture<Void> smokeConnectionCancel(){
        var state=latest.get().snapshot();var graph=state.graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();
        var edge=graph.edges().getFirst();var node=graph.nodes().stream().filter(n->n.ports().stream().anyMatch(p->p.id()==edge.input())).findFirst().orElseThrow();
        browser.getCefBrowser().executeJavaScript("window.graphFocus("+JSON.toJson(Long.toUnsignedString(node.id()))+");",origin,0);
        return pause(300).thenCompose(ignored->smokeSocket(Long.toUnsignedString(edge.input()))).thenCompose(bounds->{
            int x=(int)bounds.get("x").getAsDouble(),y=(int)bounds.get("y").getAsDouble();browser.getCefBrowser().setFocus(true);
            mouse(java.awt.event.MouseEvent.MOUSE_MOVED,x,y,false);mouse(java.awt.event.MouseEvent.MOUSE_PRESSED,x,y,true);
            return pause(150).thenCompose(ignored->smokeSocket(Long.toUnsignedString(edge.input()))).thenCompose(picked->{
                if(!picked.getAsJsonObject("gesture").get("connection").getAsBoolean()||!picked.getAsJsonObject("gesture").get("reconnect").getAsString().equals(Long.toUnsignedString(edge.id())))throw new IllegalStateException("Existing edge pickup missed: "+picked);
                if(latest.get().snapshot().revision()!=state.revision())throw new IllegalStateException("Picking existing edge committed a disconnect");
                smokeEscapeKeys();mouse(java.awt.event.MouseEvent.MOUSE_RELEASED,x,y,false);return pause(200);
            });
        }).thenRun(()->{var now=latest.get().snapshot();var current=now.graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();
            if(now.revision()!=state.revision()||!current.edges().equals(graph.edges()))throw new IllegalStateException("Escape lost existing edge");
        });
    }
    CompletableFuture<Void> smokeSelectAll(){
        if(!Boolean.getBoolean("epidemic.ui.platform.smoke"))throw new IllegalStateException("Qualification only");
        browser.getCefBrowser().executeJavaScript("document.getElementById('graph').focus();document.dispatchEvent(new KeyboardEvent('keydown',{key:'a',ctrlKey:true,bubbles:true}));document.dispatchEvent(new KeyboardEvent('keyup',{key:'Control',ctrlKey:false,bubbles:true}));",origin,0);
        return pause(150);
    }
    CompletableFuture<String> smokeHeap() {
        if(!Boolean.getBoolean("epidemic.ui.platform.smoke"))throw new IllegalStateException("Qualification only");
        var tools=new org.cef.browser.CefDevToolsClient(browser.getCefBrowser());
        return tools.executeDevToolsMethod("Runtime.getHeapUsage").orTimeout(10,java.util.concurrent.TimeUnit.SECONDS)
            .whenComplete((result,error)->tools.close());
    }
    CompletableFuture<Bridge.Reply> smokePayload(String raw) {
        if(!Boolean.getBoolean("epidemic.ui.platform.smoke"))throw new IllegalStateException("Qualification only");
        smokeResult=new CompletableFuture<>();
        browser.getCefBrowser().executeJavaScript("window.nativeGraph("+JSON.toJson(raw)+");",origin,0);
        return smokeResult.orTimeout(10,java.util.concurrent.TimeUnit.SECONDS);
    }
    CompletableFuture<String> smokeCanvasReport(int expected,int attempt) {
        if(!readiness.isDone())return readiness.orTimeout(20,java.util.concurrent.TimeUnit.SECONDS).thenCompose(ignored->smokeCanvasReport(expected,attempt));
        smokeReport=new CompletableFuture<>();
        browser.getCefBrowser().executeJavaScript("window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({count:document.querySelectorAll('[data-testid=node]').length,injected:!!window.graphInjected,images:document.querySelectorAll('img').length,literal:document.body.textContent.includes('<img src=x'),revision:document.documentElement.dataset.graphReady,ready:!window.graphGestureState().busy,metrics:window.graphMetrics})}));",origin,0);
        return smokeReport.orTimeout(10,java.util.concurrent.TimeUnit.SECONDS).thenCompose(value->{
            var state=JsonParser.parseString(value).getAsJsonObject();
            if(state.get("count").getAsInt()==expected&&state.get("ready").getAsBoolean()&&state.has("metrics")&&state.getAsJsonObject("metrics").get("nodes").getAsInt()==expected&&state.get("revision").getAsString().equals(Long.toUnsignedString(latest.get().snapshot().revision())))return CompletableFuture.completedFuture(value);
            if(attempt>=150)return CompletableFuture.failedFuture(new IllegalStateException("Rete projection timeout: "+value));
            return pause(200).thenCompose(ignored->smokeCanvasReport(expected,attempt+1));
        });
    }
    CompletableFuture<String> smokeOversizedSnapshot() {
        smokeReport=new CompletableFuture<>();
        browser.getCefBrowser().executeJavaScript("{const before=document.querySelectorAll('[data-testid=node]').length;window.graphSnapshot({revision:'1',nodes:Array(2000).fill({}),edges:[],catalogue:[]});window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({before,after:document.querySelectorAll('[data-testid=node]').length,status:document.getElementById('status').textContent})}));}",origin,0);
        return smokeReport.orTimeout(10,java.util.concurrent.TimeUnit.SECONDS);
    }
    private CompletableFuture<Void> pause(int milliseconds) {
        var result = new CompletableFuture<Void>();
        var delay = new javax.swing.Timer(milliseconds, event -> {
            if (disposed) result.completeExceptionally(new java.util.concurrent.CancellationException());
            else result.complete(null);
        });
        delay.setRepeats(false); delay.start(); return result;
    }
    private CompletableFuture<JsonObject> smokeNodeBounds() {
        smokeReport = new CompletableFuture<>();
        browser.getCefBrowser().executeJavaScript(
            "{const n=document.querySelector('[data-testid=node]');const r=(n.querySelector('[data-testid=title]')||n).getBoundingClientRect();"
            + "window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({x:r.left+r.width/2,y:r.top+r.height/2,width:r.width,height:r.height,keys:window.__qualificationKeys})}));}", origin, 0);
        return smokeReport.orTimeout(10, java.util.concurrent.TimeUnit.SECONDS).thenApply(value -> JsonParser.parseString(value).getAsJsonObject());
    }
    private void mouse(int event, int x, int y, boolean down) {
        var input = browser.getCefBrowser().getUIComponent();
        int button = (event == java.awt.event.MouseEvent.MOUSE_DRAGGED || event == java.awt.event.MouseEvent.MOUSE_MOVED) ? java.awt.event.MouseEvent.NOBUTTON : java.awt.event.MouseEvent.BUTTON1;
        browser.getCefBrowser().sendMouseEvent(new java.awt.event.MouseEvent(input, event, System.currentTimeMillis(),
            down ? java.awt.event.InputEvent.BUTTON1_DOWN_MASK : 0, x, y, 1, false, button));
    }
    CompletableFuture<Void> smokeDragCancel() {
        if (!Boolean.getBoolean("epidemic.ui.platform.smoke")) throw new IllegalStateException("Qualification only");
        browser.getCefBrowser().executeJavaScript("window.graphFocus("+JSON.toJson(Long.toUnsignedString(latest.get().snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow().nodes().getFirst().id()))+");window.__qualificationKeys=[];document.addEventListener('keydown',e=>window.__qualificationKeys.push(e.key),true);", origin, 0);
        com.intellij.openapi.wm.IdeFocusManager.getGlobalInstance().requestFocus(browser.getComponent(), true);
        long revision = latest.get().snapshot().revision();
        return pause(350).thenCompose(ignored -> smokeNodeBounds()).thenCompose(bounds -> {
            int x = (int)bounds.get("x").getAsDouble(), y = (int)bounds.get("y").getAsDouble();
            browser.getCefBrowser().setFocus(true);
            mouse(java.awt.event.MouseEvent.MOUSE_MOVED, x, y, false);
            mouse(java.awt.event.MouseEvent.MOUSE_PRESSED, x, y, true);
            mouse(java.awt.event.MouseEvent.MOUSE_DRAGGED, x + 24, y + 24, true);
            return pause(150).thenCompose(ignored -> smokeNodeBounds()).thenCompose(moved -> {
                if (Math.abs(moved.get("x").getAsDouble() - bounds.get("x").getAsDouble()) < 1)
                    throw new IllegalStateException("Input missed the node drag preview: "+bounds+" -> "+moved);
                // Pinned JCEF Windows consumes AWT rawCode/scancode.
                smokeEscapeKeys();
                return pause(150);
            }).thenCompose(ignored -> {
                mouse(java.awt.event.MouseEvent.MOUSE_RELEASED, x + 24, y + 24, false);
                return pause(350).thenCompose(ignoredAgain -> smokeNodeBounds()).thenAccept(restored -> {
                    if (Math.abs(restored.get("x").getAsDouble() - bounds.get("x").getAsDouble()) > 1 ||
                        Math.abs(restored.get("y").getAsDouble() - bounds.get("y").getAsDouble()) > 1)
                        throw new IllegalStateException("Escape did not restore the native node position");
                });
            });
        }).thenCompose(ignored -> {
            return smokeNodeBounds().thenApply(bounds -> {
                if (!bounds.getAsJsonArray("keys").toString().contains("\"Escape\"")) throw new IllegalStateException("JCEF did not receive Escape: " + bounds);
                if (latest.get().snapshot().revision() != revision) throw new IllegalStateException("Escape committed a native drag: " + bounds);
                return bounds;
            });
        }).thenCompose(bounds -> {
            int x = (int)bounds.get("x").getAsDouble(), y = (int)bounds.get("y").getAsDouble();
            mouse(java.awt.event.MouseEvent.MOUSE_MOVED, x, y, false);
            mouse(java.awt.event.MouseEvent.MOUSE_PRESSED, x, y, true);
            mouse(java.awt.event.MouseEvent.MOUSE_DRAGGED, x + 32, y + 32, true);
            return pause(150).thenCompose(ignored -> {
                mouse(java.awt.event.MouseEvent.MOUSE_RELEASED, x + 32, y + 32, false);
                return pause(650);
            });
        }).thenCompose(ignored -> smokeNodeBounds()).thenAccept(bounds -> {
            if (latest.get().snapshot().revision() != revision + 1)
                throw new IllegalStateException("Drag after Escape did not produce exactly one native edit: " + revision + " -> " + latest.get().snapshot().revision() + " " + bounds);
        });
    }
    private CompletableFuture<Bridge.Reply> smokeShortcut(String action){
        int nodes=latest.get().snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow().nodes().size();
        return smokeCanvasReport(nodes,0).thenCompose(ignored->{smokeResult=new CompletableFuture<>();shortcut(action);return smokeResult.orTimeout(15,java.util.concurrent.TimeUnit.SECONDS).exceptionally(error->{throw new java.util.concurrent.CompletionException("Graph shortcut "+action,error);});});
    }
    CompletableFuture<Void> smokeClipboardAndConnection(){
        var before=latest.get().snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();int count=before.nodes().size();
        return smokeSelectAll().thenCompose(ignored->smokeShortcut("copy")).thenCompose(ignored->smokeShortcut("paste")).thenCompose(pasted->{
            int added=pasted.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow().nodes().size()-count;
            if(added!=64)throw new IllegalStateException("Graph shortcut Paste missed selected fragment: "+added);
            return pause(400).thenCompose(ignored->smokeShortcut("cut"));
        }).thenCompose(cut->{if(cut.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow().nodes().size()!=count)throw new IllegalStateException("Cut didn't delete pasted fragment");return smokeCommand("undo");})
        .thenCompose(ignored->smokeCommand("undo")).thenCompose(ignored->{
            var state=latest.get().snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();var edge=state.edges().getFirst();
            var node=state.nodes().stream().filter(n->n.ports().stream().anyMatch(p->p.id()==edge.input())).findFirst().orElseThrow();
            return smokeCanvasReport(state.nodes().size(),0).thenCompose(ready->{
                browser.getCefBrowser().setFocus(true);
                return pause(150);
            }).thenCompose(ready->{
                return smokeDom("const focus=await window.graphFocus("+JSON.toJson(Long.toUnsignedString(node.id()))+");await new Promise(r=>requestAnimationFrame(()=>requestAnimationFrame(r)));return focus;");
            }).thenCompose(focus->smokeSocket(Long.toUnsignedString(edge.input())).thenApply(bounds->{bounds.add("focus",focus);return bounds;})).thenCompose(bounds->{
                if(bounds.get("x").getAsDouble()<0||bounds.get("y").getAsDouble()>bounds.getAsJsonObject("canvas").get("bottom").getAsDouble())throw new IllegalStateException("Delete focus missed socket: "+bounds);
                int x=(int)bounds.get("x").getAsDouble(),y=(int)bounds.get("y").getAsDouble();mouse(java.awt.event.MouseEvent.MOUSE_MOVED,x,y,false);mouse(java.awt.event.MouseEvent.MOUSE_PRESSED,x,y,true);
                return pause(150).thenCompose(nothing->smokeSocket(Long.toUnsignedString(edge.input()))).thenCompose(picked->{if(!picked.getAsJsonObject("gesture").get("reconnect").getAsString().equals(Long.toUnsignedString(edge.id())))throw new IllegalStateException("Delete test did not pick native edge: before="+bounds+" after="+picked);return smokeShortcut("delete");}).thenCompose(deleted->{mouse(java.awt.event.MouseEvent.MOUSE_RELEASED,x,y,false);if(deleted.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow().edges().size()!=state.edges().size()-1)throw new IllegalStateException("Delete didn't disconnect picked edge");return smokeCommand("undo");});
            });
        }).thenApply(ignored->null);
    }
    private CompletableFuture<JsonObject> smokeDom(String body){
        if(!Boolean.getBoolean("epidemic.ui.platform.smoke"))throw new IllegalStateException("Qualification only");
        smokeReport=new CompletableFuture<>();browser.getCefBrowser().executeJavaScript("(async()=>{const result=await(async()=>{"+body+"})();window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify(result)}));})()",origin,0);
        return smokeReport.orTimeout(10,java.util.concurrent.TimeUnit.SECONDS).thenApply(v->JsonParser.parseString(v).getAsJsonObject());
    }
    CompletableFuture<Void> smokeAddDuringConnection(){
        var graph=latest.get().snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();
        var edge=graph.edges().getFirst();var node=graph.nodes().stream().filter(n->n.ports().stream().anyMatch(p->p.id()==edge.output())).findFirst().orElseThrow();
        browser.getCefBrowser().setFocus(true);
        return pause(150).thenCompose(ignored->smokeCanvasReport(graph.nodes().size(),0)).thenCompose(ignored->smokeDom("return await window.graphFocus("+JSON.toJson(Long.toUnsignedString(node.id()))+");"))
            .thenCompose(ignored->smokeSocket(Long.toUnsignedString(edge.output()))).thenCompose(bounds->{
                int x=(int)bounds.get("x").getAsDouble(),y=(int)bounds.get("y").getAsDouble();mouse(java.awt.event.MouseEvent.MOUSE_MOVED,x,y,false);mouse(java.awt.event.MouseEvent.MOUSE_PRESSED,x,y,true);mouse(java.awt.event.MouseEvent.MOUSE_RELEASED,x,y,false);
                return pause(150).thenCompose(ignored->smokeSocket(Long.toUnsignedString(edge.output())));
            }).thenCompose(picked->{
                if(!picked.getAsJsonObject("gesture").get("connection").getAsBoolean())throw new IllegalStateException("Add test did not start wire: "+picked);
                smokeResult=new CompletableFuture<>();var created=smokeResult;
                browser.getCefBrowser().executeJavaScript("{const graph=document.getElementById('graph');graph.dispatchEvent(new MouseEvent('contextmenu',{clientX:300,clientY:180,bubbles:true,cancelable:true}));document.querySelector('#context details details button').click();}",origin,0);
                return created.orTimeout(15,java.util.concurrent.TimeUnit.SECONDS);
            }).thenCompose(created->smokeCanvasReport(graph.nodes().size()+1,0)).thenCompose(ignored->smokeSocket(Long.toUnsignedString(edge.output())))
            .thenCompose(resumed->{if(!resumed.getAsJsonObject("gesture").get("connection").getAsBoolean())throw new IllegalStateException("Add did not resume wire: "+resumed);smokeEscapeKeys();return pause(150);})
            .thenCompose(ignored->smokeCommand("undo")).thenAccept(undone->{var restored=undone.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();if(restored.nodes().size()!=graph.nodes().size()||!restored.edges().equals(graph.edges()))throw new IllegalStateException("Add/Escape/Undo changed original topology");});
    }
    CompletableFuture<Void> smokeReviewInteractions(){
        var graph=latest.get().snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow();
        var input=graph.nodes().stream().filter(n->graph.catalogue().stream().anyMatch(t->t.type()==n.type()&&t.operation()==0)).findFirst().orElseThrow();
        String node=JSON.toJson(Long.toUnsignedString(input.id()));
        return smokeDom("await window.graphFocus("+node+");await new Promise(r=>setTimeout(r,150));const element=[...document.querySelectorAll('[data-node-id]')].find(e=>e.dataset.nodeId==="+node+").querySelector('.node-literals input');if(!element)throw Error('No inline Input field');element.focus();element.value='11';element.dispatchEvent(new Event('input',{bubbles:true}));return {field:true};")
            .thenCompose(ignored->smokeCommand("save")).thenCompose(saved->{
                var changed=saved.snapshot().graphs().stream().filter(g->g.document()==key.id()).findFirst().orElseThrow().nodes().stream().filter(n->n.id()==input.id()).findFirst().orElseThrow();
                if(changed.previewDefault()!=11)throw new IllegalStateException("Save missed inline Input draft");
                return pause(300).thenCompose(ignored->smokeDom("const element=[...document.querySelectorAll('[data-node-id]')].find(e=>e.dataset.nodeId==="+node+").querySelector('.node-literals input');window.__qualificationCanvas=document.getElementById('graph').getBoundingClientRect().toJSON();element.focus();element.value='not-a-number';element.dispatchEvent(new Event('input',{bubbles:true}));return {invalid:true};"));
            }).thenCompose(ignored->smokeCommand("save").handle((saved,error)->{if(error==null)throw new IllegalStateException("Invalid inline draft was saved");return true;}))
            .thenCompose(ignored->pause(200)).thenCompose(ignored->smokeDom("const graph=document.getElementById('graph'),problem=document.getElementById('problem');const before=window.__qualificationCanvas,after=graph.getBoundingClientRect().toJSON();if(problem.hidden||before.width!==after.width||before.height!==after.height||before.y!==after.y)throw Error('Diagnostic resized canvas or is missing');const close=problem.querySelector('[aria-label=\"Dismiss diagnostic\"]');if(!close)throw Error('Diagnostic has no close button');close.click();if(!problem.hidden||graph.getBoundingClientRect().height!==before.height)throw Error('Dismiss changed canvas');return {overlay:true};"))
            .thenCompose(ignored->smokeDom("const element=[...document.querySelectorAll('[data-node-id]')].find(e=>e.dataset.nodeId==="+node+").querySelector('.node-literals input');element.dispatchEvent(new KeyboardEvent('keydown',{key:'Escape',bubbles:true}));const root=document.getElementById('graph');root.dispatchEvent(new MouseEvent('contextmenu',{clientX:30,clientY:70,bubbles:true,cancelable:true}));const menu=document.getElementById('context');const grouped=menu.querySelector('summary')?.textContent==='Add'&&[...menu.querySelectorAll('summary')].some(e=>e.textContent==='Math');root.dispatchEvent(new PointerEvent('pointerdown',{button:0,bubbles:true}));root.dispatchEvent(new PointerEvent('pointerup',{button:0,bubbles:true}));return {grouped,closed:menu.hidden,restored:element.value};"))
            .thenCompose(state->{if(!state.get("grouped").getAsBoolean()||!state.get("closed").getAsBoolean()||!state.get("restored").getAsString().equals("11"))throw new IllegalStateException("Grouped Add/outside click failed");return smokeCommand("save");}).thenApply(ignored->null);
    }
    void source(java.util.function.Consumer<String> callback) { browser.getCefBrowser().getSource(callback::accept); }
    CompletableFuture<epidemic.editor.GraphSceneDragCoordinator.GraphHit> smokeReferenceHit(long port) {
        return smokeSocket(Long.toUnsignedString(port)).thenCompose(bounds->referenceHit(new java.awt.Point((int)bounds.get("x").getAsDouble(),(int)bounds.get("y").getAsDouble())));
    }
    CompletableFuture<String> smokeReferenceReport() {
        if(!Boolean.getBoolean("epidemic.ui.platform.smoke"))throw new IllegalStateException("Qualification only");
        smokeReport=new CompletableFuture<>();
        browser.getCefBrowser().executeJavaScript("window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({missing:document.body.textContent.includes('Missing Reference'),nodes:document.querySelectorAll('[data-testid=node]').length})}));",origin,0);
        return smokeReport.orTimeout(10,java.util.concurrent.TimeUnit.SECONDS);
    }
    boolean released() { return browser != null && browser.isDisposed() && query.isDisposed(); }
    @Override public void dispose() {
        disposed = true;
        referenceDrag.cancel();
        if(inlineFlush!=null&&!inlineFlush.isDone())inlineFlush.completeExceptionally(new java.util.concurrent.CancellationException("Graph closed"));
        if(referenceDrop!=null)referenceDrop.setComponent(null);
        if(referenceHit!=null)referenceHit.completeExceptionally(new java.util.concurrent.CancellationException("Graph closed"));
        readiness.completeExceptionally(new java.util.concurrent.CancellationException("Graph FileEditor closed"));
        if (timer != null) timer.stop();
        // Disposer releases the query and browser children on tab/window close.
        component.removeAll();
    }
}

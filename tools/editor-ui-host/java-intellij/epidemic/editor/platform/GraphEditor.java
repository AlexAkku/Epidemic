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
    private final JPanel component = new JPanel(new BorderLayout());
    private JBCefBrowser browser;
    private JBCefJSQuery query;
    private javax.swing.Timer timer;
    private boolean disposed, pending;
    private long published = -1;
    private int publishedFlags = -1;
    private String publishedDiagnostic = "";
    private String diagnostic = "";
    private CompletableFuture<Bridge.Reply> smokeResult;
    private CompletableFuture<String> smokeReport;
    private final CompletableFuture<Void> readiness = new CompletableFuture<>();

    GraphEditor(Bridge.DocumentKey key, Supplier<Bridge.Reply> latest, Commands commands) {
        this.key = key; this.latest = latest; this.commands = commands;
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
                if (payload.length() > 4096 || disposed) return null;
                ApplicationManager.getApplication().invokeLater(() -> request(payload));
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
            String html = "<!doctype html><html><head><meta charset='utf-8'>"
                + "<meta http-equiv=\"Content-Security-Policy\" content=\"default-src 'none'; script-src 'sha256-" + sha
                + "'; style-src 'unsafe-inline'; img-src 'none'; connect-src 'none'; font-src 'none'; frame-src 'none'; object-src 'none'; base-uri 'none'; form-action 'none'\">"
                + "<style>html,body{height:100%;margin:0;background:#25262a;color:#eee;font:13px sans-serif}nav{height:36px;display:flex;align-items:center;gap:8px;padding:0 10px}button{background:#444;color:white;border:1px solid #666;padding:4px 10px}#graph{height:calc(100% - 36px)}#status{font-size:12px}</style></head>"
                + "<body><nav><button data-create='0'>Event</button><button data-create='1'>Native Call</button><button data-create='2'>Wait</button><span id='status'>Loading native graph…</span></nav><div id='graph'></div><script>"
                + script + "</script></body></html>";
            component.add(browser.getComponent());
            browser.loadHTML(html, origin);
            timer = new javax.swing.Timer(300, event -> refresh());
            timer.start();
        } catch (Exception error) {
            if (browser != null) Disposer.dispose(browser);
            component.removeAll(); component.add(new JLabel("Graph unavailable: " + error.getMessage()));
        }
    }
    JComponent component() { return component; }
    JComponent preferredFocus() { return browser == null || browser.isDisposed() ? component : browser.getComponent(); }
    Bridge.DocumentKey identity() { return key; }
    private void request(String payload) {
        if (Boolean.getBoolean("epidemic.ui.platform.smoke")) LOG.info("Graph query url=" + (browser == null ? "null" : browser.getCefBrowser().getURL()) + " pending=" + pending);
        if (disposed || pending || browser == null || !origin.equals(browser.getCefBrowser().getURL())) return;
        try {
            JsonObject message = JsonParser.parseString(payload).getAsJsonObject();
            if (!nonce.equals(message.get("nonce").getAsString())) throw new IllegalArgumentException("ui.graph_nonce");
            if ("__ready".equals(message.get("action").getAsString())) {
                readiness.complete(null); published = -1; refresh(); return;
            }
            if (Boolean.getBoolean("epidemic.ui.platform.smoke") && "__report".equals(message.get("action").getAsString()) && smokeReport != null) {
                smokeReport.complete(message.get("result").getAsString()); return;
            }
            Bridge.Reply reply = latest.get();
            if (reply == null || key.resolve(reply.generation(), reply.snapshot()).isEmpty())
                throw new IllegalArgumentException("ui.graph_document_stale");
            long revision = Long.parseUnsignedLong(message.get("revision").getAsString());
            if (revision != reply.snapshot().revision()) throw new IllegalArgumentException("ui.stale_revision");
            if (reply.snapshot().playing()) throw new IllegalArgumentException("ui.playing_readonly");
            String action = message.get("action").getAsString();
            var values = message.getAsJsonArray("args");
            int expected = switch (action) { case "create", "move" -> 3; case "connect" -> 2;
                case "delete", "disconnect" -> 1; case "save", "undo", "redo" -> 0;
                default -> throw new IllegalArgumentException("ui.graph_action"); };
            if (values.size() != expected) throw new IllegalArgumentException("ui.graph_arguments");
            int operation = switch (action) { case "save" -> 6; case "undo" -> 7; case "redo" -> 8; default -> 38; };
            List<String> args = new ArrayList<>();
            if (operation == 38) { args.add(action); args.add(Long.toUnsignedString(key.id())); }
            for (JsonElement value : values) {
                if (!value.isJsonPrimitive() || !value.getAsJsonPrimitive().isString() || value.getAsString().length() > 64)
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
                    diagnostic = error == null ? result.error() : error.getMessage();
                    published = -1; refresh();
                }));
        } catch (RuntimeException error) {
            if (smokeResult != null && !smokeResult.isDone()) smokeResult.completeExceptionally(error);
            diagnostic = error.getMessage(); published = -1; refresh();
        }
    }
    void refresh() {
        if (disposed || browser == null || browser.isDisposed() || !readiness.isDone()) return;
        var reply = latest.get();
        if (reply == null || key.resolve(reply.generation(), reply.snapshot()).isEmpty()) return;
        var snapshot = reply.snapshot();
        var graph = snapshot.graphs().stream().filter(g -> g.document() == key.id()).findFirst().orElse(null);
        if (graph == null) return;
        JsonObject dto = new JsonObject(); dto.addProperty("revision", Long.toUnsignedString(snapshot.revision()));
        dto.addProperty("readonly", snapshot.playing()); dto.addProperty("error", diagnostic);
        JsonArray nodes = new JsonArray(), edges = new JsonArray();
        for (var node : graph.nodes()) {
            JsonObject n = new JsonObject(); n.addProperty("id", Long.toUnsignedString(node.id()));
            n.addProperty("name", node.name()); n.addProperty("x", node.x()); n.addProperty("y", node.y());
            JsonArray ports = new JsonArray();
            for (var port : node.ports()) {
                JsonObject p = new JsonObject(); p.addProperty("id", Long.toUnsignedString(port.id()));
                p.addProperty("name", port.name()); p.addProperty("kind", port.kind()); p.addProperty("direction", port.direction());
                p.addProperty("type", Long.toUnsignedString(port.type())); ports.add(p);
            }
            n.add("ports", ports); nodes.add(n);
        }
        for (var edge : graph.edges()) {
            JsonObject e = new JsonObject(); e.addProperty("id", Long.toUnsignedString(edge.id()));
            e.addProperty("output", Long.toUnsignedString(edge.output())); e.addProperty("input", Long.toUnsignedString(edge.input())); edges.add(e);
        }
        dto.add("nodes", nodes); dto.add("edges", edges);
        // The first ready handshake can arrive after the first refresh. Keep the
        // probe in JS so no host-side polling state can lose initial publication.
        String revision = JSON.toJson(Long.toUnsignedString(snapshot.revision()));
        String js = "if(window.graphCanvasReady && document.documentElement.dataset.graphReady!==" + revision
            + " && !window.graphPublishing){window.graphPublishing=true;window.graphSnapshot(" + JSON.toJson(dto)
            + ");setTimeout(()=>window.graphPublishing=false,500);} ";
        if (published != snapshot.revision() || publishedFlags != snapshot.flags() || !publishedDiagnostic.equals(diagnostic)) js = "document.documentElement.dataset.graphReady='';" + js;
        browser.getCefBrowser().executeJavaScript(js, origin, 0);
        published = snapshot.revision(); publishedFlags = snapshot.flags(); publishedDiagnostic = diagnostic;
    }
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
            "{const r=document.querySelector('[data-testid=node]').getBoundingClientRect();"
            + "window.nativeGraph(JSON.stringify({nonce:window.graphNonce,action:'__report',result:JSON.stringify({x:r.left+r.width*.3,y:r.top+Math.min(10,r.height*.07),width:r.width,height:r.height,keys:window.__qualificationKeys})}));}", origin, 0);
        return smokeReport.orTimeout(10, java.util.concurrent.TimeUnit.SECONDS).thenApply(value -> JsonParser.parseString(value).getAsJsonObject());
    }
    private void mouse(int event, int x, int y, boolean down) {
        var input = browser.getCefBrowser().getUIComponent();
        int button = event == java.awt.event.MouseEvent.MOUSE_DRAGGED ? java.awt.event.MouseEvent.NOBUTTON : java.awt.event.MouseEvent.BUTTON1;
        browser.getCefBrowser().sendMouseEvent(new java.awt.event.MouseEvent(input, event, System.currentTimeMillis(),
            down ? java.awt.event.InputEvent.BUTTON1_DOWN_MASK : 0, x, y, 1, false, button));
    }
    CompletableFuture<Void> smokeDragCancel() {
        if (!Boolean.getBoolean("epidemic.ui.platform.smoke")) throw new IllegalStateException("Qualification only");
        browser.getCefBrowser().executeJavaScript("window.graphFit();window.__qualificationKeys=[];document.addEventListener('keydown',e=>window.__qualificationKeys.push(e.key),true);", origin, 0);
        com.intellij.openapi.wm.IdeFocusManager.getGlobalInstance().requestFocus(browser.getComponent(), true);
        long revision = latest.get().snapshot().revision();
        return pause(350).thenCompose(ignored -> smokeNodeBounds()).thenCompose(bounds -> {
            int x = (int)bounds.get("x").getAsDouble(), y = (int)bounds.get("y").getAsDouble();
            browser.getCefBrowser().setFocus(true);
            mouse(java.awt.event.MouseEvent.MOUSE_PRESSED, x, y, true);
            mouse(java.awt.event.MouseEvent.MOUSE_DRAGGED, x + 24, y + 24, true);
            return pause(150).thenCompose(ignored -> smokeNodeBounds()).thenCompose(moved -> {
                if (Math.abs(moved.get("x").getAsDouble() - bounds.get("x").getAsDouble()) < 1)
                    throw new IllegalStateException("Input missed the node drag preview");
                var input = browser.getCefBrowser().getUIComponent();
                // Pinned JCEF Windows reads AWT rawCode/scancode, not getKeyCode().
                for (int kind : new int[]{java.awt.event.KeyEvent.KEY_PRESSED, java.awt.event.KeyEvent.KEY_RELEASED}) {
                    var event = new java.awt.event.KeyEvent(input, kind, System.currentTimeMillis(), 0,
                        java.awt.event.KeyEvent.VK_ESCAPE, (char)27);
                    try {
                        var raw = java.awt.event.KeyEvent.class.getDeclaredField("rawCode"); raw.setAccessible(true); raw.setLong(event, 27);
                        var scan = java.awt.event.KeyEvent.class.getDeclaredField("scancode"); scan.setAccessible(true); scan.setLong(event, 1);
                    } catch (ReflectiveOperationException error) { throw new java.util.concurrent.CompletionException(error); }
                    browser.getCefBrowser().sendKeyEvent(event);
                }
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
    void source(java.util.function.Consumer<String> callback) { browser.getCefBrowser().getSource(callback::accept); }
    boolean released() { return browser != null && browser.isDisposed() && query.isDisposed(); }
    @Override public void dispose() {
        disposed = true;
        readiness.completeExceptionally(new java.util.concurrent.CancellationException("Graph FileEditor closed"));
        if (timer != null) timer.stop();
        // Disposer releases the query and browser children on tab/window close.
        component.removeAll();
    }
}

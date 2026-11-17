package epidemic.editor;

import epidemic.u1.NativeViewport;
import java.awt.*;
import java.awt.event.*;
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.util.*;
import java.util.List;
import java.util.function.*;
import javax.swing.*;
import javax.swing.tree.*;

/** Shared immutable snapshot reader and persistent UI views; no editable game model. */
public final class UiPanels {
    private GraphSceneDragCoordinator referenceDrag;
    private Bridge.RuntimeSession runtime=Bridge.RuntimeSession.empty();
    private final List<java.util.function.Consumer<Bridge.RuntimeSession>> runtimeReaders=new ArrayList<>();
    @FunctionalInterface public interface RuntimeCommand { java.util.concurrent.CompletableFuture<Bridge.Reply> execute(long connection,long generation,long session,String action,String[] arguments); }
    private RuntimeCommand runtimeCommand=(connection,generation,session,action,args)->java.util.concurrent.CompletableFuture.failedFuture(new IllegalStateException("runtime.unavailable"));
    private java.util.function.BooleanSupplier runtimeBuild=()->false;
    public void runtimeBuild(java.util.function.BooleanSupplier source){runtimeBuild=source;}
    public void runtimeCommands(RuntimeCommand command){runtimeCommand=command;}
    private final Deque<Bridge.RuntimeSession> runtimeHistory=new ArrayDeque<>();
    public void runtime(Bridge.RuntimeSession frame){if(runtime.session()!=0&&!runtime.trace().isEmpty()&&(runtime.session()!=frame.session()||runtime.connection()!=frame.connection()||runtime.generation()!=frame.generation())){runtimeHistory.addLast(runtime.eventsOnly());if(runtimeHistory.size()>4)runtimeHistory.removeFirst();}runtime=frame;runtimeReaders.forEach(reader->reader.accept(frame));}

    public void referenceDrag(GraphSceneDragCoordinator value){referenceDrag=value;}
    private final Catalog catalog;
    private final FeatureRegistry features;
    private final Consumer<Bridge.Document> openDocument;
    private final Consumer<Bridge.Selection> select;
    private final Runnable importPicker;
    private final Runnable cancelJobs;
    private final Consumer<Integer> createDocument;
    private BiConsumer<String,Bridge.Document> documentCommand=(action,doc)->{};
    private AssetDetails.PreviewLoader previewLoader;
    private Consumer<String> newContentFolder;
    public void contentFolderCommands(Consumer<String> action){newContentFolder=action;}
    public void documentCommands(BiConsumer<String,Bridge.Document> action){documentCommand=action;}
    public void assetPreview(AssetDetails.PreviewLoader loader){previewLoader=loader;}
    private Function<ScalarInspector.Edit,java.util.concurrent.CompletableFuture<Bridge.Reply>> editField;
    private final List<ScalarInspector> inspectors=new ArrayList<>();
    private final List<GraphTools> graphTools=new ArrayList<>();
    private Supplier<List<java.nio.file.Path>> sourceFiles=List::of;
    private BiConsumer<java.nio.file.Path,Boolean> openSource=(path,with)->{};
    private Runnable newSource=()->{};
    private BiConsumer<String,java.nio.file.Path> sourceFileAction=(action,path)->{};
    public void sourceFileActions(BiConsumer<String,java.nio.file.Path> action){sourceFileAction=action;}
    private final List<Runnable> sourceRefreshers=new ArrayList<>();
    public void sourceCommands(Supplier<List<java.nio.file.Path>> files,BiConsumer<java.nio.file.Path,Boolean> open,Runnable create){sourceFiles=files;openSource=open;newSource=create;}
    public void refreshSources(){sourceRefreshers.forEach(Runnable::run);}

    private Consumer<Bridge.GraphTemplate> sourceNavigator=template->{};
    public void sourceNavigator(Consumer<Bridge.GraphTemplate> action){sourceNavigator=action;graphTools.forEach(g->g.sourceNavigator(action));}
    private BiConsumer<String,Long> assetCommand=(action,id)->{};
    public void assetCommands(BiConsumer<String,Long> action){assetCommand=action;}
    public record HierarchyEdit(long generation,long revision,String action,long document,long node,String argument) {}
    private Consumer<HierarchyEdit> hierarchyCommand=edit->{};
    private BooleanSupplier hierarchyAvailable=()->false;
    public void hierarchyCommands(BooleanSupplier available,Consumer<HierarchyEdit> action){
        hierarchyAvailable=available;
        hierarchyCommand=action;
    }
    public record ComponentEdit(long generation,long revision,long selectionRevision,String action,String argument) {}
    private Consumer<ComponentEdit> componentCommand=edit->{};
    private BooleanSupplier componentAvailable=()->false;
    public void componentCommands(BooleanSupplier available,Consumer<ComponentEdit> action){
        componentAvailable=available;
        componentCommand=action;
    }
    public record GestureEdit(long generation,long revision,int operation,List<String> arguments) {}
    private Function<GestureEdit,java.util.concurrent.CompletableFuture<Bridge.Reply>> gestureCommand=
        edit->java.util.concurrent.CompletableFuture.failedFuture(new IllegalStateException("Scene gestures unavailable"));
    private BooleanSupplier gesturesAvailable=()->false;
    private BooleanSupplier sceneResourcesAvailable=()->false;
    private BooleanSupplier systemsAvailable=()->false;
    public void sceneResources(BooleanSupplier available){sceneResourcesAvailable=available;}
    public void systemsCommands(BooleanSupplier available){systemsAvailable=available;}
    public void sceneGestures(BooleanSupplier available,Function<GestureEdit,java.util.concurrent.CompletableFuture<Bridge.Reply>> action){
        gesturesAvailable=available;
        gestureCommand=action;
    }
    private BooleanSupplier graphAvailable=()->false;
    private Function<GestureEdit,java.util.concurrent.CompletableFuture<Bridge.Reply>> graphCommand=edit->java.util.concurrent.CompletableFuture.failedFuture(new IllegalStateException("Graph unavailable"));
    public void graphCommands(BooleanSupplier available,Function<GestureEdit,java.util.concurrent.CompletableFuture<Bridge.Reply>> action){graphAvailable=available;graphCommand=action;}
    private long generation;
    private boolean interactive;
    private boolean documentCommandsEnabled;
    private final List<AbstractButton> assetButtons=new ArrayList<>();
    private final List<AbstractButton> documentButtons = new ArrayList<>();
    private final List<Runnable> authoringControls = new ArrayList<>();
    private final Map<String,JComponent> cache = new HashMap<>();
    private final Map<String,NativeViewport> nativeViews = new HashMap<>();
    private final List<Consumer<Bridge.Snapshot>> readers = new ArrayList<>();
    private final List<Consumer<Bridge.Snapshot>> structureReaders = new ArrayList<>();
    private final JTextArea console = new JTextArea(), problems = new JTextArea();
    private Consumer<RuntimeLogView.Link> runtimeNavigation=link->{};
    private final List<RuntimeLogView> runtimeLogs=new ArrayList<>();
    private final Deque<RuntimeLogView.Entry> logHistory=new ArrayDeque<>();
    public void runtimeNavigation(Consumer<RuntimeLogView.Link> action){runtimeNavigation=action;}
    private void history(String text,int severity){var entry=RuntimeLogView.authoringEntry(text,severity,snapshot,runtime);logHistory.addLast(entry);if(logHistory.size()>512)logHistory.removeFirst();runtimeLogs.forEach(view->view.replay(entry));}
    private Bridge.Snapshot snapshot;
    private boolean refreshing;
    public UiPanels(Catalog catalog, FeatureRegistry features, Consumer<Bridge.Document> open,
                    Consumer<Bridge.Selection> select, Runnable importPicker, Runnable cancelJobs,
                    Consumer<Integer> createDocument, Runnable renameDocument) {
        this.catalog = catalog; this.features = features; this.openDocument = open;
        this.select = select; this.importPicker = importPicker; this.cancelJobs = cancelJobs; console.setEditable(false);
        this.createDocument=createDocument;this.documentCommand=(action,doc)->{if(action.equals("rename"))renameDocument.run();};
    }
    public UiPanels(Catalog catalog,FeatureRegistry features,Consumer<Bridge.Document> open,Consumer<Bridge.Selection> select,Runnable importPicker,Runnable cancelJobs,Consumer<Integer> createDocument,Runnable renameDocument,Function<ScalarInspector.Edit,java.util.concurrent.CompletableFuture<Bridge.Reply>> editField){
        this(catalog,features,open,select,importPicker,cancelJobs,createDocument,renameDocument);this.editField=editField;
    }
    public void sessionState(long generation,boolean ready){
        this.generation=generation;interactive=ready;assetButtons.forEach(button->button.setEnabled(ready&&snapshot!=null&&snapshot.storage()==2&&!snapshot.playing()));
        for(var inspector:inspectors)inspector.refresh(generation,snapshot,ready);
        authoringControls.forEach(Runnable::run);
    }
    public java.util.concurrent.CompletableFuture<Void> flushDrafts(){
        var future=java.util.concurrent.CompletableFuture.allOf(inspectors.stream().map(ScalarInspector::flush).toArray(java.util.concurrent.CompletableFuture[]::new));
        for(var graph:graphTools)future=future.thenCompose(ignored->graph.flushDraft());return future;
    }
    public void cancelDrafts(long document){if(snapshot!=null&&snapshot.selection().document()==document)inspectors.forEach(ScalarInspector::cancelDrafts);graphTools.forEach(g->g.cancelDrafts(document));}
    public boolean hasDraft(long document){return inspectors.stream().anyMatch(inspector->inspector.hasDraft(document))||graphTools.stream().anyMatch(g->g.hasDraft(document));}
    public void documentCommandsEnabled(boolean enabled) {
        documentCommandsEnabled=enabled;documentButtons.forEach(button->button.setEnabled(enabled));
    }
    public NativeViewport nativeView(String id) { return nativeViews.get(id); }
    public Map<String,NativeViewport> nativeViews() { return Map.copyOf(nativeViews); }
    public void log(String text) {
        append(console,text);history(text,0);
    }
    public void problem(String text) {
        append(problems,text);append(console,text);history(text,text.toLowerCase(Locale.ROOT).contains("warning")||text.startsWith("WARN")?1:2);
    }
    private static void append(JTextArea output,String text) {
        output.append(text + "\n");
        if (output.getDocument().getLength() > 65536)
            try { output.getDocument().remove(0, output.getDocument().getLength() - 49152); }
            catch (javax.swing.text.BadLocationException error) { throw new IllegalStateException(error); }
    }
    public void refresh(Bridge.Snapshot value) {
        refresh(value,false);
    }
    public void refresh(Bridge.Snapshot value,boolean force) {
        if (!force&&Objects.equals(snapshot, value)) return;
        Bridge.Snapshot previous = snapshot;
        boolean structureChanged = previous == null || value == null || !previous.documents().equals(value.documents()) ||
            !previous.nodes().equals(value.nodes()) || !previous.assets().equals(value.assets()) ||
            !previous.contentFolders().equals(value.contentFolders());
        snapshot = value; refreshing = true;
        try {
            if (structureChanged) for (var reader : structureReaders) reader.accept(value);
            for (var reader : readers) reader.accept(value);
        }
        finally { refreshing = false; }
    }
    public JComponent view(String id) { return cache.computeIfAbsent(id, key->{
        var panel=create(key);
        // Docking geometry belongs to the user's splitters, not the current selection.
        panel.setMinimumSize(new Dimension(0,0));panel.setPreferredSize(new Dimension(280,320));
        return panel;
    }); }
    private void observe(Consumer<Bridge.Snapshot> reader) {
        readers.add(reader);boolean previous=refreshing;refreshing=true;
        try{reader.accept(snapshot);}finally{refreshing=previous;}
    }
    private void structure(Consumer<Bridge.Snapshot> reader) { structureReaders.add(reader); reader.accept(snapshot); }
    private JComponent create(String id) {
        String base = id.contains(":") ? id.substring(0, id.indexOf(':')) : id;
        if(Set.of("game.runtime.inspector","game.session").contains(base)){var inspector=new RuntimeInspector();runtimeReaders.add(inspector::refresh);inspector.refresh(runtime);return inspector;}
        if(base.equals("game.controls")){var controls=new RuntimeGameView((c,g,s,a,args)->runtimeCommand.execute(c,g,s,a,args),false);observe(value->controls.refresh(value,runtime));runtimeReaders.add(frame->controls.refresh(snapshot,frame));return controls;}
        if(base.equals("game.view")){
            var game=new RuntimeGameView((c,g,s,a,args)->runtimeCommand.execute(c,g,s,a,args));nativeViews.put(id,game.viewport());observe(value->game.refresh(value,runtime));runtimeReaders.add(frame->game.refresh(snapshot,frame));return game;
        }
        if (Set.of("graph.palette","graph.inspector","graph.references","graph.variables").contains(base)) {
            var panel=new GraphTools(base,graphAvailable,()->generation,graphCommand);panel.sourceNavigator(sourceNavigator);graphTools.add(panel);observe(panel::refresh);return panel;
        }
        if(base.equals("type.registry")) {
            var model=new DefaultListModel<Bridge.ComponentType>();var list=new JList<Bridge.ComponentType>(model);
            list.setCellRenderer(new DefaultListCellRenderer(){{putClientProperty("html.disable",true);}public Component getListCellRendererComponent(JList<?> owner,Object value,int index,boolean selected,boolean focus){return super.getListCellRendererComponent(owner,value instanceof Bridge.ComponentType type?type.name()+" · "+Long.toUnsignedString(type.id()):value,index,selected,focus);}});
            observe(state->{var types=state==null?List.<Bridge.ComponentType>of():state.componentTypes();if(Collections.list(model.elements()).equals(types))return;var selected=list.getSelectedValue();model.clear();types.forEach(model::addElement);if(selected!=null)for(int i=0;i<model.size();i++)if(model.get(i).id()==selected.id())list.setSelectedIndex(i);});
            var root=new JPanel(new BorderLayout(6,6));root.add(new JLabel("Native component schemas · add through Inspector"),BorderLayout.NORTH);root.add(new JScrollPane(list));return root;
        }
        if (base.equals("project.explorer")) return explorer();
        if (base.equals("scene.hierarchy")) return hierarchy();
        if (base.equals("scene.systems")) return systems();
        if (base.equals("scene.editor")||base.equals("prefab.canvas")) return scene(id);
        if (base.equals("background.jobs")) {
            JPanel panel=Theme.island(new BorderLayout(6,6));
            panel.add(Theme.label("Bridge queue: at most 32 pending requests. Running mutations finish atomically.",true),BorderLayout.NORTH);
            JPanel actions=new JPanel(new FlowLayout(FlowLayout.LEFT));
            actions.add(Theme.button("Cancel native imports",cancelJobs));
            actions.add(Theme.button("Watch Content…",()->assetCommand.accept("Watch",0L)));
            panel.add(actions,BorderLayout.SOUTH);
            JTextArea state=new JTextArea();
            observe(value->{StringBuilder rows=new StringBuilder("Native import queue: bounded to 128; one worker.\n");if(value!=null){rows.append((value.flags()&64)!=0?"Import queue: busy\n":"Import queue: idle\n");rows.append((value.flags()&32)!=0?"Content watch: on\n":"Content watch: off\n");long completed=value.assetState().events().stream().filter(e->e.job()>0&&e.kind()!=0).count();rows.append("Observed terminal import outcomes (last 64 events): ").append(completed).append("\n");}if(value!=null)for(var event:value.assetState().events())rows.append("Job ").append(event.job()).append(" · ").append(event.source()).append(" · ").append(new String[]{"Importing","Updated","Missing","Failed","Recovered","Cancelled"}[event.kind()]).append(" · ").append(event.diagnostic()).append('\n');if(!state.getText().contentEquals(rows)){int caret=state.getCaretPosition();state.setText(rows.toString());state.setCaretPosition(Math.min(caret,state.getDocument().getLength()));}});
            state.setEditable(false);state.setLineWrap(true);state.setWrapStyleWord(true);panel.add(state);return panel;
        }
        if(base.equals("graph.debugger")){var debugger=new RuntimeDebugger((c,g,s,a,args)->runtimeCommand.execute(c,g,s,a,args),link->runtimeNavigation.accept(link));debugger.seedHistory(runtimeHistory,snapshot);observe(value->debugger.refresh(runtime,value));runtimeReaders.add(frame->debugger.refresh(frame,snapshot));return debugger;}
        if(base.equals("profiler")){var profiler=new RuntimeProfiler(()->List.copyOf(nativeViews.values()),()->runtimeBuild.getAsBoolean());runtimeReaders.add(profiler::refresh);profiler.refresh(runtime);return profiler;}
        if(Set.of("console","problems","build.output").contains(base)){
            var view=new RuntimeLogView(base.equals("problems"),link->runtimeNavigation.accept(link));
            for(var frame:runtimeHistory)view.refresh(frame,snapshot);view.refresh(runtime,snapshot);for(var row:logHistory)view.replay(row);runtimeLogs.add(view);runtimeReaders.add(frame->view.refresh(frame,snapshot));return view;
        }
        if (base.equals("type.registry") || base.equals("inspector") || base.equals("prefab.editor") || base.equals("graph.inspector")) {
            JPanel panel = Theme.island(new BorderLayout(6, 6));
            JTextArea summary = new JTextArea(5, 20); summary.setEditable(false); summary.setLineWrap(true); summary.setWrapStyleWord(true);
            summary.setFont(UIManager.getFont("Label.font"));
            observe(value -> {
                if(value==null){summary.setText("Select a project or object");return;}
                var selection=value.selection();
                String document=value.documents().stream().filter(doc->doc.id()==selection.document())
                    .map(doc->doc.name()+" · document revision "+doc.revision()).findFirst().orElse(value.title());
                String scope=switch(selection.kind()){
                    case 1->value.nodes().stream().anyMatch(node->node.selection().equals(selection)&&node.prefab()!=0)
                        ?"Prefab placement · edit a field to override it on this instance"
                        :"Scene entity";
                    case 2,5->"Prefab definition";
                    case 7->"Linked Prefab · inherited values. Open Linked Definition to edit.";
                    default->"Authoring selection";
                };
                summary.setText(document+"\n"+scope+"\nDocument ID: "+unsigned(selection.document())+
                    "\nElement ID: "+unsigned(selection.element()));
            });
            JPanel fields=new JPanel(new BorderLayout());Map<String,JComponent> forms=new HashMap<>();
            observe(value->{
                String schema=base.equals("prefab.editor")?"owners.prefab-definition":base.equals("graph.inspector")?"graph.inspector":base.equals("type.registry")?"code.types":"owners.project";
                if(base.equals("inspector")&&value!=null&&value.selection().document()!=0){
                    schema=switch(value.selection().kind()){
                        case 1->"owners.entity";case 2,5->"owners.prefab-definition";case 3,4->"graph.inspector";
                        default->value.documents().stream().filter(d->d.id()==value.selection().document()).findFirst().map(d->switch(d.kind()){case 0->"owners.scene";case 1->"owners.prefab-definition";default->"graph.inspector";}).orElse("owners.project");
                    };
                }
                if(!schema.equals(fields.getClientProperty("schema.id"))){fields.putClientProperty("schema.id",schema);fields.removeAll();fields.add(forms.computeIfAbsent(schema,key->InspectorForms.fixed(catalog,features,key)));fields.revalidate();fields.repaint();}
            });
            panel.add(summary, BorderLayout.NORTH);
            if(!base.equals("type.registry")){
                ScalarInspector inspector=new ScalarInspector(request->editField==null?java.util.concurrent.CompletableFuture.failedFuture(new IllegalStateException("Native Inspector unavailable")):editField.apply(request),
                    request->gestureCommand.apply(new GestureEdit(request.generation(),request.revision(),31,List.of(unsigned(request.selectionRevision()),request.key()))));
                inspectors.add(inspector);observe(value->inspector.refresh(generation,value,interactive));panel.add(inspector);
                JPanel actions=new JPanel();
                actions.setLayout(new BoxLayout(actions,BoxLayout.Y_AXIS));
                actions.add(componentActions());actions.add(resourceActions());
                panel.add(actions,BorderLayout.SOUTH);
            }else panel.add(fields);
            return panel;
        }
        if (base.equals("settings.editor")) return InspectorForms.gallery(catalog, features);
        if (base.equals("asset.details") || base.equals("asset.references")) {
            AssetDetails details=new AssetDetails(base.equals("asset.references"),idValue->{if(snapshot!=null){var doc=snapshot.documents().stream().filter(d->d.id()==idValue).findFirst();if(doc.isPresent())openDocument.accept(doc.get());else if(snapshot.assets().stream().anyMatch(asset->asset.id()==idValue))select.accept(new Bridge.Selection(6,0,idValue));}},previewLoader);
            observe(details::refresh);return details;
        }
        JPanel panel = Theme.island(new BorderLayout(6, 6));
        JLabel title = Theme.label(catalog.panel(id).label(), false); title.setFont(title.getFont().deriveFont(Font.BOLD, 18)); panel.add(title, BorderLayout.NORTH);
        JPanel content = new JPanel(new BorderLayout(6, 6));
        JTextArea explanation = new JTextArea("Choose a document or object.\nThis UI shell is available; its backend actions remain planned.\n");
        explanation.setFont(UIManager.getFont("Label.font"));
        explanation.setEditable(false); explanation.setLineWrap(true); explanation.setWrapStyleWord(true);
        if (base.equals("graph.editor")) {
            explanation.append("Graph canvas: zoom / pan / typed ports / links — F3.\nPort → Scene drag affordance — F3, disabled; no references are created.\n");
            JToolBar toolbar = new JToolBar(); toolbar.setFloatable(false);
            for (String action : new String[]{"Add Node", "Connect", "Compile", "Debug", "Port → Scene"}) toolbar.add(features.plannedButton("graph." + action, action, "F3"));
            content.add(toolbar, BorderLayout.NORTH);
        } else if (base.equals("code.space")) explanation.append("Source tree, code tabs, symbols, search and compile diagnostics — F4.\n");
        else if (base.equals("game.view")) observe(value->explanation.setText(
            "Play session: "+(value!=null&&value.playing()?"active":"stopped")+".\n"+
            "Play / Stop use the project toolbar (F8). Game View preview and advanced controls remain planned.\n"));
        else if (base.equals("agent.workspace")) explanation.append("Prompt, context, proposed edits, diff, review/approve, validation and logs — F10.\n");
        String initial = switch (base) {
            case "graph.palette" -> "graph.palette";
            case "graph.variables", "graph.references", "graph.editor" -> "graph.inspector";
            case "game.view", "game.controls" -> "game.controls";
            case "game.runtime.inspector", "game.session" -> "game.runtime";
            case "code.space", "code.symbols" -> "code.space";
            case "code.properties", "code.references" -> "code.types";
            case "ui.palette", "ui.canvas", "ui.inspector" -> "ui.widgets";
            case "ui.flow" -> "ui.flow";
            case "animation.editor", "animation.properties", "animation.timeline" -> "assets.animation-clip";
            case "sprite.editor" -> "assets.sprite-sheet";
            case "tilemap.editor" -> "assets.tilemap";
            case "material.editor" -> "assets.material-2d";
            case "audio.editor" -> "assets.audio-mixer";
            case "build.editor" -> "build.settings";
            case "profiler" -> "profile.metrics";
            case "graph.debugger" -> "debug.trace";
            case "scene.tools", "scene.layers", "scene.systems" -> "scene.tools";
            case "settings.project" -> "settings.project";
            case "agent.workspace" -> "agent.workspace";
            default -> "inspector.types";
        };
        JComponent presentation = new JScrollPane(explanation);
        if (Set.of("graph.editor","ui.flow","ui.canvas","prefab.canvas","animation.editor","sprite.editor","tilemap.editor","material.editor").contains(base)) {
            JPanel canvas = new JPanel(new BorderLayout(6,6));
            canvas.add(explanation,BorderLayout.NORTH); canvas.add(new ShellCanvas(base)); presentation=canvas;
        }
        JSplitPane details = new JSplitPane(JSplitPane.VERTICAL_SPLIT, presentation, InspectorForms.fixed(catalog, features, initial));
        details.setResizeWeight(.3); details.setBorder(null); content.add(details, BorderLayout.CENTER);
        panel.add(content); return panel;
    }
    private JComponent scene(String id) {
        JPanel panel = Theme.island(new BorderLayout(6, 6));
        JToolBar tools = new JToolBar(); tools.setFloatable(false);tools.setLayout(new GridLayout(0,4,4,4));
        boolean sceneEditor=id.startsWith("scene.editor");
        JButton fit=new JButton("Fit"),reset=new JButton("Reset Transform");
        JCheckBox snap=new JCheckBox("Snap (1 unit)"),grid=new JCheckBox("Grid",true);
        var toolButtons=new ArrayList<JToggleButton>();
        if(sceneEditor){
            tools.add(fit);
            ButtonGroup modes=new ButtonGroup();
            for(String label:List.of("Move","Rotate","Scale")){
                var button=new JToggleButton(label);modes.add(button);tools.add(button);toolButtons.add(button);
            }
            toolButtons.getFirst().setSelected(true);
            tools.add(grid);tools.add(snap);tools.add(reset);
        }
        panel.add(tools, BorderLayout.NORTH);
        try {
            NativeViewport viewport = new NativeViewport(); nativeViews.put(id, viewport); panel.add(viewport, BorderLayout.CENTER);
            fit.addActionListener(e->viewport.fit());
            grid.addActionListener(e->viewport.grid(grid.isSelected()));
            if(sceneEditor){
                var mouse=new SceneInput(id,viewport,snap);
                viewport.addMouseListener(mouse);viewport.addMouseMotionListener(mouse);viewport.addMouseWheelListener(mouse);
                if(!GraphicsEnvironment.isHeadless())installSceneDrop(id,viewport);
                viewport.addKeyListener(new KeyAdapter(){
                    @Override public void keyPressed(KeyEvent e){if(e.getKeyCode()==KeyEvent.VK_ESCAPE){mouse.cancel();e.consume();}}
                });
                viewport.addFocusListener(new FocusAdapter(){@Override public void focusLost(FocusEvent e){mouse.cancel();}});
                viewport.addHierarchyListener(e->{if(!viewport.isShowing()){mouse.cancel();if(referenceDrag!=null)referenceDrag.cancelFor(viewport);}});
                viewport.addComponentListener(new ComponentAdapter(){@Override public void componentResized(ComponentEvent e){mouse.cancel();}});
                for(int index=0;index<toolButtons.size();index++){
                    int mode=index;
                    toolButtons.get(index).addActionListener(e->{mouse.cancel();mouse.tool=mode;viewport.tool(mode);});
                }
                Runnable availability=()->{
                    boolean enabled=interactive&&snapshot!=null&&!snapshot.playing()&&gesturesAvailable.getAsBoolean();
                    toolButtons.forEach(button->button.setEnabled(enabled));
                    snap.setEnabled(enabled);
                    reset.setEnabled(enabled&&snapshot.selection().kind()==1&&snapshot.selection().document()==sceneDocument(id,snapshot));
                };
                authoringControls.add(availability);observe(value->availability.run());
                reset.addActionListener(e->{
                    if(reset.isEnabled())gestureCommand.apply(new GestureEdit(generation,snapshot.revision(),31,List.of(unsigned(snapshot.selectionRevision()))));
                });
            }
        } catch (LinkageError unavailable) { panel.add(Theme.label("Native viewport unavailable: " + unavailable.getMessage(), true)); }
        observe(value->{
            NativeViewport viewport=nativeViews.get(id);
            if(viewport==null||value==null)return;
            long document=sceneDocument(id,value);
            if(value.storage()==2)viewport.projectScene(java.nio.file.Path.of(value.path()).getParent().toString(),value.assetState().sprites().stream().filter(sprite->sprite.document()==document).toList());
            if(sceneEditor)viewport.projectItems(document,value.selection().kind()==1&&value.selection().document()==document?value.selection().element():0,
                value.sceneItems().stream().filter(item->item.document()==document).toList());
        });
        JLabel selection=Theme.label(sceneEditor?"Middle drag to pan · Wheel to zoom":"Prefab root preview",true);
        observe(value->{
            if(value==null){selection.setText("No Project");return;}
            var selected=value.selection();
            String name=value.nodes().stream().filter(node->node.selection().equals(selected)).map(Bridge.Node::label)
                .findFirst().orElse("Selection");
            selection.setText(name+" ["+unsigned(selected.element())+"]"+(sceneEditor?" · Middle drag to pan · Wheel to zoom":" · Prefab root preview"));
        });
        panel.add(selection, BorderLayout.SOUTH);
        return panel;
    }
    private static long sceneDocument(String id,Bridge.Snapshot snapshot){
        return id.contains(":")?Long.parseUnsignedLong(id.substring(id.indexOf(':')+1)):snapshot.entry();
    }
    private final class SceneInput extends MouseAdapter {
        private final String panel;
        private final NativeViewport viewport;
        private final JCheckBox snap;
        private Point pan;
        private GestureEdit active;
        private boolean pending;
        private int tool;
        SceneInput(String panel,NativeViewport viewport,JCheckBox snap){this.panel=panel;this.viewport=viewport;this.snap=snap;}
        @Override public void mousePressed(MouseEvent e){
            viewport.requestFocusInWindow();
            if(referenceDrag!=null&&e.isAltDown()&&SwingUtilities.isLeftMouseButton(e)&&canEditHierarchy()) {
                long picked=viewport.pick(e.getX(),e.getY());
                if(picked!=0)referenceDrag.startScene(viewport,sceneDocument(panel,snapshot),picked,e);
                e.consume();return;
            }
            if(SwingUtilities.isMiddleMouseButton(e)&&active==null)pan=e.getPoint();
            if(!SwingUtilities.isLeftMouseButton(e)||snapshot==null||snapshot.playing()||!interactive)return;
            long document=sceneDocument(panel,snapshot);
            var point=viewport.pointer(e.getX(),e.getY());
            if(gesturesAvailable.getAsBoolean()&&point.length==10&&point[7]!=0&&
                snapshot.selection().kind()==1&&snapshot.selection().document()==document){
                var edit=new GestureEdit(generation,snapshot.revision(),32,List.of("begin",unsigned(document),
                    tool+":"+unsigned(snapshot.selectionRevision()),Double.toString(point[0]),Double.toString(point[1]),
                    Double.toString(point[2]),Double.toString(point[3]),point[4]+","+point[5]+","+point[6]));
                active=edit;
                gestureCommand.apply(edit).whenComplete((reply,error)->SwingUtilities.invokeLater(()->{
                    if(error!=null&&active==edit)active=null;
                }));
                return;
            }
            long picked=viewport.pick(e.getX(),e.getY());
            select.accept(new Bridge.Selection(picked==0?0:1,document,picked));
        }
        @Override public void mouseDragged(MouseEvent e){
            if(pan!=null){
                viewport.pan(e.getX()-pan.x,e.getY()-pan.y);
                pan=e.getPoint();
            }else if(active!=null&&!pending){
                var edit=motion("move",e);
                if(edit==null)return;
                pending=true;
                gestureCommand.apply(edit).whenComplete((reply,error)->SwingUtilities.invokeLater(()->pending=false));
            }
        }
        @Override public void mouseReleased(MouseEvent e){
            if(SwingUtilities.isMiddleMouseButton(e))pan=null;
            if(!SwingUtilities.isLeftMouseButton(e)||active==null)return;
            var edit=motion("commit",e);
            if(edit==null)cancel();
            else {active=null;gestureCommand.apply(edit);}
        }
        @Override public void mouseWheelMoved(MouseWheelEvent e){
            e.consume();
            if(active==null)viewport.zoom(e.getX(),e.getY(),-e.getPreciseWheelRotation());
        }
        private GestureEdit motion(String action,MouseEvent e){
            var point=viewport.pointer(e.getX(),e.getY());
            return point.length!=10?null:new GestureEdit(active.generation(),active.revision(),32,
                List.of(action,Double.toString(point[0]),Double.toString(point[1]),snap.isSelected()?"1":"0"));
        }
        void cancel(){
            pan=null;
            if(active!=null){
                var edit=active;active=null;
                gestureCommand.apply(new GestureEdit(edit.generation(),edit.revision(),32,List.of("cancel")));
            }
        }
    }
    private JComponent explorer() {
        JPanel panel = Theme.island(new BorderLayout(4, 4));
        JTextField search = new JTextField(); search.setToolTipText("Search documents / assets");
        JLabel breadcrumb = Theme.label("Project / Content", true);
        JTree tree = new JTree(new DefaultMutableTreeNode("Choose a project")); tree.setRootVisible(true);
        JPanel header = new JPanel(new BorderLayout(4, 4)); header.add(search, BorderLayout.NORTH); header.add(breadcrumb, BorderLayout.SOUTH);
        JComboBox<String> mode=new JComboBox<>(new String[]{"Tree","List","Grid"});
        JCheckBox internal=new JCheckBox("Show Internal (read-only)");
        JPanel options=new JPanel(new BorderLayout());options.add(mode,BorderLayout.WEST);options.add(internal);header.add(options,BorderLayout.CENTER);
        DefaultListModel<Object> listing=new DefaultListModel<>();JList<Object> list=new JList<>(listing);
        if(!GraphicsEnvironment.isHeadless()){
            installResourceDrag(tree,()->tree.getLastSelectedPathComponent() instanceof DefaultMutableTreeNode node?node.getUserObject():null);
            installResourceDrag(list,list::getSelectedValue);
            tree.setDragEnabled(true);
            list.setDragEnabled(true);
        }
        sourceKeys(tree,()->tree.getLastSelectedPathComponent() instanceof DefaultMutableTreeNode n?n.getUserObject():null);
        sourceKeys(list,list::getSelectedValue);
        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        CardLayout cards=new CardLayout();JPanel browser=new JPanel(cards);browser.add(new JScrollPane(tree),"Tree");browser.add(new JScrollPane(list),"List");
        mode.addActionListener(e->{boolean grid=mode.getSelectedItem().equals("Grid");list.setLayoutOrientation(grid?JList.HORIZONTAL_WRAP:JList.VERTICAL);list.setVisibleRowCount(grid?-1:0);list.setFixedCellWidth(grid?220:-1);list.setFixedCellHeight(grid?64:-1);cards.show(browser,mode.getSelectedItem().equals("Tree")?"Tree":"List");});
        list.addListSelectionListener(e->{if(refreshing||e.getValueIsAdjusting())return;Object row=list.getSelectedValue();if(row instanceof AssetRow asset)select.accept(new Bridge.Selection(6,0,asset.asset().id()));else if(row instanceof DocRow doc)select.accept(new Bridge.Selection(0,doc.document().id(),0));});
        list.addMouseListener(new MouseAdapter(){@Override public void mouseClicked(MouseEvent e){if(e.getClickCount()==2){if(list.getSelectedValue() instanceof DocRow row)openDocument.accept(row.document());else if(list.getSelectedValue() instanceof SourceRow row)openSource.accept(row.path(),false);}}});
        list.addMouseListener(new MouseAdapter(){public void mousePressed(MouseEvent event){popup(event);}public void mouseReleased(MouseEvent event){popup(event);}private void popup(MouseEvent event){if(!event.isPopupTrigger())return;int index=list.locationToIndex(event.getPoint());if(index<0||!list.getCellBounds(index,index).contains(event.getPoint())||!(listing.get(index) instanceof SourceRow source))return;list.setSelectedIndex(index);var menu=new JPopupMenu();menuItem(menu,"Open With…",()->openSource.accept(source.path(),true));for(String action:List.of("copy","cut","duplicate","rename","delete"))menuItem(menu,action.substring(0,1).toUpperCase()+action.substring(1),()->sourceFileAction.accept(action,source.path()));menu.show(list,event.getX(),event.getY());}});
        panel.add(header, BorderLayout.NORTH); panel.add(browser);
        JPanel buttons = new JPanel(new GridLayout(0, 2, 4, 4));
        JButton importButton=Theme.button("Import…",importPicker);assetButtons.add(importButton);buttons.add(importButton);
        JButton create=new JButton("New Document…"),documentActions=new JButton("Document Actions…");
        JPopupMenu kinds=new JPopupMenu();
        for(int kind=0;kind<3;kind++) {
            final int type=kind;JMenuItem item=new JMenuItem(new String[]{"Scene","Prefab","Graph"}[kind]);
            item.addActionListener(event->createDocument.accept(type));kinds.add(item);
        }
        create.addActionListener(event->kinds.show(create,0,create.getHeight()));
        create.setEnabled(documentCommandsEnabled);documentButtons.add(create);buttons.add(create);
        Supplier<Object> selectedRow=()->mode.getSelectedItem().equals("Tree")
            ?(tree.getLastSelectedPathComponent() instanceof DefaultMutableTreeNode node?node.getUserObject():null)
            :list.getSelectedValue();
        Supplier<Bridge.Asset> selectedAsset=()->selectedRow.get() instanceof AssetRow asset?asset.asset():null;
        documentActions.addActionListener(event->{
            if(!(selectedRow.get() instanceof DocRow row)||snapshot==null)return;
            var doc=row.document();
            JPopupMenu menu=new JPopupMenu();
            menuItem(menu,"Rename Document…",()->documentCommand.accept("rename",doc));
            menuItem(menu,"Duplicate Document…",()->documentCommand.accept("duplicate",doc));
            JMenuItem delete=new JMenuItem("Delete Document…");
            delete.setEnabled(doc.id()!=snapshot.entry());
            delete.addActionListener(e->documentCommand.accept("delete",doc));
            menu.add(delete);
            menu.show(documentActions,0,documentActions.getHeight());
        });
        documentActions.setEnabled(false);buttons.add(documentActions);
        Map<String,JButton> assetEdits=new LinkedHashMap<>();
        for(String name:List.of("Rename / Move","Duplicate","Reimport","Delete","Restore","Assign to Prefab")){
            var button=Theme.button(name,()->{
                var selected=selectedAsset.get();
                if(selected!=null)assetCommand.accept(name,selected.id());
            });
            button.setName("asset."+name);
            assetEdits.put(name,button);buttons.add(button);
        }
        JButton watch=Theme.button("Watch Content…",()->assetCommand.accept("Watch",0L));
        assetButtons.add(watch);buttons.add(watch);
        Runnable updateAssetActions=()->{
            documentActions.setEnabled(interactive&&snapshot!=null&&!snapshot.playing()&&selectedRow.get() instanceof DocRow);
            var selected=selectedAsset.get();
            boolean ready=interactive&&snapshot!=null&&snapshot.storage()==2&&!snapshot.playing()&&selected!=null;
            assetEdits.forEach((name,button)->button.setEnabled(ready&&switch(name){
                case "Restore"->selected.trashed();
                case "Assign to Prefab"->texture(selected);
                default->!selected.trashed();
            }));
        };
        authoringControls.add(updateAssetActions);
        mode.addActionListener(event->updateAssetActions.run());
        list.addListSelectionListener(event->updateAssetActions.run());
        tree.addTreeSelectionListener(event->updateAssetActions.run());
        panel.add(buttons, BorderLayout.SOUTH);
        Set<String> shownFolders=new HashSet<>();
        Runnable rebuild = () -> {
            DefaultMutableTreeNode root = new DefaultMutableTreeNode(snapshot == null ? "Choose a project" : snapshot.title());
            DefaultMutableTreeNode docs = new DefaultMutableTreeNode("Documents"), assets = new DefaultMutableTreeNode(new FolderRow("")); root.add(docs); root.add(assets); var scripts=new DefaultMutableTreeNode("Scripts");root.add(scripts);if(internal.isSelected())root.add(new DefaultMutableTreeNode(".epidemic [service data, read-only]"));
            String filter = search.getText().toLowerCase(Locale.ROOT);
            breadcrumb.setText("Content · "+(snapshot==null?0:snapshot.assets().size())+" shown / "+(snapshot==null?0:snapshot.assetState().totalAssets())+" total · max 1000 filtered; Enter to search");
            Map<String,DefaultMutableTreeNode> folders=new LinkedHashMap<>();
            folders.put("",assets);
            if (snapshot != null) {
                for(var file:sourceFiles.get())if(file.toString().toLowerCase(Locale.ROOT).contains(filter))scripts.add(new DefaultMutableTreeNode(new SourceRow(file)));
                for (Bridge.Document doc : snapshot.documents()) if (doc.name().toLowerCase(Locale.ROOT).contains(filter)) docs.add(new DefaultMutableTreeNode(new DocRow(doc)));
                for(String folder:snapshot.contentFolders())
                    if(folder.toLowerCase(Locale.ROOT).contains(filter))contentFolder(folders,folder);
                for(Bridge.Asset asset:snapshot.assets().stream().filter(a->a.path().toLowerCase(Locale.ROOT).contains(filter))
                    .sorted(Comparator.comparing(Bridge.Asset::path)).limit(1000).toList()){
                    String path=asset.path().replaceFirst("^Content/","");
                    int slash=path.lastIndexOf('/');
                    var parent=slash<0?assets:contentFolder(folders,path.substring(0,slash));
                    parent.add(new DefaultMutableTreeNode(new AssetRow(asset)));
                }
            }
            // A rename/revision refresh must keep the native selection and the
            // user's expanded folders/scroll, without emitting another edit.
            Set<String> expanded=new HashSet<>();
            var oldRoot=new TreePath(tree.getModel().getRoot());var paths=tree.getExpandedDescendants(oldRoot);
            if(paths!=null)while(paths.hasMoreElements())expanded.add(treeKey(paths.nextElement()));
            Point scroll=tree.getParent() instanceof JViewport viewport?viewport.getViewPosition():null;
            Point listScroll=list.getParent() instanceof JViewport viewport?viewport.getViewPosition():null;
            boolean wasRefreshing=refreshing;refreshing=true;
            try{
                tree.setModel(new DefaultTreeModel(root));tree.expandRow(0);
                listing.clear();
                for(var rows=root.depthFirstEnumeration();rows.hasMoreElements();){var item=(DefaultMutableTreeNode)rows.nextElement();if(item.getUserObject() instanceof DocRow||item.getUserObject() instanceof AssetRow||item.getUserObject() instanceof SourceRow)listing.addElement(item.getUserObject());}
                if(snapshot!=null)for(var nodes=root.depthFirstEnumeration();nodes.hasMoreElements();){
                    var node=(DefaultMutableTreeNode)nodes.nextElement();TreePath path=new TreePath(node.getPath());
                    if(expanded.contains(treeKey(path))||expanded.isEmpty()&&node==docs)tree.expandPath(path);
                    if(node.getUserObject() instanceof DocRow row&&row.document().id()==snapshot.selection().document() || node.getUserObject() instanceof AssetRow assetRow&&snapshot.selection().kind()==6&&assetRow.asset().id()==snapshot.selection().element()){
                        boolean expand=tree.getExpandsSelectedPaths();tree.setExpandsSelectedPaths(false);
                        try{tree.setSelectionPath(path);}finally{tree.setExpandsSelectedPaths(expand);}
                    }
                }
                if(shownFolders.isEmpty()&&!folders.keySet().equals(Set.of("")))tree.expandPath(new TreePath(assets.getPath()));
                if(!shownFolders.isEmpty())
                    folders.forEach((key,node)->{
                        if(!shownFolders.contains(key)&&node.getParent() instanceof DefaultMutableTreeNode parent)
                            tree.expandPath(new TreePath(parent.getPath()));
                    });
                shownFolders.clear();
                if(snapshot!=null)shownFolders.addAll(folders.keySet());
                if(scroll!=null&&tree.getParent() instanceof JViewport viewport)viewport.setViewPosition(scroll);
                if(listScroll!=null&&list.getParent() instanceof JViewport viewport)viewport.setViewPosition(listScroll);
            }finally{refreshing=wasRefreshing;}
        };
        sourceRefreshers.add(rebuild);
        structure(value -> rebuild.run()); search.addActionListener(event -> {assetCommand.accept("Search:"+search.getText(),0L);rebuild.run();});internal.addActionListener(event->rebuild.run());
        observe(value->{
            if(value==null){updateAssetActions.run();return;}
            if(selectedRow.get() instanceof SourceRow){updateAssetActions.run();return;}
            int selectedIndex=-1;
            for(int i=0;i<listing.size();i++){Object row=listing.get(i);if(row instanceof DocRow doc&&doc.document().id()==value.selection().document() || row instanceof AssetRow asset&&value.selection().kind()==6&&asset.asset().id()==value.selection().element()){selectedIndex=i;break;}}
            list.setSelectedIndex(selectedIndex);
            var root=(DefaultMutableTreeNode)tree.getModel().getRoot();
            for(var nodes=root.depthFirstEnumeration();nodes.hasMoreElements();){
                var node=(DefaultMutableTreeNode)nodes.nextElement();
                if(node.getUserObject() instanceof DocRow row&&row.document().id()==value.selection().document() || node.getUserObject() instanceof AssetRow assetRow&&value.selection().kind()==6&&assetRow.asset().id()==value.selection().element()){
                    boolean expand=tree.getExpandsSelectedPaths();tree.setExpandsSelectedPaths(false);
                    tree.setSelectionPath(new TreePath(node.getPath()));tree.setExpandsSelectedPaths(expand);updateAssetActions.run();return;
                }
            }
            tree.clearSelection();
            updateAssetActions.run();
        });
        tree.addTreeSelectionListener(event->{
            if(refreshing)return;Object chosen=tree.getLastSelectedPathComponent();
            if(chosen instanceof DefaultMutableTreeNode node&&node.getUserObject() instanceof DocRow row)
                select.accept(new Bridge.Selection(0,row.document().id(),0));
            else if(chosen instanceof DefaultMutableTreeNode node&&node.getUserObject() instanceof AssetRow row)select.accept(new Bridge.Selection(6,0,row.asset().id()));
            else if(chosen==tree.getModel().getRoot())select.accept(new Bridge.Selection(0,0,0));
        });
        tree.addMouseListener(new MouseAdapter() {
            @Override public void mousePressed(MouseEvent event){folderMenu(event);}
            @Override public void mouseReleased(MouseEvent event){folderMenu(event);}
            private void folderMenu(MouseEvent event){
                if(!event.isPopupTrigger()||!interactive||snapshot==null)return;
                var path=tree.getPathForLocation(event.getX(),event.getY());
                if(path==null||!(path.getLastPathComponent() instanceof DefaultMutableTreeNode node))return;
                if(node.getUserObject() instanceof SourceRow source){tree.setSelectionPath(path);var menu=new JPopupMenu();menuItem(menu,"Open C#",()->openSource.accept(source.path(),false));menuItem(menu,"Open With…",()->openSource.accept(source.path(),true));for(String action:List.of("copy","cut","duplicate","rename","delete"))menuItem(menu,action.substring(0,1).toUpperCase()+action.substring(1),()->sourceFileAction.accept(action,source.path()));menu.show(tree,event.getX(),event.getY());return;}
                if(node.getUserObject().equals("Scripts")){var menu=new JPopupMenu();menuItem(menu,"New C# Class…",newSource);menuItem(menu,"Paste C# Source…",()->sourceFileAction.accept("paste",null));menu.show(tree,event.getX(),event.getY());return;}
                if(newContentFolder==null||snapshot.storage()!=2||snapshot.playing()||!(node.getUserObject() instanceof FolderRow folder))return;
                var menu=new JPopupMenu();
                menuItem(menu,"New Folder…",()->newContentFolder.accept(folder.path()));
                menu.show(tree,event.getX(),event.getY());
            }
            @Override public void mouseClicked(MouseEvent event) {
            if (event.getClickCount() != 2 || refreshing) return;
            Object selected = tree.getLastSelectedPathComponent();
            if (selected instanceof DefaultMutableTreeNode node && node.getUserObject() instanceof DocRow row) {
                openDocument.accept(row.document());
            }else if(selected instanceof DefaultMutableTreeNode node&&node.getUserObject() instanceof SourceRow source)openSource.accept(source.path(),false);
        } });
        return panel;
    }
    private record SourceRow(java.nio.file.Path path){public String toString(){return path.getFileName().toString();}}
    private record FolderRow(String path){
        @Override public String toString(){return path.isEmpty()?"Content":path.substring(path.lastIndexOf('/')+1);}
    }
    private static DefaultMutableTreeNode contentFolder(Map<String,DefaultMutableTreeNode> folders,String path){
        String prefix="";
        var parent=folders.get("");
        for(String part:path.split("/")){
            prefix=prefix.isEmpty()?part:prefix+"/"+part;
            String key=prefix.toLowerCase(Locale.ROOT);
            var folder=folders.get(key);
            if(folder==null){
                folder=new DefaultMutableTreeNode(new FolderRow(prefix));
                folders.put(key,folder);
                parent.add(folder);
            }
            parent=folder;
        }
        return parent;
    }
    private record AssetRow(Bridge.Asset asset){@Override public String toString(){return asset.path()+(asset.trashed()?" · Trash":!asset.present()?" · Missing":"");}}
    private record DocRow(Bridge.Document document) { @Override public String toString() { String type=switch(document.kind()){case 0->"Scene";case 1->"Prefab";case 2->"Graph";default->"Document";};return document.name().equals(type)?type:document.name()+" · "+type; } }
    private String nodeLabel(Bridge.Node row){
        if(row.kind()==3&&snapshot!=null)for(var graph:snapshot.graphs())if(graph.document()==row.document())
            for(var node:graph.nodes())if(node.id()==row.id()){
                String type=graph.catalogue().stream().filter(r->r.type()==node.type()).map(Bridge.GraphTemplate::name).findFirst().orElse("Graph");
                return node.name().equals(type)?type:node.name()+" · "+type;
            }
        if(row.kind()==2&&row.label().equals("Prefab entity "+unsigned(row.id())))
            return row.parent()==0&&snapshot!=null?snapshot.documents().stream().filter(d->d.id()==row.document()).map(Bridge.Document::name).findFirst().orElse("Entity"):"Entity";
        if(row.kind()==1){String prefix="Entity "+unsigned(row.id());if(row.label().startsWith(prefix))return "Entity"+row.label().substring(prefix.length());}
        return row.kind()==3?"Graph":row.label();
    }
    private static String treeKey(TreePath path){
        StringBuilder key=new StringBuilder();Object[] parts=path.getPath();
        for(int i=1;i<parts.length;i++){
            Object value=((DefaultMutableTreeNode)parts[i]).getUserObject();
            key.append('/').append(value instanceof DocRow row?Long.toUnsignedString(row.document().id()):value);
        }
        return key.toString();
    }
    private JComponent hierarchy() {
        JTree tree = new JTree();
        Map<Bridge.Selection,DefaultMutableTreeNode> selections=new HashMap<>();
        structure(value -> {
            DefaultMutableTreeNode root = new DefaultMutableTreeNode("World / Scene / Prefab");
            selections.clear();
            if (value != null) {
                Map<Long,DefaultMutableTreeNode> documents = new HashMap<>();
                for (Bridge.Document doc : value.documents()) {
                    DefaultMutableTreeNode node = new DefaultMutableTreeNode(new DocRow(doc)); root.add(node); documents.put(doc.id(), node);
                    selections.put(new Bridge.Selection(0,doc.id(),0),node);
                }
                Map<String,DefaultMutableTreeNode> nodes = new HashMap<>();
                Map<DefaultMutableTreeNode,DefaultMutableTreeNode> componentGroups=new HashMap<>();
                for (Bridge.Node row : value.nodes()) {
                    var node=new DefaultMutableTreeNode(row);
                    nodes.put(nodeKey(row),node);
                    selections.put(row.selection(),node);
                }
                for (Bridge.Node row : value.nodes()) {
                    DefaultMutableTreeNode parent = nodes.get(row.document() + ":" + row.parent()+":"+(row.kind()==5||row.kind()==7?2:row.kind())+":0:0");
                    if (parent == null) parent = documents.get(row.document());
                    if(parent!=null&&row.kind()==5) {
                        var group=componentGroups.get(parent);
                        if(group==null){group=new DefaultMutableTreeNode("Components");parent.add(group);componentGroups.put(parent,group);}
                        parent=group;
                    }
                    if (parent != null) parent.add(nodes.get(nodeKey(row)));
                }
            }
            Set<String> expanded=new HashSet<>();var previous=tree.getExpandedDescendants(new TreePath(tree.getModel().getRoot()));
            if(previous!=null)while(previous.hasMoreElements())expanded.add(treeKey(previous.nextElement()));
            Point scroll=tree.getParent() instanceof JViewport viewport?viewport.getViewPosition():null;
            tree.setModel(new DefaultTreeModel(root));tree.expandRow(0);
            for(var enumeration=root.depthFirstEnumeration();enumeration.hasMoreElements();){
                var node=(DefaultMutableTreeNode)enumeration.nextElement();var path=new TreePath(node.getPath());
                if(expanded.contains(treeKey(path)))tree.expandPath(path);
            }
            if(scroll!=null&&tree.getParent() instanceof JViewport viewport)viewport.setViewPosition(scroll);
        });
        observe(value->{
            var node=value==null?null:selections.get(value.selection());
            var path=node==null?null:new TreePath(node.getPath());
            if(!Objects.equals(tree.getSelectionPath(),path)){
                tree.setExpandsSelectedPaths(false);
                tree.setSelectionPath(path);
                tree.setExpandsSelectedPaths(true);
            }
        });
        tree.setCellRenderer(new DefaultTreeCellRenderer() {
            @Override public Component getTreeCellRendererComponent(JTree owner, Object value, boolean selected, boolean expanded, boolean leaf, int row, boolean focus) {
                Component result = super.getTreeCellRendererComponent(owner, value, selected, expanded, leaf, row, focus);
                if (value instanceof DefaultMutableTreeNode node && node.getUserObject() instanceof Bridge.Node data) setText(nodeLabel(data));
                return result;
            }
        });
        tree.addTreeSelectionListener(event -> {
            if (refreshing) return;
            Object value = tree.getLastSelectedPathComponent();
            if (value instanceof DefaultMutableTreeNode node && node.getUserObject() instanceof Bridge.Node row)
                select.accept(row.selection());
            else if(value instanceof DefaultMutableTreeNode node&&node.getUserObject() instanceof DocRow doc)select.accept(new Bridge.Selection(0,doc.document().id(),0));
            else if(value==tree.getModel().getRoot())select.accept(new Bridge.Selection(0,0,0));
        });
        tree.addMouseListener(new MouseAdapter(){
            @Override public void mousePressed(MouseEvent event){popup(event);}
            @Override public void mouseReleased(MouseEvent event){popup(event);}
            private void popup(MouseEvent event){
                if(!event.isPopupTrigger()||!canEditHierarchy())return;
                var path=tree.getPathForLocation(event.getX(),event.getY());
                var target=hierarchyTarget(path);
                if(target==null&&path!=null&&path.getLastPathComponent() instanceof DefaultMutableTreeNode node&&
                    node.getUserObject() instanceof Bridge.Node row&&(row.kind()==2||row.kind()==7)){
                    var menu=new JPopupMenu();
                    var base=snapshot;
                    long token=generation;
                    if(row.kind()==2&&row.parent()!=0)menuItem(menu,"Reparent Prefab Child…",()->{
                        var parents=base.nodes().stream().filter(parent->parent.kind()==2&&parent.document()==row.document()&&parent.id()!=row.id()).toList();
                        int chosen=choose(tree,"Logical Prefab Parent",parents.stream().map(Bridge.Node::label).toList());
                        if(chosen>=0)hierarchyCommand.accept(new HierarchyEdit(token,base.revision(),"prefab-parent",
                            row.document(),row.id(),unsigned(parents.get(chosen).id())));
                    });
                    if(row.kind()==2)menuItem(menu,"Link Nested Prefab…",()->{
                        var prefabs=base.documents().stream().filter(doc->doc.kind()==1&&doc.id()!=row.document()).toList();
                        if(prefabs.isEmpty())return;
                        int chosen=choose(tree,"Link Nested Prefab",prefabs.stream().map(Bridge.Document::name).toList());
                        if(chosen>=0)hierarchyCommand.accept(new HierarchyEdit(token,base.revision(),"link-prefab",
                            row.document(),row.id(),unsigned(prefabs.get(chosen).id())));
                    });
                    if(row.kind()==7){
                        menuItem(menu,"Open Linked Definition",()->base.documents().stream().filter(doc->doc.id()==row.prefab())
                            .findFirst().ifPresent(openDocument));
                        menuItem(menu,"Remove Prefab Link",()->hierarchyCommand.accept(new HierarchyEdit(token,base.revision(),
                            "unlink-prefab",row.document(),row.id(),null)));
                    }
                    menu.show(tree,event.getX(),event.getY());
                    return;
                }
                if(target==null)return;
                var menu=new JPopupMenu();
                long document=target.document(),node=target.element();
                long token=generation,revision=snapshot.revision();
                BiConsumer<String,String> edit=(action,argument)->hierarchyCommand.accept(new HierarchyEdit(token,revision,action,document,node,argument));
                menuItem(menu,node==0?"New Entity":"Create Child",()->edit.accept("create",null));
                if(node!=0){
                    menuItem(menu,"Rename",()->{
                        String name=JOptionPane.showInputDialog(tree,"Entity name");
                        if(name!=null&&!name.isBlank())edit.accept("rename",name);
                    });
                    menuItem(menu,"Delete Subtree",()->{
                        if(JOptionPane.showConfirmDialog(tree,"Delete this entity and its children?","Delete Subtree",JOptionPane.OK_CANCEL_OPTION)==JOptionPane.OK_OPTION)
                            edit.accept("delete",null);
                    });
                    menuItem(menu,"Reparent…",()->{
                        var parents=snapshot.nodes().stream().filter(row->row.kind()==1&&row.document()==document&&row.id()!=node).toList();
                        if(parents.isEmpty())return;
                        var choice=choose(tree,"Reparent (keep local transform)",parents.stream().map(row->row.label()+" ["+unsigned(row.id())+"]").toList());
                        if(choice>=0)edit.accept("parent",unsigned(parents.get(choice).id()));
                    });
                    menuItem(menu,"Unparent",()->edit.accept("parent","0"));
                    menuItem(menu,"Assign Prefab…",()->{
                        var prefabs=snapshot.documents().stream().filter(doc->doc.kind()==1).toList();
                        if(prefabs.isEmpty())return;
                        var choice=choose(tree,"Assign Prefab",prefabs.stream().map(doc->doc.name()+" ["+unsigned(doc.id())+"]").toList());
                        if(choice>=0)edit.accept("prefab",unsigned(prefabs.get(choice).id()));
                    });
                }
                menu.show(tree,event.getX(),event.getY());
            }
        });
        if(!GraphicsEnvironment.isHeadless()){
            tree.setDragEnabled(true);
            tree.setDropMode(DropMode.ON_OR_INSERT);
            final boolean[] referenceSource={false};
            final Runnable clearReference=()->tree.setToolTipText(null);
            tree.addMouseListener(new MouseAdapter(){@Override public void mousePressed(MouseEvent e){referenceSource[0]=e.isAltDown();}});
            tree.addHierarchyListener(e->{if(!tree.isShowing()&&referenceDrag!=null)referenceDrag.cancelFor(tree);});
            tree.setTransferHandler(new TransferHandler(){
                private final DataFlavor flavor=localFlavor(HierarchyEdit.class,"Scene hierarchy");
                @Override public int getSourceActions(JComponent component){return referenceSource[0]?COPY:MOVE;}
                @Override protected Transferable createTransferable(JComponent component){
                    var source=referenceSource[0]?UiPanels.this.referenceTarget(tree.getSelectionPath()):hierarchyTarget(tree.getSelectionPath());
                    if(!canEditHierarchy()||source==null||source.element()==0)return null;
                    if(referenceSource[0]&&referenceDrag!=null)return referenceDrag.sceneTransfer(tree,source.document(),source.element());
                    var move=new HierarchyEdit(generation,snapshot.revision(),"parent",source.document(),source.element(),null);
                    return new Transferable(){
                        public DataFlavor[] getTransferDataFlavors(){return new DataFlavor[]{flavor};}
                        public boolean isDataFlavorSupported(DataFlavor type){return flavor.equals(type);}
                        public Object getTransferData(DataFlavor type)throws UnsupportedFlavorException{
                            if(!isDataFlavorSupported(type))throw new UnsupportedFlavorException(type);
                            return move;
                        }
                    };
                }
                private HierarchyEdit drop(TransferSupport support){
                    if(!support.isDrop()||!canEditHierarchy()||!support.isDataFlavorSupported(flavor))return null;
                    var location=(JTree.DropLocation)support.getDropLocation();
                    var target=hierarchyTarget(location.getPath());
                    try{
                        var edit=(HierarchyEdit)support.getTransferable().getTransferData(flavor);
                        if(target==null||target.document()!=edit.document()||target.element()==edit.node()
                            ||generation!=edit.generation()||snapshot.revision()!=edit.revision())return null;
                        int index=location.getChildIndex();
                        if(index<0)
                            return new HierarchyEdit(edit.generation(),edit.revision(),"parent",edit.document(),edit.node(),unsigned(target.element()));
                        var parent=(DefaultMutableTreeNode)location.getPath().getLastPathComponent();
                        long before=0;
                        if(index<parent.getChildCount()){
                            var child=(DefaultMutableTreeNode)parent.getChildAt(index);
                            if(!(child.getUserObject() instanceof Bridge.Node row)||row.kind()!=1)return null;
                            before=row.id();
                        }
                        if(before==edit.node())return null;
                        return new HierarchyEdit(edit.generation(),edit.revision(),"order",edit.document(),edit.node(),unsigned(target.element())+":"+unsigned(before));
                    }catch(UnsupportedFlavorException|java.io.IOException error){return null;}
                }
                private Bridge.Selection referenceTarget(TransferSupport support){return support.isDrop()?UiPanels.this.referenceTarget(((JTree.DropLocation)support.getDropLocation()).getPath()):null;}
                @Override public boolean canImport(TransferSupport support){
                    if(referenceDrag!=null&&referenceDrag.fromGraph(support.getTransferable())) {
                        var target=referenceTarget(support);if(target==null)return false;
                        support.setDropAction(COPY);
                        return referenceDrag.hoverScene(support.getTransferable(),target.document(),target.element(),(valid,reason)->tree.setToolTipText(reason),clearReference);
                    }
                    return drop(support)!=null;
                }
                @Override public boolean importData(TransferSupport support){
                    if(referenceDrag!=null&&referenceDrag.fromGraph(support.getTransferable())) {
                        var target=referenceTarget(support);if(target==null)return false;
                        referenceDrag.dropScene(support.getTransferable(),target.document(),target.element(),tree);return true;
                    }
                    var edit=drop(support);if(edit==null)return false;hierarchyCommand.accept(edit);return true;
                }
                @Override protected void exportDone(JComponent source,Transferable data,int action){if(referenceDrag!=null&&data!=null)referenceDrag.sourceDone(data);}
            });
        }
        return new JScrollPane(tree);
    }
    private boolean canEditHierarchy(){return interactive&&snapshot!=null&&!snapshot.playing()&&hierarchyAvailable.getAsBoolean();}
    private boolean canEditSystems(){return interactive&&snapshot!=null&&!snapshot.playing()&&systemsAvailable.getAsBoolean();}
    private JComponent systems(){
        var panel=new JPanel(new BorderLayout(4,4));
        var world=new JCheckBox("World scope");
        var rows=new DefaultListModel<Bridge.SystemItem>();
        var list=new JList<>(rows);
        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        list.setCellRenderer(new DefaultListCellRenderer(){
            @Override public Component getListCellRendererComponent(JList<?> owner,Object value,int index,boolean selected,boolean focused){
                super.getListCellRendererComponent(owner,value,index,selected,focused);
                if(value instanceof Bridge.SystemItem system&&snapshot!=null){
                    String graph=snapshot.documents().stream().filter(doc->doc.id()==system.graph()).map(Bridge.Document::name)
                        .findFirst().orElse("Missing Graph");
                    setText((system.enabled()?"Enabled":"Disabled")+" · "+graph+" ["+unsigned(system.graph())+"]");
                }
                return this;
            }
        });
        var controls=new JPanel(new FlowLayout(FlowLayout.LEFT));
        JButton add=new JButton("Add System…"),edit=new JButton("Change Graph…");
        JButton toggle=new JButton("Enable / Disable"),remove=new JButton("Remove");
        for(var button:List.of(add,edit,toggle,remove))controls.add(button);
        panel.add(world,BorderLayout.NORTH);panel.add(new JScrollPane(list));panel.add(controls,BorderLayout.SOUTH);
        LongSupplier scope=()->world.isSelected()||snapshot==null?0:
            snapshot.documents().stream().filter(doc->doc.kind()==0&&doc.id()==snapshot.selection().document())
                .mapToLong(Bridge.Document::id).findFirst().orElse(snapshot.entry());
        Runnable availability=()->{
            boolean graph=canEditSystems()&&snapshot.documents().stream().anyMatch(doc->doc.kind()==2);
            add.setEnabled(graph);
            edit.setEnabled(graph&&list.getSelectedValue()!=null);
            toggle.setEnabled(canEditSystems()&&list.getSelectedValue()!=null);
            remove.setEnabled(toggle.isEnabled());
        };
        Runnable refresh=()->{
            var desired=snapshot==null?List.<Bridge.SystemItem>of():snapshot.systems().stream().filter(row->row.scene()==scope.getAsLong()).toList();
            if(!Collections.list(rows.elements()).equals(desired)){
                long selected=list.getSelectedValue()==null?0:list.getSelectedValue().id();
                rows.clear();desired.forEach(rows::addElement);
                for(int i=0;i<rows.size();i++)if(rows.get(i).id()==selected)list.setSelectedIndex(i);
            }
            list.repaint();availability.run();
        };
        observe(value->refresh.run());authoringControls.add(availability);
        world.addActionListener(e->refresh.run());
        list.addListSelectionListener(e->availability.run());
        BiConsumer<String,Long> chooseGraph=(action,target)->{
            var base=snapshot;
            long token=generation;
            var graphs=base.documents().stream().filter(doc->doc.kind()==2).toList();
            int chosen=choose(panel,action.equals("add")?"Add Graph System":"Change System Graph",
                graphs.stream().map(doc->doc.name()+" ["+unsigned(doc.id())+"]").toList());
            if(chosen>=0)gestureCommand.apply(new GestureEdit(token,base.revision(),34,List.of(action,unsigned(target),unsigned(graphs.get(chosen).id()))));
        };
        add.addActionListener(e->{if(add.isEnabled())chooseGraph.accept("add",scope.getAsLong());});
        edit.addActionListener(e->{if(edit.isEnabled())chooseGraph.accept("graph",list.getSelectedValue().id());});
        toggle.addActionListener(e->{
            if(!toggle.isEnabled())return;
            var system=list.getSelectedValue();
            gestureCommand.apply(new GestureEdit(generation,snapshot.revision(),34,List.of("enable",unsigned(system.id()),system.enabled()?"0":"1")));
        });
        remove.addActionListener(e->{
            if(remove.isEnabled())gestureCommand.apply(new GestureEdit(generation,snapshot.revision(),34,List.of("remove",unsigned(list.getSelectedValue().id()))));
        });
        return panel;
    }
    private record ResourceDrag(long generation,long project,long revision,long id,boolean prefab) {}
    private static final DataFlavor RESOURCE_FLAVOR=localFlavor(ResourceDrag.class,"Project resource");
    private static DataFlavor localFlavor(Class<?> type,String label){
        try{
            return new DataFlavor(DataFlavor.javaJVMLocalObjectMimeType+";class="+type.getName(),label,type.getClassLoader());
        }catch(ClassNotFoundException error){throw new ExceptionInInitializerError(error);}
    }
    private boolean canPlaceResource(){return interactive&&snapshot!=null&&!snapshot.playing()&&sceneResourcesAvailable.getAsBoolean();}
    static boolean texture(Bridge.Asset asset){
        String path=asset.path().toLowerCase(Locale.ROOT);
        return !asset.trashed()&&!asset.cookedKey().isEmpty()&&(path.endsWith(".png")||path.endsWith(".webp"));
    }
    private void installResourceDrag(JComponent source,Supplier<Object> selection){
        source.setTransferHandler(new TransferHandler(){
            @Override public int getSourceActions(JComponent component){return COPY;}
            @Override protected Transferable createTransferable(JComponent component){
                if(!canPlaceResource())return null;
                Object row=selection.get();
                ResourceDrag data;
                if(row instanceof DocRow doc&&doc.document().kind()==1)
                    data=new ResourceDrag(generation,snapshot.project(),snapshot.revision(),doc.document().id(),true);
                else if(row instanceof AssetRow asset&&texture(asset.asset()))
                    data=new ResourceDrag(generation,snapshot.project(),snapshot.revision(),asset.asset().id(),false);
                else return null;
                return new Transferable(){
                    public DataFlavor[] getTransferDataFlavors(){return new DataFlavor[]{RESOURCE_FLAVOR};}
                    public boolean isDataFlavorSupported(DataFlavor flavor){return RESOURCE_FLAVOR.equals(flavor);}
                    public Object getTransferData(DataFlavor flavor)throws UnsupportedFlavorException{
                        if(!isDataFlavorSupported(flavor))throw new UnsupportedFlavorException(flavor);
                        return data;
                    }
                };
            }
        });
    }
    private void installSceneDrop(String panel,NativeViewport viewport){
        final Runnable clearReference=()->{viewport.referenceHover(0);viewport.setCursor(Cursor.getDefaultCursor());};
        new DropTarget(viewport,DnDConstants.ACTION_COPY,new DropTargetAdapter(){
            private void hover(DropTargetDragEvent e){
                if(referenceDrag==null||!referenceDrag.fromGraph(e.getTransferable())){e.acceptDrag(DnDConstants.ACTION_COPY);return;}
                long picked=viewport.pick(e.getLocation().x,e.getLocation().y);
                boolean valid=referenceDrag.hoverScene(e.getTransferable(),sceneDocument(panel,snapshot),picked,(ok,reason)->{viewport.referenceHover(ok?picked:0);viewport.setCursor(Cursor.getPredefinedCursor(ok?Cursor.CROSSHAIR_CURSOR:Cursor.DEFAULT_CURSOR));},clearReference);
                if(valid)e.acceptDrag(DnDConstants.ACTION_COPY);else e.rejectDrag();
            }
            @Override public void dragEnter(DropTargetDragEvent e){hover(e);}
            @Override public void dragOver(DropTargetDragEvent e){hover(e);}
            @Override public void dragExit(DropTargetEvent e){viewport.referenceHover(0);viewport.setCursor(Cursor.getDefaultCursor());}
            @Override public void drop(DropTargetDropEvent event){
                if(referenceDrag!=null&&referenceDrag.fromGraph(event.getTransferable())) {
                    long picked=viewport.pick(event.getLocation().x,event.getLocation().y);
                    if(picked==0){event.rejectDrop();referenceDrag.cancel();return;}
                    event.acceptDrop(DnDConstants.ACTION_COPY);referenceDrag.dropScene(event.getTransferable(),sceneDocument(panel,snapshot),picked,viewport);event.dropComplete(true);return;
                }
                if(!canPlaceResource()||!event.isDataFlavorSupported(RESOURCE_FLAVOR)){event.rejectDrop();return;}
                try {
                    event.acceptDrop(DnDConstants.ACTION_COPY);
                    var data=(ResourceDrag)event.getTransferable().getTransferData(RESOURCE_FLAVOR);
                    var base=snapshot;
                    if(data.generation()!=generation||data.project()!=base.project()||data.revision()!=base.revision()){
                        event.dropComplete(false);return;
                    }
                    long scene=sceneDocument(panel,base);
                    Point cursor=event.getLocation();
                    if(data.prefab()){
                        double[] point=viewport.pointer(cursor.x,cursor.y);
                        if(point.length!=10){event.dropComplete(false);return;}
                        gestureCommand.apply(new GestureEdit(generation,base.revision(),33,List.of("place",unsigned(scene),
                            unsigned(data.id()),Double.toString(point[8]),Double.toString(point[9]))));
                    }else{
                        long picked=viewport.pick(cursor.x,cursor.y);
                        if(base.nodes().stream().noneMatch(node->node.document()==scene&&node.id()==picked&&node.prefab()!=0)){
                            if(picked!=0){event.dropComplete(false);return;}
                            double[] point=viewport.pointer(cursor.x,cursor.y);
                            var prefabs=base.assetState().references().stream()
                                .filter(ref->ref.source()==data.id()&&ref.origin().equals("prefab.sprite"))
                                .map(ref->base.documents().stream().filter(doc->doc.kind()==1&&doc.id()==ref.dependent()).findFirst())
                                .flatMap(Optional::stream).distinct().toList();
                            if(point.length!=10||prefabs.isEmpty()){event.dropComplete(false);return;}
                            SwingUtilities.invokeLater(()->{
                                if(!canPlaceResource()||generation!=data.generation())return;
                                int choice=choose(viewport,"Place Prefab Using This Sprite",prefabs.stream().map(Bridge.Document::name).toList());
                                if(choice>=0)gestureCommand.apply(new GestureEdit(data.generation(),base.revision(),33,
                                    List.of("place",unsigned(scene),unsigned(prefabs.get(choice).id()),Double.toString(point[8]),Double.toString(point[9]))));
                            });
                        }else{
                            SwingUtilities.invokeLater(()->assignSprite(viewport,base,data.generation(),scene,picked,data.id()));
                        }
                    }
                    event.dropComplete(true);
                }catch(UnsupportedFlavorException|java.io.IOException error){event.dropComplete(false);}
            }
        });
    }
    private void assignSprite(Component parent,Bridge.Snapshot base,long token,long document,long node,long texture){
        if(!canPlaceResource()||generation!=token)return;
        if(node!=0&&JOptionPane.showConfirmDialog(parent,"Assign to the Prefab definition? All its instances will use this sprite.",
            "Assign Sprite",JOptionPane.OK_CANCEL_OPTION)!=JOptionPane.OK_OPTION)return;
        gestureCommand.apply(new GestureEdit(token,base.revision(),33,List.of("sprite",unsigned(base.assetRevision()),
            unsigned(document),unsigned(node),unsigned(texture))));
    }
    private static long selectedPrefab(Bridge.Snapshot snapshot){
        var selection=snapshot.selection();
        if(selection.kind()==7)return 0;
        if(selection.kind()==1)return snapshot.nodes().stream().filter(node->node.kind()==1&&node.document()==selection.document()&&node.id()==selection.element())
            .mapToLong(Bridge.Node::prefab).findFirst().orElse(0);
        return snapshot.documents().stream().anyMatch(doc->doc.kind()==1&&doc.id()==selection.document())?selection.document():0;
    }
    private JPanel resourceActions(){
        var panel=new JPanel(new FlowLayout(FlowLayout.LEFT));
        var sprite=new JButton("Choose Sprite…");
        var definition=new JButton("Open Prefab Definition");
        var binding=new JLabel();
        panel.add(sprite);panel.add(definition);panel.add(binding);
        Runnable update=()->{
            long prefab=snapshot==null?0:selectedPrefab(snapshot);
            panel.setVisible(prefab!=0);
            sprite.setEnabled(prefab!=0&&canPlaceResource()&&snapshot.storage()==2&&snapshot.assets().stream().anyMatch(UiPanels::texture));
            definition.setVisible(prefab!=0&&snapshot.selection().kind()==1);
            binding.setText(prefab==0?"":snapshot.assetState().references().stream()
                .filter(ref->ref.dependent()==prefab&&ref.origin().equals("prefab.sprite")).findFirst()
                .map(ref->snapshot.assets().stream().filter(asset->asset.id()==ref.source()).findFirst()
                    .map(asset->(asset.trashed()||!asset.present()?"Missing: ":"Sprite: ")+asset.path()+" ["+unsigned(asset.id())+"]")
                    .orElse("Sprite ID "+unsigned(ref.source()))).orElse("No sprite"));
        };
        observe(value->update.run());authoringControls.add(update);
        definition.addActionListener(e->{
            if(snapshot==null)return;
            long prefab=selectedPrefab(snapshot);
            snapshot.documents().stream().filter(doc->doc.id()==prefab).findFirst().ifPresent(openDocument);
            snapshot.nodes().stream().filter(node->node.kind()==2&&node.parent()==0&&node.document()==prefab).findFirst()
                .ifPresent(node->select.accept(node.selection()));
        });
        sprite.addActionListener(e->{
            if(!canPlaceResource())return;
            var base=snapshot;
            long token=generation;
            var textures=base.assets().stream().filter(UiPanels::texture).toList();
            int chosen=choose(panel,"Choose Imported Sprite",textures.stream().map(asset->asset.path()+" ["+unsigned(asset.id())+"]").toList());
            if(chosen>=0)assignSprite(panel,base,token,base.selection().document(),base.selection().kind()==1?base.selection().element():0,textures.get(chosen).id());
        });
        return panel;
    }
    private boolean canEditComponent(){return interactive&&snapshot!=null&&!snapshot.playing()&&componentAvailable.getAsBoolean();}
    static String componentLabel(String name){
        String prefix="editor.example.";
        int at=name.indexOf(prefix);
        if(at<0||at+prefix.length()>=name.length())return name;
        int start=at+prefix.length();
        return name.substring(0,at)+Character.toUpperCase(name.charAt(start))+name.substring(start+1);
    }
    private List<Bridge.ComponentType> availableComponents(Bridge.Snapshot value){
        var selected=value.selection();
        return value.componentTypes().stream().filter(type->value.nodes().stream().noneMatch(node->
            node.kind()==5&&node.document()==selected.document()&&node.id()==selected.element()&&
                node.component()==type.id()&&node.part()==0)).toList();
    }
    private JPanel componentActions(){
        JPanel toolbar=new JPanel(new FlowLayout(FlowLayout.LEFT));
        JButton add=new JButton("Add Component"),remove=new JButton("Remove Component"),graph=new JButton("Attach Graph…");
        add.setToolTipText("Add a registered component to this Prefab entity. Placements inherit it.");
        remove.setToolTipText("Remove this component from the Prefab definition.");
        JButton child=new JButton("Create Prefab Child"),deleteChild=new JButton("Delete Prefab Child");
        for(var button:List.of(add,remove,graph,child,deleteChild))toolbar.add(button);
        Runnable update=()->{
            int kind=snapshot==null?0:snapshot.selection().kind();
            toolbar.setVisible(kind==2||kind==5);
            add.setEnabled(canEditComponent()&&kind==2&&!availableComponents(snapshot).isEmpty());
            remove.setEnabled(canEditComponent()&&(kind==5||(kind==2&&snapshot.nodes().stream().anyMatch(n->n.kind()==5&&n.document()==snapshot.selection().document()&&n.parent()==snapshot.selection().element()))));
            graph.setEnabled(canEditComponent()&&(kind==2||kind==5)&&snapshot.documents().stream().anyMatch(doc->doc.kind()==2));
            child.setEnabled(canEditComponent()&&kind==2);
            deleteChild.setEnabled(canEditComponent()&&kind==2&&snapshot.nodes().stream().anyMatch(node->
                node.kind()==2&&node.parent()!=0&&node.selection().equals(snapshot.selection())));
        };
        observe(value->update.run());
        authoringControls.add(update);
        add.addActionListener(e->{
            if(!canEditComponent()||snapshot.selection().kind()!=2)return;
            var base=snapshot;
            long token=generation;
            var types=availableComponents(base);
            int chosen=choose(toolbar,"Add Component",types.stream().map(type->componentLabel(type.name())).toList());
            if(chosen>=0)componentCommand.accept(new ComponentEdit(token,base.revision(),base.selectionRevision(),"add",unsigned(types.get(chosen).id())));
        });
        remove.addActionListener(e->{
            if(!canEditComponent())return;
            var base=snapshot;long token=generation;
            String target=null;
            if(base.selection().kind()==2){
                var components=base.nodes().stream().filter(n->n.kind()==5&&n.document()==base.selection().document()&&n.parent()==base.selection().element()).toList();
                int choice=choose(toolbar,"Remove Component",components.stream().map(Bridge.Node::label).toList());
                if(choice<0)return;var selected=components.get(choice).selection();target=unsigned(selected.component())+" "+unsigned(selected.part());
            }else if(base.selection().kind()!=5)return;
            if(JOptionPane.showConfirmDialog(toolbar,"Remove this component from its Prefab definition?","Remove Component",JOptionPane.OK_CANCEL_OPTION)==JOptionPane.OK_OPTION)
                componentCommand.accept(new ComponentEdit(token,base.revision(),base.selectionRevision(),"remove",target));
        });
        graph.addActionListener(e->{
            if(!canEditComponent())return;
            var base=snapshot;
            long token=generation;
            var graphs=base.documents().stream().filter(doc->doc.kind()==2).toList();
            int chosen=choose(toolbar,"Attach Graph",graphs.stream().map(Bridge.Document::name).toList());
            if(chosen>=0)componentCommand.accept(new ComponentEdit(token,base.revision(),base.selectionRevision(),"graph",unsigned(graphs.get(chosen).id())));
        });
        child.addActionListener(e->{
            if(child.isEnabled())componentCommand.accept(new ComponentEdit(generation,snapshot.revision(),snapshot.selectionRevision(),"child",null));
        });
        deleteChild.addActionListener(e->{
            if(!deleteChild.isEnabled())return;
            var edit=new ComponentEdit(generation,snapshot.revision(),snapshot.selectionRevision(),"delete-child",null);
            if(JOptionPane.showConfirmDialog(toolbar,"Delete the selected Prefab subtree?","Delete Prefab Child",JOptionPane.OK_CANCEL_OPTION)==JOptionPane.OK_OPTION)
                componentCommand.accept(edit);
        });
        return toolbar;
    }
    private static Bridge.Selection hierarchyTarget(TreePath path){
        if(path==null||!(path.getLastPathComponent() instanceof DefaultMutableTreeNode node))return null;
        if(node.getUserObject() instanceof Bridge.Node row&&row.kind()==1)return row.selection();
        if(node.getUserObject() instanceof DocRow row&&row.document().kind()==0)return new Bridge.Selection(0,row.document().id(),0);
        return null;
    }
    private Bridge.Selection referenceTarget(TreePath path){
        if(path==null||!(path.getLastPathComponent() instanceof DefaultMutableTreeNode node))return null;
        if(node.getUserObject() instanceof Bridge.Node row&&(row.kind()==1||row.kind()==2))return row.selection();
        if(node.getUserObject() instanceof DocRow row&&row.document().kind()==1&&snapshot!=null)
            return snapshot.nodes().stream().filter(n->n.document()==row.document().id()&&n.kind()==2&&n.parent()==0).map(Bridge.Node::selection).findFirst().orElse(null);
        return null;
    }
    private void sourceKeys(JComponent component,Supplier<Object> selected){
        for(String[] binding:new String[][]{{"control C","copy"},{"control X","cut"},{"control V","paste"},{"control D","duplicate"},{"DELETE","delete"}}){
            var key=KeyStroke.getKeyStroke(binding[0]);Object previous=component.getInputMap().get(key);var old=previous==null?null:component.getActionMap().get(previous);String name="source."+binding[1];
            component.getInputMap().put(key,name);component.getActionMap().put(name,new AbstractAction(){public void actionPerformed(ActionEvent e){Object row=selected.get();
                if(row instanceof SourceRow source)sourceFileAction.accept(binding[1],source.path());
                else if(binding[1].equals("paste")&&"Scripts".equals(row))sourceFileAction.accept("paste",null);
                else if(old!=null)old.actionPerformed(e);
            }});
        }
    }
    private static String unsigned(long id){return Long.toUnsignedString(id);}
    private static void menuItem(JPopupMenu menu,String label,Runnable action){
        var item=new JMenuItem(label);item.addActionListener(e->action.run());menu.add(item);
    }
    private static int choose(Component parent,String title,List<String> labels){
        var picker=new JComboBox<>(labels.toArray(String[]::new));picker.setRenderer(new DefaultListCellRenderer(){{putClientProperty("html.disable",true);}});
        return JOptionPane.showConfirmDialog(parent,picker,title,JOptionPane.OK_CANCEL_OPTION)==JOptionPane.OK_OPTION?picker.getSelectedIndex():-1;
    }
    private static String nodeKey(Bridge.Node row){return row.document()+":"+row.id()+":"+row.kind()+":"+row.component()+":"+row.part();}
}

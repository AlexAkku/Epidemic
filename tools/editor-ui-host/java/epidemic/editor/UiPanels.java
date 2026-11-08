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
    private final Catalog catalog;
    private final FeatureRegistry features;
    private final Consumer<Bridge.Document> openDocument;
    private final Consumer<Bridge.Selection> select;
    private final Runnable importPicker;
    private final Runnable cancelJobs;
    private final Consumer<Integer> createDocument;
    private final Runnable renameDocument;
    private Function<ScalarInspector.Edit,java.util.concurrent.CompletableFuture<Bridge.Reply>> editField;
    private final List<ScalarInspector> inspectors=new ArrayList<>();
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
    private final JTextArea console = new JTextArea();
    private Bridge.Snapshot snapshot;
    private boolean refreshing;
    public UiPanels(Catalog catalog, FeatureRegistry features, Consumer<Bridge.Document> open,
                    Consumer<Bridge.Selection> select, Runnable importPicker, Runnable cancelJobs,
                    Consumer<Integer> createDocument, Runnable renameDocument) {
        this.catalog = catalog; this.features = features; this.openDocument = open;
        this.select = select; this.importPicker = importPicker; this.cancelJobs = cancelJobs; console.setEditable(false);
        this.createDocument=createDocument;this.renameDocument=renameDocument;
    }
    public UiPanels(Catalog catalog,FeatureRegistry features,Consumer<Bridge.Document> open,Consumer<Bridge.Selection> select,Runnable importPicker,Runnable cancelJobs,Consumer<Integer> createDocument,Runnable renameDocument,Function<ScalarInspector.Edit,java.util.concurrent.CompletableFuture<Bridge.Reply>> editField){
        this(catalog,features,open,select,importPicker,cancelJobs,createDocument,renameDocument);this.editField=editField;
    }
    public void sessionState(long generation,boolean ready){
        this.generation=generation;interactive=ready;assetButtons.forEach(button->button.setEnabled(ready&&snapshot!=null&&snapshot.storage()==2&&!snapshot.playing()));
        for(var inspector:inspectors)inspector.refresh(generation,snapshot,ready);
        authoringControls.forEach(Runnable::run);
    }
    public java.util.concurrent.CompletableFuture<Void> flushDrafts(){return java.util.concurrent.CompletableFuture.allOf(inspectors.stream().map(ScalarInspector::flush).toArray(java.util.concurrent.CompletableFuture[]::new));}
    public void cancelDrafts(long document){if(snapshot!=null&&snapshot.selection().document()==document)inspectors.forEach(ScalarInspector::cancelDrafts);}
    public void documentCommandsEnabled(boolean enabled) {
        documentCommandsEnabled=enabled;documentButtons.forEach(button->button.setEnabled(enabled));
    }
    public NativeViewport nativeView(String id) { return nativeViews.get(id); }
    public Map<String,NativeViewport> nativeViews() { return Map.copyOf(nativeViews); }
    public void log(String text) {
        console.append(text + "\n");
        if (console.getDocument().getLength() > 65536)
            try { console.getDocument().remove(0, console.getDocument().getLength() - 49152); }
            catch (javax.swing.text.BadLocationException error) { throw new IllegalStateException(error); }
    }
    public void refresh(Bridge.Snapshot value) {
        refresh(value,false);
    }
    public void refresh(Bridge.Snapshot value,boolean force) {
        if (!force&&Objects.equals(snapshot, value)) return;
        Bridge.Snapshot previous = snapshot;
        boolean structureChanged = previous == null || value == null || !previous.documents().equals(value.documents()) ||
            !previous.nodes().equals(value.nodes()) || !previous.assets().equals(value.assets());
        snapshot = value; refreshing = true;
        try {
            if (structureChanged) for (var reader : structureReaders) reader.accept(value);
            for (var reader : readers) reader.accept(value);
        }
        finally { refreshing = false; }
    }
    public JComponent view(String id) { return cache.computeIfAbsent(id, this::create); }
    private void observe(Consumer<Bridge.Snapshot> reader) {
        readers.add(reader);boolean previous=refreshing;refreshing=true;
        try{reader.accept(snapshot);}finally{refreshing=previous;}
    }
    private void structure(Consumer<Bridge.Snapshot> reader) { structureReaders.add(reader); reader.accept(snapshot); }
    private JComponent create(String id) {
        String base = id.contains(":") ? id.substring(0, id.indexOf(':')) : id;
        if (base.equals("project.explorer")) return explorer();
        if (base.equals("scene.hierarchy")) return hierarchy();
        if (base.equals("scene.systems")) return systems();
        if (base.equals("scene.editor")||base.equals("prefab.canvas")) return scene(id);
        if (base.equals("background.jobs")) {
            JPanel panel=Theme.island(new BorderLayout(6,6));
            panel.add(Theme.label("Bridge queue: at most 32 pending requests. Running mutations finish atomically.",true),BorderLayout.NORTH);
            panel.add(Theme.button("Cancel native imports",()->assetCommand.accept("cancel",0L)),BorderLayout.SOUTH);
            JTextArea state=new JTextArea();
            observe(value->{StringBuilder rows=new StringBuilder("Native import queue: bounded to 128; one worker.\n");if(value!=null){rows.append((value.flags()&16)!=0?"Busy\n":"Idle\n");rows.append((value.flags()&32)!=0?"Content watch: on\n":"Content watch: off\n");long completed=value.assetState().events().stream().filter(e->e.job()>0&&e.kind()!=0).count();rows.append("Observed terminal import outcomes (last 64 events): ").append(completed).append("\n");}if(value!=null)for(var event:value.assetState().events())rows.append("Job ").append(event.job()).append(" · ").append(event.source()).append(" · ").append(new String[]{"Importing","Updated","Missing","Failed","Recovered","Cancelled"}[event.kind()]).append(" · ").append(event.diagnostic()).append('\n');state.setText(rows.toString());});
            state.setEditable(false);state.setLineWrap(true);state.setWrapStyleWord(true);panel.add(state);return panel;
        }
        if (base.equals("console") || base.equals("problems") || base.equals("build.output")) {
            JTextArea view = new JTextArea(); view.setEditable(false); view.setDocument(console.getDocument()); return new JScrollPane(view);
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
                    case 1->"Scene entity / placement";
                    case 2,5->"Prefab definition";
                    case 7->"Linked Prefab reference";
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
            AssetDetails details=new AssetDetails(base.equals("asset.references"),idValue->{if(snapshot!=null){var doc=snapshot.documents().stream().filter(d->d.id()==idValue).findFirst();if(doc.isPresent())openDocument.accept(doc.get());else if(snapshot.assets().stream().anyMatch(asset->asset.id()==idValue))select.accept(new Bridge.Selection(6,0,idValue));}});
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
        JCheckBox snap=new JCheckBox("Move Snap (1 unit)"),grid=new JCheckBox("Grid",true);
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
                viewport.addHierarchyListener(e->{if(!viewport.isShowing())mouse.cancel();});
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
        list.setSelectionMode(ListSelectionModel.SINGLE_SELECTION);
        CardLayout cards=new CardLayout();JPanel browser=new JPanel(cards);browser.add(new JScrollPane(tree),"Tree");browser.add(new JScrollPane(list),"List");
        mode.addActionListener(e->{boolean grid=mode.getSelectedItem().equals("Grid");list.setLayoutOrientation(grid?JList.HORIZONTAL_WRAP:JList.VERTICAL);list.setVisibleRowCount(grid?-1:0);list.setFixedCellWidth(grid?220:-1);list.setFixedCellHeight(grid?64:-1);cards.show(browser,mode.getSelectedItem().equals("Tree")?"Tree":"List");});
        list.addListSelectionListener(e->{if(refreshing||e.getValueIsAdjusting())return;Object row=list.getSelectedValue();if(row instanceof AssetRow asset)select.accept(new Bridge.Selection(6,0,asset.asset().id()));else if(row instanceof DocRow doc)select.accept(new Bridge.Selection(0,doc.document().id(),0));});
        list.addMouseListener(new MouseAdapter(){@Override public void mouseClicked(MouseEvent e){if(e.getClickCount()==2&&list.getSelectedValue() instanceof DocRow row)openDocument.accept(row.document());}});
        panel.add(header, BorderLayout.NORTH); panel.add(browser);
        JPanel buttons = new JPanel(new GridLayout(0, 2, 4, 4));
        JButton importButton=Theme.button("Import…",importPicker);assetButtons.add(importButton);buttons.add(importButton);
        JButton create=new JButton("New Document…"),rename=new JButton("Rename Document…");
        JPopupMenu kinds=new JPopupMenu();
        for(int kind=0;kind<3;kind++) {
            final int type=kind;JMenuItem item=new JMenuItem(new String[]{"Scene","Prefab","Graph"}[kind]);
            item.addActionListener(event->createDocument.accept(type));kinds.add(item);
        }
        create.addActionListener(event->kinds.show(create,0,create.getHeight()));
        rename.addActionListener(event->renameDocument.run());
        for(JButton button:List.of(create,rename)){button.setEnabled(documentCommandsEnabled);documentButtons.add(button);buttons.add(button);}
        for(String name:List.of("Rename / Move","Duplicate","Reimport","Delete","Restore","Assign to Prefab","Watch")){JButton button=Theme.button(name,()->assetCommand.accept(name,snapshot!=null&&snapshot.selection().kind()==6?snapshot.selection().element():0L));assetButtons.add(button);buttons.add(button);}
        panel.add(buttons, BorderLayout.SOUTH);
        Runnable rebuild = () -> {
            DefaultMutableTreeNode root = new DefaultMutableTreeNode(snapshot == null ? "Choose a project" : snapshot.title());
            DefaultMutableTreeNode docs = new DefaultMutableTreeNode("Documents"), assets = new DefaultMutableTreeNode("Content"); root.add(docs); root.add(assets); root.add(new DefaultMutableTreeNode("Scripts"));if(internal.isSelected())root.add(new DefaultMutableTreeNode(".epidemic [service data, read-only]"));
            String filter = search.getText().toLowerCase(Locale.ROOT);
            breadcrumb.setText("Content · "+(snapshot==null?0:snapshot.assets().size())+" shown / "+(snapshot==null?0:snapshot.assetState().totalAssets())+" total · max 1000 filtered; Enter to search");
            if (snapshot != null) {
                for (Bridge.Document doc : snapshot.documents()) if (doc.name().toLowerCase(Locale.ROOT).contains(filter)) docs.add(new DefaultMutableTreeNode(new DocRow(doc)));
                Map<String,DefaultMutableTreeNode> folders = new HashMap<>(); folders.put("Content", assets);
                for (Bridge.Asset asset : snapshot.assets().stream().filter(a->a.path().toLowerCase(Locale.ROOT).contains(filter)).sorted(Comparator.comparing(Bridge.Asset::path)).limit(1000).toList()) if (asset.path().toLowerCase(Locale.ROOT).contains(filter)) {
                    String[] path = asset.path().split("/"); DefaultMutableTreeNode parent = assets; String prefix = "Content";
                    for (int i = path[0].equals("Content") ? 1 : 0; i < path.length - 1; ++i) {
                        prefix += "/" + path[i]; DefaultMutableTreeNode folder = folders.get(prefix);
                        if (folder == null) { folder = new DefaultMutableTreeNode(path[i]); folders.put(prefix, folder); parent.add(folder); }
                        parent = folder;
                    }
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
                for(var rows=root.depthFirstEnumeration();rows.hasMoreElements();){var item=(DefaultMutableTreeNode)rows.nextElement();if(item.getUserObject() instanceof DocRow||item.getUserObject() instanceof AssetRow)listing.addElement(item.getUserObject());}
                if(snapshot!=null)for(var nodes=root.depthFirstEnumeration();nodes.hasMoreElements();){
                    var node=(DefaultMutableTreeNode)nodes.nextElement();TreePath path=new TreePath(node.getPath());
                    if(expanded.contains(treeKey(path))||expanded.isEmpty()&&node==docs)tree.expandPath(path);
                    if(node.getUserObject() instanceof DocRow row&&row.document().id()==snapshot.selection().document() || node.getUserObject() instanceof AssetRow assetRow&&snapshot.selection().kind()==6&&assetRow.asset().id()==snapshot.selection().element()){
                        boolean expand=tree.getExpandsSelectedPaths();tree.setExpandsSelectedPaths(false);
                        try{tree.setSelectionPath(path);}finally{tree.setExpandsSelectedPaths(expand);}
                    }
                }
                if(scroll!=null&&tree.getParent() instanceof JViewport viewport)viewport.setViewPosition(scroll);
                if(listScroll!=null&&list.getParent() instanceof JViewport viewport)viewport.setViewPosition(listScroll);
            }finally{refreshing=wasRefreshing;}
        };
        structure(value -> rebuild.run()); search.addActionListener(event -> {assetCommand.accept("Search:"+search.getText(),0L);rebuild.run();});internal.addActionListener(event->rebuild.run());
        observe(value->{
            if(value==null)return;
            for(int i=0;i<listing.size();i++){Object row=listing.get(i);if(row instanceof DocRow doc&&doc.document().id()==value.selection().document() || row instanceof AssetRow asset&&value.selection().kind()==6&&asset.asset().id()==value.selection().element()){list.setSelectedIndex(i);break;}}
            var root=(DefaultMutableTreeNode)tree.getModel().getRoot();
            for(var nodes=root.depthFirstEnumeration();nodes.hasMoreElements();){
                var node=(DefaultMutableTreeNode)nodes.nextElement();
                if(node.getUserObject() instanceof DocRow row&&row.document().id()==value.selection().document() || node.getUserObject() instanceof AssetRow assetRow&&value.selection().kind()==6&&assetRow.asset().id()==value.selection().element()){
                    boolean expand=tree.getExpandsSelectedPaths();tree.setExpandsSelectedPaths(false);
                    tree.setSelectionPath(new TreePath(node.getPath()));tree.setExpandsSelectedPaths(expand);return;
                }
            }
            tree.clearSelection();
        });
        tree.addTreeSelectionListener(event->{
            if(refreshing)return;Object chosen=tree.getLastSelectedPathComponent();
            if(chosen instanceof DefaultMutableTreeNode node&&node.getUserObject() instanceof DocRow row)
                select.accept(new Bridge.Selection(0,row.document().id(),0));
            else if(chosen instanceof DefaultMutableTreeNode node&&node.getUserObject() instanceof AssetRow row)select.accept(new Bridge.Selection(6,0,row.asset().id()));
            else if(chosen==tree.getModel().getRoot())select.accept(new Bridge.Selection(0,0,0));
        });
        tree.addMouseListener(new MouseAdapter() { @Override public void mouseClicked(MouseEvent event) {
            if (event.getClickCount() != 2 || refreshing) return;
            Object selected = tree.getLastSelectedPathComponent();
            if (selected instanceof DefaultMutableTreeNode node && node.getUserObject() instanceof DocRow row) {
                openDocument.accept(row.document());
            }
        } });
        return panel;
    }
    private record AssetRow(Bridge.Asset asset){@Override public String toString(){return asset.path()+" ["+Long.toUnsignedString(asset.id())+"] g"+asset.generation()+(asset.trashed()?" · Trash":!asset.present()?" · Missing":"");}}
    private record DocRow(Bridge.Document document) { @Override public String toString() { return document.name() + " [" + Long.toUnsignedString(document.id()) + "]"; } }
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
                if (value instanceof DefaultMutableTreeNode node && node.getUserObject() instanceof Bridge.Node data) setText(data.label());
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
            tree.setTransferHandler(new TransferHandler(){
                private final DataFlavor flavor=localFlavor(HierarchyEdit.class,"Scene hierarchy");
                @Override public int getSourceActions(JComponent component){return MOVE;}
                @Override protected Transferable createTransferable(JComponent component){
                    var source=hierarchyTarget(tree.getSelectionPath());
                    if(!canEditHierarchy()||source==null||source.element()==0)return null;
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
                @Override public boolean canImport(TransferSupport support){return drop(support)!=null;}
                @Override public boolean importData(TransferSupport support){
                    var edit=drop(support);if(edit==null)return false;hierarchyCommand.accept(edit);return true;
                }
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
    private static boolean texture(Bridge.Asset asset){
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
        new DropTarget(viewport,DnDConstants.ACTION_COPY,new DropTargetAdapter(){
            @Override public void drop(DropTargetDropEvent event){
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
    private JPanel componentActions(){
        JPanel toolbar=new JPanel(new FlowLayout(FlowLayout.LEFT));
        JButton add=new JButton("Add Component"),remove=new JButton("Remove Component"),graph=new JButton("Attach Graph…");
        JButton child=new JButton("Create Prefab Child"),deleteChild=new JButton("Delete Prefab Child");
        for(var button:List.of(add,remove,graph,child,deleteChild))toolbar.add(button);
        Runnable update=()->{
            int kind=snapshot==null?0:snapshot.selection().kind();
            toolbar.setVisible(kind==2||kind==5);
            add.setEnabled(canEditComponent()&&kind==2&&!snapshot.componentTypes().isEmpty());
            remove.setEnabled(canEditComponent()&&kind==5);
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
            var types=base.componentTypes();
            int chosen=choose(toolbar,"Add Component",types.stream().map(Bridge.ComponentType::name).toList());
            if(chosen>=0)componentCommand.accept(new ComponentEdit(token,base.revision(),base.selectionRevision(),"add",unsigned(types.get(chosen).id())));
        });
        remove.addActionListener(e->{
            if(!canEditComponent()||snapshot.selection().kind()!=5)return;
            var base=snapshot;
            long token=generation;
            if(JOptionPane.showConfirmDialog(toolbar,"Remove the selected component from its Prefab definition?","Remove Component",JOptionPane.OK_CANCEL_OPTION)==JOptionPane.OK_OPTION)
                componentCommand.accept(new ComponentEdit(token,base.revision(),base.selectionRevision(),"remove",null));
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
    private static String unsigned(long id){return Long.toUnsignedString(id);}
    private static void menuItem(JPopupMenu menu,String label,Runnable action){
        var item=new JMenuItem(label);item.addActionListener(e->action.run());menu.add(item);
    }
    private static int choose(Component parent,String title,List<String> labels){
        var picker=new JComboBox<>(labels.toArray(String[]::new));
        return JOptionPane.showConfirmDialog(parent,picker,title,JOptionPane.OK_CANCEL_OPTION)==JOptionPane.OK_OPTION?picker.getSelectedIndex():-1;
    }
    private static String nodeKey(Bridge.Node row){return row.document()+":"+row.id()+":"+row.kind()+":"+row.component()+":"+row.part();}
}

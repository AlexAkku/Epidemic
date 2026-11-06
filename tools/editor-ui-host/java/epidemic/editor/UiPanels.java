package epidemic.editor;

import epidemic.u1.NativeViewport;
import java.awt.*;
import java.awt.event.*;
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
    private long generation;
    private boolean interactive;
    private boolean documentCommandsEnabled;
    private final List<AbstractButton> assetButtons=new ArrayList<>();
    private final List<AbstractButton> documentButtons = new ArrayList<>();
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
            for (var reader : readers) reader.accept(value);
            if (structureChanged) for (var reader : structureReaders) reader.accept(value);
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
            observe(value -> summary.setText(value == null ? "Select a project / object" :
                "Revision " + value.revision() + "\nSelection kind: " + value.selection().kind() +
                "\nDocument ID: " + Long.toUnsignedString(value.selection().document()) +
                "\nElement ID: " + Long.toUnsignedString(value.selection().element()) +
                "\nAuthoring data / runtime values are separate"));
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
                ScalarInspector inspector=new ScalarInspector(request->editField==null?java.util.concurrent.CompletableFuture.failedFuture(new IllegalStateException("Native Inspector unavailable")):editField.apply(request));
                inspectors.add(inspector);observe(value->inspector.refresh(generation,value,interactive));panel.add(inspector);
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
        for (String tool : new String[]{"2D", "3D", "Move", "Rotate", "Scale", "Grid", "Snap", "Camera"})
            tools.add(features.plannedButton("scene." + tool, tool, "F2"));
        panel.add(tools, BorderLayout.NORTH);
        try {
            NativeViewport viewport = new NativeViewport(); nativeViews.put(id, viewport); panel.add(viewport, BorderLayout.CENTER);
        } catch (LinkageError unavailable) { panel.add(Theme.label("Native viewport unavailable: " + unavailable.getMessage(), true)); }
        observe(value->{
            NativeViewport viewport=nativeViews.get(id);
            if(viewport==null||value==null||value.storage()!=2)return;
            long document=id.contains(":")?Long.parseUnsignedLong(id.substring(id.indexOf(':')+1)):value.entry();
            viewport.projectScene(java.nio.file.Path.of(value.path()).getParent().toString(),value.assetState().sprites().stream().filter(sprite->sprite.document()==document).toList());
        });
        JLabel selection=Theme.label("Select Scene · native viewport shell · authoring draw integration planned F2",true);
        observe(value->selection.setText(value==null?"No Project":"Selection: "+Long.toUnsignedString(value.selection().document())+" / "+Long.toUnsignedString(value.selection().element())+" · authoring picking planned F2"));
        panel.add(selection, BorderLayout.SOUTH);
        return panel;
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
        observe(value -> {
            DefaultMutableTreeNode root = new DefaultMutableTreeNode("World / Scene / Prefab");
            if (value != null) {
                Map<Long,DefaultMutableTreeNode> documents = new HashMap<>();
                for (Bridge.Document doc : value.documents()) {
                    DefaultMutableTreeNode node = new DefaultMutableTreeNode(new DocRow(doc)); root.add(node); documents.put(doc.id(), node);
                }
                Map<String,DefaultMutableTreeNode> nodes = new HashMap<>();
                for (Bridge.Node row : value.nodes()) nodes.put(nodeKey(row), new DefaultMutableTreeNode(row));
                for (Bridge.Node row : value.nodes()) {
                    DefaultMutableTreeNode parent = nodes.get(row.document() + ":" + row.parent()+":"+(row.kind()==5?2:row.kind())+":0:0");
                    if (parent == null) parent = documents.get(row.document());
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
                if(value!=null&&(node.getUserObject() instanceof Bridge.Node row&&row.selection().equals(value.selection())||node.getUserObject() instanceof DocRow doc&&value.selection().kind()==0&&doc.document().id()==value.selection().document())){
                    tree.setExpandsSelectedPaths(false);tree.setSelectionPath(path);tree.setExpandsSelectedPaths(true);
                }
            }
            if(scroll!=null&&tree.getParent() instanceof JViewport viewport)viewport.setViewPosition(scroll);
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
        return new JScrollPane(tree);
    }
    private static String nodeKey(Bridge.Node row){return row.document()+":"+row.id()+":"+row.kind()+":"+row.component()+":"+row.part();}
}

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
    private final Map<String,JComponent> cache = new HashMap<>();
    private final Map<String,NativeViewport> nativeViews = new HashMap<>();
    private final List<Consumer<Bridge.Snapshot>> readers = new ArrayList<>();
    private final List<Consumer<Bridge.Snapshot>> structureReaders = new ArrayList<>();
    private final JTextArea console = new JTextArea();
    private Bridge.Snapshot snapshot;
    private boolean refreshing;
    public UiPanels(Catalog catalog, FeatureRegistry features, Consumer<Bridge.Document> open,
                    Consumer<Bridge.Selection> select, Runnable importPicker, Runnable cancelJobs) {
        this.catalog = catalog; this.features = features; this.openDocument = open;
        this.select = select; this.importPicker = importPicker; this.cancelJobs = cancelJobs; console.setEditable(false);
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
        if (Objects.equals(snapshot, value)) return;
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
    private void observe(Consumer<Bridge.Snapshot> reader) { readers.add(reader); reader.accept(snapshot); }
    private void structure(Consumer<Bridge.Snapshot> reader) { structureReaders.add(reader); reader.accept(snapshot); }
    private JComponent create(String id) {
        String base = id.contains(":") ? id.substring(0, id.indexOf(':')) : id;
        if (base.equals("project.explorer")) return explorer();
        if (base.equals("scene.hierarchy")) return hierarchy();
        if (base.equals("scene.editor")) return scene(id);
        if (base.equals("background.jobs")) {
            JPanel panel=Theme.island(new BorderLayout(6,6));
            panel.add(Theme.label("Bridge queue: at most 32 pending requests. Running mutations finish atomically.",true),BorderLayout.NORTH);
            panel.add(Theme.button("Cancel queued requests",cancelJobs),BorderLayout.SOUTH);
            JTextArea state=new JTextArea("Native import/build adapters are planned for F1/F9.");
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
            panel.add(summary, BorderLayout.NORTH);panel.add(fields);return panel;
        }
        if (base.equals("settings.editor")) return InspectorForms.gallery(catalog, features);
        if (base.equals("asset.details") || base.equals("asset.references")) {
            JTextArea details = new JTextArea(); details.setEditable(false); details.setLineWrap(true);
            observe(value -> {
                StringBuilder text = new StringBuilder("Asset identities and cooked keys (native read snapshot)\n\n");
                if (value != null) for (Bridge.Asset asset : value.assets()) text.append(asset.path()).append("\nAssetId ")
                    .append(Long.toUnsignedString(asset.id())).append("\nGeneration ").append(asset.generation())
                    .append("\nCooked key ").append(asset.cookedKey()).append("\nFingerprint ")
                    .append(Long.toUnsignedString(asset.fingerprint())).append("\n\n");
                details.setText(text.toString());
            }); return new JScrollPane(details);
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
        panel.add(Theme.label("Select Scene · native viewport shell · authoring draw integration planned F2", true), BorderLayout.SOUTH);
        return panel;
    }
    private JComponent explorer() {
        JPanel panel = Theme.island(new BorderLayout(4, 4));
        JTextField search = new JTextField(); search.setToolTipText("Search documents / assets");
        JLabel breadcrumb = Theme.label("Project / Content", true);
        JTree tree = new JTree(new DefaultMutableTreeNode("Choose a project")); tree.setRootVisible(true);
        JPanel header = new JPanel(new BorderLayout(4, 4)); header.add(search, BorderLayout.NORTH); header.add(breadcrumb, BorderLayout.SOUTH);
        panel.add(header, BorderLayout.NORTH); panel.add(new JScrollPane(tree));
        JPanel buttons = new JPanel(new FlowLayout(FlowLayout.LEFT, 4, 0));
        buttons.add(Theme.button("Import…", importPicker));
        for (String action : new String[]{"Create", "Rename", "Move"}) buttons.add(features.plannedButton("assets." + action, action, "F1"));
        panel.add(buttons, BorderLayout.SOUTH);
        Runnable rebuild = () -> {
            DefaultMutableTreeNode root = new DefaultMutableTreeNode(snapshot == null ? "Choose a project" : snapshot.title());
            DefaultMutableTreeNode docs = new DefaultMutableTreeNode("Documents"), assets = new DefaultMutableTreeNode("Content"); root.add(docs); root.add(assets);
            String filter = search.getText().toLowerCase(Locale.ROOT);
            if (snapshot != null) {
                for (Bridge.Document doc : snapshot.documents()) if (doc.name().toLowerCase(Locale.ROOT).contains(filter)) docs.add(new DefaultMutableTreeNode(new DocRow(doc)));
                Map<String,DefaultMutableTreeNode> folders = new HashMap<>(); folders.put("Content", assets);
                for (Bridge.Asset asset : snapshot.assets()) if (asset.path().toLowerCase(Locale.ROOT).contains(filter)) {
                    String[] path = asset.path().split("/"); DefaultMutableTreeNode parent = assets; String prefix = "Content";
                    for (int i = path[0].equals("Content") ? 1 : 0; i < path.length - 1; ++i) {
                        prefix += "/" + path[i]; DefaultMutableTreeNode folder = folders.get(prefix);
                        if (folder == null) { folder = new DefaultMutableTreeNode(path[i]); folders.put(prefix, folder); parent.add(folder); }
                        parent = folder;
                    }
                    parent.add(new DefaultMutableTreeNode(asset.path() + " [" + Long.toUnsignedString(asset.id()) + "]"));
                }
            }
            tree.setModel(new DefaultTreeModel(root)); tree.expandRow(0); tree.expandRow(1);
        };
        structure(value -> rebuild.run()); search.addActionListener(event -> rebuild.run());
        tree.addMouseListener(new MouseAdapter() { @Override public void mouseClicked(MouseEvent event) {
            if (event.getClickCount() != 2 || refreshing) return;
            Object selected = tree.getLastSelectedPathComponent();
            if (selected instanceof DefaultMutableTreeNode node && node.getUserObject() instanceof DocRow row) {
                select.accept(new Bridge.Selection(0, row.document().id(), 0)); openDocument.accept(row.document());
            }
        } });
        return panel;
    }
    private record DocRow(Bridge.Document document) { @Override public String toString() { return document.name() + " [" + Long.toUnsignedString(document.id()) + "]"; } }
    private JComponent hierarchy() {
        JTree tree = new JTree();
        structure(value -> {
            DefaultMutableTreeNode root = new DefaultMutableTreeNode("World / Scene / Prefab");
            if (value != null) {
                Map<Long,DefaultMutableTreeNode> documents = new HashMap<>();
                for (Bridge.Document doc : value.documents()) {
                    DefaultMutableTreeNode node = new DefaultMutableTreeNode(doc.name()); root.add(node); documents.put(doc.id(), node);
                }
                Map<String,DefaultMutableTreeNode> nodes = new HashMap<>();
                for (Bridge.Node row : value.nodes()) nodes.put(row.document() + ":" + row.id(), new DefaultMutableTreeNode(row));
                for (Bridge.Node row : value.nodes()) {
                    DefaultMutableTreeNode parent = nodes.get(row.document() + ":" + row.parent());
                    if (parent == null) parent = documents.get(row.document());
                    if (parent != null) parent.add(nodes.get(row.document() + ":" + row.id()));
                }
            }
            tree.setModel(new DefaultTreeModel(root)); tree.expandRow(0);
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
                select.accept(new Bridge.Selection(row.kind(), row.document(), row.id()));
        });
        return new JScrollPane(tree);
    }
}

package epidemic.editor;
import java.awt.*;
import java.util.*;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.function.*;
import javax.swing.*;
import javax.swing.event.*;
/** Persistent Tool Window controls. Every edit is sent to the native workspace. */
public final class GraphTools extends JPanel {
    private final String kind;
    private final Function<UiPanels.GestureEdit,CompletableFuture<Bridge.Reply>> command;
    private final BooleanSupplier available;
    private final LongSupplier generation;
    private Bridge.Snapshot snapshot;
    private Bridge.Graph graph;
    private Bridge.GraphNode selected;
    private final JLabel caption=Theme.label("Open a Graph document",true);
    private final JTextField search=new JTextField(), name=new JTextField(), value=new JTextField(), group=new JTextField();
    private final JTextArea comment=new JTextArea(4,20);
    private final JCheckBox bookmark=new JCheckBox("Bookmark"), breakpoint=new JCheckBox("Breakpoint marker (F-II.4)");
    private final DefaultListModel<String> rows=new DefaultListModel<>();
    private final JList<String> list=new JList<>(rows);
    private final List<Bridge.GraphTemplate> visible=new ArrayList<>();
    private final JButton apply=Theme.button("Apply",this::apply), create=Theme.button("Add Node",this::create);
    private final JTextArea report=new JTextArea();
    private final List<JButton> actions=new ArrayList<>();
    private boolean refreshing, draft, busy;
    private CompletableFuture<Bridge.Reply> active;
    private long draftRevision;
    private Consumer<Bridge.GraphTemplate> sourceNavigator=template->{};
    public void sourceNavigator(Consumer<Bridge.GraphTemplate> action){sourceNavigator=action;}
    private JButton sourceButton;
    private JButton referenceJump,referenceUses;
    private String fieldError="";
    public GraphTools(String kind,BooleanSupplier available,LongSupplier generation,Function<UiPanels.GestureEdit,CompletableFuture<Bridge.Reply>> command) {
        super(new BorderLayout(6,6));this.kind=kind;this.available=available;this.generation=generation;this.command=command;
        name.setName("graph.node.name");value.setName("graph.preview-default");
        setBorder(BorderFactory.createEmptyBorder(8,8,8,8));add(caption,BorderLayout.NORTH);
        if(kind.equals("graph.palette")) {
            var body=new JPanel(new BorderLayout(6,6));search.setToolTipText("Search operation or category");
            search.getAccessibleContext().setAccessibleName("Search Graph Palette");body.add(search,BorderLayout.NORTH);
            body.add(new JScrollPane(list),BorderLayout.CENTER);body.add(create,BorderLayout.SOUTH);add(body);
            search.getDocument().addDocumentListener(new DocumentListener(){public void insertUpdate(DocumentEvent e){palette();}public void removeUpdate(DocumentEvent e){palette();}public void changedUpdate(DocumentEvent e){palette();}});
            list.setCellRenderer(new DefaultListCellRenderer(){public Component getListCellRendererComponent(JList<?> l,Object v,int index,boolean chosen,boolean focus){
                var cell=super.getListCellRendererComponent(l,v,index,chosen,focus);cell.setEnabled(index<visible.size());return cell;}});
            list.addListSelectionListener(e->create.setEnabled(canEdit()&&list.getSelectedIndex()>=0&&list.getSelectedIndex()<visible.size()));
            list.addMouseListener(new java.awt.event.MouseAdapter(){public void mouseClicked(java.awt.event.MouseEvent e){if(e.getClickCount()==2)create();}});
        } else if(kind.equals("graph.inspector")) {
            var form=new JPanel();form.setLayout(new BoxLayout(form,BoxLayout.Y_AXIS));
            field(form,"Name",name);field(form,"Preview Input default (i64)",value);field(form,"Group",group);field(form,"Comment",new JScrollPane(comment));
            sourceButton=Theme.button("Open C# Source",()->{if(selected!=null&&graph!=null)graph.catalogue().stream().filter(type->type.type()==selected.type()&&!type.source().isEmpty()).findFirst().ifPresent(sourceNavigator);});form.add(sourceButton);
            referenceJump=Theme.button("Jump to Reference Target",()->reference("jump"));form.add(referenceJump);
            referenceUses=Theme.button("Find Target References",()->reference("uses"));form.add(referenceUses);
            form.add(bookmark);form.add(breakpoint);form.add(apply);form.add(new JScrollPane(list));add(new JScrollPane(form));
            name.addActionListener(e->apply());value.addActionListener(e->apply());group.addActionListener(e->apply());
            var edits=new DocumentListener(){private void changed(){if(!refreshing&&snapshot!=null){if(!draft)draftRevision=snapshot.revision();draft=true;fieldError="";caption.setText("Pending input · Apply/Enter or Save; Escape cancels");}}public void insertUpdate(DocumentEvent e){changed();}public void removeUpdate(DocumentEvent e){changed();}public void changedUpdate(DocumentEvent e){changed();}};
            for(var field:List.of(name,value,group))field.getDocument().addDocumentListener(edits);comment.getDocument().addDocumentListener(edits);
            for(var check:List.of(bookmark,breakpoint))check.addActionListener(e->{if(snapshot!=null){if(!draft)draftRevision=snapshot.revision();draft=true;fieldError="";caption.setText("Pending input · Apply/Enter or Save; Escape cancels");}});
            var cancel=KeyStroke.getKeyStroke(java.awt.event.KeyEvent.VK_ESCAPE,0);
            getInputMap(WHEN_ANCESTOR_OF_FOCUSED_COMPONENT).put(cancel,"restore");getActionMap().put("restore",new AbstractAction(){public void actionPerformed(java.awt.event.ActionEvent e){load();}});
        } else {
            report.setEditable(false);report.setLineWrap(true);report.setWrapStyleWord(true);add(new JScrollPane(report));
            var bar=new JPanel(new FlowLayout(FlowLayout.LEFT,4,0));
            for(var action:new String[]{"Compile","Run Preview","Find Uses"}){var button=Theme.button(action,()->send(action.equals("Compile")?"compile":action.equals("Find Uses")?"uses":"run"));actions.add(button);bar.add(button);}
            add(bar,BorderLayout.SOUTH);
        }
    }
    private static void field(JPanel target,String label,JComponent component) {
        var row=new JPanel(new BorderLayout(4,4));var caption=new JLabel(label);caption.setLabelFor(component);
        row.add(caption,BorderLayout.NORTH);row.add(component);row.setMaximumSize(new Dimension(Integer.MAX_VALUE,component instanceof JScrollPane?100:60));target.add(row);
    }
    private boolean canEdit(){return !busy&&available.getAsBoolean()&&graph!=null&&snapshot!=null&&!snapshot.playing();}
    private void reference(String action){
        if(!canEdit()||selected==null||selected.reference()==null)return;
        command.apply(new UiPanels.GestureEdit(generation.getAsLong(),snapshot.revision(),40,List.of(action,Long.toUnsignedString(graph.document()),Long.toUnsignedString(selected.id()))));
    }
    private CompletableFuture<Bridge.Reply> send(String action,String... args) {
        if(!canEdit())return CompletableFuture.failedFuture(new IllegalStateException("ui.graph_not_editable"));
        var values=new ArrayList<String>();values.add(action);values.add(Long.toUnsignedString(graph.document()));values.addAll(Arrays.asList(args));
        long revision=action.equals("settings")&&draft?draftRevision:snapshot.revision();
        if(action.equals("settings")){busy=true;refresh(snapshot);}
        var delivered=new CompletableFuture<Bridge.Reply>();active=delivered;
        command.apply(new UiPanels.GestureEdit(generation.getAsLong(),revision,38,List.copyOf(values))).whenComplete((reply,error)->SwingUtilities.invokeLater(()->{
            busy=false;
            if(error!=null)fieldError=error.getCause()==null?error.toString():error.getCause().toString();
            else if(action.equals("settings")){draft=false;if(snapshot==null||reply.snapshot().revision()>=snapshot.revision())refresh(reply.snapshot());load();}
            refresh(snapshot);
            // Save/Close/selection must observe the completed UI draft transition,
            // including a rejected edit, before their continuation can run.
            if(error!=null)delivered.completeExceptionally(error);else delivered.complete(reply);
        }));
        return delivered;
    }
    private void create(){int index=list.getSelectedIndex();if(!canEdit()||index<0||index>=visible.size())return;
        send("create",Integer.toString(graph.catalogue().indexOf(visible.get(index))),Integer.toString(80+(graph.nodes().size()%8)*260),Integer.toString(80+(graph.nodes().size()/8)*240));}
    private void apply(){flushDraft();}
    public boolean hasDraft(long document){return (draft||busy)&&graph!=null&&graph.document()==document;}
    public void cancelDrafts(long document){if(hasDraft(document)&&!busy)load();}
    public CompletableFuture<Void> flushDraft(){
        if(!SwingUtilities.isEventDispatchThread()){
            var future=new CompletableFuture<Void>();SwingUtilities.invokeLater(()->flushDraft().whenComplete((value,error)->{if(error!=null)future.completeExceptionally(error);else future.complete(null);}));return future;
        }
        if(busy&&active!=null)return active.thenApply(reply->null);
        if(!draft)return CompletableFuture.completedFuture(null);
        if(selected==null||!canEdit())return CompletableFuture.failedFuture(new IllegalStateException("ui.graph_draft_stale"));
        return send("settings",Long.toUnsignedString(selected.id()),name.getText(),value.getText(),comment.getText(),group.getText(),Integer.toString((bookmark.isSelected()?1:0)|(breakpoint.isSelected()?2:0))).thenApply(reply->null);
    }
    private void palette(){
        Long old=list.getSelectedIndex()>=0&&list.getSelectedIndex()<visible.size()?visible.get(list.getSelectedIndex()).type():null;
        visible.clear();rows.clear();if(graph==null){create.setEnabled(false);return;}
        String query=search.getText().toLowerCase(Locale.ROOT);
        for(var recipe:graph.catalogue())if((recipe.category()+" "+recipe.name()).toLowerCase(Locale.ROOT).contains(query)){visible.add(recipe);rows.addElement(recipe.category()+" / "+recipe.name());}

        if(old!=null)for(int i=0;i<visible.size();++i)if(visible.get(i).type()==old)list.setSelectedIndex(i);
        create.setEnabled(canEdit()&&list.getSelectedIndex()>=0&&list.getSelectedIndex()<visible.size());
    }
    private void load(){
        draft=false;fieldError="";refreshing=true;try{
            name.setText(selected==null?"":selected.name());value.setText(selected==null?"0":Long.toString(selected.previewDefault()));comment.setText(selected==null?"":selected.comment());group.setText(selected==null?"":selected.group());
            bookmark.setSelected(selected!=null&&(selected.markers()&1)!=0);breakpoint.setSelected(selected!=null&&(selected.markers()&2)!=0);
            rows.clear();if(selected!=null&&selected.reference()!=null){rows.addElement((selected.referenceAvailable()?"Reference: ":"Missing Reference: ")+selected.referenceLabel());rows.addElement(selected.reference().toString());if(!selected.referenceReason().isEmpty())rows.addElement(selected.referenceReason());}if(selected!=null)for(var p:selected.ports())rows.addElement((p.direction()==0?"In":"Out")+" · "+p.name()+" · "+(p.kind()==0?"data type="+Long.toUnsignedString(p.type()):p.kind()==1?"control":"event")+(p.direction()==0&&p.kind()==0?" · single source":""));
        }finally{refreshing=false;}
    }
    public void refresh(Bridge.Snapshot next){
        if(refreshing)return;var previous=selected;snapshot=next;graph=next==null?null:next.graphs().stream().filter(g->g.document()==next.selection().document()).findFirst().orElse(null);
        actions.forEach(button->button.setEnabled(canEdit()));
        selected=graph==null||next.selection().kind()!=3?null:graph.nodes().stream().filter(n->n.id()==next.selection().element()).findFirst().orElse(null);
        if(kind.equals("graph.palette")){palette();return;}
        if(kind.equals("graph.inspector")) {
            if(previous==null||selected==null||previous.id()!=selected.id()||(!draft&&!Objects.equals(previous,selected)))load();
            caption.setText(selected==null?"Select a node in Graph Canvas":"Node "+Long.toUnsignedString(selected.id())+" · "+graph.catalogue().stream().filter(t->t.type()==selected.type()).map(Bridge.GraphTemplate::name).findFirst().orElse("Unknown"));
            if(!fieldError.isEmpty())caption.setText(fieldError);else if(draft)caption.setText("Pending input · Apply/Enter or Save; Escape cancels");
            sourceButton.setEnabled(selected!=null&&graph!=null&&graph.catalogue().stream().anyMatch(type->type.type()==selected.type()&&!type.source().isEmpty()));
            referenceJump.setEnabled(canEdit()&&selected!=null&&selected.reference()!=null&&selected.referenceAvailable());
            referenceUses.setEnabled(canEdit()&&selected!=null&&selected.reference()!=null);
            apply.setEnabled(canEdit()&&selected!=null);for(var c:List.of(name,value,comment,group,bookmark,breakpoint))c.setEnabled(canEdit()&&selected!=null);
            value.setEnabled(canEdit()&&selected!=null&&graph.catalogue().stream().anyMatch(t->t.type()==selected.type()&&(t.operation()==0||t.operation()==9)));
        }else {
            report.setText(graph==null?"Open a Graph document":kind.equals("graph.variables")?
                "Input nodes expose preview defaults in Node Inspector. Runtime inputs remain scoped by existing Graph bindings. Entity/Component references are native typed values; Alt-drag from Scene/Hierarchy or a reference input.\n\n"+graph.report():graph.report());
        }
    }
}

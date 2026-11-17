package epidemic.editor;

import java.awt.*;
import java.awt.datatransfer.StringSelection;
import java.time.Instant;
import java.util.*;
import java.util.List;
import java.util.function.Consumer;
import javax.swing.*;
import javax.swing.event.DocumentEvent;
import javax.swing.event.DocumentListener;
import javax.swing.table.DefaultTableModel;

/** Bounded presentation history, not another logger/execution owner. */
public final class RuntimeLogView extends JPanel {
    public record Link(long graph,long node,long scene,long entity,String source,int line,long asset) {}
    record Entry(long connection,long generation,long session,long tick,int severity,String category,String message,String graphName,long graph,long node,long scene,long entity,long activation,String source,int line,long asset,long timestamp) {}
    private final Deque<Entry> history=new ArrayDeque<>();private final List<Entry> visible=new ArrayList<>();
    private final JTextField search=new JTextField(),graphFilter=new JTextField(),entityFilter=new JTextField(),activationFilter=new JTextField(),tickFilter=new JTextField();
    private final JComboBox<String> severity=new JComboBox<>(new String[]{"All","Info","Warning","Error"});
    private final JCheckBox current=new JCheckBox("Current session only",false);
    private final DefaultTableModel model=new DefaultTableModel(new String[]{"UTC","Session","Tick","Severity","Category","Graph / Node","Entity / Activation","Message"},0){public boolean isCellEditable(int row,int column){return false;}};
    private final JTable table=new JTable(model);private final boolean problemsOnly;private final Consumer<Link> navigate;
    private Bridge.RuntimeSession frame=Bridge.RuntimeSession.empty();private Bridge.Snapshot snapshot;private long connection=-1,session=-1,lastSequence;
    private final JButton source=new JButton("Go to Source"),entity=new JButton("Go to Entity");
    public RuntimeLogView(boolean problemsOnly,Consumer<Link> navigate){
        super(new BorderLayout(4,4));this.problemsOnly=problemsOnly;this.navigate=navigate;var plain=new javax.swing.table.DefaultTableCellRenderer();plain.putClientProperty("html.disable",true);table.setDefaultRenderer(Object.class,plain);table.setName(problemsOnly?"problems.output":"console.output");table.setAutoResizeMode(JTable.AUTO_RESIZE_OFF);
        JPanel filters=new JPanel(new GridLayout(0,2,4,4));addField(filters,"Search",search);filters.add(severity);filters.add(current);addField(filters,"Graph name / ID",graphFilter);addField(filters,"Entity ID",entityFilter);addField(filters,"Activation ID",activationFilter);addField(filters,"Tick",tickFilter);var filterBox=new JPanel(new BorderLayout());var toggle=new JToggleButton("Filters",false);filters.setVisible(false);toggle.addActionListener(e->{filters.setVisible(toggle.isSelected());filterBox.revalidate();});filterBox.add(toggle,BorderLayout.NORTH);filterBox.add(filters);add(filterBox,BorderLayout.NORTH);add(new JScrollPane(table));
        JPanel actions=new JPanel(new GridLayout(0,2,4,4));actions.add(source);actions.add(entity);JButton copy=new JButton("Copy"),clear=new JButton("Clear display");actions.add(copy);actions.add(clear);add(actions,BorderLayout.SOUTH);
        source.addActionListener(e->{var row=selected();if(row!=null)navigate.accept(new Link(row.graph,row.node,0,0,row.source,row.line,row.asset));});
        entity.addActionListener(e->{var row=selected();if(row!=null)navigate.accept(new Link(0,0,row.scene,row.entity,"",1,0));});
        copy.addActionListener(e->{StringBuilder text=new StringBuilder();int[] selected=table.getSelectedRows();if(selected.length==0)selected=java.util.stream.IntStream.range(0,model.getRowCount()).toArray();for(int row:selected){for(int c=0;c<model.getColumnCount();c++){if(c>0)text.append('\t');text.append(model.getValueAt(row,c));}text.append('\n');}Toolkit.getDefaultToolkit().getSystemClipboard().setContents(new StringSelection(text.toString()),null);});
        clear.addActionListener(e->{history.clear();rebuild();});
        table.getSelectionModel().addListSelectionListener(e->buttons());
        table.addMouseListener(new java.awt.event.MouseAdapter(){public void mouseClicked(java.awt.event.MouseEvent e){if(e.getClickCount()==2&&source.isEnabled())source.doClick();}});
        var edited=new DocumentListener(){public void insertUpdate(DocumentEvent e){rebuild();}public void removeUpdate(DocumentEvent e){rebuild();}public void changedUpdate(DocumentEvent e){rebuild();}};
        for(var field:List.of(search,graphFilter,entityFilter,activationFilter,tickFilter))field.getDocument().addDocumentListener(edited);severity.addActionListener(e->rebuild());current.addActionListener(e->rebuild());buttons();
        for(int c=0;c<table.getColumnCount();c++)table.getColumnModel().getColumn(c).setPreferredWidth(c==7?540:c==5||c==6?190:110);
    }
    private static void addField(JPanel panel,String hint,JTextField field){field.setToolTipText(hint);field.getAccessibleContext().setAccessibleName(hint);panel.add(new JLabel(hint));panel.add(field);}
    private Entry selected(){int row=table.getSelectedRow();return row>=0&&row<visible.size()?visible.get(row):null;}
    private void buttons(){var row=selected();boolean same=row!=null&&row.connection==frame.connection()&&row.generation==frame.generation();boolean graph=same&&snapshot!=null&&snapshot.graphs().stream().anyMatch(g->g.document()==row.graph&&(row.node==0||g.nodes().stream().anyMatch(n->n.id()==row.node)));
        boolean asset=same&&snapshot!=null&&snapshot.assets().stream().anyMatch(a->a.id()==row.asset);
        boolean file=same&&!row.source.isEmpty()&&snapshot!=null&&java.nio.file.Files.isRegularFile(java.nio.file.Path.of(snapshot.path()).getParent().resolve(row.source));
        source.setEnabled(graph||asset||file);entity.setEnabled(same&&snapshot!=null&&row.scene!=0&&row.entity!=0&&snapshot.nodes().stream().anyMatch(n->n.document()==row.scene&&n.id()==row.entity));}
    private void append(Entry entry){
        if(!history.isEmpty()&&history.getLast().timestamp>entry.timestamp){var sorted=new ArrayList<>(history);sorted.add(entry);sorted.sort(Comparator.comparingLong(Entry::timestamp));history.clear();history.addAll(sorted);}else history.addLast(entry);if(history.size()>512)history.removeFirst();
    }
    void replay(Entry entry){append(entry);rebuild();}
    public void authoring(String text,int level,Bridge.Snapshot snapshot){
        this.snapshot=snapshot;append(authoringEntry(text,level,snapshot,frame));rebuild();
    }
    static Entry authoringEntry(String text,int level,Bridge.Snapshot snapshot,Bridge.RuntimeSession frame){
        long graph=0,node=0,asset=0;String name="",file="";int line=1;
        var scoped=java.util.regex.Pattern.compile("graph=(\\d+)").matcher(text);if(scoped.find())try{graph=Long.parseUnsignedLong(scoped.group(1));}catch(NumberFormatException ignored){}
        var match=java.util.regex.Pattern.compile("node=(\\d+)").matcher(text);
        if(match.find()&&snapshot!=null)try{long target=Long.parseUnsignedLong(match.group(1));final long scope=graph;var candidates=snapshot.graphs().stream().filter(g->(scope==0||g.document()==scope)&&g.nodes().stream().anyMatch(n->n.id()==target)).toList();var chosen=candidates.stream().filter(g->g.document()==snapshot.selection().document()).findFirst().orElse(candidates.size()==1?candidates.getFirst():null);if(chosen!=null){graph=chosen.document();node=target;}}catch(NumberFormatException ignored){}
        var location=java.util.regex.Pattern.compile("(?i)(Scripts[/\\\\][^\\r\\n:]+\\.cs)(?::(?:line\\s*)?(\\d+)|\\((\\d+)\\))?").matcher(text);
        if(location.find()){file=location.group(1);try{line=Integer.parseInt(location.group(2)!=null?location.group(2):location.group(3)!=null?location.group(3):"1");}catch(NumberFormatException ignored){line=1;}}
        if(snapshot!=null){var assets=snapshot.assets().stream().filter(a->!a.path().isEmpty()&&text.contains(a.path())).toList();if(assets.size()==1)asset=assets.getFirst().id();final long target=graph;name=snapshot.documents().stream().filter(d->d.id()==target).map(Bridge.Document::name).findFirst().orElse("");}
        return new Entry(frame.connection(),frame.generation(),0,0,level,"Authoring",text.length()>4096?text.substring(0,4096):text,name,graph,node,0,0,0,file,line,asset,System.currentTimeMillis());
    }
    public void refresh(Bridge.RuntimeSession next,Bridge.Snapshot snapshot){
        this.snapshot=snapshot;boolean changed=connection!=next.connection()||session!=next.session()||frame.generation()!=next.generation();frame=next;
        if(changed){connection=next.connection();session=next.session();lastSequence=0;}
        for(var event:next.trace())if(event.sequence()>lastSequence){
            String name=snapshot==null?"":snapshot.documents().stream().filter(d->d.id()==event.graph()).map(Bridge.Document::name).findFirst().orElse("");
            String source="";int line=1;var location=java.util.regex.Pattern.compile("(?i)(Scripts[/\\\\][^\\r\\n:]+\\.cs)(?::(?:line\\s*)?(\\d+))?").matcher(event.detail());if(location.find()){source=location.group(1);try{line=location.group(2)==null?1:Integer.parseInt(location.group(2));}catch(NumberFormatException ignored){}}
            append(new Entry(next.connection(),next.generation(),next.session(),event.tick(),event.severity(),event.state(),event.detail(),name,event.graph(),event.node(),next.scene(),event.entity(),event.activation(),source,line,0,event.timestamp()));lastSequence=Math.max(lastSequence,event.sequence());changed=true;
        }
        if(changed)rebuild();
    }
    private static String id(long value){return Long.toUnsignedString(value);}
    private static boolean contains(String value,JTextField filter){return value.toLowerCase(Locale.ROOT).contains(filter.getText().toLowerCase(Locale.ROOT));}
    private void rebuild(){
        var selected=selected();Point scroll=table.getParent() instanceof JViewport v?v.getViewPosition():null;visible.clear();model.setRowCount(0);
        for(var row:history){
            if(problemsOnly&&row.severity==0||severity.getSelectedIndex()>0&&row.severity!=severity.getSelectedIndex()-1)continue;
            if(current.isSelected()&&(row.connection!=frame.connection()||row.generation!=frame.generation()||row.session!=frame.session()))continue;
            if(!contains(row.message+" "+row.category+" "+row.source+" "+row.graphName,search)||!contains(row.graphName+" "+id(row.graph),graphFilter)||!contains(id(row.entity),entityFilter)||!contains(id(row.activation),activationFilter)||!contains(id(row.tick),tickFilter))continue;
            visible.add(row);model.addRow(new Object[]{Instant.ofEpochMilli(row.timestamp).toString(),row.connection+" / "+row.session,row.tick,new String[]{"Info","Warning","Error"}[row.severity],row.category,row.graphName+" / "+id(row.node),id(row.entity)+" / "+id(row.activation),row.message});
        }
        if(selected!=null){int index=visible.indexOf(selected);if(index>=0)table.setRowSelectionInterval(index,index);}if(scroll!=null&&table.getParent() instanceof JViewport viewport)viewport.setViewPosition(scroll);buttons();
    }
}

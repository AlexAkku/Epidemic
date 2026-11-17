package epidemic.editor;

import java.awt.*;
import java.util.*;
import java.util.List;
import java.util.function.Consumer;
import javax.swing.*;
import javax.swing.table.DefaultTableModel;

/** Read-only native activations plus explicit, session-guarded debug commands. */
public final class RuntimeDebugger extends JPanel {
    private final JLabel status=new JLabel("Stopped");
    private final DefaultListModel<Bridge.RuntimeActivation> active=new DefaultListModel<>();private final JList<Bridge.RuntimeActivation> list=new JList<>(active);
    private final DefaultTableModel values=new DefaultTableModel(new String[]{"Port","Type","Runtime value"},0){public boolean isCellEditable(int r,int c){return false;}};
    private final UiPanels.RuntimeCommand commands;private final Consumer<RuntimeLogView.Link> navigate;
    private Bridge.RuntimeSession runtime=Bridge.RuntimeSession.empty();private Bridge.Snapshot snapshot;private boolean updating;
    private final Map<String,JButton> buttons=new LinkedHashMap<>();private final RuntimeLogView trace;
    public RuntimeDebugger(UiPanels.RuntimeCommand commands,Consumer<RuntimeLogView.Link> navigate){
        super(new BorderLayout(4,4));this.commands=commands;this.navigate=navigate;trace=new RuntimeLogView(false,navigate);
        var header=new JPanel(new BorderLayout());header.add(status,BorderLayout.NORTH);var actions=new JPanel(new GridLayout(0,3,4,4));
        for(var pair:new String[][]{{"Pause","pause"},{"Continue","resume"},{"Step Tick","step"},{"Step to Node","step-node"},{"Toggle Breakpoint","breakpoint"},{"Go to Node","node"},{"Open C# Source","source"}}){String action=pair[1];var button=new JButton(pair[0]);button.setToolTipText(action.equals("step-node")?"Next native node boundary/budget yield; does not advance Runtime time":pair[0]);button.addActionListener(e->invoke(action));buttons.put(action,button);actions.add(button);}header.add(actions,BorderLayout.SOUTH);add(header,BorderLayout.NORTH);
        list.setCellRenderer(new DefaultListCellRenderer(){public Component getListCellRendererComponent(JList<?> owner,Object value,int index,boolean selected,boolean focused){if(value instanceof Bridge.RuntimeActivation activation)value="Activation "+Long.toUnsignedString(activation.id())+" · "+activation.state()+" · node "+Long.toUnsignedString(activation.node())+" · instructions "+activation.instructions()+" · native calls "+activation.nativeCalls();return super.getListCellRendererComponent(owner,value,index,selected,focused);}});
        list.setName("runtime.activations");list.addListSelectionListener(e->{if(!updating)showValues();});
        var valueTable=new JTable(values);valueTable.setName("runtime.port.values");var plain=new javax.swing.table.DefaultTableCellRenderer();plain.putClientProperty("html.disable",true);valueTable.setDefaultRenderer(Object.class,plain);
        var detail=new JPanel(new BorderLayout());var split=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,new JScrollPane(list),new JScrollPane(valueTable));split.setResizeWeight(.35);split.getLeftComponent().setMinimumSize(new Dimension(0,0));split.getRightComponent().setMinimumSize(new Dimension(0,0));detail.add(split);
        var tabs=new JTabbedPane();tabs.addTab("Activations / Values",detail);tabs.addTab("Trace / Filters",trace);add(tabs);
    }
    public void seedHistory(Collection<Bridge.RuntimeSession> frames,Bridge.Snapshot snapshot){for(var frame:frames)trace.refresh(frame,snapshot);}
    private long[] target(){
        if(snapshot!=null&&snapshot.selection().kind()==3)return new long[]{snapshot.selection().document(),snapshot.selection().element()};
        var selected=list.getSelectedValue();return selected==null?new long[]{0,0}:new long[]{selected.graph(),selected.node()};
    }
    private void invoke(String action){
        var target=target();
        if(action.equals("node")){navigate.accept(new RuntimeLogView.Link(target[0],target[1],0,0,"",1,0));return;}
        if(action.equals("source")){
            if(snapshot!=null)for(var graph:snapshot.graphs())if(graph.document()==target[0])for(var node:graph.nodes())if(node.id()==target[1])for(var type:graph.catalogue())if(type.type()==node.type()&&!type.source().isEmpty()){navigate.accept(new RuntimeLogView.Link(0,0,0,0,type.source(),type.line(),0));return;}return;
        }
        String[] args=new String[0];
        if(action.equals("step-node"))args=new String[]{list.getSelectedValue()==null?"0":Long.toUnsignedString(list.getSelectedValue().id())};
        if(action.equals("breakpoint")){boolean on=runtime.breakpoints().stream().anyMatch(p->p.graph()==target[0]&&p.node()==target[1]);args=new String[]{Long.toUnsignedString(target[0]),Long.toUnsignedString(target[1]),on?"0":"1"};}
        commands.execute(runtime.connection(),runtime.generation(),runtime.session(),action,args).whenComplete((reply,error)->SwingUtilities.invokeLater(()->{if(error!=null)status.setText("Debug command rejected: "+error.getMessage());}));
    }
    private void showValues(){
        var rows=new ArrayList<Object[]>();var selected=list.getSelectedValue();if(selected!=null)for(var value:selected.values()){
            String name=Long.toUnsignedString(value.port());if(snapshot!=null)for(var graph:snapshot.graphs())if(graph.document()==selected.graph())for(var node:graph.nodes())for(var port:node.ports())if(port.id()==value.port())name=node.name()+" / "+(port.direction()==0?"In ":"Out ")+port.name();
            rows.add(new Object[]{name,value.type(),value.value()});
        }
        if(values.getRowCount()!=rows.size()){values.setRowCount(0);rows.forEach(values::addRow);}else for(int row=0;row<rows.size();row++)for(int col=0;col<3;col++)if(!Objects.equals(values.getValueAt(row,col),rows.get(row)[col]))values.setValueAt(rows.get(row)[col],row,col);
        long[] target=target();buttons.get("node").setEnabled(target[0]!=0&&target[1]!=0);buttons.get("breakpoint").setEnabled(runtime.live()&&target[0]!=0&&target[1]!=0);
        boolean source=false;if(snapshot!=null)for(var graph:snapshot.graphs())if(graph.document()==target[0])for(var node:graph.nodes())if(node.id()==target[1])source=graph.catalogue().stream().anyMatch(t->t.type()==node.type()&&!t.source().isEmpty());buttons.get("source").setEnabled(source);
    }
    public void refresh(Bridge.RuntimeSession frame,Bridge.Snapshot snapshot){
        this.runtime=frame;this.snapshot=snapshot;long selected=list.getSelectedValue()==null?0:list.getSelectedValue().id();Point scroll=list.getParent() instanceof JViewport v?v.getViewPosition():null;updating=true;
        try{active.clear();frame.activations().forEach(active::addElement);for(int i=0;i<active.size();i++)if(active.get(i).id()==selected)list.setSelectedIndex(i);if(list.getSelectedIndex()<0&&!active.isEmpty())list.setSelectedIndex(0);}finally{updating=false;}
        if(scroll!=null&&list.getParent() instanceof JViewport v)v.setViewPosition(scroll);showValues();
        buttons.get("pause").setEnabled(frame.phase()==2);for(String action:List.of("resume","step","step-node"))buttons.get(action).setEnabled(frame.phase()==3);
        var current=list.getSelectedValue();status.setText(frame.state()+" · Session "+frame.session()+" · Tick "+frame.tick()+" · active="+frame.activationCount()+(current!=null&&current.state().equals("Waiting")?" · Wait due tick "+current.wake()+" (remaining "+Math.max(0,current.wake()-frame.tick())+")":""));trace.refresh(frame,snapshot);
    }
}

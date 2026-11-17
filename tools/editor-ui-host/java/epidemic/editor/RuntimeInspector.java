package epidemic.editor;

import java.awt.*;
import java.util.List;
import java.util.ArrayList;
import java.util.Objects;
import javax.swing.*;
import javax.swing.table.DefaultTableModel;

/** Native immutable values only. No editor fields are used as runtime fallbacks. */
public final class RuntimeInspector extends JPanel {
    private final JLabel status=new JLabel("Stopped");
    private final JComboBox<Bridge.RuntimeEntity> entities=new JComboBox<>();
    private final DefaultTableModel fields=new DefaultTableModel(new String[]{"Component","Field","Runtime value"},0){public boolean isCellEditable(int row,int column){return false;}};
    private Bridge.RuntimeSession runtime=Bridge.RuntimeSession.empty();private boolean refreshing;
    public RuntimeInspector(){
        super(new BorderLayout(4,4));var header=new JPanel(new BorderLayout(4,4));header.add(status,BorderLayout.NORTH);header.add(entities,BorderLayout.SOUTH);add(header,BorderLayout.NORTH);
        var table=new JTable(fields);table.setName("runtime.fields");var plain=new javax.swing.table.DefaultTableCellRenderer();plain.putClientProperty("html.disable",true);table.setDefaultRenderer(Object.class,plain);table.setAutoResizeMode(JTable.AUTO_RESIZE_LAST_COLUMN);add(new JScrollPane(table));
        entities.setRenderer(new DefaultListCellRenderer(){{putClientProperty("html.disable",true);}});
        entities.addActionListener(e->{if(!refreshing)values();});
    }
    private void values(){
        var rows=new ArrayList<Object[]>();
        if(entities.getSelectedItem() instanceof Bridge.RuntimeEntity choice){
            var entity=runtime.entities().stream().filter(e->e.placement()==choice.placement()&&e.node()==choice.node()).findFirst().orElse(null);
            if(entity!=null){rows.add(new Object[]{"World","Position",entity.x()+", "+entity.y()});for(var field:entity.fields())rows.add(new Object[]{field.component()+" / part "+Long.toUnsignedString(field.part()),field.name(),field.value()});}
        }
        if(fields.getRowCount()!=rows.size()){fields.setRowCount(0);rows.forEach(fields::addRow);}else for(int row=0;row<rows.size();row++)for(int col=0;col<3;col++)if(!Objects.equals(fields.getValueAt(row,col),rows.get(row)[col]))fields.setValueAt(rows.get(row)[col],row,col);
    }
    public void refresh(Bridge.RuntimeSession next){
        boolean same=runtime.connection()==next.connection()&&runtime.generation()==next.generation()&&runtime.session()==next.session()&&entities.getItemCount()==next.entities().size()&&java.util.stream.IntStream.range(0,entities.getItemCount()).allMatch(i->{var old=entities.getItemAt(i);var value=next.entities().get(i);return old.placement()==value.placement()&&old.node()==value.node()&&old.name().equals(value.name());});
        runtime=next;var selected=entities.getSelectedItem() instanceof Bridge.RuntimeEntity e?e:null;refreshing=true;
        try{if(!same){entities.removeAllItems();next.entities().forEach(entities::addItem);if(selected!=null)for(int i=0;i<entities.getItemCount();i++){var item=entities.getItemAt(i);if(item.placement()==selected.placement()&&item.node()==selected.node())entities.setSelectedIndex(i);}}}
        finally{refreshing=false;}
        values();status.setText(next.state()+" · Session "+next.session()+" · Tick "+next.tick()+" · Read only");
        setToolTipText("Native RuntimeSnapshot · "+next.entities().size()+" / "+next.entityCount()+" entities shown · fields capped at4096 · no authoring fallback");
    }
}

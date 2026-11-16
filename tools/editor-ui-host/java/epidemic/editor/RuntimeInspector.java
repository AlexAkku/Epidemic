package epidemic.editor;

import java.awt.*;
import java.util.List;
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
        var table=new JTable(fields);table.setName("runtime.fields");table.setAutoResizeMode(JTable.AUTO_RESIZE_LAST_COLUMN);add(new JScrollPane(table));
        entities.addActionListener(e->{if(!refreshing)values();});
    }
    private void values(){
        fields.setRowCount(0);if(!(entities.getSelectedItem() instanceof Bridge.RuntimeEntity entity))return;
        fields.addRow(new Object[]{"World","Position",entity.x()+", "+entity.y()});
        for(var field:entity.fields())fields.addRow(new Object[]{field.component()+" / part "+Long.toUnsignedString(field.part()),field.name(),field.value()});
    }
    public void refresh(Bridge.RuntimeSession next){
        runtime=next;var selected=entities.getSelectedItem() instanceof Bridge.RuntimeEntity e?e:null;refreshing=true;
        try{entities.removeAllItems();next.entities().forEach(entities::addItem);if(selected!=null)for(int i=0;i<entities.getItemCount();i++){var item=entities.getItemAt(i);if(item.placement()==selected.placement()&&item.node()==selected.node())entities.setSelectedIndex(i);}}
        finally{refreshing=false;}
        values();status.setText(next.state()+" · Session "+next.session()+" · Tick "+next.tick()+" · Read only");
        setToolTipText("Native RuntimeSnapshot · "+next.entities().size()+" / "+next.entityCount()+" entities shown · no authoring fallback");
    }
}

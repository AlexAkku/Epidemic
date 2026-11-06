package epidemic.editor;

import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.List;
import java.util.concurrent.*;
import java.util.function.Function;
import javax.swing.*;

/** Native descriptors/values plus ephemeral input drafts. No authoring model. */
public final class ScalarInspector extends JPanel {
    public record Edit(long generation,long revision,long selectionRevision,String key,String value) { }
    private final Function<Edit,CompletableFuture<Bridge.Reply>> commit;
    private final JPanel body=new JPanel();
    private final Map<String,Row> rows=new LinkedHashMap<>();
    private Bridge.Snapshot snapshot;
    private long generation;
    private boolean ready,updating;
    private List<String> keys=List.of();
    private final class Row {
        Bridge.Field field;final JComponent input;final JLabel error=new JLabel(" ");
        Edit draft;boolean active=true,pending;
        CompletableFuture<Bridge.Reply> completion;
        Row(Bridge.Field field){
            this.field=field;
            if(field.kind()==2){
                JCheckBox check=new JCheckBox();input=check;
                check.addActionListener(e->{if(!updating){draft=token(field.key(),Boolean.toString(check.isSelected()));send(this);}});
            }else{
                JTextField text=new JTextField();input=text;
                text.getDocument().addDocumentListener(new javax.swing.event.DocumentListener(){
                    private void changed(){if(active&&!updating&&draft==null)draft=token(Row.this.field.key(),Row.this.field.value());}
                    public void insertUpdate(javax.swing.event.DocumentEvent event){changed();}
                    public void removeUpdate(javax.swing.event.DocumentEvent event){changed();}
                    public void changedUpdate(javax.swing.event.DocumentEvent event){changed();}
                });
                text.addFocusListener(new FocusAdapter(){
                    @Override public void focusLost(FocusEvent e){if(active&&!updating&&!text.getText().equals(Row.this.field.value()))send(Row.this);}
                });
                text.addActionListener(e->send(this));
                text.getInputMap().put(KeyStroke.getKeyStroke("ESCAPE"),"native.cancel");
                text.getActionMap().put("native.cancel",new AbstractAction(){public void actionPerformed(ActionEvent e){draft=null;error.setText(" ");refreshValue(Row.this);}});
            }
            input.setName(field.key());input.getAccessibleContext().setAccessibleName(field.name());
            JPanel box=new JPanel(new BorderLayout(3,3));box.setAlignmentX(LEFT_ALIGNMENT);
            JLabel label=new JLabel(field.name());label.setLabelFor(input);box.add(label,BorderLayout.NORTH);box.add(input);
            error.setForeground(new Color(0xe58c8c));box.add(error,BorderLayout.SOUTH);
            box.setBorder(BorderFactory.createEmptyBorder(4,0,4,0));box.setMaximumSize(new Dimension(Integer.MAX_VALUE,100));
            body.add(box);
        }
    }
    public ScalarInspector(Function<Edit,CompletableFuture<Bridge.Reply>> commit){
        super(new BorderLayout());this.commit=commit;body.setLayout(new BoxLayout(body,BoxLayout.Y_AXIS));add(new JScrollPane(body));
    }
    private Edit token(String key,String value){return new Edit(generation,snapshot.revision(),snapshot.selectionRevision(),key,value);}
    public void refresh(long currentGeneration,Bridge.Snapshot value,boolean interactive){
        boolean reset=generation!=currentGeneration||snapshot==null||value==null||!snapshot.selection().equals(value.selection());
        generation=currentGeneration;snapshot=value;ready=interactive&&value!=null&&!value.playing();
        var next=value==null?List.<String>of():value.fields().stream().map(Bridge.Field::key).toList();
        updating=true;
        try{
            if(reset||!keys.equals(next)){
                rows.values().forEach(r->r.active=false);rows.clear();body.removeAll();keys=next;
                if(value!=null)for(var field:value.fields())rows.put(field.key(),new Row(field));
                if(rows.isEmpty())body.add(new JLabel("No scalar fields for this selection"));
                body.revalidate();body.repaint();
            }
            if(value!=null)for(var field:value.fields()){
                Row row=rows.get(field.key());row.field=field;
                if(row.draft==null&&!row.pending)refreshValue(row);
                row.input.setEnabled(ready&&field.editable()&&!row.pending&&field.kind()<4);
                row.input.setToolTipText(field.source()+(field.reason().isBlank()?"":" · "+field.reason())+
                    (field.reference()==0?"":" · ID "+Long.toUnsignedString(field.reference())));
            }
        }finally{updating=false;}
    }
    private void refreshValue(Row row){
        boolean old=updating;updating=true;
        try{if(row.input instanceof JCheckBox check){check.setSelected(row.field.value().equals("true"));check.setText(row.field.value().equals("true")||row.field.value().equals("false")?"":row.field.value());}
            else ((JTextField)row.input).setText(row.field.value());
        }finally{updating=old;}
    }
    private void send(Row row){
        if(!row.active||updating||row.pending||!ready||!row.field.editable())return;
        String text=row.input instanceof JCheckBox check?Boolean.toString(check.isSelected()):((JTextField)row.input).getText();
        if(text.equals(row.field.value())){row.draft=null;return;}
        Edit captured=row.draft==null?token(row.field.key(),text):row.draft;
        Edit request=new Edit(captured.generation(),captured.revision(),captured.selectionRevision(),captured.key(),text);
        row.pending=true;row.input.setEnabled(false);row.error.setText("Applying…");
        row.completion=commit.apply(request);
        row.completion.whenComplete((reply,failure)->SwingUtilities.invokeLater(()->{
            if(!row.active)return;row.pending=false;
            if(failure!=null){Throwable cause=failure;while(cause.getCause()!=null)cause=cause.getCause();row.error.setText(cause.getMessage()+" · Esc to refresh");if(row.input instanceof JCheckBox){row.draft=null;refreshValue(row);}}
            else{row.draft=null;row.error.setText(" ");refreshValue(row);}
            row.input.setEnabled(ready&&row.field.editable());
        }));
    }
    public CompletableFuture<Void> flush(){
        List<CompletableFuture<?>> work=new ArrayList<>();
        for(Row row:rows.values()){
            String text=row.input instanceof JCheckBox check?Boolean.toString(check.isSelected()):((JTextField)row.input).getText();
            if(row.field.editable()&&!text.equals(row.field.value())&&!row.pending){
                if(!ready)return CompletableFuture.failedFuture(new IllegalStateException("ui.inspector_busy"));
                send(row);
            }
            if(row.pending&&row.completion!=null)work.add(row.completion);
        }
        return CompletableFuture.allOf(work.toArray(CompletableFuture[]::new));
    }
    public void cancelDrafts(){for(Row row:rows.values())if(!row.pending){row.draft=null;row.error.setText(" ");refreshValue(row);}}
}

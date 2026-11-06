package epidemic.editor;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.util.function.Consumer;
import javax.swing.*;

/** Snapshot-only view of the selected native asset and its cooked thumbnail. */
public final class AssetDetails extends JPanel {
    private final JTextArea text=new JTextArea(12,24);
    private final DefaultListModel<Bridge.Reference> references=new DefaultListModel<>();
    private final Preview preview=new Preview();
    private String pixels="";
    private long selectedAsset;
    private java.util.List<Bridge.Reference> shownReferences=java.util.List.of();
    public AssetDetails(boolean showReferences,Consumer<Long> navigate){
        super(new BorderLayout(5,5));text.setEditable(false);text.setLineWrap(true);text.setWrapStyleWord(true);
        add(new JScrollPane(text),BorderLayout.NORTH);
        if(showReferences){
            JList<Bridge.Reference> list=new JList<>(references);
            list.setCellRenderer(new DefaultListCellRenderer(){@Override public Component getListCellRendererComponent(JList<?> l,Object value,int index,boolean selected,boolean focus){
                var row=(Bridge.Reference)value;return super.getListCellRendererComponent(l,row.origin()+" · "+Long.toUnsignedString(row.dependent())+" → "+Long.toUnsignedString(row.source()),index,selected,focus);
            }});
            list.addListSelectionListener(e->{if(!e.getValueIsAdjusting()&&list.getSelectedValue()!=null){var row=list.getSelectedValue();navigate.accept(row.source()==selectedAsset?row.dependent():row.source());}});
            add(new JScrollPane(list));
        } else {
            add(preview);JCheckBox fit=new JCheckBox("Fit preview",true);fit.addActionListener(e->{preview.fit=fit.isSelected();preview.repaint();});add(fit,BorderLayout.SOUTH);
        }
    }
    public void refresh(Bridge.Snapshot snapshot){
        var asset=snapshot==null||snapshot.selection().kind()!=6?null:snapshot.assets().stream().filter(a->a.id()==snapshot.selection().element()).findFirst().orElse(null);
        if(asset==null){selectedAsset=0;if(!shownReferences.isEmpty()){references.clear();shownReferences=java.util.List.of();}if(!text.getText().equals("Select an asset in Project Explorer"))text.setText("Select an asset in Project Explorer");preview.image=null;pixels="";preview.repaint();return;}
        boolean sameAsset=selectedAsset==asset.id();selectedAsset=asset.id();
        boolean lastGood=!asset.cookedKey().isEmpty()&&!asset.metadata().equals("Cooked metadata unavailable")&&snapshot.assetState().previewError().isEmpty();
        String status=asset.trashed()?"Trash / missing references retained":!asset.present()?(lastGood?"Missing source · last-good cooked content retained":"Missing source · last-good unavailable"):"Source present";
        String diagnostic="";boolean failed=false;
        for(var event:snapshot.assetState().events())if(event.id()==asset.id()){diagnostic=event.kind()==1||event.kind()==4?"":event.diagnostic();failed=event.kind()==2||event.kind()==3;}
        String description=asset.path()+"\nAssetId "+Long.toUnsignedString(asset.id())+"\nType "+Long.toUnsignedString(asset.type())+
            "\nGeneration "+asset.generation()+"\nSource fingerprint "+Long.toUnsignedString(asset.fingerprint())+
            "\nCooked SHA-256 "+asset.cookedKey()+"\n"+asset.metadata()+"\n"+status+
            (asset.originalPath().isEmpty()?"":"\nOriginal path "+asset.originalPath())+
            (diagnostic.isEmpty()?"":"\n"+(failed?"Error / "+(lastGood?"last-good retained: ":"last-good unavailable: "):"Native status: ")+diagnostic)+"\n"+snapshot.assetState().previewError();
        if(!text.getText().equals(description)){int caret=sameAsset?Math.min(text.getCaretPosition(),description.length()):0;text.setText(description);text.setCaretPosition(caret);}
        var rows=snapshot.assetState().references().stream().filter(r->r.source()==asset.id()||r.dependent()==asset.id()).toList();
        if(!rows.equals(shownReferences)){references.clear();for(var row:rows)references.addElement(row);shownReferences=rows;}
        var state=snapshot.assetState();
        if(!pixels.equals(state.pixels())){
            pixels=state.pixels();preview.image=null;
            if(state.width()>0&&state.height()>0){
                BufferedImage image=new BufferedImage(state.width(),state.height(),BufferedImage.TYPE_INT_ARGB_PRE);
                int[] data=new int[state.width()*state.height()];
                for(int i=0;i<data.length;i++){int offset=i*8;int b=Integer.parseInt(pixels.substring(offset,offset+2),16),g=Integer.parseInt(pixels.substring(offset+2,offset+4),16),r=Integer.parseInt(pixels.substring(offset+4,offset+6),16),a=Integer.parseInt(pixels.substring(offset+6,offset+8),16);data[i]=(a<<24)|(r<<16)|(g<<8)|b;}
                image.getRaster().setDataElements(0,0,state.width(),state.height(),data);preview.image=image;
            }
        }
        preview.repaint();
    }
    private static final class Preview extends JComponent {
        BufferedImage image;boolean fit=true;
        @Override protected void paintComponent(Graphics graphics){
            super.paintComponent(graphics);
            for(int y=0;y<getHeight();y+=12)for(int x=0;x<getWidth();x+=12){graphics.setColor(((x/12+y/12)&1)==0?new Color(0x303030):new Color(0x454545));graphics.fillRect(x,y,12,12);}
            if(image==null)return;
            double scale=fit?Math.min((double)getWidth()/image.getWidth(),(double)getHeight()/image.getHeight()):1;
            int width=(int)(image.getWidth()*scale),height=(int)(image.getHeight()*scale);
            ((Graphics2D)graphics).setRenderingHint(RenderingHints.KEY_INTERPOLATION,RenderingHints.VALUE_INTERPOLATION_NEAREST_NEIGHBOR);
            graphics.drawImage(image,(getWidth()-width)/2,(getHeight()-height)/2,width,height,null);
        }
    }
}

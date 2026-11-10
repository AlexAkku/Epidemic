package epidemic.editor;

import java.awt.*;
import java.awt.image.BufferedImage;
import java.util.function.Consumer;
import java.util.concurrent.CompletableFuture;
import java.util.Objects;
import javax.swing.*;

/** Native asset metadata with an on-demand view of the cooked texture. */
public final class AssetDetails extends JPanel {
    private final JTextArea text=new JTextArea(12,24);
    private final DefaultListModel<Bridge.Reference> references=new DefaultListModel<>();
    @FunctionalInterface public interface PreviewLoader {
        CompletableFuture<BufferedImage> load(long asset,String key,int divisor);
    }
    private record PreviewKey(long asset,String key,int divisor) {}
    private final Preview preview=new Preview();
    private final PreviewLoader loader;
    private final JComboBox<String> quality=new JComboBox<>(new String[]{"Full","Half","Quarter"});
    private final JLabel resolution=new JLabel();
    private PreviewKey requested;
    private Bridge.Asset selected;
    private String pixels="";
    private long selectedAsset;
    private java.util.List<Bridge.Reference> shownReferences=java.util.List.of();
    public AssetDetails(boolean showReferences,Consumer<Long> navigate){
        this(showReferences,navigate,null);
    }
    public AssetDetails(boolean showReferences,Consumer<Long> navigate,PreviewLoader loader){
        super(new BorderLayout(5,5));this.loader=showReferences?null:loader;text.setEditable(false);text.setLineWrap(true);text.setWrapStyleWord(true);
        add(new JScrollPane(text),BorderLayout.NORTH);
        if(showReferences){
            JList<Bridge.Reference> list=new JList<>(references);
            list.setCellRenderer(new DefaultListCellRenderer(){@Override public Component getListCellRendererComponent(JList<?> l,Object value,int index,boolean selected,boolean focus){
                var row=(Bridge.Reference)value;return super.getListCellRendererComponent(l,row.origin()+" · "+Long.toUnsignedString(row.dependent())+" → "+Long.toUnsignedString(row.source()),index,selected,focus);
            }});
            list.addListSelectionListener(e->{if(!e.getValueIsAdjusting()&&list.getSelectedValue()!=null){var row=list.getSelectedValue();navigate.accept(row.source()==selectedAsset?row.dependent():row.source());}});
            add(new JScrollPane(list));
        } else {
            add(preview);
            JCheckBox fit=new JCheckBox("Fit preview",true);
            fit.addActionListener(e->{preview.fit=fit.isSelected();preview.repaint();});
            JPanel controls=new JPanel(new FlowLayout(FlowLayout.LEFT));
            controls.add(fit);
            if(this.loader!=null){
                quality.setEnabled(false);
                quality.setName("asset.preview.quality");
                quality.getAccessibleContext().setAccessibleName("Preview quality");
                quality.addActionListener(e->requestPreview());
                controls.add(quality);controls.add(resolution);
            }
            add(controls,BorderLayout.SOUTH);
        }
    }
    public void refresh(Bridge.Snapshot snapshot){
        var asset=snapshot==null||snapshot.selection().kind()!=6?null:snapshot.assets().stream().filter(a->a.id()==snapshot.selection().element()).findFirst().orElse(null);
        selected=asset;
        if(asset==null){requestPreview();selectedAsset=0;if(!shownReferences.isEmpty()){references.clear();shownReferences=java.util.List.of();}if(!text.getText().equals("Select an asset in Project Explorer"))text.setText("Select an asset in Project Explorer");preview.image=null;pixels="";preview.repaint();return;}
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
        if(loader!=null){requestPreview();return;}
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
    private void requestPreview(){
        if(loader==null)return;
        boolean texture=selected!=null&&!selected.cookedKey().isEmpty()&&UiPanels.texture(selected);
        quality.setEnabled(texture);
        PreviewKey key=texture?new PreviewKey(selected.id(),selected.cookedKey(),1<<quality.getSelectedIndex()):null;
        if(Objects.equals(key,requested))return;
        requested=key;
        preview.image=null;
        preview.repaint();
        resolution.setText(key==null?"":"Loading…");
        if(key==null)return;
        loader.load(key.asset(),key.key(),key.divisor()).whenComplete((image,error)->SwingUtilities.invokeLater(()->{
            if(requested!=key)return;
            if(error!=null){resolution.setText("Preview unavailable");resolution.setToolTipText(error.getMessage());return;}
            resolution.setToolTipText(null);
            preview.image=image;
            resolution.setText(image.getWidth()+" × "+image.getHeight());
            preview.repaint();
        }));
    }
    private static final class Preview extends JComponent {
        BufferedImage image;boolean fit=true;
        @Override protected void paintComponent(Graphics graphics){
            super.paintComponent(graphics);
            for(int y=0;y<getHeight();y+=12)for(int x=0;x<getWidth();x+=12){graphics.setColor(((x/12+y/12)&1)==0?new Color(0x303030):new Color(0x454545));graphics.fillRect(x,y,12,12);}
            if(image==null)return;
            double scale=fit?Math.min((double)getWidth()/image.getWidth(),(double)getHeight()/image.getHeight()):1;
            int width=(int)(image.getWidth()*scale),height=(int)(image.getHeight()*scale);
            ((Graphics2D)graphics).setRenderingHint(RenderingHints.KEY_INTERPOLATION,RenderingHints.VALUE_INTERPOLATION_BICUBIC);
            graphics.drawImage(image,(getWidth()-width)/2,(getHeight()-height)/2,width,height,null);
        }
    }
}

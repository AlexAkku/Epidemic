package epidemic.editor;

import epidemic.u1.NativeViewport;
import java.awt.*;
import java.awt.event.*;
import java.util.*;
import java.util.List;
import java.util.concurrent.CompletableFuture;
import java.util.function.BiFunction;
import javax.swing.*;

/** Read-only renderer projection. No time accumulator, simulation or game update lives here. */
public final class RuntimeGameView extends JPanel {
    private final NativeViewport viewport=new NativeViewport();
    private final JLabel status=new JLabel("Stopped"),focus=new JLabel("Click Game to control · WASD/arrows · Escape releases input");
    private final JComboBox<Bridge.Document> scenes=new JComboBox<>();
    private final Map<String,JButton> actions=new LinkedHashMap<>();
    private final Set<Integer> keys=new HashSet<>();
    private final UiPanels.RuntimeCommand commands;
    private Bridge.RuntimeSession runtime=Bridge.RuntimeSession.empty();
    private boolean refreshing,pointerDown,pointerSending;
    private record Pointer(long connection,long generation,long session,double x,double y,boolean down){}
    private Pointer pendingPointer;
    private void sendPointer(){
        if(pointerSending||pendingPointer==null)return;var value=pendingPointer;pendingPointer=null;pointerSending=true;
        commands.execute(value.connection(),value.generation(),value.session(),"pointer",new String[]{Double.toString(value.x()),Double.toString(value.y()),value.down()?"1":"0"}).whenComplete((reply,error)->SwingUtilities.invokeLater(()->{pointerSending=false;if(error!=null)status.setText("Pointer rejected: "+error.getMessage());sendPointer();}));
    }
    private final JPanel gameHost=new JPanel(null);
    private final JComboBox<String> resolution=new JComboBox<>(new String[]{"Fit viewport","640 x 360","1280 x 720","1920 x 1080"});
    public RuntimeGameView(UiPanels.RuntimeCommand commands){
        this(commands,true);
    }
    public RuntimeGameView(UiPanels.RuntimeCommand commands,boolean showViewport){
        super(new BorderLayout(4,4));this.commands=commands;
        JPanel header=new JPanel(new BorderLayout(4,4));header.add(status,BorderLayout.NORTH);
        JPanel bar=new JPanel(new GridLayout(0,showViewport?4:2,4,2));
        for(var pair:new String[][]{{"Play","play"},{"Stop","stop"},{"Pause","pause"},{"Resume","resume"},{"Step Tick","step"},{"Step to Node","step-node"},{"Restart","restart"}}){
            String action=pair[1];var button=new JButton(pair[0]);button.setName("runtime."+action);button.addActionListener(e->invoke(action,action.equals("step-node")?new String[]{"0"}:new String[0]));bar.add(button);actions.put(action,button);
        }
        var fit=new JButton("Fit");fit.addActionListener(e->viewport.fit());if(showViewport)bar.add(fit);header.add(bar,BorderLayout.CENTER);
        scenes.setRenderer(new DefaultListCellRenderer(){{putClientProperty("html.disable",true);}public Component getListCellRendererComponent(JList<?> list,Object item,int index,boolean selected,boolean focused){return super.getListCellRendererComponent(list,item instanceof Bridge.Document doc?doc.name():item,index,selected,focused);}});
        scenes.addActionListener(e->{if(!refreshing&&scenes.getSelectedItem() instanceof Bridge.Document doc)invoke("scene",Long.toUnsignedString(doc.id()));});header.add(scenes,BorderLayout.SOUTH);
        add(header,BorderLayout.NORTH);if(showViewport){
            var settings=new JPanel(new GridLayout(1,2,4,4));settings.add(resolution);var rates=new JComboBox<String>(new String[]{"Present 60 FPS","Present 120 FPS"});rates.addActionListener(e->viewport.presentationRate(rates.getSelectedIndex()==0?60:120));settings.add(rates);
            var center=new JPanel(new BorderLayout());center.add(settings,BorderLayout.NORTH);gameHost.add(viewport);center.add(new JScrollPane(gameHost));add(center);
            gameHost.addComponentListener(new ComponentAdapter(){public void componentResized(ComponentEvent e){sizeGame();}});resolution.addActionListener(e->sizeGame());
        }else add(new JLabel("Controls share the native Game session"));add(focus,BorderLayout.SOUTH);viewport.grid(false);viewport.presentationRate(60);
        viewport.addMouseListener(new MouseAdapter(){public void mousePressed(MouseEvent e){viewport.requestFocusInWindow();pointerDown=SwingUtilities.isLeftMouseButton(e);pointer(e);}public void mouseReleased(MouseEvent e){pointerDown=false;pointer(e);}});
        viewport.addMouseMotionListener(new MouseMotionAdapter(){public void mouseDragged(MouseEvent e){pointer(e);}});
        viewport.addKeyListener(new KeyAdapter(){public void keyPressed(KeyEvent e){key(e,true);}public void keyReleased(KeyEvent e){key(e,false);}});
        viewport.addFocusListener(new FocusAdapter(){public void focusLost(FocusEvent e){releaseInput();}});
        viewport.addHierarchyListener(e->{if(!viewport.isShowing())releaseInput();});
    }
    private void sizeGame(){int index=resolution.getSelectedIndex();int width=index==0?gameHost.getWidth():new int[]{0,640,1280,1920}[index],height=index==0?gameHost.getHeight():new int[]{0,360,720,1080}[index];viewport.setBounds(0,0,Math.max(1,width),Math.max(1,height));Dimension preferred=index==0?new Dimension(0,0):new Dimension(width,height);if(!gameHost.getPreferredSize().equals(preferred)){gameHost.setPreferredSize(preferred);gameHost.revalidate();}}
    private void pointer(MouseEvent event){if(!viewport.hasFocus()||!runtime.live())return;var world=viewport.pointer(event.getX(),event.getY());if(world.length>=2){pendingPointer=new Pointer(runtime.connection(),runtime.generation(),runtime.session(),world[0],world[1],pointerDown);sendPointer();}}
    public NativeViewport viewport(){return viewport;}
    private void invoke(String action,String... args){
        commands.execute(runtime.connection(),runtime.generation(),runtime.session(),action,args).whenComplete((reply,error)->SwingUtilities.invokeLater(()->{if(error!=null)status.setText("Runtime command rejected: "+error.getMessage());}));
    }
    private void releaseInput(){if(!keys.isEmpty()){keys.clear();if(runtime.live())invoke("input","0","0");}if(pointerDown||pointerSending||pendingPointer!=null){pointerDown=false;pendingPointer=null;if(runtime.live())invoke("pointer","0","0","0");}}
    private void key(KeyEvent event,boolean down){
        if(!viewport.hasFocus()||down&&(event.isControlDown()||event.isAltDown()||event.isMetaDown()))return;
        int key=event.getKeyCode();
        if(key==KeyEvent.VK_ESCAPE){releaseInput();viewport.transferFocus();event.consume();return;}
        if(!Set.of(KeyEvent.VK_W,KeyEvent.VK_A,KeyEvent.VK_S,KeyEvent.VK_D,KeyEvent.VK_LEFT,KeyEvent.VK_RIGHT,KeyEvent.VK_UP,KeyEvent.VK_DOWN).contains(key))return;
        boolean changed=down?keys.add(key):keys.remove(key);event.consume();
        if(changed&&runtime.live()){
            int x=(keys.contains(KeyEvent.VK_D)||keys.contains(KeyEvent.VK_RIGHT)?1:0)-(keys.contains(KeyEvent.VK_A)||keys.contains(KeyEvent.VK_LEFT)?1:0);
            int y=(keys.contains(KeyEvent.VK_W)||keys.contains(KeyEvent.VK_UP)?1:0)-(keys.contains(KeyEvent.VK_S)||keys.contains(KeyEvent.VK_DOWN)?1:0);
            invoke("input",""+x,""+y);
        }
    }
    public void refresh(Bridge.Snapshot snapshot,Bridge.RuntimeSession next){
        if(snapshot==null)return;
        long previous=runtime.session();if(runtime.connection()!=next.connection()||runtime.generation()!=next.generation()||previous!=next.session()){keys.clear();pointerDown=false;pendingPointer=null;}runtime=next;refreshing=true;
        try{
            List<Bridge.Document> options=snapshot.documents().stream().filter(d->d.kind()==0).toList();
            if(scenes.getItemCount()!=options.size()||!java.util.stream.IntStream.range(0,options.size()).allMatch(i->scenes.getItemAt(i).equals(options.get(i)))){scenes.removeAllItems();options.forEach(scenes::addItem);}
            for(int i=0;i<scenes.getItemCount();i++)if(scenes.getItemAt(i).id()==next.configuredScene())scenes.setSelectedIndex(i);
            scenes.setEnabled(!next.live());
        }finally{refreshing=false;}
        for(var entry:actions.entrySet())entry.getValue().setEnabled(switch(entry.getKey()){case "play"->!next.live();case "pause"->next.phase()==2;case "resume","step","step-node"->next.phase()==3;default->next.live();});
        long scene=next.scene()!=0?next.scene():snapshot.entry();
        List<Bridge.SceneItem> items=new ArrayList<>();List<String> labels=new ArrayList<>();List<Bridge.Sprite> sprites=new ArrayList<>();
        for(var entity:next.entities()){
            double width=Math.max(.5,Math.abs(entity.sx())*.5),height=Math.max(.5,Math.abs(entity.sy())*.5);
            items.add(new Bridge.SceneItem(scene,items.size()+1L,entity.x()-width,entity.y()-height,entity.x()+width,entity.y()+height,entity.x(),entity.y(),entity.rotation()));
            String health=entity.fields().stream().filter(f->f.name().equalsIgnoreCase("points")||f.name().equalsIgnoreCase("Health")).map(f->"Health="+f.value()).limit(3).collect(java.util.stream.Collectors.joining(" · "));
            labels.add(entity.name()+(health.isEmpty()?"":" · "+health));
            if(!entity.sprite().isEmpty())sprites.add(new Bridge.Sprite(scene,entity.sprite(),(float)entity.x(),(float)entity.y(),(float)entity.sx(),(float)entity.sy(),(float)entity.rotation(),(float)entity.ppu(),(float)entity.pivotX(),(float)entity.pivotY()));
        }
        viewport.projectLabels(labels);viewport.projectItems(scene,0,items);viewport.projectScene(java.nio.file.Path.of(snapshot.path()).getParent().toString(),sprites);
        if(previous!=next.session()&&viewport.isShowing())viewport.draw();
        status.setText(next.state()+" · Session "+Long.toUnsignedString(next.session())+" · Tick "+next.tick()+" · C++ Runtime 60 Hz · "+next.entityCount()+" entities");
        if(!next.live())releaseInput();
    }
}

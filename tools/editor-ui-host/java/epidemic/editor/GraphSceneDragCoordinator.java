package epidemic.editor;

import java.awt.*;
import java.awt.datatransfer.*;
import java.awt.dnd.*;
import java.awt.event.*;
import java.util.*;
import java.util.List;
import java.util.concurrent.*;
import java.util.function.*;
import javax.swing.*;

/** One transient coordinator per project. Only an unexported JVM-local nonce is
 * transferred through Windows OLE DnD; native queries/commands own all semantics. */
public final class GraphSceneDragCoordinator implements AutoCloseable {
    public interface Commands { CompletableFuture<Bridge.Reply> execute(long generation,long revision,int operation,String... arguments); }
    public record GraphHit(long document,long port,double x,double y) {}
    private record Token(UUID id) {}
    private static final DataFlavor FLAVOR=new DataFlavor(DataFlavor.javaJVMLocalObjectMimeType+";class="+Token.class.getName(),"Graph/Scene reference");
    private final Supplier<Bridge.Reply> latest;
    private final Commands commands;
    private final Consumer<String> status;
    private final javax.swing.Timer timer;
    private final KeyEventDispatcher escape;
    private Session active;
    private boolean closed;
    private static final class Session {
        final Token token=new Token(UUID.randomUUID());
        final long generation,revision,project,scene,element;
        final GraphHit graph;
        final Component source;
        final long started=System.nanoTime();
        boolean dropping;
        String probeKey="";
        List<Bridge.ReferenceCandidate> candidates=List.of();
        final Set<Runnable> clear=new HashSet<>();
        Session(Bridge.Reply r,Component source,long scene,long element,GraphHit graph) {
            generation=r.generation();revision=r.snapshot().revision();project=r.snapshot().project();
            this.source=source;this.scene=scene;this.element=element;this.graph=graph;
        }
    }
    public GraphSceneDragCoordinator(Supplier<Bridge.Reply> latest,Commands commands,Consumer<String> status) {
        this.latest=latest;this.commands=commands;this.status=status;
        escape=e->{if(e.getID()==KeyEvent.KEY_PRESSED&&e.getKeyCode()==KeyEvent.VK_ESCAPE&&active!=null){cancel();return true;}return false;};
        KeyboardFocusManager.getCurrentKeyboardFocusManager().addKeyEventDispatcher(escape);
        timer=new javax.swing.Timer(200,e->{if(active!=null&&!live(active))cancel();});timer.start();
    }
    private boolean live(Session s) {
        var r=latest.get();return !closed&&active==s&&r!=null&&r.snapshot()!=null&&r.error().isEmpty()&&r.generation()==s.generation&&
            r.snapshot().project()==s.project&&r.snapshot().revision()==s.revision&&!r.snapshot().playing()&&s.source.isShowing()&&
            System.nanoTime()-s.started<TimeUnit.SECONDS.toNanos(60);
    }
    public boolean supports(DataFlavor[] flavors){return Arrays.asList(flavors).contains(FLAVOR);}
    private Session session(Transferable transferable) {
        try {return active!=null&&live(active)&&active.token.equals(transferable.getTransferData(FLAVOR))?active:null;}
        catch(UnsupportedFlavorException|java.io.IOException e){return null;}
    }
    public boolean fromGraph(Transferable t){var s=session(t);return s!=null&&s.graph!=null;}
    public boolean fromScene(Transferable t){var s=session(t);return s!=null&&s.graph==null;}
    private Transferable transfer(Session s) {
        return new Transferable() {
            public DataFlavor[] getTransferDataFlavors(){return new DataFlavor[]{FLAVOR};}
            public boolean isDataFlavorSupported(DataFlavor f){return FLAVOR.equals(f);}
            public Object getTransferData(DataFlavor f)throws UnsupportedFlavorException {if(!FLAVOR.equals(f))throw new UnsupportedFlavorException(f);return s.token;}
        };
    }
    public Transferable sceneTransfer(Component source,long document,long element) {
        cancel();var r=latest.get();if(closed||r==null||r.snapshot()==null||r.snapshot().playing()||element==0||!source.isShowing())return null;
        active=new Session(r,source,document,element,null);status.accept("Reference drag · drop on Graph; Escape cancels");return transfer(active);
    }
    public void startScene(Component source,long document,long element,MouseEvent event) {
        var t=sceneTransfer(source,document,element);if(t!=null)start(source,event,t,active);
    }
    public void startGraph(Component source,GraphHit graph) {
        var transfer=graphTransfer(source,graph);if(transfer==null)return;
        Point p=MouseInfo.getPointerInfo().getLocation();SwingUtilities.convertPointFromScreen(p,source);
        var event=new MouseEvent(source,MouseEvent.MOUSE_DRAGGED,System.currentTimeMillis(),InputEvent.BUTTON1_DOWN_MASK|InputEvent.ALT_DOWN_MASK,p.x,p.y,1,false,MouseEvent.BUTTON1);
        start(source,event,transfer,active);
    }
    public Transferable graphTransfer(Component source,GraphHit graph) {
        cancel();var r=latest.get();if(closed||r==null||r.snapshot()==null||r.snapshot().playing()||!source.isShowing())return null;
        active=new Session(r,source,0,0,graph);
        return transfer(active);
    }
    private void start(Component source,MouseEvent event,Transferable transfer,Session s) {
        try {new Gesture(source,gesture->gesture.startDrag(DragSource.DefaultCopyNoDrop,transfer,new DragSourceAdapter(){
            @Override public void dragDropEnd(DragSourceDropEvent e){if(active==s&&!s.dropping)cancel();}
        })).start(event);}
        catch(RuntimeException e){cancel();status.accept("Reference drag unavailable: "+e.getMessage());}
    }
    private static final class Gesture extends DragGestureRecognizer {
        Gesture(Component c,DragGestureListener l){super(DragSource.getDefaultDragSource(),c,DnDConstants.ACTION_COPY,l);}
        protected void registerListeners(){}
        protected void unregisterListeners(){}
        void start(MouseEvent event){appendEvent(event);fireDragGestureRecognized(DnDConstants.ACTION_COPY,event.getPoint());}
    }
    public void sourceDone(Transferable t){var s=session(t);if(s!=null&&!s.dropping)cancel();}
    public void cancelFor(Component component){if(active!=null&&(active.source==component||SwingUtilities.isDescendingFrom(active.source,component)))cancel();}
    public void cancel(){var s=active;active=null;if(s!=null){for(var clear:s.clear)clear.run();status.accept("");}}
    private CompletableFuture<List<Bridge.ReferenceCandidate>> probe(Session s,long graph,long scene,long element,long port) {
        if(!live(s))return CompletableFuture.failedFuture(new CancellationException("Reference drag retired"));
        return commands.execute(s.generation,s.revision,40,"probe",id(graph),id(scene),id(element),id(port)).thenApplyAsync(r->{
            if(!live(s))throw new CancellationException("Reference drag retired");return r.snapshot().referenceCandidates();
        },SwingUtilities::invokeLater);
    }
    public boolean hoverScene(Transferable t,long document,long element,BiConsumer<Boolean,String> feedback,Runnable clear) {
        var s=session(t);if(s==null||s.graph==null)return false;
        if(element==0){s.probeKey="";s.candidates=List.of();clear.run();status.accept("No reference target");return false;}
        s.clear.add(clear);
        String key=document+":"+element;
        if(!key.equals(s.probeKey)) {
            s.probeKey=key;s.candidates=List.of();feedback.accept(false,"Checking reference target…");
            probe(s,s.graph.document(),document,element,s.graph.port()).whenComplete((choices,error)->SwingUtilities.invokeLater(()->{
                if(!live(s)||!key.equals(s.probeKey))return;
                if(error!=null){feedback.accept(false,error.getMessage());return;}
                s.candidates=choices;boolean valid=choices.stream().anyMatch(c->c.reason().isEmpty());
                String message=valid?"Reference target · release to link":choices.isEmpty()?"No reference target":choices.getFirst().reason();
                feedback.accept(valid,message);status.accept(message);
            }));
        }
        return s.candidates.stream().anyMatch(c->c.reason().isEmpty());
    }
    public void hoverGraph(Transferable t,GraphHit hit,BiConsumer<Boolean,String> feedback,Runnable clear) {
        var s=session(t);if(s==null||s.graph!=null)return;s.clear.add(clear);
        String key=hit.document()+":"+hit.port();if(key.equals(s.probeKey))return;
        s.probeKey=key;s.candidates=List.of();feedback.accept(false,"Checking reference target…");
        probe(s,hit.document(),s.scene,s.element,hit.port()).whenComplete((choices,error)->SwingUtilities.invokeLater(()->{
            if(!live(s)||!key.equals(s.probeKey))return;
            if(error!=null){feedback.accept(false,error.getMessage());return;}
            s.candidates=choices;boolean valid=choices.stream().anyMatch(c->c.reason().isEmpty());
            String message=valid?"Reference target · release to create link":choices.isEmpty()?"No reference target":choices.getFirst().reason();
            feedback.accept(valid,message);status.accept(message);
        }));
    }
    public void dropScene(Transferable t,long document,long element,Component target) {
        var s=session(t);if(s==null||s.graph==null)return;drop(s,s.graph,document,element,target);
    }
    public void dropGraph(Transferable t,GraphHit hit,Component target) {
        var s=session(t);if(s==null||s.graph!=null)return;drop(s,hit,s.scene,s.element,target);
    }
    public void dropGraph(Transferable t,CompletableFuture<GraphHit> hit,Component target) {
        var s=session(t);if(s==null||s.graph!=null||s.dropping)return;s.dropping=true;
        hit.whenComplete((point,error)->SwingUtilities.invokeLater(()->{
            if(!live(s))return;s.dropping=false;
            if(error!=null||!target.isShowing()){cancel();return;}drop(s,point,s.scene,s.element,target);
        }));
    }
    private void drop(Session s,GraphHit hit,long scene,long element,Component target) {
        if(s.dropping||!live(s))return;s.dropping=true;
        probe(s,hit.document(),scene,element,hit.port()).whenComplete((choices,error)->SwingUtilities.invokeLater(()->{
            if(!live(s)||!target.isShowing()){if(active==s)cancel();return;}
            if(error!=null){cancel();status.accept("Reference rejected: "+error.getMessage());return;}
            var valid=choices.stream().filter(c->c.reason().isEmpty()).toList();
            if(valid.isEmpty()){cancel();status.accept(choices.isEmpty()?"No reference target":choices.getFirst().reason());return;}
            int choice=0;
            if(valid.size()>1){Object picked=JOptionPane.showInputDialog(target,"Choose the exact reference; no Entity will be spawned.","Create Graph Reference",JOptionPane.QUESTION_MESSAGE,null,valid.stream().map(Bridge.ReferenceCandidate::label).toArray(),valid.getFirst().label());
                if(picked==null){cancel();return;}choice=valid.stream().map(Bridge.ReferenceCandidate::label).toList().indexOf(picked.toString());}
            if(choice<0||!live(s)||!target.isShowing()){cancel();return;}
            var args=new ArrayList<String>(List.of("drop",id(hit.document())));args.add(String.join(" ",valid.get(choice).value().arguments()));args.add(id(hit.port()));args.add(""+hit.x());args.add(""+hit.y());
            // Retire before publishing: duplicate/late Drop cannot reuse the nonce.
            cancel();commands.execute(s.generation,s.revision,40,args.toArray(String[]::new)).whenComplete((reply,problem)->SwingUtilities.invokeLater(()->{
                status.accept(problem==null?"Reference linked · one Undo": "Reference rejected: "+problem.getMessage());target.requestFocusInWindow();
            }));
        }));
    }
    private static String id(long n){return Long.toUnsignedString(n);}
    public int activeSessions(){return active==null?0:1;}
    @Override public void close(){closed=true;cancel();timer.stop();KeyboardFocusManager.getCurrentKeyboardFocusManager().removeKeyEventDispatcher(escape);}
}

package epidemic.u1;

import java.awt.Canvas;
import java.awt.Color;
import java.awt.Graphics;
import javax.swing.SwingUtilities;
import javax.swing.Timer;

/** U1 only: owns a renderer surface, never an authoring workspace. */
public final class NativeViewport extends Canvas {
    static {
        // A packaged launcher lives outside runtime/bin. Load JAWT explicitly so
        // Windows can resolve the viewport DLL without a machine JDK on PATH.
        if(System.getProperty("os.name").startsWith("Windows"))
            System.load(java.nio.file.Path.of(System.getProperty("java.home"),"bin","jawt.dll").toString());
        System.load(System.getProperty("epidemic.u1.native"));
    }
    private String sceneRoot="",appliedRoot="";
    private java.util.List<epidemic.editor.Bridge.Sprite> sprites=java.util.List.of(),appliedSprites=java.util.List.of();
    private long appliedSurface;
    private long document,selected,appliedDocument,appliedSelection,itemsSurface;
    private int tool,appliedTool=-1;
    private boolean grid=true;
    private java.util.List<String> labels=java.util.List.of(),appliedLabels=java.util.List.of();
    public void projectLabels(java.util.List<String> value){edt();labels=java.util.List.copyOf(value);}
    public void presentationRate(int fps){edt();presenter.setDelay(Math.max(1,1000/Math.max(1,Math.min(120,fps))));}
    private java.util.List<epidemic.editor.Bridge.SceneItem> items=java.util.List.of(),appliedItems=java.util.List.of();
    public void projectItems(long document,long selected,java.util.List<epidemic.editor.Bridge.SceneItem> items){
        edt();
        this.document=document;this.selected=selected;this.items=java.util.List.copyOf(items);
    }
    public void projectScene(String root,java.util.List<epidemic.editor.Bridge.Sprite> sprites){edt();sceneRoot=root;this.sprites=java.util.List.copyOf(sprites);}
    private static native String scene(long surface,String root,String[] keys,float[] geometry);
    private static native boolean sceneItems(long surface,long document,long selected,long[] ids,double[] bounds,String[] labels);
    private static native long sceneInput(long surface,int operation,double x,double y,double delta);
    private static native double[] scenePointer(long surface,double x,double y);
    public void tool(int value){edt();tool=value;}
    public void grid(boolean show){edt();grid=show;input(5,0,0,grid?1:0);}
    public double[] pointer(double x,double y){
        edt();
        if(surface==0||document==0||getWidth()==0||getHeight()==0)return new double[0];
        return scenePointer(surface,x/getWidth(),y/getHeight());
    }
    public long pick(double x,double y){return input(0,x,y,0);}
    private long referenceHover;
    public void referenceHover(long entity){edt();referenceHover=entity;appliedSelection=-1;}
    public void pan(double x,double y){input(1,x,y,0);}
    public void zoom(double x,double y,double steps){input(2,x,y,steps);}
    public void fit(){input(3,0,0,0);}
    private long input(int operation,double x,double y,double delta){
        edt();
        if(surface==0||document==0||getWidth()==0||getHeight()==0)return 0;
        return sceneInput(surface,operation,x/getWidth(),y/getHeight(),delta);
    }
    private long surface;
    private long root;
    private String result = "NOT RUN";
    private long submittedFrames;
    // D3D owns presentation. AWT paint notifications are not a frame clock and may
    // be skipped/coalesced during native tab reparenting or window exposure.
    private final Timer presenter = new Timer(33, event -> present());
    private static native String frame(long surface, Canvas canvas, boolean retainCapture);
    private static native String release(long surface);
    private static native String capture(long surface, String path);
    public static native long[] metrics();
    /** Existing surface/process counters without expensive system-wide thread enumeration. */
    public static native long[] profilerMetrics();
    private static native double[] frameStats(long surface);
    public double[] frameStatistics(){edt();return frameStats(surface); }
    public static native long windowHandle(java.awt.Component component);

    public NativeViewport() {
        setBackground(new Color(0x191920));
        setFocusable(true);
        setIgnoreRepaint(true);
        presenter.setCoalesce(true);
    }
    private static void edt() {
        if (!SwingUtilities.isEventDispatchThread())
            throw new IllegalStateException("Viewport operation outside EDT");
    }
    private void present() {
        try { draw(); }
        catch (RuntimeException error) {
            presenter.stop();
            result = "FAIL presentation stopped: " + error.getMessage();
            System.err.println(result);
        }
    }
    public void draw() {
        draw(false);
    }
    private void draw(boolean retainCapture) {
        edt();
        if (!isShowing() || getWidth() == 0 || getHeight() == 0) return;
        var window = SwingUtilities.getWindowAncestor(this);
        if (window instanceof java.awt.Frame owner &&
            (owner.getExtendedState() & java.awt.Frame.ICONIFIED) != 0) return;
        long previousSurface=surface,previousRoot=root;
        if(surface!=0)applyProjection();
        result = frame(surface, this, retainCapture);
        if (!result.startsWith("PASS ")) throw new IllegalStateException(result);
        String[] parts = result.split(" ");
        surface = Long.parseLong(parts[1]);
        root = Long.parseLong(parts[3]);
        if(previousSurface!=surface||previousRoot!=root){
            itemsSurface=0;appliedSurface=0;applyProjection();
            result=frame(surface,this,retainCapture);
            if(!result.startsWith("PASS "))throw new IllegalStateException(result);
            ++submittedFrames;
        }
        ++submittedFrames;
    }
    private void applyProjection(){
        if(document!=0&&(itemsSurface!=surface||appliedDocument!=document||appliedSelection!=selected||!appliedItems.equals(items)||!appliedLabels.equals(labels))){
            long[] ids=new long[items.size()];
            double[] bounds=new double[items.size()*7];
            for(int i=0;i<items.size();i++){
                var item=items.get(i);
                ids[i]=item.id();
                int at=i*7;
                bounds[at]=item.left();bounds[at+1]=item.top();bounds[at+2]=item.right();
                bounds[at+3]=item.bottom();bounds[at+4]=item.x();bounds[at+5]=item.y();bounds[at+6]=item.rotation();
            }
            if(!sceneItems(surface,document,referenceHover!=0?referenceHover:selected,ids,bounds,labels.size()==items.size()?labels.toArray(String[]::new):new String[0]))throw new IllegalStateException("Native Scene bounds rejected");
            appliedLabels=labels;itemsSurface=surface;appliedDocument=document;appliedSelection=selected;appliedItems=items;appliedTool=-1;
            sceneInput(surface,5,0,0,grid?1:0);
        }
        if(document!=0&&appliedTool!=tool){sceneInput(surface,4,0,0,tool);appliedTool=tool;}
        if(!sceneRoot.isEmpty()&&(appliedSurface!=surface||!appliedRoot.equals(sceneRoot)||!appliedSprites.equals(sprites))){
            String[] keys=new String[sprites.size()];float[] geometry=new float[sprites.size()*8];
            for(int i=0;i<sprites.size();i++){var row=sprites.get(i);keys[i]=row.key();float[] values={row.x(),row.y(),row.sx(),row.sy(),row.rotation(),row.ppu(),row.pivotX(),row.pivotY()};System.arraycopy(values,0,geometry,i*8,8);}
            String reply=scene(surface,sceneRoot,keys,geometry);if(!reply.startsWith("PASS "))throw new IllegalStateException(reply);
            appliedRoot=sceneRoot;appliedSprites=sprites;appliedSurface=surface;
        }
    }
    public long rootWindow() { return root; }
    public String result() { return result; }
    public long submittedFrames() { return submittedFrames; }
    public boolean isPresenterRunning() { return presenter.isRunning(); }
    public void captureTo(String path) {
        edt(); draw(true);
        String captured = capture(surface, path);
        if (!captured.startsWith("PASS ")) throw new IllegalStateException(captured);
    }
    public void verifyInvalidHandles() {
        edt();
        long count = metrics()[0];
        if (!frame(Long.MAX_VALUE, this, false).startsWith("FAIL Stale viewport handle") ||
            !release(Long.MAX_VALUE).startsWith("FAIL stale viewport release") ||
            metrics()[0] != count)
            throw new IllegalStateException("Invalid handle changed native surface state");
    }
    @Override public void paint(Graphics graphics) { draw(); }
    @Override public void update(Graphics graphics) { paint(graphics); }
    @Override public void addNotify() {
        edt();
        super.addNotify();
        presenter.start();
    }
    @Override public void removeNotify() {
        edt();
        presenter.stop();
        if (surface != 0) {
            String released = release(surface);
            if (!released.startsWith("PASS ")) throw new IllegalStateException(released);
            surface = 0;
            root = 0;
        }
        super.removeNotify();
    }
}

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
        result = frame(surface, this, retainCapture);
        if (!result.startsWith("PASS ")) throw new IllegalStateException(result);
        String[] parts = result.split(" ");
        surface = Long.parseLong(parts[1]);
        root = Long.parseLong(parts[3]);
        ++submittedFrames;
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

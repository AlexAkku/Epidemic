package epidemic.launcher;

/** Registered only by our EXE after creating the real statically-linked sandbox. */
public final class NativeLauncher {
    private NativeLauncher() {}
    public static native boolean sandboxCreated();
    public static native String security();
    public static boolean available() {
        try { return sandboxCreated(); }
        catch (UnsatisfiedLinkError unavailable) { return false; }
    }
}

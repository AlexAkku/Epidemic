package epidemic.editor;

import java.io.*;
import java.nio.file.*;
import java.util.*;

/** UI preferences only, never a game-folder file. Call on the app IO dispatcher. */
public final class RecentProjects {
    public record Entry(String path, String title, long opened, String version) { }
    private final Path file;
    public RecentProjects(Path profile) { file = profile.resolve("recent-projects.bin"); }
    public List<Entry> load() throws IOException {
        if (!Files.exists(file)) return List.of();
        if (Files.size(file) > 1024 * 1024) throw new IOException("Recent list too large");
        try (DataInputStream in = new DataInputStream(Files.newInputStream(file))) {
            if (in.readInt() != 0x45505231) throw new IOException("Unknown recent-project schema");
            int count = in.readInt(); if (count < 0 || count > 10) throw new IOException("Invalid recent count");
            List<Entry> entries = new ArrayList<>();
            for (int i = 0; i < count; ++i) entries.add(new Entry(in.readUTF(), in.readUTF(), in.readLong(), in.readUTF()));
            if (in.read() != -1) throw new IOException("Corrupt recent list"); return List.copyOf(entries);
        }
    }
    private void save(List<Entry> entries) throws IOException {
        Files.createDirectories(file.getParent()); Path staging = Files.createTempFile(file.getParent(), "recent-", ".tmp");
        try {
            try (DataOutputStream out = new DataOutputStream(Files.newOutputStream(staging))) {
                out.writeInt(0x45505231); out.writeInt(entries.size());
                for (Entry entry : entries) { out.writeUTF(entry.path()); out.writeUTF(entry.title()); out.writeLong(entry.opened()); out.writeUTF(entry.version()); }
            }
            try { Files.move(staging, file, StandardCopyOption.REPLACE_EXISTING, StandardCopyOption.ATOMIC_MOVE); }
            catch (AtomicMoveNotSupportedException unsupported) { Files.move(staging, file, StandardCopyOption.REPLACE_EXISTING); }
        } finally { Files.deleteIfExists(staging); }
    }
    public void opened(Bridge.Snapshot snapshot) throws IOException {
        Path canonical = Path.of(snapshot.path()).toRealPath(); List<Entry> entries = new ArrayList<>();
        entries.add(new Entry(canonical.toString(), snapshot.title(), System.currentTimeMillis(), "1.0.0"));
        for (Entry old : load()) {
            boolean same;
            try { same = Files.isSameFile(canonical, Path.of(old.path())); }
            catch (IOException absent) { same = canonical.toString().equalsIgnoreCase(old.path()); }
            if (!same && entries.size() < 10) entries.add(old);
        }
        save(entries);
    }
    public void remove(Entry entry) throws IOException { save(load().stream().filter(old -> !old.path().equals(entry.path())).toList()); }
}

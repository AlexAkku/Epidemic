package epidemic.editor;

import java.io.*;
import java.nio.charset.StandardCharsets;
import java.util.*;

public final class Catalog {
    public record Panel(String id, String label, String anchor, int order, String context, boolean initial) { }
    public record Schema(String id, String label, String phase, List<String> fields) {
        public Schema { fields = List.copyOf(fields); }
    }
    private final Map<String,Panel> panels = new LinkedHashMap<>();
    private final List<Schema> schemas = new ArrayList<>();
    public Catalog() {
        for (String[] row : rows("panels.tsv")) {
            if (row.length != 6) throw new IllegalStateException("Invalid panel registry row");
            Panel panel = new Panel(row[0], row[1], row[2], Integer.parseInt(row[3]), row[4], Boolean.parseBoolean(row[5]));
            if (panels.putIfAbsent(panel.id(), panel) != null) throw new IllegalStateException("Duplicate panel ID");
        }
        for (String[] row : rows("fields.tsv")) {
            if (row.length != 4) throw new IllegalStateException("Invalid UI field schema");
            schemas.add(new Schema(row[0], row[1], row[2], Arrays.stream(row[3].split(",")).map(String::trim).toList()));
        }
    }
    private static List<String[]> rows(String resource) {
        try (InputStream stream = Catalog.class.getResourceAsStream(resource)) {
            if (stream == null) throw new IOException("Missing " + resource);
            return new String(stream.readAllBytes(), StandardCharsets.UTF_8).lines()
                .filter(line -> !line.isBlank()).map(line -> line.split("\t", -1)).toList();
        } catch (IOException error) { throw new IllegalStateException(error); }
    }
    public Collection<Panel> panels() { return List.copyOf(panels.values()); }
    public List<Schema> schemas() { return List.copyOf(schemas); }
    public Panel panel(String id) {
        Panel found = panels.get(id);
        if (found != null) return found;
        int split = id.indexOf(':');
        if (split > 0 && panels.containsKey(id.substring(0, split)) &&
            panels.get(id.substring(0, split)).anchor().equals("dock.center.documents")) {
            try { if (Long.parseUnsignedLong(id.substring(split + 1)) == 0) throw new NumberFormatException(); }
            catch (NumberFormatException invalid) { throw new IllegalArgumentException("Invalid document ID"); }
            Panel base = panels.get(id.substring(0, split));
            return new Panel(id, base.label() + " · " + id.substring(split + 1), base.anchor(), 20, "all", false);
        }
        throw new IllegalArgumentException("Unknown panel " + id);
    }
    public boolean contains(String id) {
        try { panel(id); return true; } catch (IllegalArgumentException missing) { return false; }
    }
    public static String context(String id) {
        if (id.startsWith("graph.editor")) return "Graph";
        if (id.startsWith("game.view")) return "Game";
        if (id.startsWith("code.space")) return "Code";
        if (id.startsWith("ui.")) return "UI";
        if (id.startsWith("animation.")) return "Animation";
        return "Scene";
    }
}

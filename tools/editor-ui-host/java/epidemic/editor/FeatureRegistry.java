package epidemic.editor;

import java.util.*;
import javax.swing.*;

/** One gate for menu, toolbar, command palette and programmatic dispatch. */
public final class FeatureRegistry {
    public record Feature(String id, String label, String phase, String reason, boolean wired) { }
    private final Map<String,Feature> entries = new LinkedHashMap<>();
    public Feature add(String id, String label, String phase, String reason, boolean wired) {
        Feature feature = new Feature(id, label, phase, reason, wired);
        if (entries.putIfAbsent(id, feature) != null) throw new IllegalStateException("Duplicate feature " + id);
        return feature;
    }
    public Feature planned(String id, String label, String phase) {
        return entries.computeIfAbsent(id, key -> new Feature(key, label, phase,
            "Backend planned for " + phase + "; no project mutation is available", false));
    }
    public Collection<Feature> all() { return List.copyOf(entries.values()); }
    public void dispatch(Feature feature, boolean ready, Runnable action) {
        if (!feature.wired() || !ready) throw new IllegalStateException(feature.id() + ": " + feature.reason());
        action.run();
    }
    public JButton plannedButton(String id, String label, String phase) {
        Feature feature = planned(id, label, phase);
        JButton button = Theme.button(label, () -> { }); button.setEnabled(false);
        button.setToolTipText(id + " — " + feature.reason());
        button.getAccessibleContext().setAccessibleDescription(button.getToolTipText());
        return button;
    }
}

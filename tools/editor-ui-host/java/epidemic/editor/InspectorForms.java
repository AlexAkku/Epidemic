package epidemic.editor;

import java.awt.*;
import java.util.*;
import javax.swing.*;
import javax.swing.border.EmptyBorder;

/** UI descriptors only; no registration of imaginary runtime types or archive fields. */
public final class InspectorForms {
    private InspectorForms() { }
    public static JComponent library(Catalog catalog, FeatureRegistry features) {
        return library(catalog, features, "");
    }
    public static JComponent fixed(Catalog catalog, FeatureRegistry features, String identity) {
        Catalog.Schema schema=catalog.schemas().stream().filter(value->value.id().equals(identity)).findFirst().orElseThrow();
        return form(schema,features);
    }
    public static JComponent library(Catalog catalog, FeatureRegistry features, String initial) {
        JPanel panel = Theme.island(new BorderLayout(Theme.GAP, Theme.GAP));
        JComboBox<String> types = new JComboBox<>(catalog.schemas().stream().map(Catalog.Schema::label).toArray(String[]::new));
        types.setLightWeightPopupEnabled(false);
        JTextField search = new JTextField(); search.setToolTipText("Find UI schema / component / tool");
        search.getAccessibleContext().setAccessibleName("Find UI fields");
        JPanel header = new JPanel(new BorderLayout(Theme.GAP, Theme.GAP));
        header.add(search, BorderLayout.NORTH); header.add(types, BorderLayout.SOUTH);
        panel.add(header, BorderLayout.NORTH);
        JPanel body = new JPanel(new BorderLayout()); panel.add(body, BorderLayout.CENTER);
        Map<String,JComponent> cache = new HashMap<>();
        Runnable show = () -> {
            int index = types.getSelectedIndex(); if (index < 0) return;
            Catalog.Schema schema = catalog.schemas().get(index);
            body.removeAll();
            body.add(cache.computeIfAbsent(schema.id(), key -> form(schema, features)));
            body.revalidate(); body.repaint();
        };
        types.addActionListener(event -> show.run());
        search.addActionListener(event -> {
            String needle = search.getText().toLowerCase(Locale.ROOT);
            for (int i = 0; i < catalog.schemas().size(); ++i)
                if ((catalog.schemas().get(i).label() + catalog.schemas().get(i).fields()).toLowerCase(Locale.ROOT).contains(needle)) {
                    types.setSelectedIndex(i); break;
                }
        });
        for (int i = 0; i < catalog.schemas().size(); ++i) if (catalog.schemas().get(i).id().equals(initial)) types.setSelectedIndex(i);
        panel.putClientProperty("schema.selector", types);
        show.run(); return panel;
    }
    public static JComponent form(Catalog.Schema schema, FeatureRegistry features) {
        JPanel rows = new JPanel(); rows.setLayout(new BoxLayout(rows, BoxLayout.Y_AXIS));
        rows.setBorder(new EmptyBorder(8, 8, 8, 8));
        JLabel title = Theme.label(schema.label(), false); title.setFont(title.getFont().deriveFont(Font.BOLD, 16));
        title.setAlignmentX(Component.LEFT_ALIGNMENT); rows.add(title);
        rows.add(Theme.label("Planned fields · " + schema.phase() + " · no runtime binding", true));
        rows.add(Box.createVerticalStrut(12));
        int index = 0;
        for (String name : schema.fields()) {
            String id = schema.id() + ".field." + index++;
            FeatureRegistry.Feature feature = features.planned(id, name, schema.phase());
            JPanel row = new JPanel(new BorderLayout(8, 4));
            row.setBorder(new EmptyBorder(4, 0, 8, 0));
            row.add(Theme.label(name, false), BorderLayout.NORTH);
            JComponent input = widget(name);
            String reason = id + " — " + feature.reason();
            input.setToolTipText(reason); row.setToolTipText(reason);
            input.getAccessibleContext().setAccessibleName(name);
            input.getAccessibleContext().setAccessibleDescription(reason);
            row.add(input, BorderLayout.CENTER);
            row.setMaximumSize(new Dimension(Integer.MAX_VALUE, 82));
            row.setAlignmentX(Component.LEFT_ALIGNMENT); rows.add(row);
        }
        rows.add(Box.createVerticalGlue());
        JScrollPane scroll = new JScrollPane(rows); scroll.setBorder(null); return scroll;
    }
    public static JComponent widget(String name) {
        String text = name.toLowerCase(Locale.ROOT);
        if (text.matches(".*(enabled|visible|loop|autoplay|mute|solo|trigger|shadows|bool|checkbox).*")) {
            JCheckBox box = new JCheckBox("Not connected"); box.setEnabled(false); return box;
        }
        if (text.matches(".*(reference| ref|asset|graph|script|binding|enum|type|projection|body type).*")) {
            JComboBox<String> reference = new JComboBox<>(new String[]{"Backend not connected"});
            reference.setLightWeightPopupEnabled(false); reference.setEnabled(false); return reference;
        }
        if (text.matches(".*(vector|position|scale x|pivot|velocity|euler|quaternion).*")) {
            JPanel vector = new JPanel(new GridLayout(1, 3, 4, 0));
            for (String axis : new String[]{"X —", "Y —", "Z —"}) {
                JTextField component = new JTextField(axis); component.setEnabled(false); vector.add(component);
            }
            return vector;
        }
        if (text.matches(".*(integer|float|double|speed|mass|volume|pitch|opacity|health|damage|fov|radius|duration|fps).*")) {
            JSpinner number = new JSpinner(new SpinnerNumberModel(0.0, null, null, 0.1));
            number.setEnabled(false);
            if (number.getEditor() instanceof JSpinner.DefaultEditor editor) editor.getTextField().setText("—");
            return number;
        }
        JTextField field = new JTextField(text.contains("color") || text.contains("tint") ? "RGBA —" : "Not connected");
        field.setEnabled(false); return field;
    }
    public static JComponent gallery(Catalog catalog, FeatureRegistry features) {
        JPanel panel = new JPanel(); panel.setLayout(new BoxLayout(panel, BoxLayout.Y_AXIS));
        panel.add(Theme.label("UI Gallery — presentation samples, no game state", false));
        panel.add(new JTextField("Editable text / Кириллица"));
        panel.add(new JCheckBox("Bool sample")); panel.add(new JSpinner(new SpinnerNumberModel(1, 0, 10, 1)));
        panel.add(new JComboBox<>(new String[]{"Enum sample", "Second value"}));
        panel.add(features.plannedButton("gallery.planned", "Planned control", "F4"));
        panel.add(Theme.button("Focused / pressed / hover sample", () -> { }));
        for (String type : new String[]{"Bool", "Integer", "Float", "String", "Enum", "Reference", "Vector3", "Color RGBA", "Array/List", "Optional"}) {
            panel.add(Theme.label(type, true)); panel.add(widget(type));
        }
        panel.add(Theme.label("Loading…", true));
        JLabel error = Theme.label("Failed sample: backend unavailable", false); error.setForeground(Theme.ERROR); panel.add(error);
        JSplitPane gallery=new JSplitPane(JSplitPane.HORIZONTAL_SPLIT,new JScrollPane(panel),library(catalog,features));
        gallery.setResizeWeight(.4);return gallery;
    }
}

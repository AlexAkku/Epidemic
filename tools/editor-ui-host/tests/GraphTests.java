package epidemic.editor;

import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;

/** Production Java Bridge/private IPC/native owner integration, including a second process. */
public final class GraphTests {
    private static Bridge.Reply call(Bridge bridge, int operation, String... args) throws Exception {
        return bridge.call(operation, args).get(20, TimeUnit.SECONDS);
    }
    private static void require(boolean condition, String detail) {
        if (!condition) throw new IllegalStateException(detail);
    }
    private static String id(long value) { return Long.toUnsignedString(value); }
    private static Bridge.Graph graph(Bridge bridge, long document) {
        return bridge.latest().snapshot().graphs().stream().filter(g -> g.document() == document).findFirst().orElseThrow();
    }
    private static void reject(Bridge bridge, String action, String... args) throws Exception {
        var before = bridge.latest().snapshot();
        try { call(bridge, 38, args); throw new IllegalStateException("Accepted " + action); }
        catch (ExecutionException expected) {
            require(expected.getCause().getMessage().contains(action), "Wrong native rejection: " + expected.getCause());
            require(bridge.latest().snapshot().revision() == before.revision(), "Rejected edit changed revision");
            require(bridge.latest().snapshot().graphs().equals(before.graphs()), "Rejected edit changed graph");
        }
    }
    public static void main(String[] args) throws Exception {
        Path executable = Path.of(args[0]).toAbsolutePath();
        Path work = Files.createTempDirectory(Path.of(args[1]), "graph-native-");
        Path project = work.resolve("проект Graph");
        long document, generation;
        Bridge.Graph saved;
        try (var bridge = new Bridge(executable, work, state -> {}, Runnable::run)) {
            bridge.connect().get(20, TimeUnit.SECONDS);
            call(bridge, 3, project.toString(), "Graph roundtrip");
            call(bridge, 15, "2", "Граф authoring");
            document = bridge.latest().snapshot().selection().document();
            generation = bridge.latest().generation();
            require(graph(bridge, document).nodes().isEmpty(), "New graph isn't empty");
            for (int kind = 0; kind < 3; ++kind)
                call(bridge, 38, "create", id(document), Integer.toString(kind), Integer.toString(50 + kind * 280), "80");
            var graph = graph(bridge, document);
            require(graph.nodes().size() == 3 && graph.edges().isEmpty(), "Native node creation failed");
            var source = graph.nodes().get(0);
            var target = graph.nodes().get(1);
            var output = source.ports().stream().filter(p -> p.kind() == 0 && p.direction() == 1).findFirst().orElseThrow();
            var input = target.ports().stream().filter(p -> p.kind() == 0 && p.direction() == 0).findFirst().orElseThrow();
            var control = source.ports().stream().filter(p -> p.kind() == 1 && p.direction() == 1).findFirst().orElseThrow();
            call(bridge, 38, "connect", id(document), id(output.id()), id(input.id()));
            require(graph(bridge, document).edges().size() == 1, "Compatible connection missing");
            reject(bridge, "graph.connection_kind", "connect", id(document), id(control.id()), id(input.id()));
            reject(bridge, "graph.missing_port", "connect", id(document), "1", "2");
            reject(bridge, "editor.ui.invalid_value", "move", id(document), id(source.id()), "NaN", "0");
            reject(bridge, "editor.command_target", "move", id(document), id(source.id()), "1000001", "0");
            long stale = bridge.latest().snapshot().revision();
            call(bridge, 7); require(graph(bridge, document).edges().isEmpty(), "Undo edge failed");
            call(bridge, 8); require(graph(bridge, document).edges().size() == 1, "Redo edge failed");
            try {
                bridge.callAt(generation, stale, 38, "delete", id(document), id(source.id())).get(20, TimeUnit.SECONDS);
                throw new IllegalStateException("Accepted stale revision");
            } catch (ExecutionException expected) { require(expected.getCause().getMessage().contains("stale_revision"), "Wrong stale rejection"); }
            call(bridge, 38, "move", id(document), id(source.id()), "123.5", "-45.25");
            require(graph(bridge, document).nodes().getFirst().x() == 123.5, "Move wasn't projected");
            call(bridge, 7); require(graph(bridge, document).nodes().getFirst().x() == 50, "Move Undo failed");
            call(bridge, 8); require(graph(bridge, document).nodes().getFirst().y() == -45.25, "Move Redo failed");
            saved = graph(bridge, document);
            call(bridge, 6); require(!bridge.latest().snapshot().dirty(), "Save stayed dirty");
            call(bridge, 10); bridge.shutdown().get(20, TimeUnit.SECONDS);
        }
        try (var reopened = new Bridge(executable, work, state -> {}, Runnable::run)) {
            reopened.connect().get(20, TimeUnit.SECONDS);
            require(reopened.latest().generation() != generation, "Companion wasn't restarted");
            call(reopened, 2, project.resolve("Project.epidemicproj").toString());
            require(graph(reopened, document).equals(saved), "Graph/layout/IDs changed across Save/Open in another process");
            call(reopened, 38, "delete", id(document), id(saved.nodes().getFirst().id()));
            require(graph(reopened, document).edges().isEmpty(), "Node deletion left edges");
            call(reopened, 7); require(graph(reopened, document).equals(saved), "Delete Undo lost layout or edges");
            call(reopened, 6); call(reopened, 10); reopened.shutdown().get(20, TimeUnit.SECONDS);
        }
        System.out.println("PASS Graph: 3 native nodes, typed connection/rejection, stale guard, move/node/edge Undo/Redo, IDs/layout Save/Open in second companion");
    }
}

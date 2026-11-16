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
            var after=bridge.latest().snapshot().graphs();require(after.size()==before.graphs().size(),"Rejected edit changed document count");
            for(int i=0;i<after.size();++i){var a=after.get(i);var b=before.graphs().get(i);
                require(a.document()==b.document()&&a.nodes().equals(b.nodes())&&a.edges().equals(b.edges())&&a.catalogue().equals(b.catalogue()),"Rejected edit changed graph state");
            } // Diagnostics are transient results; they are allowed to report the rejection.
        }
    }
    private static Bridge.GraphNode create(Bridge b,long doc,String name) throws Exception {
        var before=graph(b,doc);int index=-1;for(int i=0;i<before.catalogue().size();++i)if(before.catalogue().get(i).name().equals(name))index=i;
        require(index>=0,"Missing native operation "+name);call(b,38,"create",id(doc),""+index,""+(before.nodes().size()%8*260),""+(before.nodes().size()/8*220));
        return graph(b,doc).nodes().getLast();
    }
    private static String port(Bridge.GraphNode node,String name){return id(node.ports().stream().filter(p->p.name().equals(name)).findFirst().orElseThrow().id());}
    private static void link(Bridge b,long doc,Bridge.GraphNode from,String out,Bridge.GraphNode to,String in) throws Exception {call(b,38,"connect",id(doc),port(from,out),port(to,in));}
    private static void settings(Bridge b,long doc,Bridge.GraphNode n,String value) throws Exception {call(b,38,"settings",id(doc),id(n.id()),n.name(),value,"Комментарий <img src=x onerror=alert(1)>","Group α","3");}
    private static long document(Bridge b,String name) throws Exception {call(b,15,"2",name);return b.latest().snapshot().selection().document();}
    private static String nodeIds(List<Bridge.GraphNode> nodes){return String.join(" ",nodes.stream().map(n->id(n.id())).toList());}
    private static void extended(Path executable,Path work) throws Exception {
        Path root=Files.createTempDirectory(work,"fii1-").resolve("Project");
        List<Bridge.GraphNode> savedNodes;List<Bridge.GraphEdge> savedEdges;long fragment;
        try(var b=new Bridge(executable,work,state->{},Runnable::run)){
            b.connect().get(20,TimeUnit.SECONDS);call(b,3,root.toString(),"F-II.1 native authoring");
            fragment=document(b,"Фрагмент <script>never executed</script>");
            require(graph(b,fragment).catalogue().stream().filter(t->t.operation()<=10).count()==11&&graph(b,fragment).catalogue().stream().anyMatch(t->t.name().equals("Is Entity Valid")),"Catalogue not derived from full native schema");
            var input=create(b,fragment,"Input");var add=create(b,fragment,"Add");settings(b,fragment,input,"7");
            link(b,fragment,input,"value",add,"left");
            call(b,38,"literal",id(fragment),port(add,"right"),"6");
            require(graph(b,fragment).nodes().stream().filter(n->n.id()==add.id()).findFirst().orElseThrow().ports().stream().filter(p->p.name().equals("right")).findFirst().orElseThrow().defaultValue()==6,"Native inline literal lost");
            call(b,7);require(graph(b,fragment).nodes().stream().filter(n->n.id()==add.id()).findFirst().orElseThrow().ports().stream().filter(p->p.name().equals("right")).findFirst().orElseThrow().defaultValue()==0,"Literal Undo failed");call(b,8);

            reject(b,"graph.duplicate_connection","connect",id(fragment),port(input,"value"),port(add,"left"));
            reject(b,"graph.connection_direction","connect",id(fragment),port(input,"value"),port(add,"result"));
            reject(b,"ui.graph_template","create",id(fragment),""+graph(b,fragment).catalogue().size(),"0","0");
            var originalEdge=graph(b,fragment).edges().getFirst();
            long reconnectRevision=b.latest().snapshot().revision();
            reject(b,"graph.connection_direction","reconnect",id(fragment),id(originalEdge.id()),port(input,"value"),port(add,"result"));
            call(b,38,"reconnect",id(fragment),id(originalEdge.id()),port(input,"value"),port(add,"right"));
            require(b.latest().snapshot().revision()==reconnectRevision+1&&graph(b,fragment).edges().getFirst().input()==Long.parseUnsignedLong(port(add,"right")),"Reconnect wasn't atomic");
            call(b,7);require(graph(b,fragment).edges().equals(List.of(originalEdge)),"Reconnect Undo lost original edge ID/endpoints");
            long rev=b.latest().snapshot().revision();call(b,38,"copy",id(fragment),nodeIds(graph(b,fragment).nodes()));
            require(b.latest().snapshot().revision()==rev,"Copy mutated the graph");
            call(b,38,"paste",id(fragment),"600","200");
            var pasted=graph(b,fragment);require(pasted.nodes().size()==4&&pasted.edges().size()==2,"Paste lost internal topology");
            require(pasted.nodes().stream().flatMap(n->n.ports().stream()).map(Bridge.GraphPort::id).distinct().count()==8,"Paste reused native port IDs");
            require(pasted.nodes().get(2).previewDefault()==7&&pasted.nodes().get(2).markers()==3,"Paste lost settings");
            call(b,7);require(graph(b,fragment).nodes().size()==2,"Paste wasn't one Undo");call(b,8);
            savedNodes=graph(b,fragment).nodes();savedEdges=graph(b,fragment).edges();
            long bulk=document(b,"Bulk");create(b,bulk,"Input");
            while(graph(b,bulk).nodes().size()<64){call(b,38,"copy",id(bulk),nodeIds(graph(b,bulk).nodes()));call(b,38,"paste",id(bulk),"200","200");}
            var before=graph(b,bulk).nodes();StringBuilder moves=new StringBuilder();
            for(var n:before.subList(0,50))moves.append(id(n.id())).append(' ').append(n.x()+30).append(' ').append(n.y()+40).append('\n');
            rev=b.latest().snapshot().revision();call(b,38,"moves",id(bulk),moves.toString());
            require(b.latest().snapshot().revision()==rev+1,"50-node drag wasn't one native transaction");call(b,7);
            require(graph(b,bulk).nodes().equals(before),"Single Undo did not restore 50 positions");call(b,8);
            reject(b,"editor.command_target","moves",id(bulk),id(before.getFirst().id())+" 1 2\n1 3 4");
            call(b,38,"deletebulk",id(bulk),nodeIds(graph(b,bulk).nodes().subList(0,50)));require(graph(b,bulk).nodes().size()==14,"Bulk delete failed");
            call(b,7);require(graph(b,bulk).nodes().size()==64,"Bulk delete wasn't one Undo");
            long math=document(b,"Native VM math");Bridge.GraphNode entry=create(b,math,"Entry"),out=create(b,math,"Output"),a=create(b,math,"Input"),c=create(b,math,"Input"),sum=create(b,math,"Add");
            settings(b,math,a,"7");settings(b,math,c,"6");link(b,math,a,"value",sum,"left");link(b,math,c,"value",sum,"right");link(b,math,sum,"result",out,"value");link(b,math,entry,"next",out,"in");
            call(b,38,"compile",id(math));call(b,38,"run",id(math));require(graph(b,math).report().contains("Completed")&&graph(b,math).report().contains("node="+id(out.id())+" value=13"),"Native VM arithmetic mismatch: "+graph(b,math).report());
            settings(b,math,a,Long.toString(Long.MAX_VALUE));reject(b,"graph.vm.integer_overflow","run",id(math));
            require(graph(b,math).report().contains("node="+id(sum.id())),"VM diagnostic lost Node ID");call(b,7);call(b,38,"run",id(math));
            long flow=document(b,"Entry Branch Repeat Wait");
            entry=create(b,flow,"Entry");Bridge.GraphNode branch=create(b,flow,"Branch"),repeat=create(b,flow,"Repeat"),wait=create(b,flow,"Wait");out=create(b,flow,"Output");
            Bridge.GraphNode condition=create(b,flow,"Input"),count=create(b,flow,"Input"),value=create(b,flow,"Input"),ticks=create(b,flow,"Input");
            settings(b,flow,condition,"1");settings(b,flow,count,"2");settings(b,flow,value,"7");settings(b,flow,ticks,"3");
            link(b,flow,entry,"next",branch,"in");link(b,flow,condition,"value",branch,"condition");link(b,flow,branch,"true",repeat,"in");link(b,flow,count,"value",repeat,"count");
            link(b,flow,repeat,"body",wait,"in");link(b,flow,value,"value",wait,"value");link(b,flow,ticks,"value",wait,"ticks");link(b,flow,repeat,"done",out,"in");link(b,flow,wait,"result",out,"value");
            call(b,38,"compile",id(flow));call(b,38,"run",id(flow));require(graph(b,flow).report().contains("Completed")&&graph(b,flow).report().contains("game_ticks=6")&&graph(b,flow).report().contains("node="+id(out.id())+" value=7"),"Scheduler/Wait/Repeat mismatch: "+graph(b,flow).report());
            // Real bounded native workloads, not a synthetic second graph model.
            for(int target:new int[]{100,500,1024}){
                long start=System.nanoTime();while(graph(b,bulk).nodes().size()<target){int size=Math.min(64,target-graph(b,bulk).nodes().size());call(b,38,"copy",id(bulk),nodeIds(graph(b,bulk).nodes().subList(0,size)));call(b,38,"paste",id(bulk),"100","100");}
                System.out.println("PASS native Graph workload "+target+" nodes, "+(System.nanoTime()-start)/1_000_000+" ms");
            }
            reject(b,"graph.limit_exceeded","create",id(bulk),"3","0","0");
            call(b,6);call(b,10);b.shutdown().get(20,TimeUnit.SECONDS);
        }
        try(var b=new Bridge(executable,work,state->{},Runnable::run)){
            b.connect().get(20,TimeUnit.SECONDS);call(b,2,root.resolve("Project.epidemicproj").toString());
            require(graph(b,fragment).nodes().equals(savedNodes)&&graph(b,fragment).edges().equals(savedEdges),"Settings/fragment topology changed across another process");
            call(b,10);b.shutdown().get(20,TimeUnit.SECONDS);
        }
        System.out.println("PASS F-II.1: catalogue, atomic fragments/50-node moves, settings/history/persistence, typed failures, actual native arithmetic and Entry/Branch/Repeat/Wait scheduler");
    }
    public static void main(String[] args) throws Exception {
        Path executable = Path.of(args[0]).toAbsolutePath();
        Files.createDirectories(Path.of(args[1]));
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
        extended(executable,work);
        System.out.println("PASS Graph: 3 native nodes, typed connection/rejection, stale guard, move/node/edge Undo/Redo, IDs/layout Save/Open in second companion");
    }
}

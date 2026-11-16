package epidemic.editor;

import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;

/** The native owner must keep ticking even when this Java client makes no requests. */
public final class RuntimeTests {
    private static Bridge.Reply call(Bridge b,int op,String... args)throws Exception{return b.call(op,args).get(20,TimeUnit.SECONDS);}
    private static String id(long value){return Long.toUnsignedString(value);}
    private static void check(boolean condition,String text){if(!condition)throw new IllegalStateException(text);}
    private static Bridge.Graph graph(Bridge b,long document){return b.latest().snapshot().graphs().stream().filter(g->g.document()==document).findFirst().orElseThrow();}
    private static Bridge.GraphNode node(Bridge b,long doc,String type)throws Exception{var g=graph(b,doc);int index=-1;for(int i=0;i<g.catalogue().size();i++)if(g.catalogue().get(i).name().equals(type))index=i;check(index>=0,"Missing recipe "+type);call(b,38,"create",id(doc),""+index,"200","200");return graph(b,doc).nodes().getLast();}
    private static String port(Bridge.GraphNode node,String name){return id(node.ports().stream().filter(p->p.name().equals(name)).findFirst().orElseThrow().id());}
    private static void link(Bridge b,long doc,Bridge.GraphNode a,String out,Bridge.GraphNode c,String in)throws Exception{call(b,38,"connect",id(doc),port(a,out),port(c,in));}
    private static long health(Bridge.RuntimeSession state){return Long.parseLong(state.entities().stream().flatMap(e->e.fields().stream()).filter(f->f.name().equals("points")).findFirst().orElseThrow().value());}
    private static void runtime(Bridge b,String action,String... rest)throws Exception{var args=new ArrayList<String>();args.add(action);args.add(id(b.latest().runtime().session()));args.addAll(List.of(rest));call(b,41,args.toArray(String[]::new));}
    public static void main(String[] args)throws Exception{
        Path work=Path.of(args[1]).toAbsolutePath().resolve("runtime-"+UUID.randomUUID());Files.createDirectories(work);String saved;long graphId;
        try(var b=new Bridge(Path.of(args[0]).toAbsolutePath(),work.resolve("logs"),state->{},Runnable::run)){
            b.connect().get(20,TimeUnit.SECONDS);call(b,3,work.resolve("project").toString(),"F-II.4 native Runtime");long scene=b.latest().snapshot().entry();
            call(b,15,"1","Runtime target","registered-components");long prefab=b.latest().snapshot().selection().document();long root=b.latest().snapshot().nodes().stream().filter(n->n.document()==prefab&&n.kind()==2).findFirst().orElseThrow().id();
            call(b,15,"2","Damage Wait Damage");graphId=b.latest().snapshot().selection().document();
            var event=node(b,graphId,"Event");var amount=node(b,graphId,"Input");var duration=node(b,graphId,"Input");var before=node(b,graphId,"Native Call");var wait=node(b,graphId,"Wait");var after=node(b,graphId,"Native Call");
            call(b,38,"inputvalue",id(graphId),id(amount.id()),"7");call(b,38,"inputvalue",id(graphId),id(duration.id()),"30");
            link(b,graphId,event,"next",before,"in");link(b,graphId,amount,"value",before,"argument");link(b,graphId,before,"next",wait,"in");link(b,graphId,before,"result",wait,"value");link(b,graphId,duration,"value",wait,"ticks");link(b,graphId,wait,"next",after,"in");link(b,graphId,wait,"result",after,"argument");
            call(b,9,"2",id(prefab),id(root));call(b,30,id(b.latest().snapshot().selectionRevision()),"graph",id(graphId));call(b,33,"place",id(scene),id(prefab),"0","0");call(b,6);saved=b.latest().snapshot().path();long revision=b.latest().snapshot().revision();
            call(b,13);long session=b.latest().runtime().session();
            for(int i=0;i<100;i++){call(b,42);if(!b.latest().runtime().entities().isEmpty()&&health(b.latest().runtime())==93)break;Thread.sleep(5);}
            check(health(b.latest().runtime())==93,"First damage did not execute through native fixed pump");runtime(b,"pause");long tick=b.latest().runtime().tick();Thread.sleep(100);call(b,42);check(b.latest().runtime().tick()==tick,"Java polling/no polling changed paused Runtime time");
            runtime(b,"step-node","0");check(b.latest().runtime().tick()==tick,"Node step advanced fixed clock");
            for(int i=0;i<40&&health(b.latest().runtime())!=86;i++){long old=b.latest().runtime().tick();runtime(b,"step");check(b.latest().runtime().tick()==old+1,"Step Tick did not advance exactly one native tick");}
            check(health(b.latest().runtime())==86,"Native Wait did not resume second damage");check(b.latest().snapshot().revision()==revision&&!b.latest().snapshot().dirty(),"Runtime wrote authoring");
            runtime(b,"resume");long old=b.latest().runtime().tick();Thread.sleep(100);call(b,42);check(b.latest().runtime().tick()>old,"Native clock depends on Java polling");
            runtime(b,"restart");check(b.latest().runtime().session()!=session,"Restart reused session identity");
            try{call(b,41,"pause",id(session));throw new IllegalStateException("Accepted old session");}catch(ExecutionException expected){check(expected.getCause().getMessage().contains("runtime.stale_session"),"Wrong old-session rejection");}
            call(b,14);call(b,14);check(b.latest().runtime().phase()==0&&b.latest().runtime().entities().isEmpty(),"Stop retained live Runtime entities");
            call(b,9,"2",id(prefab),id(root));check(b.latest().snapshot().fields().stream().anyMatch(f->f.name().endsWith(" / points")&&f.value().equals("100")),"Stop changed authoring Health");call(b,6);
        }
        try(var b=new Bridge(Path.of(args[0]).toAbsolutePath(),work.resolve("fresh"),state->{},Runnable::run)){b.connect().get(20,TimeUnit.SECONDS);call(b,2,saved);check(!b.latest().snapshot().playing()&&b.latest().runtime().session()==0,"Fresh process restored live Runtime");check(graph(b,graphId).nodes().size()==6,"Runtime changed saved graph");}
        System.out.println("PASS F-II.4 native clock without Java ticks; Damage/Wait/Damage 100/93/86; Pause/Step/Restart/stale session; Stop authoring100 and fresh-process reopen");
    }
}

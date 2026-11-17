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
    private static void loadBaseline(Bridge b,long target)throws Exception{
        call(b,15,"1","Load plain target","registered-components");long plain=b.latest().snapshot().selection().document();
        for(int count:new int[]{10,100,1000}){
            call(b,15,"0","Runtime load "+count);long scene=b.latest().snapshot().selection().document();int activationCount=count==10?1:100;
            for(int i=0;i<count;i++)call(b,33,"place",id(scene),id(i<activationCount?target:plain),Double.toString(i%40),Double.toString(i/40));
            runtime(b,"scene",id(scene));call(b,6);long begin=System.nanoTime();call(b,13);double load=(System.nanoTime()-begin)/1e6;
            check(b.latest().runtime().entityCount()==count&&b.latest().runtime().activationCount()==activationCount,"Standalone native load counts");
            for(int i=0;i<80;i++){Thread.sleep(10);call(b,42);}
            var frame=b.latest().runtime();check(frame.phase()==2&&frame.entities().size()==count&&frame.trace().size()<=256&&frame.activations().size()<=64,"Standalone load snapshot/state bounds");
            double p50=frame.metrics().stream().filter(m->m.name().equals("Tick p50")).findFirst().orElseThrow().value(),p95=frame.metrics().stream().filter(m->m.name().equals("Tick p95")).findFirst().orElseThrow().value(),ram=frame.metrics().stream().filter(m->m.name().equals("Backend private bytes")).findFirst().orElseThrow().value();check(p50>=0&&p95>=p50,"Native profiler unavailable under load");
            begin=System.nanoTime();call(b,14);double stop=(System.nanoTime()-begin)/1e6;check(b.latest().runtime().entityCount()==0&&b.latest().runtime().activationCount()==0,"Load Stop retained live counts");
            System.out.println("FII4_RELEASE_BASELINE entities="+count+" activations="+activationCount+" play_roundtrip_ms="+load+" native_tick_p50_us="+p50+" native_tick_p95_us="+p95+" backend_private_bytes="+ram+" stop_roundtrip_ms="+stop);
        }
    }
    public static void main(String[] args)throws Exception{
        Path work=Path.of(args[1]).toAbsolutePath().resolve("runtime-"+UUID.randomUUID());Files.createDirectories(work);String saved;long graphId,oldConnection=0,oldGeneration=0,oldSession=0;
        try(var b=new Bridge(Path.of(args[0]).toAbsolutePath(),work.resolve("logs"),state->{},Runnable::run)){
            b.connect().get(20,TimeUnit.SECONDS);call(b,3,work.resolve("project").toString(),"F-II.4 native Runtime");long scene=b.latest().snapshot().entry();
            call(b,15,"1","Runtime target","registered-components");long prefab=b.latest().snapshot().selection().document();long root=b.latest().snapshot().nodes().stream().filter(n->n.document()==prefab&&n.kind()==2).findFirst().orElseThrow().id();
            Path image=work.resolve("runtime-target.png");var pixels=new java.awt.image.BufferedImage(8,8,java.awt.image.BufferedImage.TYPE_INT_ARGB);for(int y=0;y<8;y++)for(int x=0;x<8;x++)pixels.setRGB(x,y,0xffff0000);javax.imageio.ImageIO.write(pixels,"png",image.toFile());
            call(b,18,id(b.latest().snapshot().assetRevision()),image.toString(),"runtime-target.png");
            for(int i=0;i<200&&b.latest().snapshot().assets().stream().noneMatch(a->a.path().equals("Content/runtime-target.png")&&!a.cookedKey().isEmpty());i++){Thread.sleep(5);call(b,26);}
            var sprite=b.latest().snapshot().assets().stream().filter(a->a.path().equals("Content/runtime-target.png")&&!a.cookedKey().isEmpty()).findFirst().orElseThrow();call(b,24,id(b.latest().snapshot().assetRevision()),id(prefab),id(sprite.id()));
            call(b,15,"2","Damage Wait Damage");graphId=b.latest().snapshot().selection().document();
            var event=node(b,graphId,"Event");var amount=node(b,graphId,"Input");var duration=node(b,graphId,"Input");var before=node(b,graphId,"Native Call");var wait=node(b,graphId,"Wait");var after=node(b,graphId,"Native Call");
            call(b,38,"inputvalue",id(graphId),id(amount.id()),"7");call(b,38,"inputvalue",id(graphId),id(duration.id()),"30");
            link(b,graphId,event,"next",before,"in");link(b,graphId,amount,"value",before,"argument");link(b,graphId,before,"next",wait,"in");link(b,graphId,before,"result",wait,"value");link(b,graphId,duration,"value",wait,"ticks");link(b,graphId,wait,"next",after,"in");link(b,graphId,wait,"result",after,"argument");
            call(b,9,"2",id(prefab),id(root));var tuning=b.latest().snapshot().componentTypes().stream().filter(t->t.name().toLowerCase(java.util.Locale.ROOT).contains("tuning")).findFirst().orElseThrow(()->new IllegalStateException("Missing Tuning recipe: "+b.latest().snapshot().componentTypes()));call(b,30,id(b.latest().snapshot().selectionRevision()),"add",id(tuning.id()));call(b,9,"2",id(prefab),id(root));var label=b.latest().snapshot().fields().stream().filter(f->f.name().endsWith(" / label")).findFirst().orElseThrow(()->new IllegalStateException("Missing label field: "+b.latest().snapshot().fields().stream().map(Bridge.Field::name).toList()));call(b,16,id(b.latest().snapshot().selectionRevision()),label.key(),"a"+"Ж".repeat(700));var weight=b.latest().snapshot().fields().stream().filter(f->f.name().endsWith(" / weight")).findFirst().orElseThrow(()->new IllegalStateException("Missing weight field: "+b.latest().snapshot().fields().stream().map(Bridge.Field::name).toList()));call(b,16,id(b.latest().snapshot().selectionRevision()),weight.key(),"0.0000001");call(b,30,id(b.latest().snapshot().selectionRevision()),"graph",id(graphId));call(b,33,"place",id(scene),id(prefab),"0","0");call(b,6);saved=b.latest().snapshot().path();Files.writeString(Path.of(args[1]).toAbsolutePath().resolve("runtime-project.txt"),saved);long revision=b.latest().snapshot().revision();
            call(b,13);long session=b.latest().runtime().session();oldConnection=b.latest().runtime().connection();oldGeneration=b.latest().generation();oldSession=session;
            for(int i=0;i<100;i++){call(b,42);if(!b.latest().runtime().entities().isEmpty()&&health(b.latest().runtime())==93)break;Thread.sleep(5);}
            check(health(b.latest().runtime())==93,"First damage did not execute through native fixed pump");check(b.latest().runtime().entities().stream().flatMap(e->e.fields().stream()).anyMatch(f->f.name().equals("label")&&f.value().equals("a"+"Ж".repeat(511))),"Runtime string truncation split UTF-8 or changed prefix");check(b.latest().runtime().entities().stream().flatMap(e->e.fields().stream()).anyMatch(f->f.name().equals("weight")&&Double.parseDouble(f.value())==1e-7),"Runtime double display lost native precision");runtime(b,"pause");long tick=b.latest().runtime().tick();Thread.sleep(100);call(b,42);check(b.latest().runtime().tick()==tick,"Java polling/no polling changed paused Runtime time");
            runtime(b,"step-node","0");check(b.latest().runtime().tick()==tick,"Node step advanced fixed clock");
            for(int i=0;i<40&&health(b.latest().runtime())!=86;i++){long old=b.latest().runtime().tick();runtime(b,"step");check(b.latest().runtime().tick()==old+1,"Step Tick did not advance exactly one native tick");}
            check(health(b.latest().runtime())==86,"Native Wait did not resume second damage");check(b.latest().snapshot().revision()==revision&&!b.latest().snapshot().dirty(),"Runtime wrote authoring");
            runtime(b,"resume");long old=b.latest().runtime().tick();Thread.sleep(100);call(b,42);check(b.latest().runtime().tick()>old,"Native clock depends on Java polling");
            runtime(b,"restart");check(b.latest().runtime().session()!=session,"Restart reused session identity");
            try{call(b,41,"pause",id(session));throw new IllegalStateException("Accepted old session");}catch(ExecutionException expected){check(expected.getCause().getMessage().contains("runtime.stale_session"),"Wrong old-session rejection");}
            runtime(b,"pause");runtime(b,"breakpoint",id(graphId),id(after.id()),"1");check(b.latest().runtime().breakpoints().size()==1,"Breakpoint DTO missing");
            for(int i=0;i<40&&b.latest().runtime().trace().stream().noneMatch(t->t.state().equals("Breakpoint"));i++)runtime(b,"step");
            check(b.latest().runtime().trace().stream().anyMatch(t->t.state().equals("Breakpoint")&&t.node()==after.id()),"Native breakpoint did not pause");
            check(health(b.latest().runtime())==93,"Breakpoint allowed side effect before Continue");
            check(b.latest().runtime().activations().stream().anyMatch(a->!a.values().isEmpty()),"Debugger missing native typed values");
            runtime(b,"breakpoint",id(graphId),id(after.id()),"0");runtime(b,"step-node","0");
            runtime(b,"input","1","0");runtime(b,"pointer","2","3","1");check(b.latest().runtime().pointerDown()&&b.latest().runtime().inputX()==1,"Native input DTO");runtime(b,"pause");check(!b.latest().runtime().pointerDown()&&b.latest().runtime().inputX()==0,"Pause did not release native input");
            check(b.latest().runtime().metrics().stream().anyMatch(m->m.name().equals("Tick p95")&&m.available()),"Missing native timing provider");
            call(b,14);call(b,14);check(b.latest().runtime().phase()==0&&b.latest().runtime().entityCount()==0&&b.latest().runtime().activationCount()==0&&b.latest().runtime().entities().isEmpty(),"Stop retained live Runtime entities: phase="+b.latest().runtime().phase()+" entities="+b.latest().runtime().entityCount()+" activations="+b.latest().runtime().activationCount()+" rows="+b.latest().runtime().entities().size());
            call(b,9,"2",id(prefab),id(root));check(b.latest().snapshot().fields().stream().anyMatch(f->f.name().endsWith(" / points")&&f.value().equals("100")),"Stop changed authoring Health");check(b.latest().snapshot().fields().stream().anyMatch(f->f.name().endsWith(" / label")&&f.value().equals("a"+"Ж".repeat(700))),"Runtime display truncation mutated authoring text");call(b,6);
            try{b.runtimeCommand(b.latest().runtime().connection()-1,b.latest().generation(),List.of("play","0")).get(20,TimeUnit.SECONDS);throw new IllegalStateException("Accepted stale connection");}catch(ExecutionException expected){check(expected.getCause().getMessage().contains("runtime.stale_connection"),"Wrong stale connection rejection");}
            call(b,38,"inputvalue",id(graphId),id(amount.id()),"1000");call(b,13);
            for(int i=0;i<100&&b.latest().runtime().phase()!=5;i++){Thread.sleep(5);call(b,42);}
            check(b.latest().runtime().phase()==5&&b.latest().runtime().trace().stream().anyMatch(t->t.severity()==2),"Native failed operation did not publish Failed diagnostics");
            call(b,14);call(b,14);call(b,38,"inputvalue",id(graphId),id(amount.id()),"7");call(b,6);loadBaseline(b,prefab);
        }
        try(var b=new Bridge(Path.of(args[0]).toAbsolutePath(),work.resolve("fresh"),state->{},Runnable::run)){b.connect().get(20,TimeUnit.SECONDS);call(b,2,saved);check(!b.latest().snapshot().playing()&&b.latest().runtime().session()==0,"Fresh process restored live Runtime");check(graph(b,graphId).nodes().size()==6,"Runtime changed saved graph");call(b,13);oldConnection=b.latest().runtime().connection();oldGeneration=b.latest().generation();oldSession=b.latest().runtime().session();call(b,14);}
        try(var b=new Bridge(Path.of(args[0]).toAbsolutePath(),work.resolve("reconnected"),state->{},Runnable::run)){
            b.connect().get(20,TimeUnit.SECONDS);call(b,2,saved);call(b,13);check(b.latest().runtime().session()==oldSession&&b.latest().runtime().connection()!=oldConnection,"New backend/client did not recreate session ID with a fresh connection");check(b.latest().generation()!=oldGeneration,"Native process generation was reused");try{b.runtimeCommand(oldConnection,b.latest().generation(),List.of("pause",id(oldSession))).get(20,TimeUnit.SECONDS);throw new IllegalStateException("Old view controlled a new Bridge/backend with repeated native IDs");}catch(ExecutionException expected){check(expected.getCause().getMessage().contains("runtime.stale_connection"),"Wrong old-Bridge rejection");}call(b,42);check(b.latest().runtime().phase()==2,"Stale view paused the new Runtime");call(b,14);
        }
        System.out.println("PASS F-II.4 native clock without Java ticks; Damage/Wait/Damage 100/93/86; Pause/Step/Restart/stale session; Stop authoring100 and fresh-process reopen");
    }
}

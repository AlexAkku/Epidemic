package epidemic.editor;

import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.nio.charset.StandardCharsets;

/** Real SDK -> exported schema -> native EditRequest -> Compiler/VM -> managed ABI process. */
public final class ScriptTests {
    static void check(boolean value,String message){if(!value)throw new IllegalStateException(message);}
    static String id(long value){return Long.toUnsignedString(value);}
    static Bridge.Reply call(Bridge bridge,int operation,String...args)throws Exception{return bridge.call(operation,args).get(30,TimeUnit.SECONDS);}
    static Bridge.Graph graph(Bridge bridge,long document){return bridge.latest().snapshot().graphs().stream().filter(g->g.document()==document).findFirst().orElseThrow();}
    static Bridge.GraphNode node(Bridge bridge,long doc,String name)throws Exception{var g=graph(bridge,doc);int index=0;for(;index<g.catalogue().size();index++)if(g.catalogue().get(index).name().equals(name))break;call(bridge,38,"create",id(doc),Integer.toString(index),"100","100");return graph(bridge,doc).nodes().getLast();}
    static String port(Bridge.GraphNode node,String name){return id(node.ports().stream().filter(p->p.name().equals(name)).findFirst().orElseThrow().id());}
    static void connect(Bridge bridge,long doc,Bridge.GraphNode from,String output,Bridge.GraphNode to,String input)throws Exception{call(bridge,38,"connect",id(doc),port(from,output),port(to,input));}
    static void run(Path directory,String...args)throws Exception{Process process=new ProcessBuilder(args).directory(directory.toFile()).redirectErrorStream(true).start();CompletableFuture.delayedExecutor(60,TimeUnit.SECONDS).execute(()->{if(process.isAlive()){process.descendants().forEach(ProcessHandle::destroyForcibly);process.destroyForcibly();}});var log=directory.resolve("test-build.log");try(var output=Files.newOutputStream(log)){process.getInputStream().transferTo(output);}check(process.waitFor(60,TimeUnit.SECONDS)&&process.exitValue()==0,"Build/worker failed; see "+log);}
    static String source(int version){return "using Epidemic.ScriptApi; [ScriptComponent(\"qa.Turret\")] public sealed class Turret { [ScriptField] public long Health {get;set;}=100; [ScriptField] public double Range {get;set;}=150; [ScriptField] public bool Enabled {get;set;}=true; [ScriptField] public string Label {get;set;}=\"Башня\"; [GraphOperation(\"qa.Turret.Damage.v1\")] public long ApplyDamage(long amount){Health=checked(Health-amount*"+version+");return Health;} [GraphOperation(\"qa.Turret.Heal.v1\")] public long Heal(long amount){Health=checked(Health+amount);return Health;} }";}
    public static void main(String[]args)throws Exception{
        Path backend=Path.of(args[0]).toAbsolutePath(),work=Path.of(args[1]).toAbsolutePath();Files.createDirectories(work);Path project=Files.createTempDirectory(work,"FII2 Кириллица ").resolve("Project"),managed=backend.getParent().resolve("managed");
        Path dotnet=Path.of(System.getenv("ProgramFiles"),"dotnet/dotnet.exe");String file;long graphId;
        try(var bridge=new Bridge(backend,work,s->{},Runnable::run)){
            bridge.connect().get(30,TimeUnit.SECONDS);call(bridge,3,project.toString(),"F-II.2 native C#");file=bridge.latest().snapshot().path();
            Path scripts=project.resolve("Scripts");Files.createDirectories(scripts);Files.writeString(scripts.resolve("Turret.cs"),source(1),StandardCharsets.UTF_8);
            String api=managed.resolve("Epidemic.ScriptApi.dll").toString().replace("&","&amp;");Path csproj=scripts.resolve("GameScripts.csproj");Files.writeString(csproj,"<Project Sdk=\"Microsoft.NET.Sdk\"><PropertyGroup><TargetFramework>net10.0</TargetFramework></PropertyGroup><ItemGroup><Reference Include=\"Epidemic.ScriptApi\"><HintPath>"+api+"</HintPath></Reference></ItemGroup></Project>");Files.writeString(project.resolve("NuGet.Config"),"<configuration><packageSources><clear/></packageSources></configuration>");
            long typeId=0,prefabId=0,rootId=0;
            for(int version=1;version<=2;version++){
                Files.writeString(scripts.resolve("Turret.cs"),source(version).replace("=150;","="+(150+25*(version-1))+";"),StandardCharsets.UTF_8);Path generation=project.resolve(".epidemic/scripts/v"+version);Files.createDirectories(generation);
                run(project,dotnet.toString(),"build",csproj.toString(),"-c","Release","-o",generation.toString(),"--nologo","-p:UseSharedCompilation=false","-nr:false","-p:ImportDirectoryBuildProps=false","-p:ImportDirectoryBuildTargets=false");
                Path schema=generation.resolve("types.schema"),assembly=generation.resolve("GameScripts.dll");run(project,dotnet.toString(),managed.resolve("Epidemic.ScriptWorker.dll").toString(),"schema",assembly.toString(),schema.toString(),project.relativize(assembly).toString(),project.toString());
                call(bridge,39,project.relativize(schema).toString());var type=bridge.latest().snapshot().componentTypes().stream().filter(t->t.name().equals("qa.Turret")).findFirst().orElseThrow();if(version==1)typeId=type.id();else check(typeId==type.id(),"Type ID changed after rebuild");
                if(version==1){call(bridge,15,"1","Live Schema Prefab");prefabId=bridge.latest().snapshot().selection().document();rootId=bridge.latest().snapshot().nodes().stream().filter(n->n.document()==bridge.latest().snapshot().selection().document()&&n.kind()==2).findFirst().orElseThrow().id();call(bridge,9,"2",id(prefabId),id(rootId));call(bridge,30,id(bridge.latest().snapshot().selectionRevision()),"add",id(typeId));}
                call(bridge,9,"2",id(prefabId),id(rootId));
                final long owner=prefabId,componentType=typeId;
                check(bridge.latest().snapshot().nodes().stream().anyMatch(n->n.document()==owner&&n.kind()==5&&n.component()==componentType),"Rebuild detached persisted component");
                var range=bridge.latest().snapshot().fields().stream().filter(f->f.name().endsWith(" / Range")||f.name().equals("Range")).findFirst().orElseThrow();
                check(Double.parseDouble(range.value())==150+25*(version-1),"Updated default not projected into unchanged Prefab");
                if(version==2){call(bridge,30,id(bridge.latest().snapshot().selectionRevision()),"remove",id(typeId)+" 0");check(bridge.latest().snapshot().nodes().stream().noneMatch(n->n.document()==owner&&n.kind()==5&&n.component()==componentType),"Root Remove Component failed");call(bridge,7);}
                call(bridge,15,"2","Managed V"+version);long document=bridge.latest().snapshot().selection().document();
                var entry=node(bridge,document,"Entry");var input=node(bridge,document,"Input");var invoke=node(bridge,document,"Turret.ApplyDamage");var output=node(bridge,document,"Output");
                call(bridge,38,"settings",id(document),id(input.id()),input.name(),"7","","","0");connect(bridge,document,entry,"next",invoke,"in");connect(bridge,document,input,"value",invoke,"argument");connect(bridge,document,invoke,"next",output,"in");connect(bridge,document,invoke,"result",output,"value");
                call(bridge,38,"run",id(document));check(graph(bridge,document).report().contains("result="+(100-7*version)),"Native VM did not invoke C# generation V"+version+": "+graph(bridge,document).report());
                call(bridge,13);call(bridge,14);call(bridge,13);call(bridge,14);
                call(bridge,6);
            }
            graphId=bridge.latest().snapshot().documents().stream().filter(d->d.name().equals("Managed V2")).findFirst().orElseThrow().id();
            // Wrong schema version is rejected without replacing the last-good native palette.
            Path broken=project.resolve(".epidemic/scripts/broken.schema");Files.write(broken,new byte[]{0,2});long revision=bridge.latest().snapshot().revision();
            try{call(bridge,39,project.relativize(broken).toString());throw new IllegalStateException("Accepted malformed schema");}catch(ExecutionException expected){check(expected.getCause().getMessage().contains("schema_version"),"Wrong rejection");}
            check(bridge.latest().snapshot().revision()==revision,"Bad schema changed authoring");
            call(bridge,10);bridge.shutdown().get(20,TimeUnit.SECONDS);
        }
        try(var bridge=new Bridge(backend,work,s->{},Runnable::run)){
            bridge.connect().get(30,TimeUnit.SECONDS);call(bridge,2,file);check(graph(bridge,graphId).catalogue().stream().anyMatch(t->t.name().equals("Turret.ApplyDamage")),"Reopen lost native operation schema");call(bridge,38,"run",id(graphId));check(graph(bridge,graphId).report().contains("result=86"),"New-process managed roundtrip failed: "+graph(bridge,graphId).report());call(bridge,10);bridge.shutdown().get(20,TimeUnit.SECONDS);
        }
        run(work,backend.toString(),"--script-lifecycle-smoke",file);
        check(Files.readString(work.resolve("test-build.log")).contains("PASS C# native Play/Send/Tick"),"Runtime lifecycle scenario did not run");
        System.out.println("PASS F-II.2 SDK/schema, stable native registry, C# via Graph VM/ABI, V1->Play/Stop->V2, malformed schema atomic reject, Save/Open another backend process");
    }
}

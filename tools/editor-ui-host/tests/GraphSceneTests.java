package epidemic.editor;

import java.nio.file.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.concurrent.atomic.*;
import javax.swing.*;

/** Native IPC authoring/resolution/persistence and transient coordinator regression.
 * OLE pointer capture, physical HWND/DPI/monitor behaviour stays in manual QA. */
public final class GraphSceneTests {
    private static Bridge.Reply call(Bridge b,int op,String... args)throws Exception{return b.call(op,args).get(20,TimeUnit.SECONDS);}
    private static String id(long n){return Long.toUnsignedString(n);}
    private static void require(boolean condition,String message){if(!condition)throw new IllegalStateException(message);}
    private static Bridge.Graph graph(Bridge b,long doc){return b.latest().snapshot().graphs().stream().filter(g->g.document()==doc).findFirst().orElseThrow();}
    private static Bridge.GraphNode create(Bridge b,long doc,String name)throws Exception {
        var g=graph(b,doc);int index=-1;for(int i=0;i<g.catalogue().size();++i)if(g.catalogue().get(i).name().equals(name))index=i;
        require(index>=0,"Missing native recipe "+name);call(b,38,"create",id(doc),""+index,"400","120");return graph(b,doc).nodes().getLast();
    }
    private static long port(Bridge.GraphNode n,String name){return n.ports().stream().filter(p->p.name().equals(name)).findFirst().orElseThrow().id();}
    private static List<Bridge.ReferenceCandidate> probe(Bridge b,long graph,long scene,long entity,long port)throws Exception {
        long revision=b.latest().snapshot().revision();var reply=call(b,40,"probe",id(graph),id(scene),id(entity),id(port));
        require(reply.snapshot().revision()==revision,"Preview changed authoring revision");return reply.snapshot().referenceCandidates();
    }
    private static String[] dropArgs(long doc,Bridge.GraphReference value,long port){var a=new ArrayList<>(List.of("drop",id(doc)));a.add(String.join(" ",value.arguments()));a.add(id(port));a.add("80");a.add("160");return a.toArray(String[]::new);}
    private static void drop(Bridge b,long doc,Bridge.GraphReference value,long port)throws Exception{call(b,40,dropArgs(doc,value,port));}
    private static void reject(Bridge b,String code,String... args)throws Exception {
        var before=b.latest().snapshot();try{call(b,40,args);throw new IllegalStateException("Accepted "+code);}catch(ExecutionException expected){require(expected.getMessage().contains(code),"Wrong rejection "+expected);}
        var after=b.latest().snapshot();require(before.revision()==after.revision()&&before.graphs().equals(after.graphs())&&before.nodes().equals(after.nodes()),"Rejected reference partially mutated project");
    }
    private static Bridge.GraphReference reference(Bridge b,long doc,long scene,long entity,int kind)throws Exception {
        return probe(b,doc,scene,entity,0).stream().filter(c->c.value().kind()==kind&&c.reason().isEmpty()).findFirst().orElseThrow().value();
    }
    private static final class Visible extends JPanel { @Override public boolean isShowing(){return true;} }
    private static void coordinator(Bridge b,long doc,long scene,long entity,long port)throws Exception {
        AtomicReference<GraphSceneDragCoordinator> owner=new AtomicReference<>();AtomicReference<java.awt.datatransfer.Transferable> transfer=new AtomicReference<>();
        AtomicReference<String> status=new AtomicReference<>("");var source=new Visible();var target=new Visible();
        SwingUtilities.invokeAndWait(()->owner.set(new GraphSceneDragCoordinator(b::latest,b::callAt,status::set)));
        var c=owner.get();long revision=b.latest().snapshot().revision();var beforeGraph=graph(b,doc);
        try {
            SwingUtilities.invokeAndWait(()->{
                for(int i=0;i<100;i++) {
                    var t=c.sceneTransfer(source,scene,entity);require(c.fromScene(t)&&c.activeSessions()==1,"Scene session absent");c.cancel();require(!c.fromScene(t)&&c.activeSessions()==0,"Cancelled nonce survived");
                    var g=c.graphTransfer(target,new GraphSceneDragCoordinator.GraphHit(doc,port,60,80));require(c.fromGraph(g),"Graph session absent");c.cancelFor(target);require(!c.fromGraph(g),"Closed source survived");
                }
                transfer.set(c.sceneTransfer(source,scene,entity));
                c.dropGraph(transfer.get(),new GraphSceneDragCoordinator.GraphHit(doc,port,60,80),target);
            });
            long deadline=System.nanoTime()+TimeUnit.SECONDS.toNanos(15);
            while(b.latest().snapshot().revision()==revision&&System.nanoTime()<deadline)Thread.sleep(10);
            require(b.latest().snapshot().revision()==revision+1,"Coordinator did not use native atomic transaction: "+status.get());
            SwingUtilities.invokeAndWait(()->c.dropGraph(transfer.get(),new GraphSceneDragCoordinator.GraphHit(doc,port,60,80),target));
            call(b,4);require(b.latest().snapshot().revision()==revision+1,"Duplicate Drop reused nonce");
            call(b,7);require(graph(b,doc).edges().equals(beforeGraph.edges())&&graph(b,doc).nodes().equals(beforeGraph.nodes()),"Coordinator Drop not one Undo");
            // A changed native revision retires even an otherwise valid nonce.
            SwingUtilities.invokeAndWait(()->transfer.set(c.sceneTransfer(source,scene,entity)));
            call(b,5,id(scene),"Scene changed");
            SwingUtilities.invokeAndWait(()->{require(!c.fromScene(transfer.get()),"Stale source revision survived");c.cancel();});
        } finally {SwingUtilities.invokeAndWait(c::close);}
    }
    private static void sceneClipboard(Bridge b,Path root)throws Exception {
        call(b,15,"0","Clipboard Scene");long scene=b.latest().snapshot().selection().document();
        long prefab=b.latest().snapshot().documents().stream().filter(d->d.kind()==1).findFirst().orElseThrow().id();
        call(b,33,"place",id(scene),id(prefab),"10","20");long parent=b.latest().snapshot().selection().element();
        call(b,29,"rename",id(scene),id(parent),"Parent");call(b,29,"create",id(scene),id(parent));long child=b.latest().snapshot().selection().element();
        call(b,33,"place",id(scene),id(prefab),"30","40");long sibling=b.latest().snapshot().selection().element();
        String parentLabel=b.latest().snapshot().nodes().stream().filter(n->n.document()==scene&&n.id()==parent).findFirst().orElseThrow().label();
        String selection=id(parent)+" "+id(child)+" "+id(sibling);long revision=b.latest().snapshot().revision();
        call(b,29,"copy",id(scene),"0",selection);require(b.latest().snapshot().revision()==revision,"Copy mutated authoring");
        call(b,29,"paste",id(scene),"0");
        require(b.latest().snapshot().nodes().stream().filter(n->n.document()==scene&&n.kind()==1).count()==6,"Subtree+multiroot paste lost or doubled descendants");
        long copiedParent=b.latest().snapshot().nodes().stream().filter(n->n.document()==scene&&n.kind()==1&&n.id()!=parent&&n.label().equals(parentLabel)).findFirst().orElseThrow(()->new IllegalStateException("Copied names: "+b.latest().snapshot().nodes().stream().filter(n->n.document()==scene&&n.kind()==1).map(n->n.id()+" parent="+n.parent()+" label="+n.label()).toList())).id();
        var sourceItem=b.latest().snapshot().sceneItems().stream().filter(n->n.document()==scene&&n.id()==parent).findFirst().orElseThrow();
        var copyItem=b.latest().snapshot().sceneItems().stream().filter(n->n.document()==scene&&n.id()==copiedParent).findFirst().orElseThrow();
        require(sourceItem.x()==10&&sourceItem.y()==20&&copyItem.x()==sourceItem.x()&&copyItem.y()==sourceItem.y()&&copyItem.rotation()==sourceItem.rotation()&&copyItem.left()==sourceItem.left()&&copyItem.right()==sourceItem.right(),"Paste lost placement transform");
        call(b,7);require(b.latest().snapshot().nodes().stream().filter(n->n.document()==scene&&n.kind()==1).count()==3,"Paste was not one Undo");call(b,8);
        long beforeCut=b.latest().snapshot().revision();call(b,29,"cut",id(scene),"0",selection);call(b,29,"paste",id(scene),"0");
        require(b.latest().snapshot().revision()==beforeCut&&b.latest().snapshot().nodes().stream().filter(n->n.document()==scene&&n.kind()==1&&(n.id()==parent||n.id()==child||n.id()==sibling)).count()==3,"Same-scene no-op Cut changed identities/history");
        call(b,29,"cut",id(scene),"0",id(child));call(b,29,"paste",id(scene),"0");
        require(b.latest().snapshot().nodes().stream().anyMatch(n->n.document()==scene&&n.id()==child&&n.parent()==0),"Same-scene Cut did not preserve the moved node identity");call(b,7);
        require(b.latest().snapshot().nodes().stream().anyMatch(n->n.document()==scene&&n.id()==child&&n.parent()==parent),"Cut reparent Undo did not restore hierarchy");
        call(b,29,"copy",id(scene),"0",selection);call(b,15,"0","Clipboard Destination");long destination=b.latest().snapshot().selection().document();call(b,29,"paste",id(destination),"0");
        require(b.latest().snapshot().nodes().stream().filter(n->n.document()==destination&&n.kind()==1).count()==3,"Cross-scene paste lost hierarchy");
        call(b,29,"copy",id(scene),"0",id(child));call(b,29,"paste",id(destination),"0");long detached=b.latest().snapshot().selection().element();
        var detachedItem=b.latest().snapshot().sceneItems().stream().filter(n->n.document()==destination&&n.id()==detached).findFirst().orElseThrow();require(detachedItem.x()==10&&detachedItem.y()==20,"Copy nested child to Scene root lost world transform");call(b,7);
        call(b,29,"delete-many",id(destination),"0",b.latest().snapshot().nodes().stream().filter(n->n.document()==destination&&n.kind()==1).map(n->id(n.id())).collect(java.util.stream.Collectors.joining(" ")));
        require(b.latest().snapshot().nodes().stream().noneMatch(n->n.document()==destination&&n.kind()==1),"Delete-many failed");call(b,7);
        call(b,6);call(b,10);call(b,2,root.resolve("Project.epidemicproj").toString());
        require(b.latest().snapshot().nodes().stream().filter(n->n.document()==destination&&n.kind()==1).count()==3,"Scene clipboard commands lost Save/Open state");
        try{call(b,29,"paste",id(destination),"0");throw new IllegalStateException("Clipboard leaked into new project lifetime");}catch(ExecutionException expected){require(expected.getCause().getMessage().contains("clipboard_empty"),"Unexpected clipboard lifetime rejection");}
        System.out.println("PASS Scene native copy/cut/paste/delete-many, subtree identities/transforms, cross-scene, atomic history and Save/Open clipboard lifetime");
    }
    public static void main(String[] args)throws Exception {
        Path backend=Path.of(args[0]),work=Path.of(args[1]);Files.createDirectories(work);Path root=Files.createTempDirectory(work,"fii3-").resolve("Проект");
        long scene,doc,placementA,placementB;List<Bridge.GraphNode> saved;List<Bridge.GraphEdge> edges;
        try(var b=new Bridge(backend,work,state->{},Runnable::run)) {
            b.connect().get(20,TimeUnit.SECONDS);call(b,3,root.toString(),"Graph Scene References");scene=b.latest().snapshot().entry();
            call(b,15,"1","Enemy","registered-components");long prefab=b.latest().snapshot().selection().document();
            call(b,33,"place",id(scene),id(prefab),"-100","0");placementA=b.latest().snapshot().selection().element();
            call(b,33,"place",id(scene),id(prefab),"100","0");placementB=b.latest().snapshot().selection().element();
            call(b,15,"2","Stable References");doc=b.latest().snapshot().selection().document();
            var check=create(b,doc,"Is Entity Valid");long input=port(check,"reference");
            var a=reference(b,doc,scene,placementA,0);var other=reference(b,doc,scene,placementB,0);
            require(a.prefab()==other.prefab()&&a.entity()==other.entity()&&a.placement()!=other.placement(),"Placements alias their authoring identity");
            int entityCount=b.latest().snapshot().nodes().size();long revision=b.latest().snapshot().revision();
            drop(b,doc,a,input);require(b.latest().snapshot().revision()==revision+1&&graph(b,doc).nodes().size()==2&&graph(b,doc).edges().size()==1,"Reference+connect not atomic");
            require(b.latest().snapshot().nodes().stream().filter(n->n.kind()==1).count()==2,"Reference spawned an Entity");
            call(b,7);require(graph(b,doc).nodes().size()==1&&graph(b,doc).edges().isEmpty(),"Reference Undo not atomic");call(b,8);
            var ref=graph(b,doc).nodes().getLast();require(ref.reference().equals(a)&&ref.referenceAvailable(),"Native reference DTO lost identity");
            call(b,29,"rename",id(scene),id(placementA),"Переименован");require(graph(b,doc).nodes().getLast().reference().equals(a)&&graph(b,doc).nodes().getLast().referenceLabel().contains("Переименован"),"Rename broke stable reference");
            call(b,29,"parent",id(scene),id(placementA),id(placementB));require(graph(b,doc).nodes().getLast().referenceAvailable(),"Reparent broke stable reference");
            call(b,29,"delete",id(scene),id(placementA));require(!graph(b,doc).nodes().getLast().referenceAvailable(),"Delete did not mark Missing Reference");
            call(b,7);require(graph(b,doc).nodes().getLast().referenceAvailable(),"Restore did not revive reference");
            var bad=new Bridge.GraphReference(a.kind(),a.document(),a.placement(),a.prefab(),a.entity()+1,0,0);
            reject(b,"editor.reference_entity_missing",dropArgs(doc,bad,0));
            var instance=reference(b,doc,scene,placementA,1);reject(b,"graph.connection_type",dropArgs(doc,instance,input));
            call(b,7); // undo Reparent; leave first linked reference intact
            var checkB=create(b,doc,"Is Entity Valid");drop(b,doc,other,port(checkB,"reference"));
            var components=probe(b,doc,scene,placementA,0).stream().filter(c->c.value().kind()==3).toList();require(!components.isEmpty(),"No component reference choices");
            var component=components.getFirst().value();drop(b,doc,component,0);require(graph(b,doc).nodes().getLast().reference().component()==component.component()&&graph(b,doc).nodes().getLast().reference().part()==component.part(),"Component/part identity lost");
            long rootEntity=b.latest().snapshot().nodes().stream().filter(n->n.document()==prefab&&n.kind()==2&&n.parent()==0).findFirst().orElseThrow().id();
            var definition=probe(b,doc,prefab,rootEntity,0).stream().filter(c->c.value().kind()==2).findFirst().orElseThrow().value();drop(b,doc,definition,0);
            call(b,15,"0","Another Scene");long wrongScene=b.latest().snapshot().selection().document();call(b,29,"create",id(wrongScene),"0");long wrong=b.latest().snapshot().selection().element();
            var outside=new Bridge.GraphReference(0,wrongScene,wrong,0,0,0,0);reject(b,"editor.reference_scope",dropArgs(doc,outside,0));
            var free=create(b,doc,"Is Entity Valid");coordinator(b,doc,scene,placementA,port(free,"reference"));
            // Real native VM preview consumes typed reference via resolver, never i64.
            call(b,38,"delete",id(doc),id(free.id()));
            // Preserve the incomplete authoring draft through a new process.
            call(b,6);saved=graph(b,doc).nodes();edges=graph(b,doc).edges();
            require(b.latest().snapshot().nodes().size()>=entityCount,"Authoring nodes unexpectedly removed");
        }
        try(var b=new Bridge(backend,work,state->{},Runnable::run)) {
            b.connect().get(20,TimeUnit.SECONDS);call(b,2,root.resolve("Project.epidemicproj").toString());
            require(graph(b,doc).nodes().equals(saved)&&graph(b,doc).edges().equals(edges),"Fresh process lost native reference payload/coordinates");
            var refs=graph(b,doc).nodes().stream().filter(n->n.reference()!=null).toList();require(refs.size()==4&&refs.stream().allMatch(Bridge.GraphNode::referenceAvailable),"Restored references not live");
            require(refs.stream().anyMatch(n->n.reference().placement()==placementA)&&refs.stream().anyMatch(n->n.reference().placement()==placementB),"Placements aliased after reopen");
            sceneClipboard(b,root);
        }
        System.out.println("PASS F-II.3 native typed references, two placements/component/definition, preview/atomic Undo, rename/reparent/delete/restore, wrong scope/type, 100 transient cycles, duplicate/stale Drop and fresh-process Save/Open");
    }
}

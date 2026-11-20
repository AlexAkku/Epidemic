package epidemic.editor;

import java.nio.file.*;
import java.io.*;
import java.util.*;
import java.security.MessageDigest;
import java.util.concurrent.*;

public final class UiTests {
    static void require(boolean result, String message) {
        if (!result) throw new IllegalStateException(message);
    }
    static Bridge.Reply result(CompletableFuture<Bridge.Reply> value) throws Exception {
        return value.get(20, TimeUnit.SECONDS);
    }
    static void nativeReject(Bridge bridge,String code,int operation,String...args) throws Exception {
        try { result(bridge.call(operation,args)); }
        catch(ExecutionException expected){require(expected.getCause().getMessage().contains(code),"Wrong rejection: "+expected.getCause());return;}
        throw new IllegalStateException("Native operation unexpectedly accepted: "+operation+" / "+code);
    }
    interface Checked { void run() throws Exception; }
    static void rejects(Checked action, String reason) throws Exception {
        try { action.run(); } catch (IOException | IllegalArgumentException expected) { return; }
        throw new IllegalStateException(reason);
    }
    static void presentation() throws Exception {
        Catalog catalog = new Catalog();
        require(catalog.panels().size() == 46, "Panel coverage changed");
        require(catalog.schemas().size() == 77 && catalog.schemas().stream().mapToInt(s -> s.fields().size()).sum() == 570, "Field coverage changed");
        require(catalog.panel("prefab.editor").anchor().equals("dock.center.right.primary"), "Prefab must be on right");
        require(catalog.panel("project.explorer").order() == 1 && catalog.panel("scene.hierarchy").order() == 2, "Left tab order");
        require(!catalog.contains("scene.editor:0") && !catalog.contains("scene.editor:wrong"), "Invalid typed document ID accepted");
        FeatureRegistry registry = new FeatureRegistry(); boolean[] executed = {false};
        var planned = registry.planned("test.future", "Future", "F3");
        try { registry.dispatch(planned, true, () -> executed[0] = true); } catch (IllegalStateException expected) { }
        require(!executed[0], "Planned command escaped central gate");
        for (String name : List.of("CON", "COM1.txt", "LPT9", "a/b", "a?", "bad.", "bad ", "\n", "..", ""))
            require(!ProjectNames.valid(name), "Invalid Windows name accepted: " + name);
        require(ProjectNames.valid("Новая игра 2") && ProjectNames.valid("CONSOLE"), "Valid Unicode name rejected");
        System.out.println("PASS U4/U5/U6 registry 46 panels, 77 schemas/570 fields, planned gate and project names; Platform owns layout");
    }
    static Bridge.Snapshot recentSnapshot(Path path, int n) {
        return new Bridge.Snapshot(1,1,1,0,2,n,n,path.toString(),"Игра " + n,new Bridge.Selection(0,0,0),List.of(),List.of(),List.of());
    }
    static void recent(Path directory) throws Exception {
        Path area = Files.createTempDirectory(directory,"recent-");
        RecentProjects recent = new RecentProjects(area.resolve("profile"));
        for(int i=0;i<12;i++) { Path project=Files.createDirectory(area.resolve("game-"+i)); recent.opened(recentSnapshot(project,i)); }
        require(recent.load().size()==10 && recent.load().getFirst().title().equals("Игра 11"),"Recent limit/order");
        recent.opened(recentSnapshot(area.resolve("game-8/../game-8"),8));
        var restored=new RecentProjects(area.resolve("profile")).load();
        require(restored.size()==10 && restored.getFirst().title().equals("Игра 8"),"Recent path identity/restart");
        Files.delete(area.resolve("game-8")); recent.remove(restored.getFirst());
        require(recent.load().size()==9,"Missing recent removal");
        System.out.println("PASS U3 recent 10 limit, canonical deduplication, Unicode, restart, missing removal");
    }
    static Map<String,String> hashes(Path directory) throws Exception {
        Map<String,String> result = new TreeMap<>();
        try(var files=Files.walk(directory)) {
            for(Path file:files.filter(Files::isRegularFile).toList()) {
                String relative=directory.relativize(file).toString();
                if(relative.startsWith(".epidemic"))continue; // Native lock/cache are explicitly mutable.
                result.put(relative,HexFormat.of().formatHex(MessageDigest.getInstance("SHA-256").digest(Files.readAllBytes(file))));
            }
        } return result;
    }
    static void compatibility(Path executable, Path directory) throws Exception {
        Path repository=Path.of("").toAbsolutePath();
        Path legacy=repository.resolve("Game/Editor/example/EditorDemo/EditorDemo.epidemic");
        Path copy=Files.createTempDirectory(directory,"legacy-").resolve("EditorDemo.epidemic");
        Files.copy(legacy,copy); byte[] before=Files.readAllBytes(copy);
        try(Bridge bridge=new Bridge(executable,directory,state->{ })) {
            result(bridge.connect()); var opened=result(bridge.call(2,copy.toString()));
            require(opened.snapshot().storage()==1 && !opened.snapshot().documents().isEmpty(),"Legacy reader lost documents");
            var prefabNode=opened.snapshot().nodes().stream().filter(n->n.kind()==2).findFirst().orElseThrow();
            result(bridge.call(9,"2",Long.toUnsignedString(prefabNode.document()),Long.toUnsignedString(prefabNode.id())));
            var readonly=bridge.latest().snapshot().fields().stream().filter(f->!f.editable()).findFirst().orElseThrow();
            require(bridge.latest().snapshot().fields().stream().map(Bridge.Field::key).distinct().count()==bridge.latest().snapshot().fields().size(),"Readonly reference rows have duplicate keys");
            nativeReject(bridge,"inspector_readonly",16,Long.toUnsignedString(bridge.latest().snapshot().selectionRevision()),readonly.key(),"change");
            result(bridge.call(9,"0","0","0"));require(bridge.latest().snapshot().selection().document()==0&&bridge.latest().snapshot().fields().isEmpty()&&!bridge.latest().snapshot().dirty(),"Clear/project selection mutated authoring");
            result(bridge.call(4));
            require(result(bridge.call(13)).snapshot().playing(),"Existing demo Play failed");
            require(!result(bridge.call(14)).snapshot().playing(),"Existing demo Stop failed");
            result(bridge.call(6)); result(bridge.call(10)); result(bridge.call(11));
        }
        require(Arrays.equals(before,Files.readAllBytes(copy)),"No-op legacy save changed file");
        Path source=repository.resolve("Temp/U/fixtures/P04-backup");
        if(!Files.isRegularFile(source.resolve("Project.epidemicproj")))throw new IOException("P04 qualification backup unavailable; preserve/copy a real fixture first");
        Map<String,String> sourceBefore=hashes(source);
        Path folder=Files.createTempDirectory(directory,"p04-");
        Files.createDirectory(folder.resolve(".epidemic")); // Never copy the writer lock; immutable cooked images are project data.
        try(var paths=Files.walk(source)) {
            for(Path file:paths.toList()) {
                Path relative=source.relativize(file);
                if(relative.startsWith(Path.of(".epidemic"))&&!relative.startsWith(Path.of(".epidemic/Cache/images")))continue;
                Path destination=folder.resolve(relative);
                if(Files.isDirectory(file))Files.createDirectories(destination);else Files.copy(file,destination);
            }
        }
        Map<String,String> cooked=hashes(source.resolve(".epidemic/Cache/images"));
        require(!cooked.isEmpty()&&cooked.equals(hashes(folder.resolve(".epidemic/Cache/images"))),"P04 fixture lost immutable cooked data");
        Map<String,String> copied=hashes(folder);
        try(Bridge bridge=new Bridge(executable,directory,state->{ })) {
            result(bridge.connect()); var opened=result(bridge.call(2,folder.resolve("Project.epidemicproj").toString()));
            require(!opened.snapshot().documents().isEmpty(),"P04 documents absent");
            require(!opened.snapshot().assets().isEmpty(),"P04 source assets absent");
            var ids=opened.snapshot().assets().stream().map(Bridge.Asset::id).toList();
            var keys=opened.snapshot().assets().stream().map(Bridge.Asset::cookedKey).toList();
            result(bridge.call(6)); result(bridge.call(10));
            var reopened=result(bridge.call(2,folder.resolve("Project.epidemicproj").toString()));
            require(ids.equals(reopened.snapshot().assets().stream().map(Bridge.Asset::id).toList()),"P04 asset IDs changed");
            require(keys.equals(reopened.snapshot().assets().stream().map(Bridge.Asset::cookedKey).toList()),"P04 cooked keys changed");
            Path cloned=folder.resolveSibling(folder.getFileName()+"-clone");
            result(bridge.call(12,cloned.toString()));
            require(cooked.equals(hashes(cloned.resolve(".epidemic/Cache/images"))),"Clone lost last-good cooked bytes");
            result(bridge.call(10));
            var cloneOpened=result(bridge.call(2,cloned.resolve("Project.epidemicproj").toString()));
            require(ids.equals(cloneOpened.snapshot().assets().stream().map(Bridge.Asset::id).toList()),"Clone changed stable asset IDs");
            require(keys.equals(cloneOpened.snapshot().assets().stream().map(Bridge.Asset::cookedKey).toList()),"Clone changed cooked identities");
            require(result(bridge.call(13)).snapshot().playing(),"P04 clone native Play failed");
            require(!result(bridge.call(14)).snapshot().playing(),"P04 clone native Stop failed");
            result(bridge.call(10)); result(bridge.call(11));
        }
        require(copied.equals(hashes(folder)) && sourceBefore.equals(hashes(source)),"P04 files changed on no-op/new layout");
        System.out.println("PASS U7 legacy reader/no-op save, P04 source hashes/stable assets/cooked keys, source fixtures untouched");
    }
    static void lifecycle(Path executable,Path directory) throws Exception {
        Path project=directory.resolve("F0-"+System.nanoTime()),clone=project.resolveSibling(project.getFileName()+"-copy");
        try(Bridge bridge=new Bridge(executable,directory,state->{ })){
            var hello=result(bridge.connect());
            require(hello.capabilities().containsAll(Set.of("clone-folder","play","stop")),"Missing F0 capability negotiation");
            nativeReject(bridge,"no_project",13);nativeReject(bridge,"argument_count",12);nativeReject(bridge,"argument_count",14,"extra");
            var opened=result(bridge.call(3,project.toString(),"F0 исходник"));
            var initial=opened.snapshot();var doc=initial.documents().getFirst();
            var playing=result(bridge.call(13)).snapshot();
            require(playing.playing()&&!playing.dirty()&&playing.revision()==initial.revision()&&playing.selection().equals(initial.selection()),"Play changed authoring/selection/savepoint");
            nativeReject(bridge,"already_playing",13);
            require(!result(bridge.call(14)).snapshot().playing(),"Stop did not stop actual session");
            require(!result(bridge.call(14)).snapshot().playing(),"Repeated Stop is not idempotent");
            var renamed=result(bridge.call(5,Long.toUnsignedString(doc.id()),"Сцена F0"));
            nativeReject(bridge,"folder_dirty",12,clone.toString());require(!Files.exists(clone),"Rejected dirty clone created output");
            try{result(bridge.callAt(opened.generation(),initial.revision(),13));throw new IllegalStateException("Stale Play accepted");}
            catch(ExecutionException expected){require(expected.getCause().getMessage().contains("stale_revision"),"Wrong stale Play rejection");}
            var dirtyPlay=result(bridge.call(13)).snapshot();
            require(dirtyPlay.playing()&&dirtyPlay.dirty()&&dirtyPlay.revision()==renamed.snapshot().revision(),"Play lost dirty authoring/history");
            require(result(bridge.call(14)).snapshot().dirty(),"Stop discarded unsaved authoring");
            result(bridge.call(6));Map<String,String> committed=hashes(project);
            nativeReject(bridge,"folder_clone",12,project.resolve("nested").toString());
            var copied=result(bridge.call(12,clone.toString()));
            require(copied.snapshot().project()==initial.project()&&copied.snapshot().path().equals(initial.path()),"Clone switched active Workspace");
            Map<String,String> cloneBytes=hashes(clone);
            nativeReject(bridge,"folder_exists",12,clone.toString());
            require(cloneBytes.equals(hashes(clone))&&committed.equals(hashes(project)),"Rejected clone overwrote existing/source data");
            try(Bridge second=new Bridge(executable,directory,state->{ })){
                result(second.connect());var copy=result(second.call(2,clone.resolve("Project.epidemicproj").toString()));
                require(copy.snapshot().project()==initial.project()&&copy.snapshot().documents().getFirst().id()==doc.id()&&copy.snapshot().documents().getFirst().name().equals("Сцена F0"),"Cross-process clone identity/committed state lost");
                result(second.call(5,Long.toUnsignedString(doc.id()),"Только копия"));result(second.call(6));result(second.call(10));result(second.call(11));
            }
            require(committed.equals(hashes(project)),"Editing independent clone changed the source project");
            result(bridge.call(13));require(result(bridge.call(10)).snapshot()==null,"Close while playing retained native project");
            var reopened=result(bridge.call(2,project.resolve("Project.epidemicproj").toString()));
            require(!reopened.snapshot().playing()&&!reopened.snapshot().dirty()&&reopened.snapshot().documents().getFirst().name().equals("Сцена F0"),"Reopen retained runtime or lost authoring");
            try{result(bridge.callAt(opened.generation(),reopened.snapshot().revision(),13));throw new IllegalStateException("Old-generation Play accepted");}
            catch(ExecutionException expected){require(expected.getCause().getMessage().contains("stale_generation"),"Wrong old-generation rejection");}
            result(bridge.call(10));result(bridge.call(11));
        }
        System.out.println("PASS F0 actual clone/no-overwrite/independent process, Play/Stop/dirty isolation, stale/arity/no-project rejection and close during Play");
    }
    static void documents(Path executable, Path directory) throws Exception {
        Path folder=directory.resolve("Документы-"+System.nanoTime());
        List<Bridge.Document> saved;
        try(Bridge bridge=new Bridge(executable,directory,state->{ })) {
            result(bridge.connect());
            nativeReject(bridge,"no_project",15,"0","Scene");
            result(bridge.call(3,folder.toString(),"Document workflow"));
            var baseline=bridge.latest();
            nativeReject(bridge,"argument_count",15,"0");
            nativeReject(bridge,"document_kind",15,"3","Wrong kind");
            nativeReject(bridge,"document_name",15,"0","");
            nativeReject(bridge,"document_name",15,"0","bad\nname");
            nativeReject(bridge,"document_name",15,"0","я".repeat(129));
            require(bridge.latest().snapshot().equals(baseline.snapshot()),"Rejected create mutated snapshot");
            Set<Long> ids=new HashSet<>();ids.add(baseline.snapshot().documents().getFirst().id());
            for(int kind=0;kind<3;kind++) {
                var before=bridge.latest();
                var created=result(bridge.call(15,Integer.toString(kind),"Документ "+kind));
                long id=created.snapshot().selection().document();
                var doc=created.snapshot().documents().stream().filter(d->d.id()==id).findFirst().orElseThrow();
                require(id!=0&&ids.add(id)&&doc.kind()==kind&&created.snapshot().dirty()&&doc.revision()>0,"Create identity/kind/revision");
                require(created.snapshot().revision()>before.snapshot().revision(),"Create did not advance revision");
                result(bridge.call(7));
                require(bridge.latest().snapshot().documents().stream().noneMatch(d->d.id()==id),"Undo retained document");
                require(result(bridge.call(8)).snapshot().documents().contains(doc),"Redo changed document identity");
                try { result(bridge.callAt(before.generation(),before.snapshot().revision(),15,"0","Stale"));throw new IllegalStateException("Stale create accepted"); }
                catch(ExecutionException expected){require(expected.getCause().getMessage().contains("stale_revision"),"Wrong stale create diagnostic");}
                var selected=result(bridge.call(9,"0",Long.toUnsignedString(id),"0"));
                var renamed=result(bridge.call(5,Long.toUnsignedString(id),"Имя "+kind));
                require(renamed.snapshot().selection().document()==id&&renamed.snapshot().revision()>selected.snapshot().revision(),"Rename lost selection or revision");
            }
            nativeReject(bridge,"document_name",5,Long.toUnsignedString(ids.iterator().next())," ");
            result(bridge.call(6));saved=bridge.latest().snapshot().documents();
            require(!bridge.latest().snapshot().dirty(),"Save All did not clear dirty");
            result(bridge.call(13));
            nativeReject(bridge,"playing_readonly",15,"0","During Play");
            nativeReject(bridge,"playing_readonly",5,Long.toUnsignedString(saved.getFirst().id()),"During Play");
            result(bridge.call(14));result(bridge.call(10));result(bridge.call(11));
        }
        try(Bridge reopened=new Bridge(executable,directory,state->{ })) {
            result(reopened.connect());var reply=result(reopened.call(2,folder.resolve("Project.epidemicproj").toString()));
            // Revisions are session-local optimistic concurrency tokens. The
            // native archive rebuilds them on Open; persisted identities/names
            // and kinds must survive unchanged, not the old session's tokens.
            require(reply.snapshot().documents().size()==saved.size()&&!reply.snapshot().dirty(),"Separate-process Save/Open lost documents/savepoint");
            for(var doc:saved)require(reply.snapshot().documents().stream().anyMatch(d->d.id()==doc.id()&&d.kind()==doc.kind()&&d.name().equals(doc.name())&&d.revision()>0),"Separate-process Save/Open lost ID/kind/name or valid revision");
            var first=saved.getFirst();var key=new Bridge.DocumentKey(reply.generation(),reply.snapshot().project(),first.kind(),first.id());
            require(key.resolve(reply.generation(),reply.snapshot()).isPresent(),"Document key lost live native identity");
            require(key.resolve(reply.generation()+1,reply.snapshot()).isEmpty(),"Old generation key resolved");
            require(new Bridge.DocumentKey(reply.generation(),reply.snapshot().project()+1,first.kind(),first.id()).resolve(reply.generation(),reply.snapshot()).isEmpty(),"Foreign project key resolved");
            require(new Bridge.DocumentKey(reply.generation(),reply.snapshot().project(),(first.kind()+1)%3,first.id()).resolve(reply.generation(),reply.snapshot()).isEmpty(),"Wrong kind key resolved");
            require(new Bridge.DocumentKey(reply.generation(),reply.snapshot().project(),first.kind(),0).resolve(reply.generation(),reply.snapshot()).isEmpty(),"Missing ID key resolved");
            long id=saved.getLast().id(),revision=reply.snapshot().revision();
            result(reopened.call(9,"0",Long.toUnsignedString(id),"0"));
            require(reopened.latest().snapshot().revision()==revision&&!reopened.latest().snapshot().dirty(),"Selection dirtied document");
            result(reopened.call(10));result(reopened.call(11));
        }
        System.out.println("PASS F0.2 Scene/Prefab/Graph creation, typed IDs, revision, invalid/stale/Play rejection, Undo/Redo, Rename, separate-process Save/Open");
    }
    static javax.swing.JTree explorerTree(java.awt.Container component) {
        if(component instanceof javax.swing.JTree tree)return tree;
        for(var child:component.getComponents())if(child instanceof java.awt.Container container){
            var tree=explorerTree(container);if(tree!=null)return tree;
        }
        return null;
    }
    static void documentReaders() throws Exception {
        javax.swing.SwingUtilities.invokeAndWait(()->{
            Catalog catalog=new Catalog();FeatureRegistry features=new FeatureRegistry();int[] selections={0};
            var original=new Bridge.Snapshot(1,1,1,0,2,1,8,"test","Project",new Bridge.Selection(0,8,0),
                List.of(new Bridge.Document(8,1,0,"Original")),List.of(),List.of());
            var renamed=new Bridge.Snapshot(2,1,1,1,2,1,8,"test","Project",new Bridge.Selection(0,8,0),
                List.of(new Bridge.Document(8,2,0,"Renamed")),List.of(),List.of());
            for(int view=0;view<2;view++){
                UiPanels reader=new UiPanels(catalog,features,d->{},s->selections[0]++,()->{},()->{},k->{},()->{});
                reader.refresh(original);var tree=explorerTree(reader.view("project.explorer"));
                require(tree!=null&&tree.getSelectionPath()!=null,"Explorer did not project native selection");
                var documents=tree.getSelectionPath().getParentPath();tree.collapsePath(documents);
                reader.refresh(renamed);
                require(tree.getSelectionPath()!=null&&tree.getSelectionPath().getLastPathComponent().toString().startsWith("Renamed"),"Rename lost highlighted native document");
                require(!tree.isExpanded(tree.getSelectionPath().getParentPath()),"Rename reset user's collapsed document folder");
            }
            require(selections[0]==0,"Snapshot refresh emitted a selection command");
            var selected=new java.util.concurrent.atomic.AtomicReference<Bridge.Selection>();
            UiPanels projection=new UiPanels(catalog,features,d->{},selected::set,()->{},()->{},k->{},()->{});
            projection.refresh(original);var tree=explorerTree(projection.view("project.explorer"));
            tree.setSelectionPath(new javax.swing.tree.TreePath(tree.getModel().getRoot()));
            require(new Bridge.Selection(0,0,0).equals(selected.get()),"Project root did not clear native object selection");
        });
        System.out.println("PASS F0.2 two immutable Explorer readers retain selected ID and collapsed folders on native rename without writing selection");
    }
    static void explorerInteractions()throws Exception {
        javax.swing.SwingUtilities.invokeAndWait(()->{
            try{
                var actions=new java.util.ArrayList<UiPanels.ExplorerEdit>();
                var state=new Bridge.Snapshot(1,1,1,0,2,1,8,"C:/Fixture/Project.epidemicproj","Project",new Bridge.Selection(0,8,0),List.of(new Bridge.Document(8,1,0,"One"),new Bridge.Document(9,1,2,"Two")),List.of(),List.of());
                var panels=new UiPanels(new Catalog(),new FeatureRegistry(),d->{},selection->{},()->{},()->{},kind->{},()->{});
                panels.sessionState(1,true);panels.documentCommandsEnabled(true);
                panels.explorerCommands(edit->{actions.add(edit);return java.util.concurrent.CompletableFuture.completedFuture(null);});panels.refresh(state);
                var view=panels.view("project.explorer");var tree=explorerTree(view);var root=(javax.swing.tree.DefaultMutableTreeNode)tree.getModel().getRoot();var docs=(javax.swing.tree.DefaultMutableTreeNode)root.getChildAt(0);
                tree.setSelectionPaths(new javax.swing.tree.TreePath[]{new javax.swing.tree.TreePath(((javax.swing.tree.DefaultMutableTreeNode)docs.getChildAt(0)).getPath()),new javax.swing.tree.TreePath(((javax.swing.tree.DefaultMutableTreeNode)docs.getChildAt(1)).getPath())});
                tree.getActionMap().get("explorer.copy").actionPerformed(new java.awt.event.ActionEvent(tree,0,"copy"));require(actions.size()==1&&actions.getFirst().items().size()==2,"Explorer Ctrl+C lost multiselection");
                panels.refresh(state,true);require(tree.getSelectionCount()==2,"Native primary selection acknowledgement collapsed multiselection");
                var nativePath=tree.getSelectionPaths()[0];Object before=nativePath.getLastPathComponent();String label=before.toString();tree.getModel().valueForPathChanged(nativePath,"Переименовано");require(actions.getLast().action().equals("rename")&&actions.getLast().target().equals("Переименовано")&&before.toString().equals(label),"Inline rename mutated the projection before native acknowledgement");
                var method=UiPanels.class.getDeclaredMethod("explorerMenu",java.util.List.class,String.class);method.setAccessible(true);
                var menu=(javax.swing.JPopupMenu)method.invoke(panels,List.of(new UiPanels.ExplorerItem("folder",0,"Documents/Nested")),"Documents/Nested");
                var create=(javax.swing.JMenu)menu.getComponent(0);create.getItem(2).doClick();require(actions.getLast().action().equals("new:2")&&actions.getLast().target().equals("Documents/Nested"),"Context New lost destination folder");
            }catch(Exception error){throw new RuntimeException(error);}
        });System.out.println("PASS Explorer multiselection/hotkey, primary acknowledgement and context New folder target");
    }
    static Bridge.Field field(Bridge bridge,int kind) {
        return bridge.latest().snapshot().fields().stream().filter(f->f.kind()==kind&&f.editable()).findFirst().orElseThrow();
    }
    static Bridge.Reply edit(Bridge bridge,Bridge.Field field,String text) throws Exception {
        return result(bridge.call(16,Long.toUnsignedString(bridge.latest().snapshot().selectionRevision()),field.key(),text));
    }
    static String value(Bridge bridge,String key) {
        return bridge.latest().snapshot().fields().stream().filter(f->f.key().equals(key)).findFirst().orElseThrow().value();
    }
    static void scalarAndDiscard(Path executable,Path directory) throws Exception {
        Path folder=directory.resolve("F0-scalars-"+System.nanoTime());long prefab,root,oldGeneration;Map<String,String> stored=new HashMap<>();
        try(Bridge bridge=new Bridge(executable,directory,state->{})) {
            result(bridge.connect());nativeReject(bridge,"no_project",16,"0","missing","1");
            result(bridge.call(3,folder.toString(),"Scalar contract"));
            long scene=bridge.latest().snapshot().entry();
            nativeReject(bridge,"prefab_template",15,"0","Wrong","registered-components");
            var created=result(bridge.call(15,"1","Player","registered-components"));prefab=created.snapshot().selection().document();
            root=created.snapshot().nodes().stream().filter(n->n.document()==prefab&&n.kind()==2).findFirst().orElseThrow().id();
            result(bridge.call(6));
            result(bridge.call(9,"2",Long.toUnsignedString(prefab),Long.toUnsignedString(root)));
            require(bridge.latest().snapshot().fields().size()==5&&bridge.latest().snapshot().fields().stream().anyMatch(f->f.name().endsWith("/ maximum")&&f.editable()&&f.value().equals("100"))&&!bridge.latest().snapshot().dirty(),"Real scalar schema/defaults or selection dirty");
            String[] values={"123","1.2345678901234567","false","Игрок Ω"};
            for(int kind=0;kind<4;kind++) {
                var f=field(bridge,kind);String before=f.value();long revision=bridge.latest().snapshot().revision();
                var changed=edit(bridge,f,values[kind]);
                require(changed.snapshot().revision()==revision+1&&value(bridge,f.key()).equals(values[kind]),"Scalar did not commit one exact transaction");
                result(bridge.call(7));require(value(bridge,f.key()).equals(before),"Scalar Undo failed");
                result(bridge.call(8));require(value(bridge,f.key()).equals(values[kind]),"Scalar Redo failed");
                stored.put(f.key(),values[kind]);
            }
            var i=field(bridge,0);var d=field(bridge,1);var b=field(bridge,2);var text=field(bridge,3);
            long revision=bridge.latest().snapshot().revision(),selection=bridge.latest().snapshot().selectionRevision();
            for(String invalid:List.of("9223372036854775808","1.5"))nativeReject(bridge,"invalid_value",16,Long.toUnsignedString(selection),i.key(),invalid);
            nativeReject(bridge,"component.value_rejected",16,Long.toUnsignedString(selection),i.key(),"-1");
            for(String invalid:List.of("NaN","Inf","1e999","x"))nativeReject(bridge,"invalid_value",16,Long.toUnsignedString(selection),d.key(),invalid);
            nativeReject(bridge,"invalid_value",16,Long.toUnsignedString(selection),b.key(),"1");
            nativeReject(bridge,"inspector_string",16,Long.toUnsignedString(selection),text.key(),"bad\nline");
            nativeReject(bridge,"invalid_utf8_text",16,Long.toUnsignedString(selection),text.key(),String.valueOf((char)0xd800));
            nativeReject(bridge,"string_limit",16,Long.toUnsignedString(selection),text.key(),"я".repeat(600000));
            nativeReject(bridge,"string_limit",16,Long.toUnsignedString(selection),text.key(),"x".repeat(1024*1024+1));
            require(result(bridge.call(4)).snapshot().revision()==revision,"Rejected local encoding damaged the IPC stream or authoring state");
            nativeReject(bridge,"field_missing",16,Long.toUnsignedString(selection),"missing","1");
            require(bridge.latest().snapshot().revision()==revision,"Rejected scalar edits mutated project");
            var component=bridge.latest().snapshot().nodes().stream().filter(n->n.document()==prefab&&n.kind()==5).findFirst().orElseThrow();
            result(bridge.call(9,"5",Long.toUnsignedString(prefab),Long.toUnsignedString(root),Long.toUnsignedString(component.component()),Long.toUnsignedString(component.part())));
            require(bridge.latest().snapshot().selection().component()==component.component(),"Component selection lost type/part identity");
            nativeReject(bridge,"stale_selection",16,Long.toUnsignedString(selection),i.key(),"5");
            nativeReject(bridge,"selection_missing",9,"5",Long.toUnsignedString(prefab),Long.toUnsignedString(root),"1","0");
            result(bridge.call(9,"2",Long.toUnsignedString(prefab),Long.toUnsignedString(root)));
            result(bridge.call(5,Long.toUnsignedString(scene),"Unsaved scene"));
            result(bridge.call(17,Long.toUnsignedString(scene)));
            require(bridge.latest().snapshot().dirty()&&bridge.latest().snapshot().documents().stream().anyMatch(doc->doc.id()==prefab&&doc.dirty())&&bridge.latest().snapshot().documents().stream().noneMatch(doc->doc.id()==scene&&doc.dirty()),"Discard affected other document dirty state");
            result(bridge.call(7));require(bridge.latest().snapshot().documents().stream().anyMatch(doc->doc.id()==scene&&doc.name().equals("Unsaved scene")),"Discard Undo failed");
            result(bridge.call(8));result(bridge.call(6));
            var bytes=hashes(folder);result(bridge.call(6));require(bytes.equals(hashes(folder)),"No-op Save rewrote documents");
            edit(bridge,field(bridge,3),"Discard me");result(bridge.call(17,Long.toUnsignedString(prefab)));
            require(!bridge.latest().snapshot().dirty()&&value(bridge,text.key()).equals(values[3]),"Discard last dirty document did not restore exact saved value/savepoint");
            result(bridge.call(15,"2","Unsaved graph"));long draft=bridge.latest().snapshot().selection().document();
            result(bridge.call(17,Long.toUnsignedString(draft)));require(!bridge.latest().snapshot().dirty()&&bridge.latest().snapshot().documents().stream().noneMatch(doc->doc.id()==draft),"Discard new document retained draft/dirty");
            result(bridge.call(7));require(bridge.latest().snapshot().documents().stream().anyMatch(doc->doc.id()==draft),"New document discard Undo lost stable ID");result(bridge.call(8));
            nativeReject(bridge,"command_target",17,"1");
            result(bridge.call(13));nativeReject(bridge,"playing_readonly",17,Long.toUnsignedString(prefab));
            result(bridge.call(6));result(bridge.call(14));require(bytes.equals(hashes(folder)),"Play/Save serialized runtime state");
            oldGeneration=bridge.latest().generation();result(bridge.call(10));result(bridge.call(11));
        }
        try(Bridge bridge=new Bridge(executable,directory,state->{})) {
            result(bridge.connect());var reopened=result(bridge.call(2,folder.resolve("Project.epidemicproj").toString()));
            require(reopened.generation()!=oldGeneration,"Backend restart reused generation");
            try { result(bridge.callAt(oldGeneration,reopened.snapshot().revision(),17,Long.toUnsignedString(prefab)));throw new IllegalStateException("Previous-process edit accepted"); }
            catch(ExecutionException expected){require(expected.getCause().getMessage().contains("stale_generation"),"Wrong cross-process stale diagnostic");}
            result(bridge.call(9,"2",Long.toUnsignedString(prefab),Long.toUnsignedString(root)));
            for(var item:stored.entrySet())require(value(bridge,item.getKey()).equals(item.getValue()),"Cross-process scalar value/ID lost");
            require(!bridge.latest().snapshot().dirty(),"Reopen/selection became dirty");result(bridge.call(10));result(bridge.call(11));
        }
        System.out.println("PASS F0 typed Selection, scalar exact commits/history/reopen, stale/type/range errors, isolated document discard/history/savepoint, Play isolation");
    }
    static javax.swing.JComponent named(java.awt.Container root,String key) {
        if(root instanceof javax.swing.JComponent component&&key.equals(component.getName()))return component;
        for(var child:root.getComponents())if(child instanceof java.awt.Container container){var found=named(container,key);if(found!=null)return found;}
        return null;
    }
    static void scalarWidgets(Path executable,Path directory) throws Exception {
        var readers=new java.util.concurrent.atomic.AtomicReference<List<ScalarInspector>>(List.of());
        var pending=new java.util.concurrent.atomic.AtomicReference<CompletableFuture<Bridge.Reply>>();
        try(Bridge bridge=new Bridge(executable,directory,state->{
            if(state.reply()!=null)for(var reader:readers.get())reader.refresh(state.reply().generation(),state.reply().snapshot(),state.service()==Bridge.Service.Ready);
        })) {
            result(bridge.connect());result(bridge.call(3,directory.resolve("widget-"+System.nanoTime()).toString(),"Widget regression"));
            result(bridge.call(15,"1","Player","registered-components"));long prefab=bridge.latest().snapshot().selection().document();
            var node=bridge.latest().snapshot().nodes().stream().filter(n->n.document()==prefab&&n.kind()==2).findFirst().orElseThrow();
            result(bridge.call(9,"2",Long.toUnsignedString(prefab),Long.toUnsignedString(node.id())));
            javax.swing.SwingUtilities.invokeAndWait(()->{
                java.util.function.Function<ScalarInspector.Edit,CompletableFuture<Bridge.Reply>> send=edit->{
                    var operation=bridge.callAt(edit.generation(),edit.revision(),16,Long.toUnsignedString(edit.selectionRevision()),edit.key(),edit.value());pending.set(operation);return operation;
                };
                readers.set(List.of(new ScalarInspector(send),new ScalarInspector(send)));
                for(var reader:readers.get())reader.refresh(bridge.latest().generation(),bridge.latest().snapshot(),true);
            });
            var text=field(bridge,3);long before=bridge.latest().snapshot().revision();
            javax.swing.SwingUtilities.invokeAndWait(()->{
                var input=(javax.swing.JTextField)named(readers.get().getFirst(),text.key());input.setText("Cancelled draft");
                input.getActionMap().get("native.cancel").actionPerformed(new java.awt.event.ActionEvent(input,0,"escape"));
                require(input.getText().equals(text.value()),"Escape did not restore native scalar");
            });
            require(pending.get()==null&&bridge.latest().snapshot().revision()==before,"Escape mutated native authoring");
            javax.swing.SwingUtilities.invokeAndWait(()->{var input=(javax.swing.JTextField)named(readers.get().getFirst(),text.key());input.setText("UI Кириллица");input.postActionEvent();});
            result(pending.get());javax.swing.SwingUtilities.invokeAndWait(()->{});javax.swing.SwingUtilities.invokeAndWait(()->{
                require(((javax.swing.JTextField)named(readers.get().get(1),text.key())).getText().equals("UI Кириллица"),"Second Inspector did not receive shared native commit");
            });
            javax.swing.SwingUtilities.invokeAndWait(()->((javax.swing.JTextField)named(readers.get().getFirst(),text.key())).setText("Pending second edit"));
            result(bridge.call(4));javax.swing.SwingUtilities.invokeAndWait(()->{});javax.swing.SwingUtilities.invokeAndWait(()->{
                var input=(javax.swing.JTextField)named(readers.get().getFirst(),text.key());require(input.getText().equals("Pending second edit"),"Native refresh overwrote an uncommitted second edit");
                input.getActionMap().get("native.cancel").actionPerformed(new java.awt.event.ActionEvent(input,0,"escape"));
            });
            var decimal=field(bridge,1);
            javax.swing.SwingUtilities.invokeAndWait(()->{var input=(javax.swing.JTextField)named(readers.get().getFirst(),decimal.key());input.setText("NaN");input.postActionEvent();});
            try{result(pending.get());throw new IllegalStateException("Inspector accepted NaN");}catch(ExecutionException expected){require(expected.getCause().getMessage().contains("invalid_value"),"Wrong widget error");}
            javax.swing.SwingUtilities.invokeAndWait(()->{});javax.swing.SwingUtilities.invokeAndWait(()->{
                var input=(javax.swing.JTextField)named(readers.get().getFirst(),decimal.key());
                require(input.getText().equals("NaN")&&((javax.swing.JTextField)named(readers.get().get(1),decimal.key())).getText().equals(decimal.value()),"Failed draft overwrote native/other Inspector");
                input.getActionMap().get("native.cancel").actionPerformed(new java.awt.event.ActionEvent(input,0,"escape"));
            });
            javax.swing.SwingUtilities.invokeAndWait(()->{
                var input=(javax.swing.JTextField)named(readers.get().getFirst(),text.key());
                input.setText("Stale UI draft");
            });
            result(bridge.call(5,Long.toUnsignedString(bridge.latest().snapshot().entry()),"Concurrent scene edit"));javax.swing.SwingUtilities.invokeAndWait(()->{});
            javax.swing.SwingUtilities.invokeAndWait(()->((javax.swing.JTextField)named(readers.get().getFirst(),text.key())).postActionEvent());
            try{result(pending.get());throw new IllegalStateException("Stale UI draft accepted");}catch(ExecutionException expected){require(expected.getCause().getMessage().contains("stale_revision"),"Wrong stale widget diagnostic");}
            javax.swing.SwingUtilities.invokeAndWait(()->{});javax.swing.SwingUtilities.invokeAndWait(()->{
                var input=(javax.swing.JTextField)named(readers.get().getFirst(),text.key());input.getActionMap().get("native.cancel").actionPerformed(new java.awt.event.ActionEvent(input,0,"escape"));
                var previous=pending.get();readers.get().getFirst().refresh(bridge.latest().generation()+1,bridge.latest().snapshot(),true);input.setText("Old generation control");input.postActionEvent();
                require(pending.get()==previous,"Detached old-generation control sent a write");readers.get().getFirst().refresh(bridge.latest().generation(),bridge.latest().snapshot(),true);
            });
            var flushing=new java.util.concurrent.atomic.AtomicReference<CompletableFuture<Void>>();
            javax.swing.SwingUtilities.invokeAndWait(()->{
                ((javax.swing.JTextField)named(readers.get().getFirst(),text.key())).setText("Saved draft");flushing.set(readers.get().getFirst().flush());
            });
            flushing.get().get(20,TimeUnit.SECONDS);result(bridge.call(6));
            require(value(bridge,text.key()).equals("Saved draft")&&!bridge.latest().snapshot().dirty(),"Save flush lost pending native scalar");
            result(bridge.call(10));result(bridge.call(11));
        }
        System.out.println("PASS F0 real scalar widgets: Escape/no write, Enter/native commit, shared second view, inline invalid draft isolation, flush-before-Save");
    }
    public static void main(String[] args) throws Exception {
        Path directory = Path.of(args[0]); Files.createDirectories(directory);
        Path project = directory.resolve("Game-" + System.nanoTime());
        Path executable = Path.of(System.getProperty("epidemic.ui.backend"));
        presentation(); documentReaders(); explorerInteractions(); recent(directory); compatibility(executable,directory);lifecycle(executable,directory);documents(executable,directory);scalarAndDiscard(executable,directory);scalarWidgets(executable,directory);AssetTests.run(executable,directory);
        try (Bridge bridge = new Bridge(executable, directory, state -> { })) {
            require(result(bridge.connect()).snapshot() == null, "Handshake created a project");
            var opened = result(bridge.call(3, project.toString(), "Новая игра"));
            require(Files.isRegularFile(project.resolve("Project.epidemicproj")), "Descriptor absent");
            for (String name : new String[]{"Content","ProjectData","Scripts",".epidemic"})
                require(Files.isDirectory(project.resolve(name)), "Missing project directory " + name);
            require(opened.snapshot().documents().size() == 1 && !opened.snapshot().dirty(), "New folder snapshot");
            var document = opened.snapshot().documents().getFirst();
            var renamed = result(bridge.call(5, Long.toUnsignedString(document.id()), "Сцена"));
            require(renamed.snapshot().dirty(), "Rename did not use authoring history");
            try { result(bridge.call(10)); throw new IllegalStateException("Dirty project closed silently"); }
            catch(ExecutionException expected){require(expected.getCause().getMessage().contains("unsaved"),"Wrong dirty-close failure");}
            try {
                result(bridge.callAt(opened.generation(), opened.snapshot().revision(), 5,
                                     Long.toUnsignedString(document.id()), "Stale"));
                throw new IllegalStateException("Stale revision accepted");
            } catch (ExecutionException expected) {
                require(expected.getCause().getMessage().contains("stale_revision"), "Wrong stale error");
            }
            require(result(bridge.call(4)).snapshot().documents().getFirst().name().equals("Сцена"), "Rejected edit mutated data");
            require(!result(bridge.call(7)).snapshot().dirty(), "Undo savepoint lost");
            require(result(bridge.call(8)).snapshot().dirty(), "Redo lost");
            result(bridge.call(6));
            try (Bridge second = new Bridge(executable, directory, state -> { })) {
                result(second.connect());
                try {
                    result(second.call(2, project.resolve("Project.epidemicproj").toString()));
                    throw new IllegalStateException("Second writer opened the project");
                } catch (ExecutionException expected) {
                    require(expected.getCause().getMessage().contains("lock"), "Unexpected lock failure");
                }
            }
            result(bridge.call(10));
            var reopened = result(bridge.call(2, project.resolve("Project.epidemicproj").toString()));
            try { result(bridge.callAt(opened.generation(), reopened.snapshot().revision(), 5, Long.toUnsignedString(document.id()),"Old generation")); throw new IllegalStateException("Old workspace completion accepted"); }
            catch(ExecutionException expected){require(expected.getCause().getMessage().contains("stale_generation"),"Wrong generation failure");}
            require(reopened.snapshot().project() == opened.snapshot().project(), "Stable project ID changed");
            require(reopened.snapshot().documents().getFirst().name().equals("Сцена"), "Reopen lost Save");
            result(bridge.call(9, "0", Long.toUnsignedString(document.id()), "0"));
            require(bridge.latest().snapshot().selection().document() == document.id(), "Selection lost typed ID");
            List<CompletableFuture<Bridge.Reply>> pending=new ArrayList<>();
            for(int i=0;i<1000;i++)pending.add(bridge.call(4));
            int cancelled=bridge.cancelQueued(), full=0;
            javax.swing.SwingUtilities.invokeAndWait(()->{ }); // EDT can process work while the IO queue drains.
            for(var future:pending)try{result(future);}catch(CancellationException expected){ }
            catch(ExecutionException expected){require(expected.getCause().getMessage().contains("queue_full"),"Unexpected queued failure");++full;}
            require(cancelled+full>0,"Bounded backpressure/cancellation not exercised");
            long[] latency=new long[30];for(int i=0;i<latency.length;i++)latency[i]=result(bridge.call(4)).latencyNanos();Arrays.sort(latency);
            System.out.printf(Locale.ROOT,"PASS U2 bounded IO/EDT heartbeat: cancelled=%d rejected=%d roundtrip p50=%.3fms p95=%.3fms%n",cancelled,full,latency[15]/1e6,latency[28]/1e6);
            result(bridge.call(10)); result(bridge.call(11));
        }
        try(Bridge crashed=new Bridge(executable,directory,state->{ })){
            result(crashed.connect());result(crashed.call(2,project.resolve("Project.epidemicproj").toString()));
            ProcessHandle child=ProcessHandle.current().children().filter(p->p.info().command().map(c->Path.of(c).equals(executable.toAbsolutePath())).orElse(false)).findFirst().orElseThrow();
            child.destroyForcibly();child.onExit().get(5,TimeUnit.SECONDS);
            try{result(crashed.call(4));throw new IllegalStateException("Disconnected child returned success");}catch(ExecutionException expected){ }
        }
        try(Bridge reconnected=new Bridge(executable,directory,state->{ })){
            result(reconnected.connect());result(reconnected.call(2,project.resolve("Project.epidemicproj").toString()));result(reconnected.call(10));result(reconnected.call(11));
        }
        System.out.println("PASS U2 crash/disconnect/reconnect released native writer ownership");
        System.out.println("PASS U2/U3 native handshake, Unicode New/Save/reopen, history, stale rejection, lock, selection, close");
    }
}

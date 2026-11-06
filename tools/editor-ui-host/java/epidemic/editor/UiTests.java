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
    public static void main(String[] args) throws Exception {
        Path directory = Path.of(args[0]); Files.createDirectories(directory);
        Path project = directory.resolve("Game-" + System.nanoTime());
        Path executable = Path.of(System.getProperty("epidemic.ui.backend"));
        presentation(); recent(directory); compatibility(executable,directory);lifecycle(executable,directory);
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

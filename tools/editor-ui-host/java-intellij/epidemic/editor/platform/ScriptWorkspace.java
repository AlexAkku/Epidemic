package epidemic.editor.platform;

import com.intellij.openapi.application.ApplicationManager;
import com.intellij.openapi.command.WriteCommandAction;
import com.intellij.openapi.editor.Document;
import com.intellij.openapi.fileEditor.*;
import com.intellij.openapi.project.Project;
import com.intellij.openapi.util.Key;
import com.intellij.openapi.vfs.*;
import epidemic.editor.Bridge;
import java.awt.BorderLayout;
import java.nio.file.*;
import java.nio.charset.StandardCharsets;
import java.security.MessageDigest;
import java.io.*;
import java.util.*;
import java.util.concurrent.*;
import java.util.function.*;
import java.util.regex.*;
import javax.swing.*;

/** Real source files and Platform Documents. No copy of source text in the native model. */
public final class ScriptWorkspace implements AutoCloseable {
    static final Key<ScriptWorkspace> OWNER=Key.create("epidemic.script.owner");
    private record Source(VirtualFile file,Document document,Path path,byte[] fingerprint){}
    public record Diagnostic(Path file,int line,int column,String message){public String toString(){return file.getFileName()+":"+line+":"+column+" "+message;}}
    private final Project project;
    private final Supplier<Path> root;
    private final BiFunction<String,Path,CompletableFuture<Bridge.Reply>> register;
    private final Consumer<String> problems;
    private final Supplier<Bridge.Snapshot> snapshot;
    private final Map<VirtualFile,Source> sources=new LinkedHashMap<>();
    private final java.util.prefs.Preferences preferences=java.util.prefs.Preferences.userRoot().node("epidemic/editor/code");
    private final ExecutorService background=Executors.newSingleThreadExecutor(r->{var t=new Thread(r,"Epidemic script build");t.setDaemon(true);return t;});
    private final List<JLabel> statuses=new ArrayList<>();
    private final DefaultListModel<Path> files=new DefaultListModel<>();
    private final DefaultListModel<Diagnostic> diagnostics=new DefaultListModel<>();
    private volatile Process build;
    private volatile boolean cancelled,closed;
    private volatile long epoch;
    private volatile boolean building;
    private VirtualFile reloadAllowed;
    private Path trustedRoot,builtRoot;
    private boolean rebuildRequired=true;
    private Runnable filesChanged=()->{};
    void filesChanged(Runnable action){filesChanged=action;}
    List<Path> sourceFiles(){Path base=root.get();return base==null?List.of():Collections.list(files.elements()).stream().filter(p->p.toAbsolutePath().normalize().startsWith(base.toAbsolutePath().normalize().resolve("Scripts"))).toList();}
    private String status="C# offline highlighting · LSP is not installed";
    private static final Pattern ERROR=Pattern.compile("^(.+?)\\((\\d+),(\\d+)\\):\\s*(?:error|warning)\\s+(.+?)(?: \\[.*)?$");
    ScriptWorkspace(Project project,Supplier<Path> root,BiFunction<String,Path,CompletableFuture<Bridge.Reply>> register,Consumer<String> problems,Supplier<Bridge.Snapshot> snapshot){this.project=project;this.root=root;this.register=register;this.problems=problems;this.snapshot=snapshot;}
    private void ui(Runnable action){ApplicationManager.getApplication().invokeLater(()->{if(!closed&&!project.isDisposed())action.run();});}
    private void status(String value){status=value;for(var label:statuses)label.setText(value);}
    private void fail(Throwable error){String message=error.getMessage()==null?error.toString():error.getMessage();status(message);problems.accept(message);}
    private static byte[] hash(Path path)throws Exception{
        var digest=MessageDigest.getInstance("SHA-256");try(var stream=Files.newInputStream(path)){byte[] buffer=new byte[8192];int count;long total=0;while((count=stream.read(buffer))!=-1){total+=count;if(total>1024*1024)throw new IOException("Source exceeds 1 MiB");digest.update(buffer,0,count);}}return digest.digest();
    }
    private Map<Path,byte[]> sourceHashes(Path scripts)throws Exception{
        Map<Path,byte[]> result=new LinkedHashMap<>();try(var paths=Files.walk(scripts,12)){
            var entries=paths.limit(4097).toList();if(entries.size()>4096)throw new IOException("Scripts tree entry limit (4096)");
            for(Path path:entries){
                if(!path.toRealPath().startsWith(scripts))throw new IOException("Scripts path escapes its root: "+path);
                if(!path.toString().toLowerCase(Locale.ROOT).endsWith(".cs")||path.toString().contains(File.separator+"obj"+File.separator)||path.toString().contains(File.separator+"bin"+File.separator))continue;
                if(result.size()>=512)throw new IOException("Script file limit (512)");result.put(checked(path),hash(path));
            }
        }return result;
    }
    private void unchanged(Map<Path,byte[]> before,Path scripts)throws Exception{
        var after=sourceHashes(scripts);if(!after.keySet().equals(before.keySet()))throw new IOException("Source set changed during build; rebuild required");
        for(var entry:before.entrySet())if(!MessageDigest.isEqual(entry.getValue(),after.get(entry.getKey())))throw new IOException("Source changed during build; rebuild required: "+entry.getKey());
    }
    Path checked(Path path)throws IOException{
        Path base=root.get();if(base==null)throw new IOException("Open a project first");base=base.toRealPath();Path real=path.toRealPath();
        if(!real.startsWith(base.resolve("Scripts"))||!real.getFileName().toString().toLowerCase(Locale.ROOT).endsWith(".cs")||Files.size(real)>1024*1024)throw new IOException("C# source must be inside Scripts and at most 1 MiB");return real;
    }
    private Path managed(){return Path.of(System.getProperty("epidemic.ui.backend")).getParent().resolve("managed");}
    private JButton button(String name,Runnable action){var button=new JButton(name);button.addActionListener(e->{try{action.run();}catch(Exception error){fail(error);}});return button;}
    public JComponent panel(){
        var panel=new JPanel(new BorderLayout(6,6));var bar=new JPanel(new java.awt.FlowLayout(java.awt.FlowLayout.LEADING,6,4));
        bar.add(button("New C# Class…",this::newClass));bar.add(button("Open C#…",this::choose));bar.add(button("Refresh",this::refresh));bar.add(button("Build & Register",this::build));bar.add(button("Cancel Build",this::cancel));
        var label=new JLabel(status);statuses.add(label);var north=new JPanel(new BorderLayout());north.add(bar,BorderLayout.NORTH);north.add(label,BorderLayout.SOUTH);panel.add(north,BorderLayout.NORTH);
        var list=new JList<Path>(files);list.addMouseListener(new java.awt.event.MouseAdapter(){public void mouseClicked(java.awt.event.MouseEvent e){if(e.getClickCount()==2&&list.getSelectedValue()!=null)open(list.getSelectedValue(),1,false);}});
        var popup=new JPopupMenu();var with=new JMenuItem("Open With…");with.addActionListener(e->{if(list.getSelectedValue()!=null)open(list.getSelectedValue(),1,true);});popup.add(with);list.setComponentPopupMenu(popup);
        var errors=new JList<Diagnostic>(diagnostics);errors.addMouseListener(new java.awt.event.MouseAdapter(){public void mouseClicked(java.awt.event.MouseEvent e){if(e.getClickCount()==2&&errors.getSelectedValue()!=null){var d=errors.getSelectedValue();open(d.file(),d.line(),false);}}});
        panel.putClientProperty("epidemic.script.release",(Runnable)()->{list.setModel(new DefaultListModel<>());errors.setModel(new DefaultListModel<>());statuses.remove(label);});
        var split=new JSplitPane(JSplitPane.VERTICAL_SPLIT,new JScrollPane(list),new JScrollPane(errors));split.setResizeWeight(.75);panel.add(split);refresh();return panel;
    }
    void releasePanel(java.awt.Component component){if(component instanceof JComponent panel&&panel.getClientProperty("epidemic.script.release") instanceof Runnable release){panel.putClientProperty("epidemic.script.release",null);release.run();}if(component instanceof java.awt.Container container)for(var child:container.getComponents())releasePanel(child);}
    Object editorToken(VirtualFile file){return sources.get(file);}
    void editorReleased(VirtualFile file,Object token){ui(()->{var source=sources.get(file);if(source!=token)return;if(FileEditorManager.getInstance(project).getAllEditors(file).length==0&&source!=null&&!FileDocumentManager.getInstance().isDocumentUnsaved(source.document())){sources.remove(file);file.putUserData(OWNER,null);}});}
    public void refresh(){files.clear();try{Path base=root.get();if(base!=null&&Files.isDirectory(base.resolve("Scripts")))try(var paths=Files.walk(base.resolve("Scripts"),12)){paths.filter(p->p.toString().endsWith(".cs")&&!p.toString().contains(File.separator+"obj"+File.separator)).sorted().limit(512).forEach(files::addElement);}}catch(Exception error){fail(error);}filesChanged.run();}
    static String template(String name){return "using Epidemic.ScriptApi;\n\n[ScriptComponent(\"user."+name+"\")]\npublic sealed class "+name+"\n{\n    [ScriptField] public long Damage { get; set; } = 25;\n    [ScriptField] public double Range { get; set; } = 150.0;\n    [ScriptField] public double FireRate { get; set; } = 2.0;\n    [ScriptField] public long Health { get; set; } = 100;\n    [ScriptField] public bool Enabled { get; set; } = true;\n    [ScriptField] public string Label { get; set; } = \""+name+"\";\n\n    [GraphOperation(\"user."+name+".ApplyDamage.i64.v1\")]\n    public long ApplyDamage(long amount)\n    {\n        Health = checked(Health - amount);\n        return Health;\n    }\n}\n";}
    public void newClass(){String name=JOptionPane.showInputDialog(null,"Class name (a C# identifier)","New C# Class",JOptionPane.PLAIN_MESSAGE);if(name==null)return;
        try{if(!name.matches("[\\p{L}_][\\p{L}\\p{N}_]{0,63}"))throw new IOException("Enter a C# class identifier");Path base=root.get().toRealPath();Path folder=base.resolve("Scripts");Files.createDirectories(folder);if(!folder.toRealPath().equals(base.resolve("Scripts")))throw new IOException("Scripts points outside project");Path file=folder.resolve(name+".cs");Files.writeString(file,template(name),StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW);refresh();open(file,1,false);}catch(Exception error){fail(error);}}
    public void choose(){var picker=new JFileChooser(root.get()==null?null:root.get().resolve("Scripts").toFile());picker.setFileFilter(new javax.swing.filechooser.FileNameExtensionFilter("C# source","cs"));if(picker.showOpenDialog(null)==JFileChooser.APPROVE_OPTION)open(picker.getSelectedFile().toPath(),1,false);}
    public void open(Path path,int line,boolean openWith){try{
        path=checked(path);String choice=preferences.get(projectKey(),preferences.get("choice","built-in"));
        if(openWith){String[] options={"Built-in Code Space","JetBrains Rider","Choose executable…"};var remember=new JCheckBox("Remember my choice");var projectOnly=new JCheckBox("For this project only");var form=new JPanel(new BorderLayout());form.add(new JLabel("Open "+path.getFileName()),BorderLayout.NORTH);var scope=new JPanel(new java.awt.GridLayout(0,1));scope.add(remember);scope.add(projectOnly);form.add(scope,BorderLayout.SOUTH);int selected=JOptionPane.showOptionDialog(null,form,"Open With",JOptionPane.DEFAULT_OPTION,JOptionPane.PLAIN_MESSAGE,null,options,options[0]);if(selected<0)return;choice=selected==0?"built-in":selected==1?"rider":"custom";if(selected==2){var picker=new JFileChooser();if(picker.showOpenDialog(null)!=JFileChooser.APPROVE_OPTION)return;preferences.put("custom",picker.getSelectedFile().getAbsolutePath());}if(remember.isSelected())preferences.put(projectOnly.isSelected()?projectKey():"choice",choice);}
        if(!choice.equals("built-in")){Path executable=choice.equals("rider")?rider():Path.of(preferences.get("custom",""));if(executable!=null&&Files.isRegularFile(executable)&&executable.toString().toLowerCase(Locale.ROOT).endsWith(".exe")){var args=new ArrayList<String>();args.add(executable.toString());if(choice.equals("rider"))args.addAll(List.of("--line",Integer.toString(Math.max(1,line)),"--column","1"));args.add(path.toString());try{new ProcessBuilder(args).start();return;}catch(IOException failed){problems.accept("External editor could not start: "+failed.getMessage());}}else problems.accept("External editor unavailable; opening built-in Code Space");}
        openBuiltIn(path,line);
    }catch(Exception error){fail(error);}}
    private void openBuiltIn(Path path,int line)throws Exception{
        VirtualFile file=LocalFileSystem.getInstance().refreshAndFindFileByNioFile(checked(path));if(file==null)throw new IOException("Cannot resolve source in VFS");file.putUserData(OWNER,this);Document document=FileDocumentManager.getInstance().getDocument(file);if(document==null)throw new IOException("Cannot create source Document");var previous=sources.get(file);if(previous==null||FileEditorManager.getInstance(project).getAllEditors(file).length==0)sources.put(file,new Source(file,document,path.toRealPath(),previous==null?hash(path):previous.fingerprint()));var opened=FileEditorManager.getInstance(project).openFile(file,true);
        for(var view:opened)if(view instanceof TextEditor text){var editor=text.getEditor();editor.getCaretModel().moveToLogicalPosition(new com.intellij.openapi.editor.LogicalPosition(Math.max(0,Math.min(line-1,document.getLineCount()-1)),0));editor.getScrollingModel().scrollToCaret(com.intellij.openapi.editor.ScrollType.CENTER);editor.getContentComponent().requestFocusInWindow();break;}
    }
    private String projectKey(){var state=snapshot.get();return "project."+(state==null?"none":Long.toUnsignedString(state.project()));}
    public void preferences(){
        String[] modes={"built-in","rider","custom"};var mode=new JComboBox<>(modes);mode.setSelectedItem(preferences.get(projectKey(),preferences.get("choice","built-in")));
        var path=new JTextField(preferences.get(mode.getSelectedItem().equals("rider")?"rider":"custom",""),40);var local=new JCheckBox("Override for this project only",preferences.get(projectKey(),null)!=null);
        var form=new JPanel(new java.awt.GridLayout(0,1,4,4));form.add(new JLabel("Default C# editor"));form.add(mode);form.add(new JLabel("Executable (machine-local; empty Rider path enables discovery)"));form.add(path);form.add(button("Choose executable…",()->{var picker=new JFileChooser();if(picker.showOpenDialog(null)==JFileChooser.APPROVE_OPTION)path.setText(picker.getSelectedFile().getAbsolutePath());}));form.add(local);
        mode.addActionListener(event->path.setText(preferences.get(mode.getSelectedItem().equals("rider")?"rider":"custom","")));
        String[] options={"Save","Use Global","Cancel"};int selected=JOptionPane.showOptionDialog(null,form,"Code Space Preferences",JOptionPane.DEFAULT_OPTION,JOptionPane.PLAIN_MESSAGE,null,options,options[0]);
        if(selected==1){preferences.remove(projectKey());return;}if(selected!=0)return;
        String choice=(String)mode.getSelectedItem();String executable=path.getText().strip();
        try{if(!choice.equals("built-in")&&!executable.isEmpty()){Path file=Path.of(executable).toRealPath();if(!Files.isRegularFile(file)||!file.toString().toLowerCase(Locale.ROOT).endsWith(".exe"))throw new IOException("Choose an existing .exe");preferences.put(choice,file.toString());}
            if(local.isSelected())preferences.put(projectKey(),choice);else{preferences.put("choice",choice);preferences.remove(projectKey());}
        }catch(Exception error){fail(error);}
    }
    private Path rider(){String configured=preferences.get("rider","");if(!configured.isBlank()&&Files.isRegularFile(Path.of(configured)))return Path.of(configured);
        for(String entry:System.getenv().getOrDefault("PATH","").split(File.pathSeparator)){try{Path candidate=Path.of(entry).resolve("rider64.exe");if(Files.isRegularFile(candidate))return candidate;}catch(InvalidPathException ignored){}}
        for(Path parent:List.of(Path.of(System.getenv("ProgramFiles"),"JetBrains"),Path.of(System.getenv("LOCALAPPDATA"),"JetBrains/Toolbox/apps"))){if(Files.isDirectory(parent))try(var paths=Files.walk(parent,6)){var found=paths.filter(p->p.getFileName().toString().equalsIgnoreCase("rider64.exe")).findFirst();if(found.isPresent())return found.get();}catch(IOException ignored){}}return null;}
    void beforeEdit(Document document){rebuildRequired=true;if(building){cancelled=true;++epoch;}status("Rebuild required · last registered assembly has not changed");for(var source:List.copyOf(sources.values()))if(source.document()==document)try{if(!FileDocumentManager.getInstance().isDocumentUnsaved(document)&&Files.readString(source.path()).replace("\r\n","\n").equals(document.getText()))saved(source);}catch(Exception ignored){}}
    boolean mayReload(VirtualFile file,Document document){return file==reloadAllowed||!FileDocumentManager.getInstance().isDocumentUnsaved(document);}
    private void reload(Source source)throws Exception{reloadAllowed=source.file();try{source.file().refresh(false,false);FileDocumentManager.getInstance().reloadFromDisk(source.document());saved(source);}finally{reloadAllowed=null;}}
    boolean safeToSave(Document document){for(var source:sources.values())if(source.document()==document)try{return MessageDigest.isEqual(source.fingerprint(),hash(source.path()));}catch(Exception error){return false;}return true;}
    private void saved(Source source)throws Exception{sources.put(source.file(),new Source(source.file(),source.document(),source.path(),hash(source.path())));}
    public void saveAll()throws Exception{for(var source:List.copyOf(sources.values()))save(source);}
    private void save(Source source)throws Exception{
        if(!FileDocumentManager.getInstance().isDocumentUnsaved(source.document())){saved(source);return;}checked(source.path());if(!MessageDigest.isEqual(source.fingerprint(),hash(source.path())))throw new IOException("External change conflict: "+source.path()+". Use Resolve External Change before Save.");FileDocumentManager.getInstance().saveDocument(source.document());if(FileDocumentManager.getInstance().isDocumentUnsaved(source.document()))throw new IOException("Source Save rejected: "+source.path());saved(source);
    }
    boolean canClose(VirtualFile file){Source source=sources.get(file);if(source==null||!FileDocumentManager.getInstance().isDocumentUnsaved(source.document()))return true;String[] options={"Save","Discard","Cancel"};int answer=JOptionPane.showOptionDialog(null,"Save changes to "+file.getName()+"?","Close C# File",JOptionPane.DEFAULT_OPTION,JOptionPane.QUESTION_MESSAGE,null,options,options[0]);try{if(answer==0){save(source);return true;}if(answer==1){reload(source);return true;}}catch(Exception error){fail(error);}return false;}
    private void closeEverywhere(VirtualFile file){((com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project)).closeFile(file,true,true);}
    public boolean closeSources(){for(var file:List.copyOf(sources.keySet()))if(!canClose(file))return false;for(var file:List.copyOf(sources.keySet()))closeEverywhere(file);for(var file:sources.keySet())file.putUserData(OWNER,null);sources.clear();return true;}
    public void resolve(VirtualFile file){Source source=sources.get(file);if(source==null)return;String[] options={"Compare","Reload from Disk","Keep Current / Save As…","Cancel"};int answer=JOptionPane.showOptionDialog(null,"External changes to "+file.getName()+". No file has been overwritten.","Source Conflict",JOptionPane.DEFAULT_OPTION,JOptionPane.WARNING_MESSAGE,null,options,options[0]);try{
        if(answer==0){var factory=com.intellij.diff.DiffContentFactory.getInstance();com.intellij.diff.DiffManager.getInstance().showDiff(project,new com.intellij.diff.requests.SimpleDiffRequest(file.getName(),factory.create(project,source.document()),factory.create(project,Files.readString(source.path())),"Current Document","Disk"));}
        if(answer==1)reload(source);if(answer==2){var picker=new JFileChooser(source.path().getParent().toFile());if(picker.showSaveDialog(null)==JFileChooser.APPROVE_OPTION){Path path=picker.getSelectedFile().toPath();if(Files.exists(path))throw new IOException("Save As requires a new file");Files.writeString(path,source.document().getText(),StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW);}}
    }catch(Exception error){fail(error);}}
    public JComponent editorBar(VirtualFile file){var bar=new JPanel(new java.awt.FlowLayout(java.awt.FlowLayout.LEADING,6,4));bar.add(button("Open With…",()->open(Path.of(file.getPath()),1,true)));bar.add(button("Build & Register",this::build));bar.add(button("Resolve External Change…",()->resolve(file)));bar.add(button("Find Graph Uses",()->findUses(file)));var label=new JLabel(status);statuses.add(label);var panel=new JPanel(new BorderLayout());panel.add(bar,BorderLayout.NORTH);panel.add(label,BorderLayout.SOUTH);panel.putClientProperty("epidemic.script.release",(Runnable)()->statuses.remove(label));return panel;}
    boolean trustExecution(){
        Path current=root.get();
        if(rebuildRequired||!Objects.equals(current,builtRoot)){
            String[] choices={"Run Last Successful Generation","Cancel"};
            if(JOptionPane.showOptionDialog(null,"C# sources are unverified or out of date. This runs the saved compiled generation, not the current source. Use Build & Register to apply source changes.","C# Rebuild Required",JOptionPane.DEFAULT_OPTION,JOptionPane.WARNING_MESSAGE,null,choices,choices[1])!=0)return false;
        }
        if(Objects.equals(current,trustedRoot))return true;
        if(JOptionPane.showConfirmDialog(null,"Execute C# from this project? Script code is trusted code, not sandboxed. Native scene and Graph VM remain the execution owners.","Trust C# Execution",JOptionPane.OK_CANCEL_OPTION,JOptionPane.WARNING_MESSAGE)!=JOptionPane.OK_OPTION)return false;
        trustedRoot=current;return true;
    }
    private void findUses(VirtualFile file){
        var current=snapshot.get();if(current==null)return;Path source=Path.of(file.getPath()).toAbsolutePath().normalize();var uses=new ArrayList<String>();
        for(var graph:current.graphs())for(var node:graph.nodes())if(graph.catalogue().stream().anyMatch(type->type.type()==node.type()&&!type.source().isEmpty()&&root.get().resolve(type.source()).toAbsolutePath().normalize().equals(source)))
            uses.add(current.documents().stream().filter(d->d.id()==graph.document()).map(Bridge.Document::name).findFirst().orElse("Graph")+" · "+node.name()+" · node="+Long.toUnsignedString(node.id()));
        JOptionPane.showMessageDialog(null,uses.isEmpty()?"No registered Graph references to this source":String.join("\n",uses),"Native Graph Uses",JOptionPane.INFORMATION_MESSAGE);
    }
    public void build(){if(building){fail(new IOException("A script build is already active"));return;}Path projectRoot=root.get();if(projectRoot==null)return;
        if(JOptionPane.showConfirmDialog(null,"Build trusted C# sources? MSBuild and C# constructors execute project code. No build runs automatically on open. Dependencies are offline only.","Trust Script Build",JOptionPane.OK_CANCEL_OPTION,JOptionPane.WARNING_MESSAGE)!=JOptionPane.OK_OPTION)return;
        trustedRoot=projectRoot;startBuild(projectRoot);
    }
    private CompletableFuture<Bridge.Reply> startBuild(Path projectRoot){
        if(building)return CompletableFuture.failedFuture(new IOException("A script build is already active"));
        try{saveAll();}catch(Exception error){fail(error);return CompletableFuture.failedFuture(error);}
        var result=new CompletableFuture<Bridge.Reply>();long token=++epoch;cancelled=false;building=true;rebuildRequired=true;diagnostics.clear();status("Building scripts…");background.execute(()->{try{
            Path base=projectRoot.toRealPath(),scripts=base.resolve("Scripts");Files.createDirectories(scripts);if(!scripts.toRealPath().equals(base.resolve("Scripts")))throw new IOException("Scripts outside project");Path generation=base.resolve(".epidemic/scripts/"+UUID.randomUUID());Files.createDirectories(generation);if(!generation.toRealPath().startsWith(base))throw new IOException("Build output outside project");
            Path api=managed().resolve("Epidemic.ScriptApi.dll");if(!Files.isRegularFile(api))throw new IOException("Managed tooling is not included in this distribution");
            Path apiCopy=base.resolve(".epidemic/ScriptApi");Files.createDirectories(apiCopy);if(!apiCopy.toRealPath().startsWith(base))throw new IOException("API output outside project");Files.copy(api,apiCopy.resolve(api.getFileName()),StandardCopyOption.REPLACE_EXISTING);
            var inputHashes=sourceHashes(scripts);
            Path csproj=scripts.resolve("GameScripts.csproj");if(!Files.exists(csproj))Files.writeString(csproj,"<Project Sdk=\"Microsoft.NET.Sdk\"><PropertyGroup><TargetFramework>net10.0</TargetFramework></PropertyGroup><ItemGroup><Reference Include=\"Epidemic.ScriptApi\"><HintPath>../.epidemic/ScriptApi/Epidemic.ScriptApi.dll</HintPath></Reference></ItemGroup></Project>",StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW);
            String xml="<Project Sdk=\"Microsoft.NET.Sdk\"><PropertyGroup><TargetFramework>net10.0</TargetFramework><EnableDefaultCompileItems>false</EnableDefaultCompileItems><EnableNETAnalyzers>false</EnableNETAnalyzers></PropertyGroup><ItemGroup><Compile Include=\""+escape(scripts.toRealPath()+"/**/*.cs")+"\" Exclude=\""+escape(scripts+"/obj/**/*.cs")+"\"/><Reference Include=\"Epidemic.ScriptApi\"><HintPath>"+escape(api.toString())+"</HintPath></Reference></ItemGroup></Project>";
            Path derived=generation.resolve("GameScripts.csproj");Files.writeString(derived,xml);Files.writeString(generation.resolve("NuGet.Config"),"<configuration><packageSources><clear/></packageSources></configuration>");
            Path dotnet=Path.of(System.getenv("ProgramFiles"),"dotnet/dotnet.exe");if(!Files.isRegularFile(dotnet))throw new IOException(".NET SDK missing; source editing remains available");
            run(List.of(dotnet.toString(),"build",derived.toString(),"-c","Release","-o",generation.toString(),"--nologo","-p:UseSharedCompilation=false","-nr:false","-p:ImportDirectoryBuildProps=false","-p:ImportDirectoryBuildTargets=false","-p:ImportDirectoryPackagesProps=false","-p:RestoreConfigFile="+generation.resolve("NuGet.Config")),base,token);
            Path schema=generation.resolve("types.schema"),assembly=generation.resolve("GameScripts.dll");run(List.of(dotnet.toString(),managed().resolve("Epidemic.ScriptWorker.dll").toString(),"schema",assembly.toString(),schema.toString(),base.relativize(assembly).toString(),base.toString()),base,token);
            unchanged(inputHashes,scripts);
            ui(()->{
                if(token!=epoch||!Objects.equals(root.get(),projectRoot)){building=false;result.completeExceptionally(new IOException("Build is out of date"));return;}
                register.apply(base.relativize(schema).toString(),base).whenComplete((reply,error)->{
                    building=false;
                    if(token!=epoch)result.completeExceptionally(new IOException("Build is out of date"));else if(error!=null)result.completeExceptionally(error);else result.complete(reply);
                    ui(()->{if(token!=epoch)return;if(error!=null)fail(error);else {builtRoot=projectRoot;rebuildRequired=false;status("Build succeeded · net10.0 · "+base.relativize(assembly)+" · native types registered");}});
                });
            });
        }catch(Exception error){building=false;result.completeExceptionally(error);ui(()->{if(token==epoch)fail(error);});}finally{build=null;}});
        return result;
    }
    private static String escape(String value){return value.replace("&","&amp;").replace("\"","&quot;").replace("<","&lt;");}
    private void run(List<String> command,Path directory,long token)throws Exception{
        if(cancelled||closed||token!=epoch)throw new IOException("Script build cancelled");var launcher=new ProcessBuilder(command).directory(directory.toFile()).redirectErrorStream(true);launcher.environment().put("DOTNET_CLI_UI_LANGUAGE","en-US");launcher.environment().put("DOTNET_CLI_TELEMETRY_OPTOUT","1");Process process=launcher.start();build=process;if(cancelled||closed||token!=epoch){process.descendants().forEach(ProcessHandle::destroyForcibly);process.destroyForcibly();throw new IOException("Script build cancelled");}
        CompletableFuture.delayedExecutor(60,TimeUnit.SECONDS).execute(()->{if(process.isAlive()){process.descendants().forEach(ProcessHandle::destroyForcibly);process.destroyForcibly();}});
        int lines=0;String failureDetail="";try(var reader=process.inputReader(StandardCharsets.UTF_8)){String line;while((line=reader.readLine())!=null){if(++lines>10000){process.descendants().forEach(ProcessHandle::destroyForcibly);process.destroyForcibly();throw new IOException("Build output limit");}if(line.length()>16384)line=line.substring(0,16384);var match=ERROR.matcher(line);if(line.toLowerCase(Locale.ROOT).contains("error")||line.contains("SDK")||line.startsWith("script.worker:"))failureDetail=line;
            if(line.startsWith("script.worker:")){String diagnostic=line;ui(()->problems.accept(diagnostic));}if(match.matches()){var diagnostic=new Diagnostic(Path.of(match.group(1)),Integer.parseInt(match.group(2)),Integer.parseInt(match.group(3)),match.group(4));ui(()->{if(token==epoch){diagnostics.addElement(diagnostic);problems.accept(diagnostic.toString());}});}}}
        int result=process.waitFor();build=null;if(cancelled||token!=epoch)throw new IOException("Build cancelled; previous registration retained");if(result!=0)throw new IOException("C# build/schema failed ("+result+"); previous schema retained. "+failureDetail);
    }
    CompletableFuture<Bridge.Reply> smokeBuild()throws Exception {
        Path base=root.get(),source=base.resolve("Scripts/SdkSmoke.cs"),target=base.resolve("Directory.Build.targets"),marker=base.resolve("sdk-target-must-not-run.txt");
        Files.createDirectories(source.getParent());Files.writeString(source,template("SdkSmoke"),StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW);
        boolean ownTarget=!Files.exists(target);if(ownTarget)Files.writeString(target,"<Project><Target Name=\"UntrustedFixture\" BeforeTargets=\"BeforeBuild\"><WriteLinesToFile File=\""+escape(marker.toString())+"\" Lines=\"should not run\" Overwrite=\"true\"/></Target></Project>",StandardCharsets.UTF_8,StandardOpenOption.CREATE_NEW);
        var result=new CompletableFuture<Bridge.Reply>();
        startBuild(base).whenComplete((good,error)->ui(()->{
            if(error!=null){result.completeExceptionally(error);return;}
            try{
                if(ownTarget&&Files.exists(marker))throw new IllegalStateException("Project Directory.Build.targets executed");
                if(good.snapshot().componentTypes().stream().noneMatch(type->type.name().equals("user.SdkSmoke")))throw new IllegalStateException("Production Build & Register did not publish native type");
                Files.writeString(source,"public sealed class SdkSmoke { this is not C#;",StandardCharsets.UTF_8);
                startBuild(base).whenComplete((bad,failed)->ui(()->{
                    try{
                        if(failed==null||diagnostics.isEmpty())throw new IllegalStateException("Malformed source lacked real SDK diagnostics");
                        if(snapshot.get().revision()!=good.snapshot().revision()||snapshot.get().componentTypes().stream().noneMatch(type->type.name().equals("user.SdkSmoke")))throw new IllegalStateException("Failed build replaced last-good native registry");
                        Files.delete(source);if(ownTarget)Files.delete(target);result.complete(good);
                    }catch(Exception failure){result.completeExceptionally(failure);}
                }));
            }catch(Exception failure){result.completeExceptionally(failure);}
        }));return result;
    }
    CompletableFuture<Void> smoke()throws Exception{
        int editorsBefore=com.intellij.openapi.editor.EditorFactory.getInstance().getAllEditors().length;
        Path directory=root.get().resolve("Scripts");Files.createDirectories(directory);Path path=directory.resolve("Проверка.cs");Files.writeString(path,template("Проверка"));openBuiltIn(path,1);
        VirtualFile file=LocalFileSystem.getInstance().findFileByNioFile(path);Source source=sources.get(file);if(source==null||Arrays.stream(FileEditorManager.getInstance(project).getEditors(file)).noneMatch(e->e instanceof ScriptEditorProvider.SourceEditor))throw new IllegalStateException("Not a Platform C# Editor");
        Document document=source.document();WriteCommandAction.runWriteCommandAction(project,()->document.insertString(document.getTextLength(),"// кириллица\n"));saveAll();if(!Files.readString(path).contains("кириллица"))throw new IllegalStateException("Source Save failed");source=sources.get(file);byte[] saved=Files.readAllBytes(path);
        WriteCommandAction.runWriteCommandAction(project,()->document.insertString(document.getTextLength(),"// dirty\n"));Files.writeString(path,"// external change\n");if(safeToSave(document))throw new IllegalStateException("Auto-save guard missed conflict");try{saveAll();throw new IllegalStateException("Conflict overwritten");}catch(IOException expected){if(!expected.getMessage().contains("External change"))throw expected;}
        if(!Files.readString(path).equals("// external change\n")||!document.getText().contains("// dirty"))throw new IllegalStateException("Conflict lost source data");reload(source);if(!document.getText().contains("external change"))throw new IllegalStateException("Explicit reload failed");Files.write(path,saved);reload(sources.get(file));FileEditorManager.getInstance(project).closeFile(file);sources.remove(file);openBuiltIn(path,1);FileEditorManager.getInstance(project).closeFile(file);sources.remove(file);Files.delete(path);
        var released=new CompletableFuture<Void>();
        var timer=new javax.swing.Timer(50,new java.awt.event.ActionListener(){int attempts;public void actionPerformed(java.awt.event.ActionEvent event){
            int count=com.intellij.openapi.editor.EditorFactory.getInstance().getAllEditors().length;
            if(count==editorsBefore){((javax.swing.Timer)event.getSource()).stop();released.complete(null);}
            else if(++attempts>=40){((javax.swing.Timer)event.getSource()).stop();released.completeExceptionally(new IllegalStateException("Platform Editor remains after bounded close: before="+editorsBefore+" after="+count+" manager="+FileEditorManager.getInstance(project).getEditors(file).length+" unsaved="+FileDocumentManager.getInstance().isDocumentUnsaved(document)+" open="+Arrays.toString(FileEditorManager.getInstance(project).getOpenFiles())));}
        }});timer.start();return released;
    }
    public void cancel(){cancelled=true;++epoch;Process process=build;if(process!=null){process.descendants().forEach(ProcessHandle::destroyForcibly);process.destroyForcibly();}status("Build cancellation requested; published generations are retained");}
    public void close(){closed=true;cancel();background.shutdownNow();}
}

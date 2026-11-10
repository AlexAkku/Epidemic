package epidemic.editor.platform;

import epidemic.editor.*;
import epidemic.u1.NativeViewport;
import com.intellij.openapi.Disposable;
import com.intellij.openapi.actionSystem.*;
import com.intellij.openapi.application.ApplicationManager;
import com.intellij.openapi.extensions.PluginId;
import com.intellij.openapi.fileEditor.*;
import com.intellij.openapi.fileChooser.*;
import com.intellij.openapi.project.*;
import com.intellij.openapi.ui.Messages;
import com.intellij.openapi.vfs.VirtualFile;
import com.intellij.openapi.wm.*;
import com.intellij.testFramework.LightVirtualFile;
import com.intellij.ui.content.*;
import com.intellij.ide.ui.LafManager;
import java.awt.*;
import java.awt.event.*;
import java.awt.image.BufferedImage;
import java.nio.file.*;
import java.io.*;
import java.util.*;
import java.util.List;
import java.util.concurrent.*;
import javax.imageio.ImageIO;
import javax.swing.*;

/** One native workspace per IDE container. IntelliJ is the only window/docking owner. */
public final class PlatformSession implements Disposable {
    public record DocumentView(UiPanels reader,JComponent component,VirtualFile file,JLabel title){}
    public final Catalog catalog=new Catalog();
    private final Project project;
    private final FeatureRegistry features=new FeatureRegistry();
    private final Map<String,VirtualFile> documents=new LinkedHashMap<>();
    private final List<DocumentView> views=new ArrayList<>();
    private final Map<String,Content> contents=new HashMap<>();
    private final Map<String,ToolWindow> groups=new LinkedHashMap<>();
    private final Map<String,String> targets=new HashMap<>();
    private final Map<String,Content> selectedContextTabs=new HashMap<>();
    private final ExecutorService io=Executors.newSingleThreadExecutor(r->{Thread t=new Thread(r,"Epidemic preferences");t.setDaemon(true);return t;});
    private final Path output=Path.of(System.getProperty("epidemic.ui.output"));
    private final Path profile=Path.of(System.getProperty("epidemic.ui.profile"));
    private final RecentProjects recent=new RecentProjects(profile);
    private final boolean smoke=Boolean.getBoolean("epidemic.ui.platform.smoke");
    private final long started=System.nanoTime();
    private Bridge bridge;
    private Bridge.Snapshot snapshot;
    private Bridge.Service service=Bridge.Service.Unavailable;
    private UiPanels tools;
    private JFrame launcher;
    private JLabel launcherStatus;
    private DefaultListModel<RecentProjects.Entry> recentRows;
    private boolean disposed,opened,closing;
    private boolean expectedDisconnect,recoveringSmoke;
    private long recoveryProject;
    private int recoveryViews;
    private Set<DocumentView> recoveryReaders=Set.of();
    private Rectangle recoveryBounds;
    private String context="",selectedPanel="scene.editor";
    private DefaultActionGroup menu;
    private AnAction saveAction,saveAllAction,undoAction,redoAction,playAction,stopAction,playToggleAction;
    private javax.swing.Timer assetPoll;
    private boolean assetPolling;
    private long loggedContentRevision;
    private String importFolder="",assetQuery="";
    private record PickerImport(Bridge participant,long generation,List<VirtualFile> files,String folder) {}
    private PickerImport pickerImport;
    private Bridge.Selection pendingSelection;
    private Bridge.Selection reconnectSelection;
    private boolean selecting;
    private boolean rejectedCommand;
    private final Set<VirtualFile> pendingDocumentCloses=new HashSet<>();
    private final Deque<Runnable> readyActions=new ArrayDeque<>();
    private record FloatingGuard(List<WindowListener> originals,WindowListener guard){}
    private Integer smokeWindowChoice;
    private int smokeClosePrompts;
    private CompletableFuture<Bridge.Reply> activeSave;
    private final Map<Window,FloatingGuard> floatingGuards=new HashMap<>();
    private FeatureRegistry.Feature lifecycleFeature,playFeature;
    private ToolWindow navigation,primary,secondary,bottom;
    private final VetoableProjectManagerListener closeGuard=new VetoableProjectManagerListener(){
        public boolean canClose(Project candidate){
            if(candidate!=project||!opened)return true;
            edt(()->close(true));return false;
        }
    };
    public PlatformSession(Project project){this.project=project;}
    public boolean isDisposed(){return disposed;}
    public void activateWindow(){edt(()->{
        Window target=opened?WindowManager.getInstance().getFrame(project):launcher;
        if(target==null)return;
        if(target instanceof Frame frame)frame.setExtendedState(frame.getExtendedState()&~Frame.ICONIFIED);
        target.setVisible(true);target.toFront();target.requestFocus();
    });}
    private void edt(Runnable action){ApplicationManager.getApplication().invokeLater(()->{if(!disposed&&!project.isDisposed())try{action.run();}catch(Throwable error){failure(error);}});}
    private void record(String message){
        System.out.println(message);
        try{Files.createDirectories(output);Files.writeString(output.resolve("platform-status.log"),message+"\n",StandardOpenOption.CREATE,StandardOpenOption.APPEND);}
        catch(IOException error){throw new UncheckedIOException(error);}
    }
    public void start(){
        record("INFO host="+(Boolean.getBoolean("epidemic.ui.standalone")?"Epidemic standalone product":"IntelliJ development plugin")+" owner="+ToolWindowManager.getInstance(project).getClass().getName()+" provider=epidemic.platform.panel; Platform owns all docking");
        if(Boolean.getBoolean("epidemic.ui.standalone")){
            // The game product has its own menu/document commands. IDE project
            // and run-configuration widgets would expose an unrelated workflow.
            var settings=com.intellij.ide.ui.UISettings.getInstance();
            settings.setShowMainToolbar(true);
            settings.setShowNewMainToolbar(true);
            settings.setMainMenuDisplayMode(com.intellij.ide.ui.MainMenuDisplayMode.MERGED_WITH_MAIN_TOOLBAR);
            settings.setShowNavigationBar(false);
            settings.fireUISettingsChanged();
        }
        var laf=LafManager.getInstance();
        var themes=laf.getInstalledThemes().iterator();
        while(themes.hasNext()){
            var theme=themes.next();
            if(theme.getName().toLowerCase(Locale.ROOT).contains("islands dark")){
                laf.setCurrentUIThemeLookAndFeel(theme);laf.updateUI();break;
            }
        }
        record("INFO JetBrains lookAndFeel="+laf.getCurrentUIThemeLookAndFeel().getName());
        ProjectManager.getInstance().addProjectManagerListener(closeGuard);
        JPopupMenu.setDefaultLightWeightPopupEnabled(false);ToolTipManager.sharedInstance().setLightWeightPopupEnabled(false);
        com.intellij.openapi.util.registry.Registry.get("ide.allow.split.and.reorder.in.tool.window").setValue(true);
        JFrame frame=WindowManager.getInstance().getFrame(project);if(frame!=null){
            if(Boolean.getBoolean("epidemic.ui.standalone"))frame.getRootPane().putClientProperty(com.intellij.openapi.wm.impl.content.ToolWindowContentUi.HIDE_ID_LABEL,"true");
            frame.setVisible(false);
        }
        project.getMessageBus().connect(this).subscribe(FileEditorManagerListener.FILE_EDITOR_MANAGER,new FileEditorManagerListener(){
            @Override public void selectionChanged(FileEditorManagerEvent event){
                VirtualFile file=event.getNewFile();String id=file==null?null:file.getUserData(PanelEditorProvider.PANEL);
                if(id!=null&&!Objects.equals(file,event.getOldFile())){
                    selectedPanel=id;context(Catalog.context(id));selectDocument(file);
                }
            }
        });
        launcher();
        require(bridge==null&&tools==null,"Launcher created native workspace early");
        record("PASS Platform Launcher first; no native workspace/viewport before project choice");
        if(smoke)later(400,()->connect(3,output.resolve("platform-game-"+System.nanoTime()).toString(),"JetBrains UI smoke"));
    }
    private void launcher(){
        if(launcher!=null){launcher.setVisible(true);refreshRecent();return;}
        launcher=new JFrame("Epidemic Engine");launcher.setDefaultCloseOperation(WindowConstants.DO_NOTHING_ON_CLOSE);
        launcher.addWindowListener(new WindowAdapter(){@Override public void windowClosing(WindowEvent event){exit();}});
        JPanel root=Theme.island(new BorderLayout(16,16));root.setBorder(BorderFactory.createEmptyBorder(28,28,28,28));
        JLabel heading=new JLabel("Epidemic Engine");heading.setFont(heading.getFont().deriveFont(Font.BOLD,26));
        JPanel title=new JPanel(new BorderLayout(8,8));title.add(heading);title.add(new JLabel("Create or open a project"),BorderLayout.SOUTH);root.add(title,BorderLayout.NORTH);
        JPanel actions=new JPanel(new GridLayout(0,1,8,8));
        actions.add(new JButton(new AbstractAction("Create Project…"){@Override public void actionPerformed(ActionEvent event){newProject();}}));
        actions.add(new JButton(new AbstractAction("Open Project…"){@Override public void actionPerformed(ActionEvent event){openPicker();}}));
        actions.add(new JButton(new AbstractAction("Settings / UI Gallery"){@Override public void actionPerformed(ActionEvent event){JOptionPane.showMessageDialog(launcher,InspectorForms.gallery(catalog,features),"Editor Preferences",JOptionPane.PLAIN_MESSAGE);}}));
        actions.add(new JButton(new AbstractAction("Exit"){@Override public void actionPerformed(ActionEvent event){exit();}}));root.add(actions,BorderLayout.WEST);
        recentRows=new DefaultListModel<>();JList<RecentProjects.Entry> list=new JList<>(recentRows);
        list.setCellRenderer((owner,value,index,selected,focus)->{JLabel row=new JLabel(value.title()+"  ·  "+value.path());row.setOpaque(true);row.setBackground(selected?Theme.TAB:Theme.TOOL);row.setForeground(Theme.TEXT);row.setBorder(BorderFactory.createEmptyBorder(8,8,8,8));return row;});
        JPanel recents=new JPanel(new BorderLayout(8,8));recents.add(new JLabel("Recent Projects — last 10 successful opens"),BorderLayout.NORTH);recents.add(new JScrollPane(list));
        JPanel buttons=new JPanel();JButton open=new JButton("Open"),remove=new JButton("Remove from List");buttons.add(open);buttons.add(remove);recents.add(buttons,BorderLayout.SOUTH);
        open.addActionListener(event->{var entry=list.getSelectedValue();if(entry!=null)connect(2,entry.path());});
        remove.addActionListener(event->{var entry=list.getSelectedValue();if(entry!=null)CompletableFuture.runAsync(()->{try{recent.remove(entry);}catch(IOException error){throw new UncheckedIOException(error);}},io).whenComplete((v,e)->edt(()->{if(e!=null)failure(e);refreshRecent();}));});
        root.add(recents);launcherStatus=new JLabel("Choose or create a game project");root.add(launcherStatus,BorderLayout.SOUTH);
        launcher.setContentPane(root);launcher.setSize(1050,540);launcher.setLocationRelativeTo(null);launcher.setVisible(true);refreshRecent();
    }
    private void refreshRecent(){CompletableFuture.supplyAsync(()->{try{return recent.load();}catch(IOException e){throw new UncheckedIOException(e);}},io).whenComplete((entries,error)->edt(()->{if(error!=null){failure(error);return;}recentRows.clear();entries.forEach(recentRows::addElement);}));}
    private void newProject(){
        JTextField name=new JTextField("MyGame"),parent=new JTextField(System.getProperty("user.home"));
        JPanel form=new JPanel(new GridLayout(0,1,6,6));form.add(new JLabel("Project name"));form.add(name);form.add(new JLabel("Parent directory"));form.add(parent);
        form.add(new JComboBox<>(new String[]{"Empty Scene — current native schema"}));
        if(JOptionPane.showConfirmDialog(launcher,form,"Create Project",JOptionPane.OK_CANCEL_OPTION)!=JOptionPane.OK_OPTION)return;
        String value=name.getText();
        if(!ProjectNames.valid(value)){failure(new IOException("Invalid Windows project name"));return;}
        connect(3,Path.of(parent.getText()).resolve(value).toString(),value);
    }
    private void openPicker(){
        FileChooserDescriptor descriptor=new FileChooserDescriptor(true,false,false,false,false,false)
            .withTitle("Open Project.epidemicproj or legacy .epidemic");
        FileChooser.chooseFile(descriptor,project,launcher,null,file->connect(2,file.getPath()));
    }
    private void connect(int operation,String...args){
        if(service==Bridge.Service.Loading||opened)return;
        service=Bridge.Service.Loading;
        if(bridge==null){bridge=new Bridge(Path.of(System.getProperty("epidemic.ui.backend")),output,this::state,this::edt);bridge.connect().thenCompose(reply->bridge.call(operation,args)).whenComplete((reply,error)->edt(()->{if(error!=null)failure(error);else workspace(reply.snapshot());}));}
        else bridge.call(operation,args).whenComplete((reply,error)->edt(()->{if(error!=null)failure(error);else workspace(reply.snapshot());}));
    }
    private void state(Bridge.State state){
        service=state.service();
        boolean rejected=service==Bridge.Service.Ready&&!state.diagnostic().isEmpty();
        if(service==Bridge.Service.Ready)rejectedCommand=rejected;
        if(rejected){
            pendingSelection=null;
            if(tools!=null){
                if(state.diagnostic().startsWith("Import rejected:"))reportProblem(state.diagnostic());
                else tools.problem(state.diagnostic());
            }
        }
        if(state.reply()!=null&&state.reply().snapshot()!=null){snapshot=state.reply().snapshot();if(tools!=null)tools.refresh(snapshot,rejected);for(var view:List.copyOf(views)){view.reader().refresh(snapshot,rejected);refreshDocument(view);}refreshDocumentCommands();
            JFrame frame=WindowManager.getInstance().getFrame(project);if(frame!=null)frame.setTitle("Epidemic — "+snapshot.title()+" · "+snapshot.path()+(snapshot.dirty()?" · Unsaved":"")+(service==Bridge.Service.Loading?" · Working…":service==Bridge.Service.Error?" · Disconnected — read-only":""));
        }
        if(snapshot!=null && tools!=null && snapshot.assetState().revision()!=loggedContentRevision){
            var events=snapshot.assetState().events();int count=(int)Math.min(events.size(),Math.max(0,snapshot.assetState().revision()-loggedContentRevision));
            for(var event:events.subList(events.size()-count,events.size())){
                if(event.kind()==2||event.kind()==3)reportProblem(event.diagnostic());
                else tools.log(event.diagnostic());
            }
            loggedContentRevision=snapshot.assetState().revision();
        }
        if(state.service()==Bridge.Service.Ready){startAssetPolling();if(snapshot!=null&&snapshot.selection().kind()==6&&!context.equals("Asset"))context("Asset");else if(snapshot!=null&&snapshot.selection().kind()!=6&&context.equals("Asset"))context(Catalog.context(selectedPanel));flushSelection();}
        if(state.service()==Bridge.Service.Ready&&!readyActions.isEmpty()){var next=readyActions.removeFirst();edt(next);}
        if(state.service()==Bridge.Service.Error){if(expectedDisconnect)record("INFO Expected smoke disconnect; immutable snapshot/layout retained");else failure(new IOException(state.diagnostic()));}
    }
    private UiPanels reader(){UiPanels result=new UiPanels(catalog,features,this::openDocument,this::select,this::importPicker,this::cancelImports,this::newDocument,this::renameDocument,this::editField);result.assetCommands(this::assetAction);result.documentCommands(this::documentAction);
        result.assetPreview((id,key,divisor)->{
            Bridge participant=bridge;
            var current=participant==null?null:participant.latest();
            if(current==null||!current.capabilities().contains("asset-preview"))
                return CompletableFuture.failedFuture(new IOException("ui.preview_unavailable"));
            return participant.assetPreview(current.generation(),id,key,divisor);
        });result.hierarchyCommands(()->supports("scene-hierarchy"),this::hierarchyEdit);result.componentCommands(()->supports("component-authoring"),this::componentEdit);result.sceneGestures(()->supports("scene-gestures"),this::sceneGesture);result.sceneResources(()->supports("scene-resources"));result.systemsCommands(()->supports("scene-systems"));return result;}
    private CompletableFuture<Bridge.Reply> sceneGesture(UiPanels.GestureEdit edit){
        if(!supports(switch(edit.operation()){case 33->"scene-resources";case 34->"scene-systems";default->"scene-gestures";})||snapshot.playing())
            return CompletableFuture.failedFuture(new IOException("ui.readonly_or_busy"));
        var future=bridge.callAt(edit.generation(),edit.revision(),edit.operation(),edit.arguments().toArray(String[]::new));
        if(!Set.of("move","cancel").contains(edit.arguments().getFirst()))
            future.whenComplete((reply,error)->{if(error!=null)edt(()->documentFailure(error));});
        return future;
    }
    private void componentEdit(UiPanels.ComponentEdit edit){
        if(!supports("component-authoring")||snapshot.playing())return;
        String selection=Long.toUnsignedString(edit.selectionRevision());
        String[] args=edit.argument()==null?new String[]{selection,edit.action()}:new String[]{selection,edit.action(),edit.argument()};
        bridge.callAt(edit.generation(),edit.revision(),30,args)
            .whenComplete((reply,error)->{if(error!=null)edt(()->documentFailure(error));});
    }
    private void hierarchyEdit(UiPanels.HierarchyEdit edit){
        if(!supports("scene-hierarchy")||snapshot.playing())return;
        var args=new ArrayList<String>();
        args.add(edit.action());
        args.add(Long.toUnsignedString(edit.document()));
        args.add(Long.toUnsignedString(edit.node()));
        if(edit.argument()!=null)args.add(edit.argument());
        bridge.callAt(edit.generation(),edit.revision(),29,args.toArray(String[]::new))
            .whenComplete((reply,error)->{if(error!=null)edt(()->documentFailure(error));});
    }
    private CompletableFuture<Bridge.Reply> editField(ScalarInspector.Edit edit){
        if(!supports("scalar-inspector")||snapshot.playing()||!pendingDocumentCloses.isEmpty())return CompletableFuture.failedFuture(new IOException("ui.readonly_or_busy"));
        service=Bridge.Service.Loading;refreshDocumentCommands();
        return bridge.callAt(edit.generation(),edit.revision(),16,Long.toUnsignedString(edit.selectionRevision()),edit.key(),edit.value());
    }
    private CompletableFuture<Bridge.Reply> saveProject(){
        if(activeSave!=null&&!activeSave.isDone())return activeSave;
        CompletableFuture<Bridge.Reply> result=new CompletableFuture<>();Bridge participant=bridge;
        activeSave=result;
        long projectId=snapshot==null?0:snapshot.project();
        List<CompletableFuture<Void>> flushes=new ArrayList<>();if(tools!=null)flushes.add(tools.flushDrafts());
        for(var view:views)flushes.add(view.reader().flushDrafts());
        CompletableFuture.allOf(flushes.toArray(CompletableFuture[]::new)).whenComplete((v,error)->edt(()->{
            if(error!=null)result.completeExceptionally(error);else saveReady(participant,projectId,result);
        }));return result;
    }
    private void saveReady(Bridge participant,long projectId,CompletableFuture<Bridge.Reply> result){
        if(bridge!=participant||snapshot==null||snapshot.project()!=projectId||service==Bridge.Service.Error){result.completeExceptionally(new IOException("ui.session_changed"));return;}
        if(service==Bridge.Service.Loading){if(readyActions.size()>=32)result.completeExceptionally(new IOException("ui.queue_full"));else readyActions.addLast(()->saveReady(participant,projectId,result));return;}
        service=Bridge.Service.Loading;refreshDocumentCommands();
        participant.call(6).whenComplete((reply,error)->edt(()->{if(error!=null)result.completeExceptionally(error);else result.complete(reply);}));
    }
    private void select(Bridge.Selection selection){pendingSelection=selection;flushSelection();}
    private void flushSelection(){
        if(!ready()||selecting||pendingSelection==null||!pendingDocumentCloses.isEmpty())return;
        var next=pendingSelection;pendingSelection=null;
        if(next.equals(snapshot.selection()))return;
        selecting=true;Bridge participant=bridge;
        participant.call(9,Integer.toString(next.kind()),Long.toUnsignedString(next.document()),Long.toUnsignedString(next.element()),Long.toUnsignedString(next.component()),Long.toUnsignedString(next.part())).whenComplete((reply,error)->edt(()->{
            if(bridge!=participant)return;selecting=false;if(error!=null)documentFailure(error);flushSelection();
        }));
    }
    private void refreshDocumentCommands(){
        boolean enabled=supports("create-document")&&!snapshot.playing();
        if(tools!=null)tools.documentCommandsEnabled(enabled);
        for(var view:views)view.reader().documentCommandsEnabled(enabled);
        long generation=bridge==null||bridge.latest()==null?0:bridge.latest().generation();
        boolean interactive=ready()&&pendingDocumentCloses.isEmpty();
        if(tools!=null)tools.sessionState(generation,interactive);
        for(var view:views)view.reader().sessionState(generation,interactive);
    }
    private Bridge.Document resolveDocument(VirtualFile file){
        var key=file.getUserData(PanelEditorProvider.DOCUMENT);
        return key==null||bridge==null||bridge.latest()==null?null:key.resolve(bridge.latest().generation(),snapshot).orElse(null);
    }
    private void selectDocument(VirtualFile file){
        var doc=resolveDocument(file);
        if(doc!=null&&snapshot.selection().document()!=doc.id())select(new Bridge.Selection(0,doc.id(),0));
    }
    private void openDocument(Bridge.Document document){
        if(!ready())return;
        var key=new Bridge.DocumentKey(bridge.latest().generation(),snapshot.project(),document.kind(),document.id());
        if(key.resolve(bridge.latest().generation(),snapshot).isEmpty())return;
        open(key.panel()+":"+Long.toUnsignedString(key.id()));
    }
    private void newDocument(int kind){
        if(!supports("create-document")||snapshot.playing())return;
        Bridge participant=bridge;long generation=participant.latest().generation(),revision=snapshot.revision();
        String label=new String[]{"Scene","Prefab","Graph"}[kind];
        JTextField nameInput=new JTextField(label);JCheckBox defaults=new JCheckBox("Registered Health / Movement components",true);
        JPanel form=new JPanel(new GridLayout(0,1,6,6));form.add(new JLabel("Document name"));form.add(nameInput);if(kind==1)form.add(defaults);
        if(JOptionPane.showConfirmDialog(WindowManager.getInstance().getFrame(project),form,"New "+label,JOptionPane.OK_CANCEL_OPTION)!=JOptionPane.OK_OPTION)return;
        String[] args=kind==1&&defaults.isSelected()?new String[]{"1",nameInput.getText(),"registered-components"}:new String[]{Integer.toString(kind),nameInput.getText()};
        participant.callAt(generation,revision,15,args).whenComplete((reply,error)->edt(()->{
            if(error!=null){documentFailure(error);return;}
            if(bridge!=participant||bridge.latest().generation()!=reply.generation())return;
            reply.snapshot().documents().stream().filter(d->d.id()==reply.snapshot().selection().document()).findFirst().ifPresent(this::openDocument);
        }));
    }
    private void renameDocument(){
        if(!supports("rename-document")||snapshot.playing())return;
        snapshot.documents().stream().filter(d->d.id()==snapshot.selection().document())
            .findFirst().ifPresent(doc->documentAction("rename",doc));
    }
    private void documentAction(String action,Bridge.Document selected){
        if(!supports(action.equals("rename")?"rename-document":"document-lifecycle")||snapshot.playing()||
            snapshot.documents().stream().noneMatch(doc->doc.id()==selected.id()))return;
        Bridge participant=bridge;
        long generation=participant.latest().generation(),revision=snapshot.revision();
        String id=Long.toUnsignedString(selected.id());
        CompletableFuture<Bridge.Reply> request;
        if(action.equals("delete")){
            if(selected.id()==snapshot.entry())return;
            if(Messages.showYesNoDialog(project,"Delete "+selected.name()+"? Its open tabs will close. You can undo the deletion.",
                "Delete Document",Messages.getWarningIcon())!=Messages.YES)return;
            request=participant.callAt(generation,revision,35,"delete",id);
        }else{
            boolean duplicate=action.equals("duplicate");
            String title=duplicate?"Duplicate Document":"Rename Document";
            String name=Messages.showInputDialog(project,"Document name",title,Messages.getQuestionIcon(),
                selected.name()+(duplicate?" Copy":""),null);
            if(name==null)return;
            request=duplicate?participant.callAt(generation,revision,35,"duplicate",id,name)
                :participant.callAt(generation,revision,5,id,name);
        }
        request.whenComplete((reply,error)->edt(()->{
            if(bridge!=participant||participant.latest().generation()!=generation)return;
            if(error!=null){documentFailure(error);return;}
            if(action.equals("duplicate")){
                reply.snapshot().documents().stream().filter(doc->doc.id()==reply.snapshot().selection().document())
                    .findFirst().ifPresent(this::openDocument);
            }else if(action.equals("delete")){
                var iterator=documents.values().iterator();
                while(iterator.hasNext()){
                    VirtualFile file=iterator.next();
                    var key=file.getUserData(PanelEditorProvider.DOCUMENT);
                    if(key!=null&&key.id()==selected.id()&&key.generation()==generation){
                        iterator.remove();
                        ((com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project)).closeFile(file,true,true);
                    }
                }
            }
        }));
    }
    private void documentFailure(Throwable error){
        failure(error);if(smoke)return;
        while(error.getCause()!=null)error=error.getCause();
        Messages.showErrorDialog(project,error.getMessage(),"Document operation failed");
    }
    private void reportProblem(String message){
        if(!opened||tools==null)return;
        tools.problem(message);
        open("problems");
    }
    private void refreshDocument(DocumentView view){
        var key=view.file().getUserData(PanelEditorProvider.DOCUMENT);if(key==null)return;
        var doc=resolveDocument(view.file());
        view.title().setText(doc==null?"Document missing or stale — refresh/open from Project Explorer":
            doc.name()+" · ID "+Long.toUnsignedString(doc.id())+" · Revision "+doc.revision()+(doc.dirty()?" · Unsaved":""));
        if(doc!=null){
            Boolean old=view.file().getUserData(PanelEditorProvider.DIRTY);view.file().putUserData(PanelEditorProvider.DIRTY,doc.dirty());
            if(!Objects.equals(old,doc.dirty())){
                for(FileEditor editor:FileEditorManager.getInstance(project).getAllEditors(view.file()))if(editor instanceof PanelEditorProvider.Editor bound)bound.modified(Boolean.TRUE.equals(old),doc.dirty());
                FileEditorManager.getInstance(project).updateFilePresentation(view.file());
            }
        }
        if(doc!=null&&!view.file().getName().equals(doc.name()))ApplicationManager.getApplication().runWriteAction(()->{
            try{view.file().rename(this,doc.name());FileEditorManager.getInstance(project).updateFilePresentation(view.file());}catch(IOException error){failure(error);}
        });
    }
    private ToolWindow group(String id,ToolWindowAnchor anchor,boolean split){
        ToolWindow tool=ToolWindowManager.getInstance(project).registerToolWindow(RegisterToolWindowTask.notClosable(id,anchor));
        tool.getComponent().putClientProperty(com.intellij.openapi.wm.impl.content.ToolWindowContentUi.HIDE_ID_LABEL,"true");
        com.intellij.openapi.wm.impl.content.ToolWindowContentUi.setAllowTabsReordering(tool,true);
        tool.setStripeTitle(id.replace("Epidemic ",""));
        tool.setContentUiType(ToolWindowContentUiType.TABBED,null);tool.setSplitMode(split,null);groups.put(id,tool);return tool;
    }
    private void workspace(Bridge.Snapshot value){
        snapshot=value;opened=true;launcher.setVisible(false);
        // Reconnect explicitly rebinds presentation keys only after the native
        // project and document identities have been confirmed in a new snapshot.
        for(VirtualFile file:documents.values()){
            var old=file.getUserData(PanelEditorProvider.DOCUMENT);
            if(old!=null&&old.project()==value.project())file.putUserData(PanelEditorProvider.DOCUMENT,
                new Bridge.DocumentKey(bridge.latest().generation(),old.project(),old.kind(),old.id()));
        }
        for(var view:views)refreshDocument(view);
        refreshDocumentCommands();
        if(!assetQuery.isEmpty())bridge.call(28,assetQuery).whenComplete((r,e)->{if(e!=null)edt(()->documentFailure(e));});
        if(reconnectSelection!=null){
            var wanted=reconnectSelection;reconnectSelection=null;
            if(wanted.document()==0||wanted.kind()==0&&value.documents().stream().anyMatch(d->d.id()==wanted.document())||value.nodes().stream().anyMatch(n->n.selection().equals(wanted)))select(wanted);
        }
        CompletableFuture.runAsync(()->{try{recent.opened(value);}catch(IOException e){throw new UncheckedIOException(e);}},io).exceptionally(e->{edt(()->failure(e));return null;});
        if(tools==null){
            tools=reader();tools.refresh(snapshot);
            navigation=group("Epidemic Navigation",ToolWindowAnchor.LEFT,false);
            primary=group("Epidemic Inspector",ToolWindowAnchor.RIGHT,false);
            secondary=group("Epidemic Context",ToolWindowAnchor.RIGHT,true);
            bottom=group("Epidemic Output",ToolWindowAnchor.BOTTOM,false);
            add("project.explorer",navigation);add("scene.hierarchy",navigation);navigation.getContentManager().setSelectedContent(contents.get("project.explorer"));
            installActions();
        }
        boolean initialDocuments=documents.isEmpty();
        JFrame frame=WindowManager.getInstance().getFrame(project);require(frame!=null,"No IntelliJ frame");
        if(initialDocuments){
            if(!smoke&&Boolean.getBoolean("epidemic.ui.standalone"))frame.setExtendedState(frame.getExtendedState()|Frame.MAXIMIZED_BOTH);
            else {frame.setExtendedState(Frame.NORMAL);frame.setSize(1280,800);}
            for(String id:List.of("game.view","scene.editor","graph.editor","code.space"))open(id);
            open("scene.editor");context("Scene");navigation.show(null);primary.show(null);secondary.show(null);
        }
        frame.setVisible(true);
        record("PASS Platform native project="+Long.toUnsignedString(snapshot.project())+" documents="+snapshot.documents().size()+" tool contents="+navigation.getContentManager().getContentCount()+" FileEditorProvider="+PanelEditorProvider.class.getName());
        if(initialDocuments&&(smoke||!Files.exists(profile.resolve("platform-panels.properties"))))later(400,()->{
            if(navigation instanceof com.intellij.openapi.wm.ex.ToolWindowEx left)left.stretchWidth(260-navigation.getComponent().getWidth());
            if(primary instanceof com.intellij.openapi.wm.ex.ToolWindowEx right)right.stretchWidth(320-primary.getComponent().getWidth());
        });
        if(smoke){
            if(recoveringSmoke)later(500,()->{
                require(snapshot.project()==recoveryProject&&views.size()==recoveryViews&&Set.copyOf(views).equals(recoveryReaders)&&frame.getBounds().equals(recoveryBounds),"Reconnect duplicated/lost project, view identities or frame bounds");
                for(var view:views)if(view.file().getUserData(PanelEditorProvider.DOCUMENT)!=null)require(resolveDocument(view.file())!=null&&!view.title().getText().startsWith("Document missing"),"Reconnect left a stale document view");
                record("PASS Platform crash/reconnect: same project identity and view instances; single replacement companion");smokeCloseAllViews();
            });else later(1800,this::verify);
        }else if(initialDocuments&&Files.exists(profile.resolve("platform-panels.properties")))restorePanels();
    }
    private void add(String id,ToolWindow window){
        Content content=contents.get(id);
        if(content==null){content=ContentFactory.getInstance().createContent(tools.view(id),catalog.panel(id).label(),true);contents.put(id,content);}
        if(saveAction!=null)registerWorkspaceShortcuts(content.getComponent());
        if(content.getManager()!=window.getContentManager()){
            if(content.getManager()!=null)content.getManager().removeContent(content,false);
            window.getContentManager().addContent(content);
        }targets.put(id,window.getId());
    }
    private void context(String value){
        if(tools==null||value.equals(context))return;
        for(ToolWindow window:List.of(primary,secondary)){
            Content selected=window.getContentManager().getSelectedContent();
            if(selected!=null)selectedContextTabs.put(context+":"+window.getId(),selected);
        }
        context=value;
        for(ToolWindow window:List.of(primary,secondary)){
            String anchor=window==primary?"dock.center.right.primary":"dock.center.right.secondary";
            for(Content content:window.getContentManager().getContents()){
                String id=contents.entrySet().stream().filter(e->e.getValue()==content).map(Map.Entry::getKey).findFirst().orElse("");
                // A user-moved Window-mode group is preserved by IntelliJ; do not steal its contents.
                if(window.getType()==ToolWindowType.DOCKED&&catalog.contains(id)&&catalog.panel(id).context().equals(value)==false)window.getContentManager().removeContent(content,false);
            }
            catalog.panels().stream().filter(p->p.anchor().equals(anchor)&&p.context().equals(value)&&p.initial()).sorted(Comparator.comparingInt(Catalog.Panel::order)).forEach(p->add(p.id(),window));
            Content previous=selectedContextTabs.get(value+":"+window.getId());
            if(previous!=null&&previous.getManager()==window.getContentManager())window.getContentManager().setSelectedContent(previous);
            else if(window.getContentManager().getContentCount()>0)window.getContentManager().setSelectedContent(window.getContentManager().getContent(0));
        }
    }
    public void open(String id){
        if(!opened)return;Catalog.Panel panel=catalog.panel(id);
        if(panel.anchor().equals("dock.center.documents")){
            Bridge.DocumentKey identity=null;Bridge.Document document=null;
            if(id.contains(":")){
                long nativeId=Long.parseUnsignedLong(id.substring(id.indexOf(':')+1));
                document=snapshot.documents().stream().filter(d->d.id()==nativeId).findFirst().orElse(null);
                if(document==null)return;
                identity=new Bridge.DocumentKey(bridge.latest().generation(),snapshot.project(),document.kind(),nativeId);
                if(!id.startsWith(identity.panel()+":"))return;
            }
            VirtualFile file=documents.get(id);
            if(file==null){LightVirtualFile created=new LightVirtualFile(document==null?panel.label():document.name(),"");created.putUserData(PanelEditorProvider.PANEL,id);created.putUserData(PanelEditorProvider.DOCUMENT,identity);created.putUserData(PanelEditorProvider.OWNER,this);documents.put(id,created);file=created;}
            FileEditorManager manager=FileEditorManager.getInstance(project);manager.openFile(file,true);manager.setSelectedEditor(file,"epidemic.platform.panel");selectedPanel=id;context(Catalog.context(id));
            selectDocument(file);
        }else{
            ToolWindow target=groups.get(targets.get(id));
            if(target==null)target=panel.anchor().equals("dock.left.navigation")?navigation:panel.anchor().equals("dock.bottom.global")?bottom:panel.anchor().equals("dock.center.right.secondary")?secondary:primary;
            add(id,target);target.getContentManager().setSelectedContent(contents.get(id));target.show(null);
        }
    }
    public DocumentView documentView(VirtualFile file){
        String id=file.getUserData(PanelEditorProvider.PANEL);
        UiPanels reader=reader();reader.refresh(snapshot);JPanel root=new JPanel(new BorderLayout());
        DefaultActionGroup commands=new DefaultActionGroup();commands.add(saveAction);
        commands.add(undoAction);commands.add(redoAction);
        commands.addSeparator();commands.add(playAction);commands.add(stopAction);commands.add(action("Build · F9",()->{},()->false));
        commands.add(action("Jobs",()->open("background.jobs"),()->opened));commands.add(action("Find Panel",this::palette,()->opened));
        ActionToolbar toolbar=ActionManager.getInstance().createActionToolbar("Epidemic.Document.Toolbar",commands,true);toolbar.setTargetComponent(root);
        JPanel header=new JPanel(new BorderLayout());
        if(!Boolean.getBoolean("epidemic.ui.standalone"))header.add(toolbar.getComponent(),BorderLayout.NORTH);
        JLabel title=new JLabel();if(file.getUserData(PanelEditorProvider.DOCUMENT)!=null)header.add(title,BorderLayout.SOUTH);root.add(header,BorderLayout.NORTH);root.add(reader.view(id));
        registerWorkspaceShortcuts(root);
        DocumentView view=new DocumentView(reader,root,file,title);views.add(view);
        root.addHierarchyListener(event->{if((event.getChangeFlags()&(HierarchyEvent.PARENT_CHANGED|HierarchyEvent.SHOWING_CHANGED))!=0)edt(()->{
            installCheckedTabClose(file);
            guardFloatingWindow(view);
        });});
        edt(()->installCheckedTabClose(file));
        refreshDocument(view);refreshDocumentCommands();return view;
    }
    public void release(DocumentView view){
        views.remove(view);
        Object shortcuts=view.component().getClientProperty("epidemic.shortcuts");
        if(shortcuts instanceof Disposable lifetime)com.intellij.openapi.util.Disposer.dispose(lifetime);
        view.component().putClientProperty("epidemic.shortcuts",null);
        view.component().removeAll();
    }
    public boolean documentModified(VirtualFile file){var doc=resolveDocument(file);return doc!=null&&doc.dirty();}
    private boolean documentNeedsClosePrompt(VirtualFile file){
        var doc=resolveDocument(file);
        return doc!=null&&(doc.dirty()||tools!=null&&tools.hasDraft(doc.id())||
            views.stream().anyMatch(view->view.reader().hasDraft(doc.id())));
    }
    private void installCheckedTabClose(VirtualFile file){
        if(file.getUserData(PanelEditorProvider.DOCUMENT)==null)return;
        var manager=(com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project);
        for(var window:manager.getWindows()){
            var tabs=window.getTabbedPane().getTabs();
            var component=tabs.getComponent();
            if(component.getClientProperty("epidemic.scene.wheel")==null){
                for(var listener:component.getMouseWheelListeners()){
                    component.removeMouseWheelListener(listener);
                    component.addMouseWheelListener(event->{
                        var deepest=SwingUtilities.getDeepestComponentAt(component,event.getX(),event.getY());
                        if(!event.isConsumed()&&!(deepest instanceof NativeViewport))listener.mouseWheelMoved(event);
                    });
                }
                component.putClientProperty("epidemic.scene.wheel",Boolean.TRUE);
            }
            var tab=tabs.findInfo(file);
            if(tab==null||!(tab.getTabLabelActions() instanceof DefaultActionGroup existing))continue;
            var group=new DefaultActionGroup();
            boolean changed=false;
            for(var action:existing.getChildren(ActionManager.getInstance())){
                if(action instanceof com.intellij.openapi.fileEditor.impl.tabActions.CloseTab){
                    group.add(new CheckedTabClose(file,window,action));
                    changed=true;
                }else group.add(action);
            }
            if(changed)tab.setTabLabelActions(group,"EditorTab");
        }
    }
    private final class CheckedTabClose extends com.intellij.openapi.project.DumbAwareAction {
        private final VirtualFile file;
        private final com.intellij.openapi.fileEditor.impl.EditorWindow window;
        private final AnAction original;
        CheckedTabClose(VirtualFile file,com.intellij.openapi.fileEditor.impl.EditorWindow window,AnAction original){
            this.file=file;this.window=window;this.original=original;copyFrom(original);
        }
        @Override public ActionUpdateThread getActionUpdateThread(){return original.getActionUpdateThread();}
        @Override public void update(AnActionEvent event){original.update(event);}
        @Override public void actionPerformed(AnActionEvent event){
            if(canCloseDocument(file,window))original.actionPerformed(event);
        }
    }
    public boolean canCloseDocument(VirtualFile file){
        if(file.getUserData(PanelEditorProvider.DOCUMENT)==null)return true;
        var manager=(com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project);
        var current=manager.getCurrentWindow();
        var target=current!=null&&current.getComposite(file)!=null?current:
            Arrays.stream(manager.getWindows()).filter(window->window.getComposite(file)!=null).findFirst().orElse(null);
        return canCloseDocument(file,target);
    }
    private void closeTabChecked(VirtualFile file,com.intellij.openapi.fileEditor.impl.EditorWindow target){
        if(opened&&target!=null&&target.getComposite(file)!=null&&canCloseDocument(file,target))
            ((com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project)).closeFile(file,target);
    }
    private boolean canCloseDocument(VirtualFile file,com.intellij.openapi.fileEditor.impl.EditorWindow target){
        if(activeSave!=null&&!activeSave.isDone()){
            activeSave.whenComplete((reply,error)->{if(error==null)edt(()->closeTabChecked(file,target));});return false;
        }
        if(!ready()){
            if(service==Bridge.Service.Loading&&readyActions.size()<32){
                readyActions.addLast(()->closeTabChecked(file,target));
            }
            return false;
        }
        if(!documentNeedsClosePrompt(file))return true;
        if(!ready()||pendingDocumentCloses.contains(file))return false;
        int decision=promptDocumentClose(file);
        requestDocumentClose(file,decision,null,target);return false;
    }
    private int promptDocumentClose(VirtualFile file){if(smoke&&smokeWindowChoice!=null){smokeClosePrompts++;return smokeWindowChoice;}return Messages.showYesNoCancelDialog(project,"Save all project changes, or discard only this document? Other documents and imported assets are retained.","Close "+file.getName(),"Save All","Discard Document","Cancel",Messages.getQuestionIcon());}
    private void guardFloatingWindow(DocumentView view){
        Window window=SwingUtilities.getWindowAncestor(view.component());
        if(window==null||window==WindowManager.getInstance().getFrame(project)||floatingGuards.containsKey(window))return;
        if(smoke)record("INFO F0 floating candidate="+window.getClass().getName()+" showing="+window.isShowing()+" listeners="+Arrays.stream(window.getWindowListeners()).map(listener->listener.getClass().getName()).toList());
        // Platform 253 closes DockWindow contents without VirtualFilePreCloseCheck.
        // Keep its original lifecycle/docking handler, gating only native dirty close.
        List<WindowListener> originals=Arrays.stream(window.getWindowListeners()).filter(listener->Set.of("com.intellij.ui.docking.impl.DockWindow$installListeners$1","com.intellij.openapi.ui.FrameWrapper$createContents$windowListener$1").contains(listener.getClass().getName())).toList();
        if(originals.size()!=2)return;
        WindowAdapter guard=new WindowAdapter(){
            @Override public void windowClosing(WindowEvent event){com.intellij.openapi.application.WriteIntentReadAction.run((Runnable)()->closeFloatingWindow(window,smoke?smokeWindowChoice:null));}
            @Override public void windowClosed(WindowEvent event){floatingGuards.remove(window);originals.forEach(listener->listener.windowClosed(event));}
            @Override public void windowOpened(WindowEvent event){originals.forEach(listener->listener.windowOpened(event));}
            @Override public void windowIconified(WindowEvent event){originals.forEach(listener->listener.windowIconified(event));}
            @Override public void windowDeiconified(WindowEvent event){originals.forEach(listener->listener.windowDeiconified(event));}
            @Override public void windowActivated(WindowEvent event){originals.forEach(listener->listener.windowActivated(event));}
            @Override public void windowDeactivated(WindowEvent event){originals.forEach(listener->listener.windowDeactivated(event));}
        };
        originals.forEach(window::removeWindowListener);window.addWindowListener(guard);floatingGuards.put(window,new FloatingGuard(originals,guard));
    }
    private void closeFloatingWindow(Window window,Integer suppliedDecision){
        FloatingGuard guard=floatingGuards.get(window);if(guard==null||!window.isDisplayable())return;
        if(activeSave!=null&&!activeSave.isDone()){activeSave.whenComplete((reply,error)->{if(error==null)edt(()->closeFloatingWindow(window,suppliedDecision));});return;}
        if(!ready()){
            if(service==Bridge.Service.Loading&&readyActions.size()<32)readyActions.addLast(()->closeFloatingWindow(window,suppliedDecision));
            return;
        }
        var dirty=views.stream().filter(view->SwingUtilities.getWindowAncestor(view.component())==window).map(DocumentView::file).filter(this::documentNeedsClosePrompt).findFirst();
        if(dirty.isEmpty()){var event=new WindowEvent(window,WindowEvent.WINDOW_CLOSING);guard.originals().forEach(listener->listener.windowClosing(event));return;}
        int decision=suppliedDecision==null?promptDocumentClose(dirty.get()):suppliedDecision;
        var manager=(com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project);
        var target=Arrays.stream(manager.getWindows()).filter(editor->editor.getComposite(dirty.get())!=null&&SwingUtilities.getWindowAncestor(editor.getTabbedPane().getComponent())==window).findFirst().orElse(null);
        requestDocumentClose(dirty.get(),decision,()->closeFloatingWindow(window,null),target);
    }
    private void requestDocumentClose(VirtualFile file,int decision,Runnable afterClose,com.intellij.openapi.fileEditor.impl.EditorWindow target){
        if(decision==Messages.CANCEL||pendingDocumentCloses.contains(file))return;
        if(!ready()){
            if(service==Bridge.Service.Loading&&readyActions.size()<32)readyActions.addLast(()->{if(opened&&(!rejectedCommand||decision==Messages.NO))requestDocumentClose(file,decision,afterClose,target);});
            return;
        }
        if(target==null||target.getComposite(file)==null){documentFailure(new IOException("ui.document_window_missing"));return;}
        var doc=resolveDocument(file);if(doc==null){pendingDocumentCloses.remove(file);return;}
        Bridge participant=bridge;var identity=file.getUserData(PanelEditorProvider.DOCUMENT);
        if(decision==Messages.NO){if(tools!=null)tools.cancelDrafts(doc.id());for(var view:views)view.reader().cancelDrafts(doc.id());}
        if(decision==Messages.NO&&!doc.dirty()){
            ((com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project)).closeFile(file,target);
            if(afterClose!=null)edt(afterClose);
            return;
        }
        var operation=decision==Messages.YES?saveProject():participant.callAt(identity.generation(),snapshot.revision(),17,Long.toUnsignedString(doc.id()));
        pendingDocumentCloses.add(file);
        refreshDocumentCommands();
        operation.whenComplete((reply,error)->edt(()->{
            pendingDocumentCloses.remove(file);
            if(bridge!=participant)return;
            if(error!=null){documentFailure(error);refreshDocumentCommands();return;}
            if(identity.project()==snapshot.project()&&target.getComposite(file)!=null)((com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project)).closeFile(file,target);
            refreshDocumentCommands();flushSelection();
            if(afterClose!=null)edt(afterClose);
            if(!readyActions.isEmpty())edt(readyActions.removeFirst());
        }));
    }
    private boolean ready(){return opened&&!closing&&pendingDocumentCloses.isEmpty()&&bridge!=null&&service==Bridge.Service.Ready&&snapshot!=null;}
    private boolean supports(String capability){return ready()&&bridge.latest()!=null&&bridge.latest().capabilities().contains(capability);}
    private boolean canClone(){return supports("clone-folder")&&snapshot.storage()==2&&!snapshot.dirty();}
    private void playback(boolean play){
        if(!supports(play?"play":"stop"))return;
        features.dispatch(playFeature,true,()->command(play?13:14));
    }
    private void togglePlayback(){if(supports("play")&&supports("stop"))playback(!snapshot.playing());}
    private void cloneProject(){
        if(!canClone())return;
        Bridge participant=bridge;long generation=participant.latest().generation(),revision=snapshot.revision();
        Path source=Path.of(snapshot.path()).getParent(),parentPath=source.getParent();
        JTextField name=new JTextField(source.getFileName()==null?"Project Copy":source.getFileName()+" Copy");
        JTextField parent=new JTextField(parentPath==null?System.getProperty("user.home"):parentPath.toString());
        JPanel form=new JPanel(new GridLayout(0,1,6,6));form.add(new JLabel("Clone folder name"));form.add(name);form.add(new JLabel("Parent directory"));form.add(parent);
        JFrame owner=WindowManager.getInstance().getFrame(project);
        if(JOptionPane.showConfirmDialog(owner,form,"Clone Saved Project",JOptionPane.OK_CANCEL_OPTION)!=JOptionPane.OK_OPTION)return;
        try{
            if(!ProjectNames.valid(name.getText())||parent.getText().isBlank())throw new IOException("Invalid clone folder name or parent");
            Path destination=Path.of(parent.getText()).resolve(name.getText()).toAbsolutePath();
            if(!Files.isDirectory(destination.getParent()))throw new IOException("Clone parent directory does not exist");
            features.dispatch(lifecycleFeature,canClone(),()->participant.callAt(generation,revision,12,destination.toString()).whenComplete((reply,error)->edt(()->{
                if(error!=null)failure(error);else Messages.showInfoMessage(project,"Saved project cloned to:\n"+destination,"Clone Project");
            })));
        }catch(IOException|RuntimeException error){failure(error);}
    }
    private void command(int operation,String...args){if(!ready())return;bridge.call(operation,args).whenComplete((r,e)->{if(e!=null)edt(()->failure(e));});}
    private void startAssetPolling(){
        if(assetPoll!=null)return;
        assetPoll=new javax.swing.Timer(500,event->{
            if(smoke||!ready()||assetPolling||!supports("asset-pipeline"))return;
            assetPolling=true;Bridge participant=bridge;
            participant.call(26).whenComplete((reply,error)->edt(()->{assetPolling=false;if(bridge==participant&&error!=null&&service!=Bridge.Service.Error)documentFailure(error);}));
        });assetPoll.start();com.intellij.openapi.util.Disposer.register(this,()->assetPoll.stop());
    }
    private void assetAction(String action,Long id){
        if(!supports("asset-pipeline"))return;
        if(action.startsWith("Search:")){assetQuery=action.substring(7);bridge.call(28,assetQuery).whenComplete((r,e)->{if(e!=null)edt(()->documentFailure(e));});return;}
        if(action.equals("Watch")){int choice=Messages.showYesNoCancelDialog(project,"Automatically reimport files changed inside this project's Content folder?","Watch Content","Enable","Disable","Cancel",Messages.getQuestionIcon());if(choice!=Messages.CANCEL)bridge.call(27,choice==Messages.YES?"1":"0");return;}
        var asset=snapshot.assets().stream().filter(a->a.id()==id).findFirst().orElse(null);if(asset==null){Messages.showInfoMessage(project,"Select an asset in Project Explorer first.","Asset command");return;}
        long generation=bridge.latest().generation(),revision=snapshot.revision(),assetRevision=snapshot.assetRevision();
        String token=Long.toUnsignedString(assetRevision),assetId=Long.toUnsignedString(id);int operation;String[] args;
        switch(action){
            case "Rename / Move","Duplicate"->{
                String suggested=asset.path().replaceFirst("^Content/","");
                if(action.equals("Duplicate")){
                    int dot=suggested.lastIndexOf('.');
                    if(dot>suggested.lastIndexOf('/'))suggested=suggested.substring(0,dot)+" Copy"+suggested.substring(dot);
                    else suggested+=" Copy";
                }
                String path=Messages.showInputDialog(project,"New path inside Content. The destination folder must already exist.",action,Messages.getQuestionIcon(),suggested,null);
                if(path==null)return;operation=action.equals("Duplicate")?21:20;args=new String[]{token,assetId,path};
            }
            case "Reimport"->{operation=19;args=new String[]{token,assetId};}
            case "Restore"->{operation=23;args=new String[]{token,assetId};}
            case "Delete"->{
                var incoming=snapshot.assetState().references().stream().filter(r->r.source()==id).toList();
                String names=incoming.stream().limit(20).map(r->snapshot.documents().stream().filter(d->d.id()==r.dependent()).findFirst().map(d->d.name()+" ["+Long.toUnsignedString(d.id())+"]").orElse(Long.toUnsignedString(r.dependent()))+" · "+r.origin()).collect(java.util.stream.Collectors.joining("\n"));
                String message="Stage delete "+asset.path()+"?\nIncoming references ("+incoming.size()+"):\n"+names+(incoming.size()>20?"\n...see Asset References for the remaining IDs":"")+"\nStable identities remain missing; Restore is available.";
                if(Messages.showYesNoDialog(project,message,"Delete asset",Messages.getWarningIcon())!=Messages.YES)return;operation=22;args=new String[]{token,assetId,incoming.isEmpty()?"0":"1"};
            }
            case "Assign to Prefab"->{var prefabs=snapshot.documents().stream().filter(d->d.kind()==1).toList();if(prefabs.isEmpty()){Messages.showInfoMessage(project,"Create a Prefab first.","Assign sprite");return;}String[] choices=prefabs.stream().map(d->d.name()+" ["+Long.toUnsignedString(d.id())+"]").toArray(String[]::new);JComboBox<String> picker=new JComboBox<>(choices);if(JOptionPane.showConfirmDialog(WindowManager.getInstance().getFrame(project),picker,"Assign Sprite",JOptionPane.OK_CANCEL_OPTION)!=JOptionPane.OK_OPTION)return;int choice=picker.getSelectedIndex();if(choice<0)return;operation=24;args=new String[]{token,Long.toUnsignedString(prefabs.get(choice).id()),assetId};}
            default->{return;}
        }
        bridge.callAt(generation,revision,operation,args).whenComplete((r,e)->edt(()->{if(e!=null)documentFailure(e);}));
    }
    private void importPicker(){
        // Platform owns modal/focus state and chooses a supported picker for the
        // runtime (its own dialog on standard OpenJDK, native chooser on JBR).
        // A raw AWT FileDialog bypasses that state in an IntelliJ application.
        if(!supports("asset-pipeline")||snapshot.storage()!=2||snapshot.playing()||pickerImport!=null)return;
        Bridge participant=bridge;
        long generation=participant.latest().generation();
        FileChooserDescriptor descriptor=new FileChooserDescriptor(true,false,false,false,false,true)
            .withTitle("Import PNG / WebP / PCM WAV / TTF / OTF").withFileFilter(file->{if(file.isDirectory())return true;String ext=file.getExtension();return ext!=null&&Set.of("png","webp","wav","ttf","otf").contains(ext.toLowerCase(Locale.ROOT));});
        Window active=KeyboardFocusManager.getCurrentKeyboardFocusManager().getActiveWindow();
        Component pickerParent=active==null?WindowManager.getInstance().getFrame(project):active;
        FileChooser.chooseFiles(descriptor,project,pickerParent,null,files->{
            if(files.isEmpty()||bridge!=participant||participant.latest().generation()!=generation)return;
            String destination=Messages.showInputDialog(project,"Existing Content-relative destination folder (empty = Content root)","Import Files",Messages.getQuestionIcon(),importFolder,null);
            if(destination==null||pickerImport!=null)return;
            importFolder=destination;
            pickerImport=new PickerImport(participant,generation,List.copyOf(files),destination);
            submitImport(pickerImport,0);
        });
    }
    private void submitImport(PickerImport batch,int index){
        if(pickerImport!=batch)return;
        if(index==batch.files().size()||bridge!=batch.participant()||service==Bridge.Service.Error||
            bridge.latest().generation()!=batch.generation()){
            pickerImport=null;
            return;
        }
        VirtualFile file=batch.files().get(index);
        String prefix=batch.folder().isBlank()?"":batch.folder()+"/";
        queueImport(batch.participant(),batch.generation(),file.getPath(),prefix+file.getName())
            .whenComplete((ignored,error)->edt(()->submitImport(batch,index+1)));
    }
    private void cancelImports(){
        pickerImport=null;
        Bridge participant=bridge;
        var current=participant==null?null:participant.latest();
        if(!opened||closing||service==Bridge.Service.Error||current==null||!current.capabilities().contains("asset-pipeline"))return;
        // Both this action and batch submission run on the EDT. This request follows
        // the last submitted import and no remaining picker items can follow it.
        participant.callAt(current.generation(),0,25).whenComplete((reply,error)->edt(()->{
            if(bridge!=participant)return;
            if(error!=null)documentFailure(error);
            else tools.log("Import cancellation requested. Published assets are kept.");
        }));
    }
    private CompletableFuture<Void> queueImport(Bridge participant,long generation,String source,String destination){
        var latest=participant.latest();
        if(latest==null||latest.snapshot()==null||latest.generation()!=generation)return CompletableFuture.completedFuture(null);
        var current=latest.snapshot();
        return participant.callAt(generation,current.revision(),18,Long.toUnsignedString(current.assetRevision()),source,destination)
            .handle((reply,error)->null);
    }
    private AnAction action(String label,Runnable run,java.util.function.BooleanSupplier enabled){
        return new AnAction(label){
            {getTemplatePresentation().putClientProperty(com.intellij.openapi.actionSystem.ex.ActionUtil.SHOW_TEXT_IN_TOOLBAR,true);}
            public ActionUpdateThread getActionUpdateThread(){return ActionUpdateThread.EDT;}
            public void update(AnActionEvent event){event.getPresentation().setEnabled(enabled.getAsBoolean());}
            public void actionPerformed(AnActionEvent event){run.run();}
        };
    }
    private void installActions(){
        menu=new DefaultActionGroup("Epidemic",true);Map<String,DefaultActionGroup> sub=new LinkedHashMap<>();
        for(String name:List.of("File","Edit","View","Project","Assets","GameObject","Graph","Build","Tools","Window","Help")){DefaultActionGroup group=new DefaultActionGroup(name,true);menu.add(group);sub.put(name,group);}
        lifecycleFeature=features.add("project.lifecycle","Project commands","F0","Native Workspace lifecycle",true);
        playFeature=features.add("play.session","Play / Stop","F0","Native isolated session lifecycle; Game View integration remains planned",true);
        saveAction=action("Save",()->saveProject().whenComplete((r,e)->{if(e!=null)edt(()->documentFailure(e));}),()->opened&&!closing&&service!=Bridge.Service.Error);
        saveAllAction=action("Save All",()->saveProject().whenComplete((r,e)->{if(e!=null)edt(()->documentFailure(e));}),()->opened&&!closing&&service!=Bridge.Service.Error);
        undoAction=action("Undo",()->command(7),()->ready()&&snapshot.canUndo()&&!editableTextFocus());
        redoAction=action("Redo",()->command(8),()->ready()&&snapshot.canRedo()&&!editableTextFocus());
        playAction=action("Play",()->playback(true),()->supports("play")&&supports("stop")&&!snapshot.playing());
        stopAction=action("Stop",()->playback(false),()->supports("stop")&&snapshot.playing());
        playToggleAction=action("Play / Stop",this::togglePlayback,()->supports("play")&&supports("stop"));
        playAction.getTemplatePresentation().setDescription("Start an isolated Play session. Game View preview is not connected yet.");
        sub.get("File").add(saveAction);sub.get("File").add(saveAllAction);sub.get("File").add(action("Clone Saved Project…",this::cloneProject,this::canClone));sub.get("File").add(action("Close Project",()->close(false),()->opened));
        for(int kind=0;kind<3;kind++){
            final int type=kind;sub.get("File").add(action("New "+new String[]{"Scene","Prefab","Graph"}[kind]+"…",()->newDocument(type),()->supports("create-document")&&!snapshot.playing()));
        }
        sub.get("Edit").add(undoAction);sub.get("Edit").add(redoAction);
        sub.get("Edit").add(action("Rename selected document…",this::renameDocument,()->supports("rename-document")&&!snapshot.playing()&&snapshot.documents().stream().anyMatch(d->d.id()==snapshot.selection().document())));
        for(Catalog.Panel panel:catalog.panels())sub.get("View").add(action(panel.label(),()->open(panel.id()),()->opened));
        sub.get("Project").add(action("Project Settings",()->open("settings.project"),()->opened));sub.get("Project").add(action("Reconnect native backend",this::reconnect,()->opened&&service==Bridge.Service.Error));
        sub.get("Project").addSeparator();sub.get("Project").add(playAction);sub.get("Project").add(stopAction);
        sub.get("Assets").add(action("Import…",this::importPicker,()->supports("asset-pipeline")&&snapshot.storage()==2&&!snapshot.playing()&&pickerImport==null));
        sub.get("Assets").add(action("Watch Content…",()->assetAction("Watch",0L),()->supports("asset-pipeline")&&snapshot.storage()==2));
        sub.get("Assets").add(action("Cancel native imports",this::cancelImports,()->opened&&service!=Bridge.Service.Error));
        for(String group:List.of("GameObject","Graph","Build"))sub.get(group).add(action("Backend planned for "+(group.equals("Graph")?"F3":group.equals("Build")?"F9":"F2"),()->{},()->false));
        sub.get("Tools").add(action("UI Gallery",()->open("settings.editor"),()->opened));sub.get("Tools").add(action("Type Registry",()->open("type.registry"),()->opened));sub.get("Tools").add(action("Find Panel…",this::palette,()->opened));
        sub.get("Window").add(action("Move Inspector to Windows window",()->primary.setType(ToolWindowType.WINDOWED,()->primary.show(null)),()->opened));
        sub.get("Window").add(action("Dock Inspector",()->primary.setType(ToolWindowType.DOCKED,()->primary.show(null)),()->opened));
        sub.get("Window").add(action("Save panel layout",this::savePanels,()->opened));sub.get("Window").add(action("Restore panel layout",this::restorePanels,()->opened));
        sub.get("Window").add(action("Reset panel layout",this::resetPanels,()->opened));
        sub.get("Help").add(action("About Epidemic UI",()->Messages.showInfoMessage(project,"JetBrains IntelliJ Platform 253.31033.145\nNative C++ Workspace / versioned IPC\nToolWindowManager and FileEditorProvider own docking.\nUnwired future features remain disabled.","Epidemic Editor"),()->true));
        ActionManager actions=ActionManager.getInstance();actions.registerAction("Epidemic.MainMenu",menu,PluginId.getId(Boolean.getBoolean("epidemic.ui.standalone")?"com.intellij":"epidemic.editor.platform"));
        DefaultActionGroup mainMenu=(DefaultActionGroup)actions.getAction("MainMenu");
        if(Boolean.getBoolean("epidemic.ui.standalone")){
            mainMenu.removeAll();mainMenu.addAll(menu.getChildren(actions));
            var center=(DefaultActionGroup)actions.getAction("MainToolbarCenter");
            if(center!=null){
                center.removeAll();
                for(var button:List.of(saveAction,undoAction,redoAction,playAction,stopAction,
                    action("Jobs",()->open("background.jobs"),()->opened),
                    action("Find Panel",this::palette,()->opened)))center.add(button);
            }
            removeToolbarAction(actions,"MainToolbarLeft","main.toolbar.Project");
            removeToolbarAction(actions,"MainToolbarRight","NewUiRunWidget");
            removeToolbarAction(actions,"MainToolbarRight","ExecutionTargetsToolbarGroup");
            com.intellij.ide.ui.UISettings.getInstance().fireUISettingsChanged();
        }
        else mainMenu.add(menu);
        registerWorkspaceShortcuts(WindowManager.getInstance().getFrame(project).getRootPane());
        for(Content content:contents.values())registerWorkspaceShortcuts(content.getComponent());
    }
    private static void removeToolbarAction(ActionManager manager,String groupId,String actionId){
        var action=manager.getAction(actionId);
        if(action!=null&&manager.getAction(groupId) instanceof DefaultActionGroup group)
            group.remove(action,manager);
    }
    private static boolean editableTextFocus(){
        return KeyboardFocusManager.getCurrentKeyboardFocusManager().getFocusOwner() instanceof javax.swing.text.JTextComponent text&&text.isEditable();
    }
    private void registerWorkspaceShortcuts(JComponent target){
        if(target.getClientProperty("epidemic.shortcuts")!=null)return;
        Disposable lifetime=com.intellij.openapi.util.Disposer.newDisposable("Epidemic workspace shortcuts");
        com.intellij.openapi.util.Disposer.register(this,lifetime);
        target.putClientProperty("epidemic.shortcuts",lifetime);
        saveAction.registerCustomShortcutSet(CustomShortcutSet.fromString("control S"),target,lifetime);
        saveAllAction.registerCustomShortcutSet(CustomShortcutSet.fromString("control shift S"),target,lifetime);
        undoAction.registerCustomShortcutSet(CustomShortcutSet.fromString("control Z"),target,lifetime);
        redoAction.registerCustomShortcutSet(CustomShortcutSet.fromString("control Y","control shift Z"),target,lifetime);
        playToggleAction.registerCustomShortcutSet(CustomShortcutSet.fromString("F8"),target,lifetime);
    }
    private void palette(){String[] labels=catalog.panels().stream().map(Catalog.Panel::label).toArray(String[]::new);Object chosen=JOptionPane.showInputDialog(WindowManager.getInstance().getFrame(project),"Panel","Find Panel",JOptionPane.PLAIN_MESSAGE,null,labels,labels[0]);if(chosen!=null)catalog.panels().stream().filter(p->p.label().equals(chosen)).findFirst().ifPresent(p->open(p.id()));}
    private void resetPanels(){for(ToolWindow window:List.of(navigation,primary,secondary,bottom))window.setType(ToolWindowType.DOCKED,null);navigation.setAnchor(ToolWindowAnchor.LEFT,null);primary.setAnchor(ToolWindowAnchor.RIGHT,null);secondary.setAnchor(ToolWindowAnchor.RIGHT,null);secondary.setSplitMode(true,null);bottom.setAnchor(ToolWindowAnchor.BOTTOM,null);context="";context(Catalog.context(selectedPanel));}
    private CompletableFuture<Void> savePanels(){Properties prefs=new Properties();prefs.setProperty("version","1");prefs.setProperty("selected",selectedPanel);prefs.setProperty("documents",String.join(",",documents.keySet()));for(var entry:groups.entrySet()){ToolWindow tool=entry.getValue();prefs.setProperty(entry.getKey()+".type",tool.getType().toString());prefs.setProperty(entry.getKey()+".anchor",tool.getAnchor().toString());prefs.setProperty(entry.getKey()+".split",Boolean.toString(tool.isSplitMode()));}
        return CompletableFuture.runAsync(()->{try{Files.createDirectories(profile);Path staging=Files.createTempFile(profile,"panels-",".tmp");try(Writer writer=Files.newBufferedWriter(staging)){prefs.store(writer,"Epidemic panel IDs; detailed editor split/window geometry is owned/persisted by IntelliJ in its SDK container");}try{Files.move(staging,profile.resolve("platform-panels.properties"),StandardCopyOption.REPLACE_EXISTING,StandardCopyOption.ATOMIC_MOVE);}catch(AtomicMoveNotSupportedException e){Files.move(staging,profile.resolve("platform-panels.properties"),StandardCopyOption.REPLACE_EXISTING);}}catch(IOException e){throw new UncheckedIOException(e);}},io).whenComplete((v,e)->{if(e!=null)edt(()->failure(e));});}
    private void restorePanels(){CompletableFuture.supplyAsync(()->{Properties prefs=new Properties();try(Reader reader=Files.newBufferedReader(profile.resolve("platform-panels.properties"))){prefs.load(reader);if(!prefs.getProperty("version","").equals("1"))throw new IOException("Unknown panel preference version");return prefs;}catch(IOException e){throw new UncheckedIOException(e);}},io).whenComplete((prefs,error)->edt(()->{if(error!=null){failure(error);return;}try{for(String id:prefs.getProperty("documents","").split(","))if(catalog.contains(id))open(id);for(var entry:groups.entrySet()){ToolWindow tool=entry.getValue();String key=entry.getKey();tool.setType(ToolWindowType.valueOf(prefs.getProperty(key+".type","DOCKED")),null);tool.setAnchor(ToolWindowAnchor.fromText(prefs.getProperty(key+".anchor",tool.getAnchor().toString())),null);tool.setSplitMode(Boolean.parseBoolean(prefs.getProperty(key+".split","false")),null);}String selected=prefs.getProperty("selected","scene.editor");if(catalog.contains(selected))open(selected);}catch(Exception e){failure(e);resetPanels();}}));}
    private void reconnect(){String path=snapshot.path();reconnectSelection=snapshot.selection();pendingSelection=null;selecting=false;Bridge old=bridge;bridge=null;if(old!=null)old.close();service=Bridge.Service.Unavailable;opened=false;connect(2,path);}
    private void close(boolean exit){
        if(closing)return;
        if(activeSave!=null&&!activeSave.isDone()&&service!=Bridge.Service.Error){activeSave.whenComplete((reply,error)->{if(error==null)edt(()->close(exit));});return;}
        if(service==Bridge.Service.Loading||!pendingDocumentCloses.isEmpty()){if(readyActions.size()<32)readyActions.addLast(()->close(exit));return;}
        if(service==Bridge.Service.Error){
            if(Messages.showYesNoDialog(project,"The backend is disconnected. Unsaved native edits cannot be saved. Close this session and keep the last committed project?","Disconnected Project",Messages.getWarningIcon())!=Messages.YES){return;}
            if(bridge!=null)bridge.close();bridge=null;closedUi(exit);return;
        }
        boolean discard=false;
        if(snapshot!=null&&snapshot.dirty()){
            int answer=Messages.showYesNoCancelDialog(project,"Save project changes before closing?","Epidemic Project","Save","Discard","Cancel",Messages.getQuestionIcon());
            if(answer==Messages.CANCEL){return;}
            if(answer==Messages.YES){saveProject().whenComplete((r,e)->edt(()->{if(e!=null)documentFailure(e);else close(exit);}));return;}
            discard=true;
        }
        closing=true;
        bridge.call(10,discard?new String[]{"discard"}:new String[0]).whenComplete((reply,error)->edt(()->{
            closing=false;if(error!=null){failure(error);return;}closedUi(exit);
        }));
    }
    private void closedUi(boolean exit){opened=false;snapshot=null;pickerImport=null;loggedContentRevision=0;pendingSelection=null;selecting=false;readyActions.clear();pendingDocumentCloses.clear();if(tools!=null){tools.refresh(null);tools.sessionState(0,false);}var manager=(com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project);for(VirtualFile file:documents.values())manager.closeFile(file,true,true);documents.clear();for(ToolWindow tool:groups.values())tool.hide(null);if(exit)finish();else{WindowManager.getInstance().getFrame(project).setVisible(false);launcher();}}
    private void exit(){if(opened){close(true);return;}finish();}
    private void finish(){if(bridge==null){exitApplication();return;}bridge.shutdown().whenComplete((v,e)->edt(()->{if(e!=null){failure(e);return;}if(smoke)require(NativeViewport.metrics()[0]==0,"Project close leaked native surfaces");record("PASS Platform native shutdown; surfaces="+NativeViewport.metrics()[0]);exitApplication();}));}
    private void exitApplication(){
        if(smoke){
            int errors=com.intellij.diagnostic.MessagePool.getInstance().getFatalErrors(true,true).size();
            if(errors>0){record("FAIL Platform smoke: "+errors+" fatal Platform diagnostics; see idea.log");ApplicationManager.getApplication().exit(true,true,false,1);return;}
            record("PASS Platform smoke: no fatal Platform diagnostics");
        }
        ApplicationManager.getApplication().exit(true,true,false);
    }
    private void failure(Throwable error){while(error.getCause()!=null)error=error.getCause();record("ERROR "+error);if(launcherStatus!=null)launcherStatus.setText(error.getMessage());if(tools!=null)tools.log(error.toString());if(smoke){record("FAIL Platform smoke");ApplicationManager.getApplication().exit(true,true,false,1);}}
    private static void require(boolean result,String message){if(!result)throw new IllegalStateException(message);}
    private void later(int milliseconds,Runnable action){javax.swing.Timer timer=new javax.swing.Timer(milliseconds,event->edt(action));timer.setRepeats(false);timer.start();}
    private NativeViewport viewport(){for(DocumentView view:views)for(NativeViewport nativeView:view.reader().nativeViews().values())if(nativeView.isShowing())return nativeView;return null;}
    private void capture(String name){try{JFrame frame=WindowManager.getInstance().getFrame(project);BufferedImage image=new BufferedImage(frame.getWidth(),frame.getHeight(),BufferedImage.TYPE_INT_RGB);Graphics2D graphics=image.createGraphics();frame.printAll(graphics);NativeViewport viewport=viewport();if(viewport!=null){Path gpu=output.resolve(name+"-gpu.png");viewport.captureTo(gpu.toString());Point point=SwingUtilities.convertPoint(viewport,0,0,frame);graphics.drawImage(ImageIO.read(gpu.toFile()),point.x,point.y,viewport.getWidth(),viewport.getHeight(),null);}graphics.dispose();ImageIO.write(image,"png",output.resolve(name+".png").toFile());record("INFO own-window diagnostic print/GPU capture="+name+".png");}catch(IOException e){throw new UncheckedIOException(e);}}
    private void verify(){
        require(navigation.getContentManager().getContentCount()==2,"Navigation tabs not grouped");
        require(contents.get("prefab.editor").getManager()==primary.getContentManager()&&primary.getAnchor()==ToolWindowAnchor.RIGHT,"Prefab not on right");
        require(FileEditorManager.getInstance(project).getOpenFiles().length==4,"Missing four Platform documents");
        NativeViewport nativeView=viewport();require(nativeView!=null&&nativeView.submittedFrames()>1,"No native viewport in IntelliJ editor");
        long main=nativeView.rootWindow();capture("ui-platform-scene");record("PASS Platform 4 documents/2 navigation tabs/right Prefab/native viewport HWND="+main+" launcher-to-project-ms="+(System.nanoTime()-started)/1e6);
        primary.setType(ToolWindowType.WINDOWED,()->primary.show(()->later(700,()->{
            Window detached=SwingUtilities.getWindowAncestor(primary.getContentManager().getComponent());long root=NativeViewport.windowHandle(detached);require(root!=0&&root!=main,"IntelliJ Window mode has no distinct HWND");
            Content active=primary.getContentManager().getSelectedContent();require(active!=null,"Floating Inspector has no selected content");
            require(active.getComponent().getClientProperty("epidemic.shortcuts") instanceof Disposable,"Floating Inspector content has no workspace actions: "+active.getDisplayName());
            require(SwingUtilities.getWindowAncestor(active.getComponent())==detached,"Floating Inspector active content is outside its window");
            try{BufferedImage image=new BufferedImage(detached.getWidth(),detached.getHeight(),BufferedImage.TYPE_INT_RGB);Graphics2D graphics=image.createGraphics();detached.printAll(graphics);graphics.dispose();ImageIO.write(image,"png",output.resolve("ui-platform-inspector-windowed.png").toFile());}catch(IOException e){throw new UncheckedIOException(e);}
            record("PASS Platform Window mode distinct HWND="+root+" native selection="+snapshot.selection());
            primary.setType(ToolWindowType.DOCKED,()->primary.show(()->{capture("ui-platform-redocked");screens(0);}));
        })));
    }
    private long unchangedRevision;
    private void screens(int index){
        List<String> ids=catalog.panels().stream().map(Catalog.Panel::id).toList();
        if(index==0)unchangedRevision=snapshot.revision();
        if(index==ids.size()){
            require(snapshot.revision()==unchangedRevision&&!snapshot.dirty(),"Shell navigation mutated authoring");
            for(Catalog.Schema schema:catalog.schemas()){
                JComponent form=InspectorForms.form(schema,features);form.setSize(520,720);layout(form);
                try{BufferedImage image=new BufferedImage(520,720,BufferedImage.TYPE_INT_RGB);Graphics2D graphics=image.createGraphics();form.printAll(graphics);graphics.dispose();Path folder=output.resolve("schemas");Files.createDirectories(folder);ImageIO.write(image,"png",folder.resolve(schema.id()+".png").toFile());}catch(IOException e){throw new UncheckedIOException(e);}
            }
            savePanels().whenComplete((v,e)->edt(()->{if(e!=null){failure(e);return;}record("PASS Platform U2-U7 shell smoke: actual ToolWindowManager/FileEditorProvider/ActionManager, shared native project, 46 panels captured; 77 form diagnostic prints; navigation left authoring clean");smokeDocuments(0);}));return;
        }
        String id=ids.get(index);open(id);later(350,()->{capture("ui-platform-"+id.replace('.','-'));screens(index+1);});
    }
    private static void layout(Container parent){parent.doLayout();for(Component child:parent.getComponents())if(child instanceof Container nested)layout(nested);}
    private void smokeDocuments(int kind){
        if(kind==3){
            var doc=snapshot.documents().stream().filter(d->d.kind()==2).findFirst().orElseThrow();String id="graph.editor:"+Long.toUnsignedString(doc.id());
            bridge.call(5,Long.toUnsignedString(doc.id()),"Renamed native document").whenComplete((renamed,error)->edt(()->{
                if(error!=null){failure(error);return;}
                VirtualFile file=documents.get(id);
                require(file!=null&&file.getName().equals("Renamed native document"),"Document tab title not refreshed from native snapshot");
                for(var view:views)if(view.file()==file)require(view.title().getText().startsWith("Renamed native document"),"Document header not refreshed");
                bridge.call(6).whenComplete((saved,failure)->edt(()->{
                    if(failure!=null){failure(failure);return;}
                    require(!snapshot.dirty(),"Document Save All did not commit");
                    record("PASS F0.2 Platform native Scene/Prefab/Graph creation, typed document tabs, same-ID reuse, shared rename/header and Save All");smokeScalarClose();
                }));
            }));return;
        }
        bridge.call(15,Integer.toString(kind),"Native document "+kind).whenComplete((created,error)->edt(()->{
            if(error!=null){failure(error);return;}
            var doc=created.snapshot().documents().stream().filter(d->d.id()==created.snapshot().selection().document()).findFirst().orElseThrow();
            openDocument(doc);awaitDocument(doc,kind,0);
        }));
    }
    private void awaitDocument(Bridge.Document doc,int kind,int attempts){
        String id=new Bridge.DocumentKey(bridge.latest().generation(),snapshot.project(),doc.kind(),doc.id()).panel()+":"+Long.toUnsignedString(doc.id());
        VirtualFile file=documents.get(id);
        if(file==null||FileEditorManager.getInstance(project).getEditors(file).length==0||!ready()){
            require(attempts<30,"Native document FileEditor did not become ready");later(100,()->awaitDocument(doc,kind,attempts+1));return;
        }
        require(resolveDocument(file).id()==doc.id(),"FileEditor has no current typed native identity");
        int count=views.size();var existing=Set.copyOf(views);openDocument(doc);
        later(150,()->{
            require(documents.get(id)==file&&views.size()==count&&Set.copyOf(views).equals(existing),"Duplicate native document view/owner on second Open");
            smokeDocuments(kind+1);
        });
    }
    private static JComponent named(Container root,String name){
        if(root instanceof JComponent component&&name.equals(component.getName()))return component;
        for(Component child:root.getComponents())if(child instanceof Container container){var found=named(container,name);if(found!=null)return found;}
        return null;
    }
    private void waitFor(java.util.function.BooleanSupplier condition,int attempt,Runnable next){
        if(condition.getAsBoolean()){next.run();return;}
        require(attempt<50,"Timed out waiting for asynchronous document close/open; service="+service+" native="+Arrays.toString(NativeViewport.metrics())+" sprites="+(snapshot==null?0:snapshot.assetState().sprites().size())+" guards="+floatingGuards.size()+" views="+views.size()+" windows="+floatingGuards.keySet().stream().map(w->"showing="+w.isShowing()+", displayable="+w.isDisplayable()).toList());later(100,()->waitFor(condition,attempt+1,next));
    }
    private void smokeScalarClose(){
        require(com.intellij.openapi.vfs.VirtualFilePreCloseCheck.Companion.getEP_NAME().getExtensionsIfPointIsRegistered().stream().anyMatch(check->check instanceof DocumentCloseCheck),"Native document pre-close extension not registered");
        bridge.call(15,"1","Scalar Player","registered-components").thenCompose(created->{
            long id=created.snapshot().selection().document();var node=created.snapshot().nodes().stream().filter(n->n.document()==id&&n.kind()==2).findFirst().orElseThrow();
            return bridge.call(9,"2",Long.toUnsignedString(id),Long.toUnsignedString(node.id()));
        }).thenCompose(selected->bridge.call(6)).whenComplete((saved,error)->edt(()->{
            if(error!=null){failure(error);return;}
            long id=snapshot.selection().document();var doc=snapshot.documents().stream().filter(d->d.id()==id).findFirst().orElseThrow();
            openDocument(doc);open("prefab.editor");String panel="prefab.canvas:"+Long.toUnsignedString(id);
            waitFor(()->documents.containsKey(panel)&&FileEditorManager.getInstance(project).getEditors(documents.get(panel)).length>0&&ready(),0,()->{
                var field=snapshot.fields().stream().filter(f->f.kind()==3&&f.editable()).findFirst().orElseThrow();
                JTextField input=(JTextField)named(tools.view("prefab.editor"),field.key());require(input!=null,"No real scalar control in Inspector");input.setText("F0 saved UI draft");
                saveProject().whenComplete((reply,failed)->edt(()->{
                    if(failed!=null){failure(failed);return;}
                    require(!snapshot.dirty()&&snapshot.fields().stream().anyMatch(f->f.key().equals(field.key())&&f.value().equals("F0 saved UI draft")),"Save raced or ignored uncommitted scalar input");
                    smokeTabCloseAfterEdit(documents.get(panel),id,field.key());
                }));
            });
        }));
    }
    private void smokeTabCloseAfterEdit(VirtualFile file,long id,String key){
        int prompts=smokeClosePrompts;smokeWindowChoice=Messages.CANCEL;
        JTextField input=(JTextField)named(tools.view("prefab.editor"),key);input.setText("Pending tab-close edit");input.postActionEvent();
        var manager=(com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project);
        var tab=manager.getCurrentWindow().getTabbedPane().getTabs().findInfo(file);
        require(tab!=null,"Native document tab missing");
        var close=Arrays.stream(((DefaultActionGroup)tab.getTabLabelActions()).getChildren(ActionManager.getInstance()))
            .filter(CheckedTabClose.class::isInstance).findFirst().orElseThrow();
        close.actionPerformed(AnActionEvent.createEvent(DataContext.EMPTY_CONTEXT,new Presentation(),"EditorTab",ActionUiKind.NONE,null));
        require(manager.isFileOpen(file),"Tab X closed before pending edit completed");
        waitFor(()->smokeClosePrompts>prompts&&ready(),0,()->{
            smokeWindowChoice=null;
            require(manager.isFileOpen(file)&&snapshot.fields().stream().anyMatch(f->f.key().equals(key)&&f.value().equals("Pending tab-close edit")),"Deferred tab Cancel lost the committed edit/view");
            record("PASS F0 real tab X deferred until scalar completion; Cancel retained native edit and view");smokeCloseChoices(file,id);
        });
    }
    private void inFloatingEditor(VirtualFile file,java.util.function.Consumer<Window> next){
        ((com.intellij.openapi.fileEditor.impl.FileEditorManagerImpl)FileEditorManager.getInstance(project)).openFileInNewWindow(file,false);
        waitFor(()->floatingGuards.keySet().stream().anyMatch(window->window.isShowing()&&views.stream().anyMatch(view->view.file()==file&&SwingUtilities.getWindowAncestor(view.component())==window))&&ready(),0,()->{
            Window window=floatingGuards.keySet().stream().filter(w->views.stream().anyMatch(view->view.file()==file&&SwingUtilities.getWindowAncestor(view.component())==w)).findFirst().orElseThrow();
            require(NativeViewport.windowHandle(window)!=0&&window!=WindowManager.getInstance().getFrame(project),"No independent Platform document window");next.accept(window);
        });
    }
    private void smokeWindowClose(Window window,int decision){
        smokeWindowChoice=decision;
        try{window.dispatchEvent(new WindowEvent(window,WindowEvent.WINDOW_CLOSING));}finally{smokeWindowChoice=null;}
    }
    private void smokeCloseChoices(VirtualFile file,long id){
        inFloatingEditor(file,window->bridge.call(5,Long.toUnsignedString(id),"Saved close name").whenComplete((renamed,error)->edt(()->{
            if(error!=null){failure(error);return;}
            require(documentModified(file),"FileEditor did not project native per-document dirty");
            long revision=snapshot.revision();smokeWindowClose(window,Messages.CANCEL);
            require(snapshot.revision()==revision&&window.isDisplayable()&&FileEditorManager.getInstance(project).isFileOpen(file),"Floating Cancel mutated/closed document");
            smokeWindowClose(window,Messages.YES);
            waitFor(()->!window.isDisplayable()&&!pendingDocumentCloses.contains(file)&&ready(),0,()->{
                require(!snapshot.dirty(),"Save close lost native save point");
                var doc=snapshot.documents().stream().filter(d->d.id()==id).findFirst().orElseThrow();openDocument(doc);
                waitFor(()->FileEditorManager.getInstance(project).getEditors(file).length>0&&ready(),0,()->inFloatingEditor(file,second->{
                    long other=snapshot.documents().stream().filter(d->d.id()!=id&&d.kind()==2).findFirst().orElseThrow().id();
                    bridge.call(5,Long.toUnsignedString(id),"Discard close name").thenCompose(reply->bridge.call(5,Long.toUnsignedString(other),"Keep other document edit")).whenComplete((changed,failed)->edt(()->{
                        if(failed!=null){failure(failed);return;}
                        smokeWindowClose(second,Messages.NO);
                        waitFor(()->!second.isDisplayable()&&!pendingDocumentCloses.contains(file)&&ready(),0,()->{
                            require(snapshot.dirty()&&snapshot.documents().stream().anyMatch(d->d.id()==id&&!d.dirty()&&d.name().equals("Saved close name"))&&snapshot.documents().stream().anyMatch(d->d.id()==other&&d.dirty()&&d.name().equals("Keep other document edit")),"Discard close changed another document or failed to restore save point");
                            saveProject().whenComplete((saved,failure)->edt(()->{if(failure!=null){failure(failure);return;}record("PASS F0 native scalar widget/flush-before-Save, per-document dirty, real floating HWND Cancel/Save/Discard close, other-document preservation; pre-close extension registered");smokeOrderedSaveClose(file,id);}));
                        });
                    }));
                }));
            });
        })));
    }
    private void smokeOrderedSaveClose(VirtualFile file,long id){
        var doc=snapshot.documents().stream().filter(d->d.id()==id).findFirst().orElseThrow();openDocument(doc);
        inFloatingEditor(file,window->bridge.call(5,Long.toUnsignedString(id),"Ordered Save close").whenComplete((changed,error)->edt(()->{
            if(error!=null){failure(error);return;}
            saveProject();smokeWindowClose(window,Messages.YES);
            waitFor(()->!window.isDisplayable()&&ready(),0,()->{
                require(!snapshot.dirty()&&snapshot.documents().stream().anyMatch(d->d.id()==id&&d.name().equals("Ordered Save close")),"Close raced an outstanding Save");
                record("PASS F0 immediate Save then floating close awaited actual native commit");smokeAssets();
            });
        })));
    }
    private void captureAssetColor(NativeViewport canvas,String filename,int rgb){
        Path target=output.resolve(filename);canvas.captureTo(target.toString());
        try{BufferedImage image=ImageIO.read(target.toFile());int count=0,left=image.getWidth(),right=0,top=image.getHeight(),bottom=0;
            for(int y=0;y<image.getHeight();y++)for(int x=0;x<image.getWidth();x++)if((image.getRGB(x,y)&0xffffff)==rgb){++count;left=Math.min(left,x);right=Math.max(right,x);top=Math.min(top,y);bottom=Math.max(bottom,y);}
            require(count>0,"Native Renderer capture has no published sprite color: "+filename);
            require(Math.abs((right-left+1)-2*(bottom-top+1))<=2,"Native sprite aspect/clip mismatch: "+filename);
            record("PASS F1 native GPU pixels "+filename+" matching="+count+" bounds="+(right-left+1)+"x"+(bottom-top+1));
        }catch(IOException error){throw new UncheckedIOException(error);}
    }
    private void smokeAssetPoll(java.util.function.BooleanSupplier condition,int attempt,Runnable next){
        if(attempt>300){failure(new IllegalStateException("F1 native asset publication timeout"));return;}
        bridge.call(26).whenComplete((reply,error)->edt(()->{if(error!=null){failure(error);return;}if(condition.getAsBoolean())next.run();else later(40,()->smokeAssetPoll(condition,attempt+1,next));}));
    }
    private void smokeAssets(){
        WindowManager.getInstance().getFrame(project).setExtendedState(java.awt.Frame.MAXIMIZED_BOTH);
        Path external=output.resolve("f1-import.png");
        CompletableFuture.runAsync(()->{try{BufferedImage image=new BufferedImage(8,4,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<4;y++)for(int x=0;x<8;x++)image.setRGB(x,y,0xffff0000);ImageIO.write(image,"png",external.toFile());}catch(IOException error){throw new UncheckedIOException(error);}},io)
        .thenCompose(v->bridge.call(18,Long.toUnsignedString(bridge.latest().snapshot().assetRevision()),external.toString(),"f1-import.png"))
        .whenComplete((reply,error)->edt(()->{
            if(error!=null){failure(error);return;}
            smokeAssetPoll(()->snapshot.assets().stream().anyMatch(asset->asset.path().equals("Content/f1-import.png")),0,()->{
                var asset=snapshot.assets().stream().filter(a->a.path().equals("Content/f1-import.png")).findFirst().orElseThrow();
                var prefab=snapshot.documents().stream().filter(d->d.kind()==1).findFirst().orElseThrow();
                bridge.call(24,Long.toUnsignedString(snapshot.assetRevision()),Long.toUnsignedString(prefab.id()),Long.toUnsignedString(asset.id())).whenComplete((assigned,failed)->edt(()->{
                    if(failed!=null){failure(failed);return;}openDocument(prefab);
                    String panel="prefab.canvas:"+Long.toUnsignedString(prefab.id());
                    waitFor(()->NativeViewport.metrics().length==9&&NativeViewport.metrics()[7]>0&&NativeViewport.metrics()[8]==0,0,()->{
                        NativeViewport canvas=views.stream().filter(view->view.file()==documents.get(panel)).map(view->view.reader().nativeView(panel)).filter(Objects::nonNull).findFirst().orElseThrow();
                        captureAssetColor(canvas,"f1-native-sprite-before.png",0x00ff0000);
                        smokeAssetReimport(external,asset.id(),asset.cookedKey(),canvas);
                    });
                }));
            });
        }));
    }
    private void smokeAssetReimport(Path external,long id,String oldKey,NativeViewport canvas){
        Path source=Path.of(snapshot.path()).getParent().resolve("Content/f1-import.png");
        CompletableFuture.runAsync(()->{try{BufferedImage image=new BufferedImage(8,4,BufferedImage.TYPE_INT_ARGB);for(int y=0;y<4;y++)for(int x=0;x<8;x++)image.setRGB(x,y,0xff0000ff);ImageIO.write(image,"png",source.toFile());}catch(IOException error){throw new UncheckedIOException(error);}},io)
        .thenCompose(v->bridge.call(19,Long.toUnsignedString(bridge.latest().snapshot().assetRevision()),Long.toUnsignedString(id)))
        .whenComplete((reply,error)->edt(()->{
            if(error!=null){failure(error);return;}
            smokeAssetPoll(()->snapshot.assets().stream().anyMatch(a->a.id()==id&&!a.cookedKey().equals(oldKey)),0,()->{
                bridge.call(9,"6","0",Long.toUnsignedString(id)).whenComplete((selected,failed)->edt(()->{
                    if(failed!=null){failure(failed);return;}
                    waitFor(()->NativeViewport.metrics()[7]>0&&NativeViewport.metrics()[8]==0,0,()->later(300,()->{
                        captureAssetColor(canvas,"f1-native-sprite-after.png",0x000000ff);capture("f1-asset-details");
                        smokeExplorerActions(external,id);
                    }));
                }));
            });
        }));
    }
    private void smokeExplorerActions(Path external,long assetId){
        var explorer=tools.view("project.explorer");
        var reimport=(JButton)named(explorer,"asset.Reimport");
        require(reimport!=null&&reimport.isEnabled(),"Explorer action did not resolve the selected asset row");
        require(snapshot.selection().kind()==6&&snapshot.selection().element()==assetId,"Asset selection changed");
        long revision=snapshot.assetState().revision();
        reimport.doClick();
        smokeAssetPoll(()->snapshot.assetState().revision()>revision&&snapshot.selection().element()==assetId,0,()->
            queueImport(bridge,bridge.latest().generation(),external.toString(),"f1-import.png").whenComplete((reply,error)->edt(()->{
                if(error!=null){failure(error);return;}
                var problem=contents.get("problems");
                var scroll=(JScrollPane)tools.view("problems");
                var text=(JTextArea)scroll.getViewport().getView();
                require(problem!=null&&problem.getManager().getSelectedContent()==problem&&
                    text.getText().contains("Import rejected: f1-import.png"),"Import conflict was not presented in Problems");
                var settings=com.intellij.ide.ui.UISettings.getInstance();
                require(!Boolean.getBoolean("epidemic.ui.standalone")||
                    settings.getMainMenuDisplayMode()==com.intellij.ide.ui.MainMenuDisplayMode.MERGED_WITH_MAIN_TOOLBAR,
                    "Standalone Main Menu merge setting was lost");
                saveProject().whenComplete((saved,saveError)->edt(()->{
                    if(saveError!=null){failure(saveError);return;}
                    record("PASS F1 Explorer button retained asset ID; duplicate destination opened Problems; merged toolbar mode retained");
                    smokePickerCancel(text);
                }));
            })));
    }
    private void smokePickerCancel(JTextArea problems){
        var console=(JTextArea)((JScrollPane)tools.view("console")).getViewport().getView();
        require(console.getText().contains("editor.asset_imported")&&!problems.getText().contains("editor.asset_imported")&&
            !problems.getText().contains("editor.asset_queued"),"Routine asset activity leaked into Problems");
        var batch=new PickerImport(bridge,bridge.latest().generation(),List.of(
            new LightVirtualFile("picker-first.png"),new LightVirtualFile("picker-unsent.png")),"");
        pickerImport=batch;
        submitImport(batch,0);
        cancelImports();
        smokeAssetPoll(()->snapshot.assetState().events().stream().anyMatch(e->e.kind()==5&&e.source().equals("picker-first.png")),0,()->{
            require(pickerImport==null&&(snapshot.flags()&64)==0&&snapshot.assetState().events().stream().noneMatch(e->
                e.source().equals("picker-unsent.png")),"Picker resubmitted files after native cancel");
            record("PASS F1 Problems/Console separation and picker cancellation before a later submission");
            smokeInstanceInspector();
        });
    }
    private void smokeInstanceInspector(){
        bridge.call(15,"1","Health placement fixture","registered-components").thenCompose(created->{
            String prefab=Long.toUnsignedString(created.snapshot().selection().document());
            String scene=Long.toUnsignedString(created.snapshot().entry());
            return bridge.call(33,"place",scene,prefab,"0","0").thenCompose(first->{
                String firstNode=Long.toUnsignedString(first.snapshot().selection().element());
                return bridge.call(33,"place",scene,prefab,"2","0").thenCompose(second->
                    bridge.call(9,"1",scene,firstNode).thenApply(selected->second.snapshot().selection()));
            });
        }).whenComplete((other,error)->edt(()->{
            if(error!=null){failure(error);return;}
            open("inspector");
            var points=snapshot.fields().stream().filter(f->f.name().endsWith(".health / points")&&f.editable()).findFirst().orElseThrow();
            var control=(JTextField)named(tools.view("inspector"),points.key());
            require(control!=null&&control.isEnabled(),"Instance Health control is unavailable");
            control.setText("150");control.postActionEvent();
            waitFor(()->ready()&&snapshot.fields().stream().anyMatch(f->f.key().equals(points.key())&&f.value().equals("150")&&
                f.source().equals("instance override")),0,()->{
                var first=snapshot.selection();
                bridge.call(9,"1",Long.toUnsignedString(other.document()),Long.toUnsignedString(other.element())).thenCompose(selected->{
                    require(selected.snapshot().fields().stream().anyMatch(f->f.name().equals(points.name())&&
                        f.value().equals(points.value())&&f.source().equals("definition")),"Editing one placement changed its peer");
                    return bridge.call(9,"1",Long.toUnsignedString(first.document()),Long.toUnsignedString(first.element()));
                }).thenCompose(selected->bridge.call(31,Long.toUnsignedString(selected.snapshot().selectionRevision()),points.key()))
                    .thenCompose(reverted->{
                        require(reverted.snapshot().fields().stream().anyMatch(f->f.name().equals(points.name())&&
                            f.value().equals(points.value())&&f.source().equals("definition")),"Revert did not restore the definition default");
                        return bridge.call(6);
                    }).whenComplete((saved,failed)->edt(()->{
                        if(failed!=null){failure(failed);return;}
                        record("PASS F2 Health scalar widget committed an instance override; peer and Revert retained definition");
                        smokePlayback();
                    }));
            });
        }));
    }
    private void smokePlayback(){
        long revision=snapshot.revision(),identity=snapshot.project();
        bridge.call(13).thenCompose(reply->{
            require(reply.snapshot().playing()&&!reply.snapshot().dirty()&&reply.snapshot().revision()==revision,"Play changed authoring or failed to start");
            return bridge.call(14);
        }).whenComplete((reply,error)->edt(()->{
            if(error!=null){failure(error);return;}
            if(reply.snapshot().playing()||reply.snapshot().project()!=identity||reply.snapshot().revision()!=revision||reply.snapshot().dirty()){
                failure(new IllegalStateException("Stop changed authoring identity/savepoint"));return;
            }
            record("PASS F0 native Play/Stop lifecycle; authoring identity/revision/savepoint retained");recoverSmoke();
        }));
    }
    private void smokeCloseAllViews(){
        var doc=snapshot.documents().stream().filter(d->d.kind()==0).findFirst().orElseThrow();openDocument(doc);
        String panel="scene.editor:"+Long.toUnsignedString(doc.id());
        waitFor(()->documents.containsKey(panel)&&FileEditorManager.getInstance(project).getEditors(documents.get(panel)).length>0&&ready(),0,()->inFloatingEditor(documents.get(panel),window->{
            require(FileEditorManager.getInstance(project).getAllEditors(documents.get(panel)).length>=2,"No peer Scene views for project-close regression");
            record("INFO F0 project close with peer Scene editors/native surfaces across independent windows");close(true);
        }));
    }
    private void recoverSmoke(){
        recoveryProject=snapshot.project();recoveryViews=views.size();recoveryReaders=Set.copyOf(views);recoveryBounds=WindowManager.getInstance().getFrame(project).getBounds();expectedDisconnect=true;recoveringSmoke=true;
        Path executable=Path.of(System.getProperty("epidemic.ui.backend")).toAbsolutePath();
        ProcessHandle child=ProcessHandle.current().children().filter(p->p.info().command().map(c->Path.of(c).equals(executable)).orElse(false)).findFirst().orElseThrow();
        child.destroyForcibly();child.onExit().whenComplete((v,e)->edt(()->later(250,()->{
            require(service==Bridge.Service.Error&&snapshot.project()==recoveryProject&&opened,"Disconnect discarded layout/read snapshot");
            expectedDisconnect=false;reconnect();
        })));
    }
    @Override public void dispose(){disposed=true;ProjectManager.getInstance().removeProjectManagerListener(closeGuard);if(bridge!=null)bridge.close();if(launcher!=null)launcher.dispose();if(menu!=null){ActionManager actions=ActionManager.getInstance();((DefaultActionGroup)actions.getAction("MainMenu")).remove(menu);actions.unregisterAction("Epidemic.MainMenu");}io.shutdownNow();for(var entry:List.copyOf(floatingGuards.entrySet())){entry.getKey().removeWindowListener(entry.getValue().guard());entry.getValue().originals().forEach(entry.getKey()::addWindowListener);}floatingGuards.clear();views.clear();}
}

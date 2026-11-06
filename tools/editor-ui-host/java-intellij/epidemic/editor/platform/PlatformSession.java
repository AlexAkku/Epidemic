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
    public record DocumentView(UiPanels reader,JComponent component){}
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
    private void edt(Runnable action){ApplicationManager.getApplication().invokeLater(()->{if(!disposed&&!project.isDisposed())action.run();});}
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
            settings.setShowMainToolbar(false);
            settings.setShowNewMainToolbar(false);
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
                if(id!=null){selectedPanel=id;context(Catalog.context(id));}
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
        if(bridge==null){bridge=new Bridge(Path.of(System.getProperty("epidemic.ui.backend")),output,this::state);bridge.connect().thenCompose(reply->bridge.call(operation,args)).whenComplete((reply,error)->edt(()->{if(error!=null)failure(error);else workspace(reply.snapshot());}));}
        else bridge.call(operation,args).whenComplete((reply,error)->edt(()->{if(error!=null)failure(error);else workspace(reply.snapshot());}));
    }
    private void state(Bridge.State state){
        service=state.service();
        if(state.reply()!=null&&state.reply().snapshot()!=null){snapshot=state.reply().snapshot();if(tools!=null)tools.refresh(snapshot);for(var view:List.copyOf(views))view.reader().refresh(snapshot);}
        if(state.service()==Bridge.Service.Error){if(expectedDisconnect)record("INFO Expected smoke disconnect; immutable snapshot/layout retained");else failure(new IOException(state.diagnostic()));}
    }
    private UiPanels reader(){return new UiPanels(catalog,features,document->{String base=switch(document.kind()){case 0->"scene.editor";case 1->"prefab.canvas";default->"graph.editor";};open(base+":"+Long.toUnsignedString(document.id()));},selection->command(9,Integer.toString(selection.kind()),Long.toUnsignedString(selection.document()),Long.toUnsignedString(selection.element())),this::importPicker,()->{if(bridge!=null)tools.log("Cancelled queued requests: "+bridge.cancelQueued());});}
    private ToolWindow group(String id,ToolWindowAnchor anchor,boolean split){
        ToolWindow tool=ToolWindowManager.getInstance(project).registerToolWindow(RegisterToolWindowTask.notClosable(id,anchor));
        tool.getComponent().putClientProperty(com.intellij.openapi.wm.impl.content.ToolWindowContentUi.HIDE_ID_LABEL,"true");
        com.intellij.openapi.wm.impl.content.ToolWindowContentUi.setAllowTabsReordering(tool,true);
        tool.setStripeTitle(id.replace("Epidemic ",""));
        tool.setContentUiType(ToolWindowContentUiType.TABBED,null);tool.setSplitMode(split,null);groups.put(id,tool);return tool;
    }
    private void workspace(Bridge.Snapshot value){
        snapshot=value;opened=true;launcher.setVisible(false);
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
                record("PASS Platform crash/reconnect: same project identity and view instances; single replacement companion");close(true);
            });else later(1800,this::verify);
        }else if(initialDocuments&&Files.exists(profile.resolve("platform-panels.properties")))restorePanels();
    }
    private void add(String id,ToolWindow window){
        Content content=contents.get(id);
        if(content==null){content=ContentFactory.getInstance().createContent(tools.view(id),catalog.panel(id).label(),true);contents.put(id,content);}
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
            VirtualFile file=documents.computeIfAbsent(id,key->{LightVirtualFile created=new LightVirtualFile(panel.label(),"");created.putUserData(PanelEditorProvider.PANEL,key);return created;});
            FileEditorManager manager=FileEditorManager.getInstance(project);manager.openFile(file,true);manager.setSelectedEditor(file,"epidemic.platform.panel");selectedPanel=id;context(Catalog.context(id));
        }else{
            ToolWindow target=groups.get(targets.get(id));
            if(target==null)target=panel.anchor().equals("dock.left.navigation")?navigation:panel.anchor().equals("dock.bottom.global")?bottom:panel.anchor().equals("dock.center.right.secondary")?secondary:primary;
            add(id,target);target.getContentManager().setSelectedContent(contents.get(id));target.show(null);
        }
    }
    public DocumentView documentView(String id){
        UiPanels reader=reader();reader.refresh(snapshot);JPanel root=new JPanel(new BorderLayout());
        DefaultActionGroup commands=new DefaultActionGroup();commands.add(saveAction);
        commands.add(undoAction);commands.add(redoAction);
        commands.addSeparator();commands.add(playAction);commands.add(stopAction);commands.add(action("Build · F9",()->{},()->false));
        commands.add(action("Jobs",()->open("background.jobs"),()->opened));commands.add(action("Find Panel",this::palette,()->opened));
        ActionToolbar toolbar=ActionManager.getInstance().createActionToolbar("Epidemic.Document.Toolbar",commands,true);toolbar.setTargetComponent(root);root.add(toolbar.getComponent(),BorderLayout.NORTH);root.add(reader.view(id));
        registerWorkspaceShortcuts(root);
        DocumentView view=new DocumentView(reader,root);views.add(view);return view;
    }
    public void release(DocumentView view){
        views.remove(view);
        Object shortcuts=view.component().getClientProperty("epidemic.shortcuts");
        if(shortcuts instanceof Disposable lifetime)com.intellij.openapi.util.Disposer.dispose(lifetime);
        view.component().putClientProperty("epidemic.shortcuts",null);
        view.component().removeAll();
    }
    private boolean ready(){return opened&&!closing&&bridge!=null&&service==Bridge.Service.Ready&&snapshot!=null;}
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
    private void importPicker(){
        // Platform owns modal/focus state and chooses a supported picker for the
        // runtime (its own dialog on standard OpenJDK, native chooser on JBR).
        // A raw AWT FileDialog bypasses that state in an IntelliJ application.
        FileChooserDescriptor descriptor=new FileChooserDescriptor(true,false,false,false,false,false)
            .withTitle("Import source file — planned F1");
        FileChooser.chooseFile(descriptor,project,WindowManager.getInstance().getFrame(project),null,
            file->tools.log("Selected "+file.getPath()+"; import mutation planned for F1"));
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
        saveAction=action("Save",()->command(6),this::ready);
        saveAllAction=action("Save All",()->command(6),this::ready);
        undoAction=action("Undo",()->command(7),()->ready()&&snapshot.canUndo()&&!editableTextFocus());
        redoAction=action("Redo",()->command(8),()->ready()&&snapshot.canRedo()&&!editableTextFocus());
        playAction=action("Play",()->playback(true),()->supports("play")&&supports("stop")&&!snapshot.playing());
        stopAction=action("Stop",()->playback(false),()->supports("stop")&&snapshot.playing());
        playToggleAction=action("Play / Stop",this::togglePlayback,()->supports("play")&&supports("stop"));
        playAction.getTemplatePresentation().setDescription("Start an isolated Play session. Game View preview is not connected yet.");
        sub.get("File").add(saveAction);sub.get("File").add(saveAllAction);sub.get("File").add(action("Clone Saved Project…",this::cloneProject,this::canClone));sub.get("File").add(action("Close Project",()->close(false),()->opened));
        sub.get("Edit").add(undoAction);sub.get("Edit").add(redoAction);
        sub.get("Edit").add(action("Rename selected document…",()->{var selected=snapshot.documents().stream().filter(d->d.id()==snapshot.selection().document()).findFirst();if(selected.isEmpty())return;long generation=bridge.latest().generation(),revision=snapshot.revision();String name=Messages.showInputDialog(project,"Document name","Rename",Messages.getQuestionIcon(),selected.get().name(),null);if(name!=null)bridge.callAt(generation,revision,5,Long.toUnsignedString(selected.get().id()),name).whenComplete((r,e)->{if(e!=null)edt(()->failure(e));});},this::ready));
        for(Catalog.Panel panel:catalog.panels())sub.get("View").add(action(panel.label(),()->open(panel.id()),()->opened));
        sub.get("Project").add(action("Project Settings",()->open("settings.project"),()->opened));sub.get("Project").add(action("Reconnect native backend",this::reconnect,()->opened&&service==Bridge.Service.Error));
        sub.get("Project").addSeparator();sub.get("Project").add(playAction);sub.get("Project").add(stopAction);
        sub.get("Assets").add(action("Import…",this::importPicker,()->opened));
        for(String group:List.of("GameObject","Graph","Build"))sub.get(group).add(action("Backend planned for "+(group.equals("Graph")?"F3":group.equals("Build")?"F9":"F2"),()->{},()->false));
        sub.get("Tools").add(action("UI Gallery",()->open("settings.editor"),()->opened));sub.get("Tools").add(action("Type Registry",()->open("type.registry"),()->opened));sub.get("Tools").add(action("Find Panel…",this::palette,()->opened));
        sub.get("Window").add(action("Move Inspector to Windows window",()->primary.setType(ToolWindowType.WINDOWED,()->primary.show(null)),()->opened));
        sub.get("Window").add(action("Dock Inspector",()->primary.setType(ToolWindowType.DOCKED,()->primary.show(null)),()->opened));
        sub.get("Window").add(action("Save panel layout",this::savePanels,()->opened));sub.get("Window").add(action("Restore panel layout",this::restorePanels,()->opened));
        sub.get("Window").add(action("Reset panel layout",this::resetPanels,()->opened));
        sub.get("Help").add(action("About Epidemic UI",()->Messages.showInfoMessage(project,"JetBrains IntelliJ Platform 253.31033.145\nNative C++ Workspace / versioned IPC\nToolWindowManager and FileEditorProvider own docking.\nUnwired future features remain disabled.","Epidemic Editor"),()->true));
        ActionManager actions=ActionManager.getInstance();actions.registerAction("Epidemic.MainMenu",menu,PluginId.getId(Boolean.getBoolean("epidemic.ui.standalone")?"com.intellij":"epidemic.editor.platform"));
        DefaultActionGroup mainMenu=(DefaultActionGroup)actions.getAction("MainMenu");
        if(Boolean.getBoolean("epidemic.ui.standalone")){mainMenu.removeAll();mainMenu.addAll(menu.getChildren(actions));}
        else mainMenu.add(menu);
        registerWorkspaceShortcuts(WindowManager.getInstance().getFrame(project).getRootPane());
    }
    private static boolean editableTextFocus(){
        return KeyboardFocusManager.getCurrentKeyboardFocusManager().getFocusOwner() instanceof javax.swing.text.JTextComponent text&&text.isEditable();
    }
    private void registerWorkspaceShortcuts(JComponent target){
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
    private void reconnect(){String path=snapshot.path();Bridge old=bridge;bridge=null;if(old!=null)old.close();service=Bridge.Service.Unavailable;opened=false;connect(2,path);}
    private void close(boolean exit){
        if(closing)return;
        if(service==Bridge.Service.Error){
            if(Messages.showYesNoDialog(project,"The backend is disconnected. Unsaved native edits cannot be saved. Close this session and keep the last committed project?","Disconnected Project",Messages.getWarningIcon())!=Messages.YES){return;}
            if(bridge!=null)bridge.close();bridge=null;closedUi(exit);return;
        }
        boolean discard=false;
        if(snapshot!=null&&snapshot.dirty()){
            int answer=Messages.showYesNoCancelDialog(project,"Save project changes before closing?","Epidemic Project","Save","Discard","Cancel",Messages.getQuestionIcon());
            if(answer==Messages.CANCEL){return;}
            if(answer==Messages.YES){command(6);bridge.call(4).whenComplete((r,e)->edt(()->{if(e!=null)failure(e);else close(exit);}));return;}
            discard=true;
        }
        closing=true;
        bridge.call(10,discard?new String[]{"discard"}:new String[0]).whenComplete((reply,error)->edt(()->{
            closing=false;if(error!=null){failure(error);return;}closedUi(exit);
        }));
    }
    private void closedUi(boolean exit){opened=false;snapshot=null;FileEditorManager manager=FileEditorManager.getInstance(project);for(VirtualFile file:documents.values())manager.closeFile(file);documents.clear();for(ToolWindow tool:groups.values())tool.hide(null);if(exit)finish();else{WindowManager.getInstance().getFrame(project).setVisible(false);launcher();}}
    private void exit(){if(opened){close(true);return;}finish();}
    private void finish(){if(bridge==null){exitApplication();return;}bridge.shutdown().whenComplete((v,e)->edt(()->{if(e!=null){failure(e);return;}record("PASS Platform native shutdown; surfaces="+NativeViewport.metrics()[0]);exitApplication();}));}
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
    private void later(int milliseconds,Runnable action){javax.swing.Timer timer=new javax.swing.Timer(milliseconds,event->{try{action.run();}catch(Throwable e){failure(e);}});timer.setRepeats(false);timer.start();}
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
            savePanels().whenComplete((v,e)->edt(()->{if(e!=null){failure(e);return;}record("PASS Platform U2-U7 shell smoke: actual ToolWindowManager/FileEditorProvider/ActionManager, shared native project, 46 panels captured; 77 form diagnostic prints; navigation left authoring clean");smokePlayback();}));return;
        }
        String id=ids.get(index);open(id);later(350,()->{capture("ui-platform-"+id.replace('.','-'));screens(index+1);});
    }
    private static void layout(Container parent){parent.doLayout();for(Component child:parent.getComponents())if(child instanceof Container nested)layout(nested);}
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
    private void recoverSmoke(){
        recoveryProject=snapshot.project();recoveryViews=views.size();recoveryReaders=Set.copyOf(views);recoveryBounds=WindowManager.getInstance().getFrame(project).getBounds();expectedDisconnect=true;recoveringSmoke=true;
        Path executable=Path.of(System.getProperty("epidemic.ui.backend")).toAbsolutePath();
        ProcessHandle child=ProcessHandle.current().children().filter(p->p.info().command().map(c->Path.of(c).equals(executable)).orElse(false)).findFirst().orElseThrow();
        child.destroyForcibly();child.onExit().whenComplete((v,e)->edt(()->later(250,()->{
            require(service==Bridge.Service.Error&&snapshot.project()==recoveryProject&&opened,"Disconnect discarded layout/read snapshot");
            expectedDisconnect=false;reconnect();
        })));
    }
    @Override public void dispose(){disposed=true;ProjectManager.getInstance().removeProjectManagerListener(closeGuard);if(bridge!=null)bridge.close();if(launcher!=null)launcher.dispose();if(menu!=null){ActionManager actions=ActionManager.getInstance();((DefaultActionGroup)actions.getAction("MainMenu")).remove(menu);actions.unregisterAction("Epidemic.MainMenu");}io.shutdownNow();views.clear();}
}

package epidemic.editor.platform;

import com.intellij.openapi.fileEditor.*;
import com.intellij.openapi.project.*;
import com.intellij.openapi.util.*;
import com.intellij.openapi.vfs.VirtualFile;
import javax.swing.*;
import java.beans.PropertyChangeListener;

/** IntelliJ owns editor tabs, splits, floating editor windows and their disposal. */
public final class PanelEditorProvider implements FileEditorProvider, DumbAware {
    public static final Key<String> PANEL=Key.create("epidemic.ui.panel.id");
    public boolean accept(Project project,VirtualFile file){return file.getUserData(PANEL)!=null;}
    public FileEditor createEditor(Project project,VirtualFile file){return new Editor(project.getService(PlatformSession.class),file);}
    public String getEditorTypeId(){return "epidemic.platform.panel";}
    public FileEditorPolicy getPolicy(){return FileEditorPolicy.HIDE_DEFAULT_EDITOR;}
    static final class Editor extends UserDataHolderBase implements FileEditor {
        final PlatformSession session;final VirtualFile file;final PlatformSession.DocumentView view;
        Editor(PlatformSession session,VirtualFile file){this.session=session;this.file=file;view=session.documentView(file.getUserData(PANEL));}
        public JComponent getComponent(){return view.component();}
        public JComponent getPreferredFocusedComponent(){return view.component();}
        public String getName(){return session.catalog.panel(file.getUserData(PANEL)).label();}
        public void setState(FileEditorState state){}
        public boolean isModified(){return false;}
        public boolean isValid(){return !session.isDisposed()&&file.isValid();}
        public VirtualFile getFile(){return file;}
        public void addPropertyChangeListener(PropertyChangeListener listener){}
        public void removePropertyChangeListener(PropertyChangeListener listener){}
        public void dispose(){session.release(view);}
    }
}

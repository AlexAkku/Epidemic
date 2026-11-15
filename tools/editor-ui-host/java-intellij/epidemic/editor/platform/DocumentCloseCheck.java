package epidemic.editor.platform;

import com.intellij.openapi.vfs.VirtualFile;
import com.intellij.openapi.vfs.VirtualFilePreCloseCheck;

/** Platform calls this before user tab/window closure; IPC never blocks EDT. */
public final class DocumentCloseCheck implements VirtualFilePreCloseCheck {
    @Override public boolean canCloseFile(VirtualFile file){
        var source=file.getUserData(ScriptWorkspace.OWNER);
        if(source!=null)return source.canClose(file);
        PlatformSession owner=file.getUserData(PanelEditorProvider.OWNER);
        return owner==null||owner.isDisposed()||owner.canCloseDocument(file);
    }
}

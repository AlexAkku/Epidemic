package epidemic.editor.platform;

import com.intellij.openapi.fileEditor.*;
import com.intellij.openapi.editor.Document;
import com.intellij.openapi.vfs.VirtualFile;

/** Platform auto-save cannot bypass the source fingerprint guard. */
public final class ScriptSaveGuard extends FileDocumentSynchronizationVetoer {
    public boolean maySaveDocument(Document document,boolean explicit){
        VirtualFile file=FileDocumentManager.getInstance().getFile(document);
        ScriptWorkspace owner=file==null?null:file.getUserData(ScriptWorkspace.OWNER);
        return owner==null||owner.safeToSave(document);
    }
    public boolean mayReloadFileContent(VirtualFile file,Document document){
        var owner=file.getUserData(ScriptWorkspace.OWNER);return owner==null||owner.mayReload(file,document);
    }
}

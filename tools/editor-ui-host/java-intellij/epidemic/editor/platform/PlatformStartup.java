package epidemic.editor.platform;

import com.intellij.openapi.project.*;
import com.intellij.openapi.startup.ProjectActivity;
import com.intellij.openapi.application.ApplicationManager;

/** SDK container project only; a game workspace is never mapped to IDE Project/VFS. */
public final class PlatformStartup implements ProjectActivity, DumbAware {
    @Override public Object execute(Project project,kotlin.coroutines.Continuation<? super kotlin.Unit> continuation) {
        if(!Boolean.getBoolean("epidemic.ui.platform.enabled"))return kotlin.Unit.INSTANCE;
        ApplicationManager.getApplication().invokeLater(()->{
            if(!project.isDisposed())project.getService(PlatformSession.class).start();
        });
        return kotlin.Unit.INSTANCE;
    }
}

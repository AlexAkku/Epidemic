package epidemic.launcher;

import com.intellij.openapi.application.*;
import com.intellij.openapi.project.ex.ProjectManagerEx;
import com.intellij.ide.impl.OpenProjectTask;
import com.intellij.ide.impl.OpenProjectTaskBuilder;
import java.nio.file.*;
import java.util.List;

/** Opens only the product-owned, empty host container, never a user's game in VFS. */
public final class EpidemicStarter implements ApplicationStarter {
    public int getRequiredModality(){return NOT_IN_EDT;}
    public boolean canProcessExternalCommandLine(){return true;}
    public Object processExternalCommandLine(List<String> args,String directory,kotlin.coroutines.Continuation<? super com.intellij.ide.CliResult> continuation){
        for(var project:com.intellij.openapi.project.ProjectManager.getInstance().getOpenProjects())
            if(!project.isDisposed())project.getService(epidemic.editor.platform.PlatformSession.class).activateWindow();
        return com.intellij.ide.CliResult.OK;
    }
    public void main(List<String> args){
        // Write owner diagnostics only after Platform acquired the profile lock.
        // An unsuccessful second launch must not erase the running owner's log/PID.
        try{
            Path log=Path.of(System.getProperty("epidemic.ui.output"));
            Files.writeString(log.resolve("platform-status.log"),"INFO DirectoryLock source="+com.intellij.platform.ide.bootstrap.DirectoryLock.class.getResource("DirectoryLock.class")+"\n");
            Files.writeString(log.resolve("owned-main.pid"),Long.toString(ProcessHandle.current().pid()));
        }catch(java.io.IOException error){throw new java.io.UncheckedIOException(error);}
        Path container=Path.of(System.getProperty("epidemic.ui.container"));
        // This is our empty generated shell, with no external project/build scripts.
        // Never apply this creation flag to a user's game or arbitrary IDE project.
        var task=new OpenProjectTaskBuilder().build(builder->{
            builder.setProjectCreatedWithWizard(true);
            builder.setRunConfigurators(false);
            return kotlin.Unit.INSTANCE;
        }).withProjectName("Epidemic Workspace").withForceOpenInNewFrame(true);
        var manager=ProjectManagerEx.getInstanceEx();
        if(!Files.exists(container.resolve(".idea"))){
            var created=manager.newProject(container,task.asNewProject());
            if(created==null)throw new IllegalStateException("Cannot create the owned shell container");
            task=task.withProject(created);
        }
        if(manager.openProject(container,task)==null)throw new IllegalStateException("Cannot open the owned shell container");
    }
}

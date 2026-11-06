package epidemic.launcher;

import java.nio.file.*;
import java.util.*;

/** Product bootstrap. Never reads IDEA installation paths or loads its plugins. */
public final class Main {
    public static void main(String[] args) throws Exception {
        String location=Objects.requireNonNull(Main.class.getResource("Main.class")).toExternalForm();
        int separator=location.indexOf("!/");
        if(!location.startsWith("jar:")||separator<0)throw new IllegalStateException("Run the packaged Epidemic product JAR");
        Path jar=Path.of(java.net.URI.create(location.substring(4,separator)));
        Path home=jar.getParent();if(home.getFileName().toString().equals("lib"))home=home.getParent();
        boolean smoke=Arrays.asList(args).contains("--smoke");
        boolean qualification=Arrays.asList(args).contains("--qualification");
        Path state=qualification?home.resolve("qualification-state"):smoke?home.resolve("smoke-state"):Path.of(System.getenv("LOCALAPPDATA"),"EpidemicEngine","StandaloneEditor");
        for(String dir:List.of("config","system","log","preferences","container"))Files.createDirectories(state.resolve(dir));
        Files.createDirectories(home.resolve("plugins"));
        Map<String,String> properties=new LinkedHashMap<>();
        properties.put("idea.home.path",home.toString());properties.put("idea.platform.prefix","Epidemic");
        // Maven artifacts are deliberately assembled on one locked core classpath.
        // Use the upstream loader mode for that layout to preserve service type identity.
        properties.put("idea.force.use.core.classloader","true");
        // The Platform container has no game files. Game watching belongs to C++.
        properties.put("idea.filewatcher.disabled","true");
        properties.put("idea.vendor.name","Epidemic Engine");properties.put("idea.paths.selector","EpidemicEditor");
        // A new disposable cache directory leaves legacy AF_UNIX reparse files
        // untouched. Config, host-container layout and game preferences persist.
        properties.put("idea.config.path",state.resolve("config").toString());properties.put("idea.system.path",state.resolve("system-tcp-v1").toString());
        properties.put("idea.log.path",state.resolve("log").toString());properties.put("idea.plugins.path",home.resolve("plugins").toString());
        properties.put("epidemic.ui.output",state.resolve("log").toString());properties.put("epidemic.ui.profile",state.resolve("preferences").toString());
        properties.put("epidemic.ui.container",state.resolve("container").toString());
        properties.put("epidemic.ui.backend",home.resolve("native/EpidemicEditorBackend.exe").toString());
        properties.put("epidemic.u1.native",home.resolve("native/EpidemicUiViewport.dll").toString());
        properties.put("epidemic.ui.platform.enabled","true");properties.put("epidemic.ui.standalone","true");
        if(smoke)properties.put("epidemic.ui.platform.smoke","true");
        properties.forEach(System::setProperty);
        com.intellij.idea.Main.main(new String[]{"epidemic"});
    }
}

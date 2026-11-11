# Native Windows sandbox launcher

The product EXE statically links the official CEF 137 sandbox library (/MT),
creates a real CefScopedSandboxInfo in the main executable, and retains it for
JVM lifetime. Chromium subprocesses use this same EXE and call the public JBR
`jcef_helper.dll!execute_subprocess` before any JVM is created. This preserves
JCEF's renderer message router. The parent supplies the real 16-digit hexadecimal
pointer and matching CEF version to JCEF; it forces in-process JCEF.

The JNI invocation API needs an explicit list of locked product JARs and the
system context classloader; it does not expand the Java launcher's classpath.
The product JAR comes first so the existing DirectoryLock/MainImpl bootstrap
corrections take precedence over their upstream classes.
NativeLauncher JNI methods are registered by this EXE, so a property alone cannot
advertise sandbox support. Graph additionally rejects no_sandbox or remote mode.
The smoke inspects owned renderer OS tokens: restricted, low/untrusted integrity,
and Win32k lockdown. Job membership is reported but is not the sole proof (the
build runner itself uses a job). No OS security settings are changed.

Build: `Build-Launcher.ps1 -JdkRoot <pinned Microsoft JDK 21>`. This has its own
small CMake project, independent of EngineRuntime. Build-Standalone uses jpackage
for app-image assembly and replaces its generic EXE with this launcher. Runtime
DLL lookup is restricted to system/app directories and the bundled runtime bin.
The upstream CEF delay-load list is used; loading GUI DLLs eagerly in a renderer
would break Win32k lockdown.

SDK: https://cef-builds.spotifycdn.com/cef_binary_137.0.17+gf354b0e+chromium-137.0.7151.104_windows64_minimal.tar.bz2
Publisher index: https://cef-builds.spotifycdn.com/index.json
Publisher SHA1: 2924f6807fdbd24a9ab45458b798b8e4752e3e83
Pinned SHA512: 76bcc2f4e778679408e011fab5e010e65d0aa9b989c8bd9d8c9a1e1cf3ce8e2241bb4a45105de4b9147baacc380bc0eaa7bfdd88bdb2f78d28c6bc70dd153153

The archive is verified on every build, used offline after initial resolution,
and not shipped as a runtime dependency. Its headers, CMake settings and Release
cef_sandbox.lib come from that same archive. The product retains CEF-BSD.txt and
the full Chromium credits. Launcher sources, manifest and build script are copied
to the product notices directory for reproducibility. Updating JBR/JCEF requires
updating and qualifying the matching SDK together; do not spoof the version.

Contracts consulted:
- https://chromiumembedded.github.io/cef/sandbox_setup
- https://github.com/JetBrains/intellij-community/blob/idea/253.31033.145/native/XPlatLauncher/src/lib.rs
- https://github.com/JetBrains/intellij-community/blob/idea/253.31033.145/native/XPlatLauncher/src/cef_sandbox.rs

The launcher implementation here is Epidemic code; it uses the public CEF/JBR
contracts and has no dependency on an installed JetBrains product.

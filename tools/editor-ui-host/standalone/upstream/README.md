# Pinned open Platform bootstrap correction

`com/intellij/platform/ide/bootstrap/DirectoryLock.java` comes from Apache-2.0 IntelliJ Community `idea/253.31033.145`, `platform/platform-impl/bootstrap/src/com/intellij/platform/ide/bootstrap/DirectoryLock.java`.

Product delta: both competing-PID checks exclude the current JVM and its immediate Windows native jpackage parent. Windows jpackage keeps a native parent process with the same executable path. After a crash leaves the Unix-domain socket file behind, the upstream native-command scan mistakes that parent for another application and refuses stale-socket recovery. Real independent processes, more distant ancestors and normal live-owner activation retain the original checks; no live profile lock is deleted or globally disabled.

Second Windows correction: use the existing upstream loopback TCP transport instead of AF_UNIX. Microsoft OpenJDK 21 cannot access some dead socket reparse points in the actual LocalAppData profile. `Main` selects a new disposable `system-tcp-v1` cache directory, preserving config, container/layout and game preferences and leaving the unusable old socket untouched. No native deletion workaround, ACL changes or weakening of live-owner checks is used.

The product JAR must precede upstream JARs on the classpath. jpackage already lists the main JAR first; the development Java launch does so explicitly. Owner diagnostics record the loaded class resource. The original upstream SHA-256 is `9af77fe8cb98f4aa0ee35f56fbce8524044748d54aefa592d295fece5898fa31`. Apache LICENSE/NOTICE are included with the product.

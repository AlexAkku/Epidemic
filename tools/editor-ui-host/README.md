# Epidemic Editor — standalone IntelliJ Platform product

Sources: `Temp/!Tasks/U_Phase.md`, `Temp/!Tasks/UI_Roadmap.md`. ToolWindowManager/ContentManager own docking; FileEditorManager owns documents; ActionManager owns commands. One PlatformSession owns one C++ companion. The empty Platform container stays outside game folders; `.idea` never replaces `Project.epidemicproj`.

## Build and run

From the repository root, using Windows PowerShell (no `pwsh` required):

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\editor-ui-host\Build-Standalone.ps1 -JdkRoot "$env:USERPROFILE\.jdks\ms-21.0.7" -Resolve -Run
```

Prerequisites: VS 2026 C++ tools, CMake, Python, Maven (`mvn.cmd`), Microsoft OpenJDK **21.0.7+6-LTS**. Initial resolution needs Internet access. IDEA installation, account and license are not required. Default configuration is Release; the compiler remains Microsoft OpenJDK 21.0.7 while the product bundles the separately downloaded public JBR. The first runtime resolution needs network access. Close any running copy of this app image before rebuilding; the script checks this before changing generated files and again before packaging.

The script resolves public Platform **253.31033.145** artifacts, checks `standalone/dependencies.lock.json`, builds the native backend/viewport, compiles the product, resolves public vanilla JBR 21.0.10+1-b1163.108 with pinned publisher SHA512 and a Windows app image with jpackage. `-UpdateLock` is only for intentional dependency updates. No installed IDEA JAR, launcher, runtime or artwork is copied.

Launch directly after building:

```powershell
& '.\Temp\U\distribution\EpidemicEditor\EpidemicEditor.exe'
```

Distribute the **whole `EpidemicEditor` directory**, including `app`, `runtime` and notices. This is an app image, not an MSI. The matching Microsoft release CRT is bundled app-local with the native binaries and a derived JBR runtime; system DLLs and the verified upstream JBR cache are unchanged. Clean-machine deployment remains a separate check.

Entry: own EXE → `epidemic.launcher.Main` → open Platform bootstrap → `EpidemicStarter` → `PlatformStartup` → `PlatformSession`. Retired custom Swing owners and the U1 probe are removed. The live JNI adapter is part of this host. `Build-PlatformUI.ps1` is an optional same-code development plugin; `Build-EditorUI.ps1 -Test` runs common contracts and exposes no competing app launch route.

F0: File / Project Explorer → New Scene/Prefab/Graph; double-click opens a
native-ID tab. New Prefab can initialize real registered Health/Movement
components (clear the checkbox for an empty root). In Scene Hierarchy, choose
its root or component to edit the native i64/double/bool/string Inspector.
Enter/focus loss commits once; Esc cancels a draft; invalid/stale input stays
an error. Save flushes pending edits before the native atomic project save.
Tabs show their own native ID/revision and dirty state. Closing a modified tab
or detached document window offers Save All / Discard Document / Cancel;
Discard is scoped to that document, including undoable restoration of its saved
state. Other documents/assets are retained. Shortcuts also bind to tool content
so they follow the Inspector into a separate window.

Current automated results and **separate owner visual acceptance** are tracked
in `Development/EditorUI/FI_FEATURE_MATRIX.md`. This is F0 document/scalar
integration; F1 now wires asset workflows below. Scene picking/gizmos/components remain F2.

## F1 asset workflow

Project Explorer is the only browser: Tree/List/Grid, Enter-to-search through the
native catalog (1000-result window plus selection, real total shown), native IDs
and generations, missing/trash entries. Select an asset for Asset Details with
metadata, alpha/aspect preview and References; reference navigation opens the
actual document. Source/cooked state stays in ProjectFolder.

Import uses the existing parented Platform FileChooser with multi-select and a
Content-relative existing destination folder (empty = Content root). Supported:
PNG, static WebP, integer PCM WAV (1/2ch, 8/16/24/32-bit, 8–192kHz), TTF/OTF
SFNT registration. WAV/font metadata/IDs are real; playback/glyph authoring is
later. Ogg and animated WebP are not advertised. First native configure downloads
SHA-256-pinned libwebp 1.6.0; decoder links statically, notices ship in licenses.
The native companion embeds longPathAware; Windows long-path policy must also
be enabled for paths beyond MAX_PATH (qualification did not change that policy).

Select the asset, then Rename / Move, Duplicate, Reimport, Delete, Restore or
Assign to Prefab. Media extension changes are rejected. A move needs an existing
parent in Content; cross-volume moves are unsupported. Delete names incoming
references and retains stable missing identities; permanent deletion is disabled.
Asset edits use native mixed Undo/Redo and project dirty/Save All (bindings and
asset metadata are not separate document serialization).

Background Jobs shows native IDs/phases/outcomes and Cancel. Problems/Console
retains per-file failures. Watch observes the imported source inside **Content**;
changing the original external file alone does not change the project copy.
Reimport/Watch keeps last-good pixels on invalid source and clears current error
on recovery. Native Prefab/Scene previews follow published texture generations;
128px reduced previews are for qualification, full scene authoring is F2.

## Behavior

Launcher: New/Open, Settings, ten Recent Projects. Interactive state: `%LOCALAPPDATA%/EpidemicEngine/StandaloneEditor`; smoke state: generated product's `smoke-state`. The product creates its empty container with the Platform new-project API. Games never enter IDE VFS or build scripts. C++ owns game watching; the unused IDE watcher is disabled for the empty container.

Workspace opens maximized. Wired: snapshots, Rename Document, Save (Ctrl+S), Save All (Ctrl+Shift+S), Undo (Ctrl+Z), Redo (Ctrl+Y / Ctrl+Shift+Z), typed Select and Close. F0 adds File → Clone Saved Project and actual native Play/Stop (toolbar/Project menu, F8 toggle). Clone requires a saved folder project and leaves the source open; the destination must be new. Play/Stop preserve authoring/history. Continuous game stepping/live Game View remain planned for F-II/F8 and are outside the F0 contract. Text fields retain their own editing commands. No-op Save preserves bytes. Dirty close offers Save/Discard/Cancel. Disconnect keeps the snapshot and disables writes; reconnect starts one replacement companion.

The main menu contains eleven product groups. View discovers panels; Tools opens Gallery/panel lookup; Window controls native Window mode and layout. The retired native editor launch route is removed in L0. Existing C++ Project/Assets/Scene/Play APIs are retained for F integration; their availability does not imply complete new-UI bindings.

38 canonical panels plus eight on-demand views, 77 schemas, 570 UI fields. Planned controls stay disabled and show their phase. Inspector follows selection; the full schema picker is only in UI Gallery. Import uses the Platform FileChooser API; selection/cancel performs no import. The default public JBR selects the supported Windows native chooser. -StandardOpenJdk retains the Platform dialog as a diagnostic fallback. Whole-group docking and saved layout are accepted under the revised owner contract (see QA matrix). Scene rendering is the existing native renderer; Scene authoring parity remains F2.

## Verification

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\editor-ui-host\Build-Standalone.ps1 -JdkRoot "$env:USERPROFILE\.jdks\ms-21.0.7" -Smoke
powershell.exe -NoProfile -ExecutionPolicy Bypass -File .\tools\editor-ui-host\Run-U8-Checks.ps1 -JdkRoot "$env:USERPROFILE\.jdks\ms-21.0.7"
```

Batch checks run sequentially and continue after failures: common contracts, packaged product smoke, actual EXE second-launch/crash/restart regression, protocol, spec coverage, native regressions, C++-only build, full engine and analyzers. The batch stays nonzero while mandatory acceptance is incomplete. The U/L0 full-engine closure passed **161/161**, zero skipped; missing process drivers are restored under `tools/ci`. This is historical evidence, not a claim that every later F change has run the full profile. Existing analyzer findings remain classified, not suppressed. **U-GATE PASS under the owner contract dated 2026-11-05; see QA for exact L0/F status and retained nonblocking diagnostics.**

## Diagnostics

Native build: `Temp/b/ui-host`; assembly: `Temp/U/standalone`; EXE: `Temp/U/distribution/EpidemicEditor`. Interactive logs: `%LOCALAPPDATA%/EpidemicEngine/StandaloneEditor/log`; packaged smoke logs: `Temp/U/distribution/EpidemicEditor/app/smoke-state/log`. `platform-status.log` identifies the host/assertions; `idea.log` contains Platform diagnostics; `owned-main.pid` identifies the JVM. Smoke fails on any entry in Platform's fatal-message pool.

On Windows the product uses the upstream loopback TCP directory-lock transport and a disposable `system-tcp-v1` cache. Config, container and preferences retain their existing paths; old AF_UNIX cache files are left untouched. Repeated launch activates the live owner. Do not delete a live profile lock. `standalone/upstream/README.md` describes the pinned jpackage-parent correction; `tests/Test-StandaloneRestart.ps1` exercises it against the actual EXE.

`-NativeReady` reuses the requested native configuration. Rebuild normally after C++ changes. `-NoPackage -Smoke` checks bootstrap without validating jpackage; `-Configuration Debug -NoPackage` is for development. JAWT loads from the selected runtime explicitly. Surfaces stay on EDT and release before peer destruction.

Captures combine own-window Swing prints and retained GPU frames. They do not prove mouse dragging, occlusion, IME, physical DPI or absence of flicker. Actual statuses: `Development/EditorUI/UI_QA_MATRIX.md`. Notices/POMs live under product `licenses`; jlink retains runtime legal files. See `standalone/README.md` for composition limits.


## F-II.2 Code Space / C#

The optional managed authoring profile targets .NET 10; tested SDK: 10.0.402. `Build-Standalone.ps1` packages the repository-owned ScriptApi/ScriptWorker when the SDK is available; `-ManagedOff` omits them. The C++ build has no new .NET linkage. `dotnet publish` artifacts go under `.ai/managed-build`, never the source tree. There are no NuGet package dependencies; the worker build uses local SDK packs and cleared package sources.

Use File → New C# Class, Build → Build & Register C#, or Scripts in Project Explorer. The built-in view is an IntelliJ Platform Editor; Rider is optional. Build is explicit/trusted, runs in the background and does not import project Directory.Build targets. Incompatible schema changes retain the last registered data; no hot type patch is performed during Play. [Implementation/limits](../../Development/EditorUI/FII2_SCHEMA_CODESPACE.md) · [Manual QA](../../Development/EditorUI/FII2_MANUAL_QA.md).

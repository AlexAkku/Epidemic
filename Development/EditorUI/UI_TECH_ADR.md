# UI technology decision

Source: `Temp/!Tasks/UI_Roadmap.md`, owner contract 2026-11-05. IntelliJ Platform is the sole target Editor UI; C++23 remains the engine and authoritative editor backend.

- Actual ToolWindowManager/ContentManager, FileEditorManager/FileEditorProvider and ActionManager own groups, documents, windows and actions. Whole-group Tool Window movement is accepted; arbitrary individual instrument-tab transfer is not required. No custom docking/window owner is needed.
- The independent product uses pinned public Platform **253.31033.145**, 353 coordinate/SHA-locked artifacts. It owns launcher, starter, descriptors and app image. Installed IDEA is only optional development tooling; no installed product/runtime is a packaging source.
- Microsoft OpenJDK **21.0.7+6-LTS** compiles the Java sources. Default runtime is independently downloaded public vanilla **JBR 21.0.10+1-b1163.108**, verified with the publisher SHA512 pin in Resolve-Runtime.ps1. It supplies Platform's supported Windows chooser and font/native integration. `-StandardOpenJdk` is an explicit diagnostic fallback. No Kotlin compiler, Gradle or Compose is used.
- Product descriptors compose actual published services. The two unpublished IDE identifier-highlighting modules and unavailable IDE-internal Theme Color Picker are omitted. IDE startup antivirus-exclusion suggestions are not part of the game product; no security setting is changed. Service-free library wrappers describe real locked bytecode, not service stubs. Core-classloader mode preserves type identity on the flat Maven classpath.
- A small pinned Apache DirectoryLock delta excludes only the immediate native jpackage parent from competing-PID checks. Genuine owners retain activation protection. Windows uses upstream TCP transport and new disposable cache path; old unusable AF_UNIX files remain untouched. Actual EXE second-launch/crash/restart regression guards this correction.
- Native rendering remains existing D3D11/D2D Renderer through EDT-owned JNI/JAWT surface lifetimes. Reused Workspace, Scene/Inspector projections, assets, Play and serializer services remain C++. Immutable bounded IPC carries typed IDs and expected generation/revision, not native addresses.
- Win32 desktop and diagnostic Swing/U1 owners are retired in L0; optional SDK debugging uses the same PlatformSession. Removing their UI does not remove engine features, old data readers or semantic regressions. Exact inventory and retained dependencies: UI_MIGRATION_MAP.md.

U-GATE PASS follows the owner's qualification and the required automated checks in UI_QA_MATRIX.md. Complete F parity, clean-machine deployment, full transitive-license review and resource budgets remain separate work. Platform descriptor-loading compatibility warnings and classified historical analyzer findings remain visible in backlog; no validator or warning gate is disabled.

## F0 document close on Platform 253

VirtualFilePreCloseCheck gates tabs but the locked DockWindow path calls its
container closeAll and FrameWrapper.close directly. The host therefore wraps
those two closing WindowListeners only on its own floating document windows,
retaining and restoring the original handlers. The adapter awaits native
Save/Discard, captures the requested EditorWindow and preserves peer views.
It does not create windows, implement docking or modify signed Platform modules.
Actual AWT close events on independent HWNDs exercise Cancel/Save/Discard and
pending-Save ordering in packaged smoke; owner mouse acceptance stays separate.


## F1 native asset bindings (2026-11-06)

F0 visual acceptance is confirmed by the owner at 195e0c7. F1 keeps one
EditorWorkspace/ProjectFolder owner: immutable catalog queries/selection,
QueueImportPng (retained historical API name) generalized to validated typed
media, canonical Content watcher, mixed asset history and refs. Bridge v3 adds
asset-revision writes, bounded catalog queries and real job/preview projections.
The public Framework ABI/archive source schema is unchanged.

Static SHA-256-pinned libwebp1.6.0 is a private EditorCore dependency; no system
codec or additional DLL is required. Existing bounded WAV PCM / SFNT probes feed
the same staging/cook/publication flow. Codec notices ship with product and SDK.
Native companion longPathAware fixes >260 path operations on a system whose
long-path policy is enabled; no registry setting is changed by the application.

Native viewport reads immutable cooked images asynchronously through retained
ImageResourceLoader/ResourceServices, retaining ResourceLease and Renderer
texture ownership per surface. Upload occurs before Begin; new keys replace
old generations and reparent/close releases their surfaces. SceneView-derived
world data and Prefab bindings are projections, not a second authoring Scene.
128px previews are intentionally bounded; full-resolution authoring/camera and
picking remain F2. Physical acceptance is separate from API/GPU qualification.

## F2.0 missing bridge contracts

F2.1 can reuse the current hierarchy rows and typed selection. Extend the
native snapshot for the missing per-node visibility and placement identity,
keeping SceneNodeId scoped to its document. Expose only commands supported by
the native owner. Start with AddSceneNode, DeleteSceneNode, SetSceneParent and
PlacePrefab through EditorWorkspace. Allocate new IDs natively. Batch child
creation or placement with its related edits under one expected revision.
Scene node rename, enabled and subtree duplication need a native contract
before activation.

Later slices need read projections for local TRS, world bounds and camera
geometry, plus a command path for native SceneView and SceneDrag gestures.
Keep gesture state native and commit through SetSceneTransform once per
completed gesture. Expose component descriptors and per-placement overrides
through the existing Inspector/Workspace operations as each UI slice needs
them. Carry document, node, component type, part and property IDs with the
existing generation/revision checks. The backend rejects stale writes and
invalid document scope. Java carries display DTOs and input events.

System assignment and nested Prefab asset instances first need native
persistence and validation. Their absence from the current archive is a domain
gap beyond exposing a bridge operation. See FI_FEATURE_MATRIX.md for the
existing owners and retained renderer path.


Release qualification found the packaged JBR's MSVCP140 14.29 shadowing the
VS2026 native companion's STL: actual access violation in std::future/mutex
(first async import). The same Release companion passed outside the package.
Build-Standalone now resolves the active toolset's licensed x64 release CRT,
checks it is not older, and copies it unchanged into app/native and a derived
runtime. No system installation/registry update, cached-upstream mutation,
signed-JAR modification or static/mixed-CRT workaround. This covers both JNI
and companion initialization; the actual packaged EXE regression remains mandatory.
[Microsoft runtime compatibility](https://learn.microsoft.com/en-us/lifecycle/faq/visual-c-faq).

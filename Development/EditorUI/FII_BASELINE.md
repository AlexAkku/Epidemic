# Базовая ревизия F-II.0

| Проверка | Результат |
|---|---|
| Рабочая ветка и дерево на входе | `v2`, чистое рабочее дерево |
| Локальный HEAD, `origin/v2` и свежий `git ls-remote origin refs/heads/v2` | `ce06956da8691df292db26a48c0e261dea5c18b2` |
| Коммит | Akku, 2026-11-10 22:17:35 +01:00, «Freeze F-I after Content folder and nested Prefab fixes» |
| Remote | [AlexAkku/Epidemic, ветка v2](https://github.com/AlexAkku/Epidemic/tree/v2) |
| Машина | Windows x64, 24H2, build 26100. VS 2026, CMake, Python, Maven, Microsoft OpenJDK 21.0.7+6-LTS, Node 24.16.0, npm 11.13.0 |
| Профили .NET | SDK 8.0.422, 10.0.301 и 10.0.402. Managed-профиль использовал SDK 10.0.402 и host pack / hostfxr 10.0.13 |

## Владельцы и доступные контракты

| Область | Текущая реализация и пределы | Источники в дереве |
|---|---|---|
| Standalone и docking | `FileEditorProvider`, `FileEditorManager`, `ActionManager`, Tool Windows. Один companion на `PlatformSession`, отдельный Platform container. | `tools/editor-ui-host/java-intellij/epidemic/editor/platform/PanelEditorProvider.java`, `PlatformSession.java`, `tools/editor-ui-host/Build-Standalone.ps1` |
| IPC и authoring | Private IPC v4, framed stdio, capability handshake, immutable snapshots, process generation, project/selection/asset revisions. `EditorWorkspace::Execute` передаёт `EditRequest` владельцу ProjectFolder/EditorProject. Native Undo/Redo, Save/Open, стабильные IDs. | `tools/editor-ui-host/backend.cpp`, `tools/editor-ui-host/java/epidemic/editor/Bridge.java`, `Game/Editor/ui/src/editor_workspace.cpp`, `Game/Editor/src/editor_commands.cpp`, `Game/Editor/src/project_folder.cpp` |
| Graph | `GraphDefinition`, `ValidateGraph`, compiler и VM, typed ports, data/control/event, wait/repeat, native calls. `AddGraphNode`, `DeleteGraphNode`, `ConnectPorts`, `DisconnectPorts` существуют в `EditRequest`. IPC проецирует строки Graph nodes. Полный DTO портов и рёбер, правки графа из UI и сохранение координат требуют реализации. | `EngineRuntime/Graph/include/Epidemic/Runtime/Graph/graph_definition.h`, `EngineRuntime/Graph/src/graph_compiler.cpp`, `graph_flow_compiler.cpp`, `graph_flow_vm.cpp`, `Game/Editor/include/Epidemic/Game/Editor/editor_commands.h`, `tools/editor-ui-host/backend.cpp` |
| Schema и Gameplay | `NativeSchemaRegistry` поддерживает регистрацию типов и операций для C++/Graph. В GameFramework есть Type Registry и существующие runtime-контракты. Динамическая schema пользовательского C# и editor registration принадлежат F-II.2. | `EngineRuntime/Schema/include/Epidemic/Runtime/Schema/native_schema.h`, `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/type_registry.h`, `Game/Editor/example/demo_game.h` |
| Play и Renderer | `PlaySession::Tick(GameDuration)` использует существующие Time, Scheduler и Simulation. Standalone вызывает native Play/Stop. Scene использует D3D11/JNI viewport. Live Game View и управление tick из editor UI запланированы. | `Game/Editor/src/play_session.cpp`, `EngineRuntime/Time/src/time_runtime_impl.cpp`, `EngineRuntime/Support/src/graph_scheduler.cpp`, `EngineRuntime/Simulation/src/simulation_runtime_impl.cpp`, `tools/editor-ui-host/native_viewport.cpp`, `tools/editor-ui-host/java/epidemic/editor/UiPanels.java` |
| Managed | Opt-in `net10.0`, C ABI v1. `ManagedHost::Load(HostOptions)`, `Open(shared_ptr<INativeInvocation>)`, `ManagedSession::InvokeBatch(span, span)` и `Close()`, `CreateManagedGraphCallable(host, nativeFactory)`. Windows hostfxr загружает статический `GetApi`. Invoke синхронный и привязан к потоку открытия, Close активного вызова возвращает Busy. | `EngineRuntime/Support/include/Epidemic/Runtime/Support/managed_bridge.h`, `managed_bridge_abi.h`, `EngineRuntime/Support/Managed/native/managed_bridge.cpp`, `EngineRuntime/Support/Managed/Epidemic.ManagedBridge/BridgeRuntime.cs` |

## Повторная квалификация

- **PASS**. `Build-Standalone.ps1 -JdkRoot C:/Users/Akku/.jdks/ms-21.0.7 -SideBySide -Smoke` заново собрал Release и запустил EXE. Артефакт в `Temp/U/verification-images/b531e583f73948e6a8f4d1367c3d1dc6/EpidemicEditor/EpidemicEditor.exe`. Размер `app` 378 572 966 байт без сгенерированного smoke-профиля, `runtime` 166 469 319 байт. Один замер `launcher-to-project` в smoke составил 13 349,681 мс. Пик задания 1001,88 MiB включает сборку и тест.
- **PASS**. 17 адресных CTest в `Temp/b/ui-validation`, Debug, managed OFF. В выборку вошли архитектура, Graph definition/compiler/VM/control/scheduler/serialization, Schema, Renderer и Editor Demo/Workspace/SceneView/Core. JUnit `Temp/U/fii0-native.xml`, лог `.ai/logs/fii0-native.stdout.log`.
- **PASS**. Native Graph roundtrip 2/2 на `EpidemicEditorWriteProject` и `EpidemicEditorReadPlayProject`. JUnit `Temp/U/fii0-graph-roundtrip.xml`.
- **PASS**. Свежий build `EpidemicManagedBridgeTests` и 2/2 CTest в `Temp/b/managed`, Debug, managed ON. JUnit `Temp/U/fii0-managed.xml`.
- **PASS**. 16 IPC framing cases и coverage каталога. Release smoke, framing и catalog в `.ai/logs/fii0-release.stdout.log`.
- Полный CTest 161/161 относится к квалификации зафиксированного F-I. В F-II.0 повторили адресную выборку. История и ранее принятые условия в [F-I matrix](FI_FEATURE_MATRIX.md) и [UI QA](UI_QA_MATRIX.md).
- **ACCEPTED LIMITATION**. F-I №18 остаётся PASS* с повтором на сложных проектах. Физический повтор обновлённых ПКМ/Inspector, заполнение диска и перезапуск Windows при импорте имеют статус NOT RUN. Пространственный preview вложенных спрайтов отложен.

## Команды повторного прогона

Запускать из корня. В этой рабочей копии `Temp/b/ui-validation` уже настроен
с managed OFF, а `Temp/b/managed` — с managed ON, существующим host pack
и hostfxr 10.0.13.

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/editor-ui-host/Build-Standalone.ps1 -JdkRoot C:/Users/Akku/.jdks/ms-21.0.7 -SideBySide -Smoke
ctest --test-dir Temp/b/ui-validation -C Debug -R 'Epidemic(ArchitectureFreezeSelfTests|ManagedArchitectureTests|RuntimeGraphTests|GraphVMTests|GraphControlTests|GraphSerializationTests|SchemaGraphVMTests|GraphSchedulerTests|GraphEventSchedulerTests|GraphTimeIntegrationTests|RuntimeSchemaTests|RuntimeRendererTests|EditorSceneViewTests|EditorWorkspaceTests|EditorCoreTests|EditorDemoEdit|EditorDemoReadPlay)' --output-on-failure --no-tests=error
ctest --test-dir Temp/b/ui-validation -C Debug -R '^EpidemicEditor(WriteProject|ReadPlayProject)$' --output-on-failure --no-tests=error
cmake --build Temp/b/managed --config Debug --target EpidemicManagedBridgeTests --parallel 2
ctest --test-dir Temp/b/managed -C Debug -R '^EpidemicManaged(Bridge|Contract)Tests$' --output-on-failure --no-tests=error
python tools/editor-ui-host/tests/test_protocol.py Temp/b/ui-host/Release/EpidemicEditorBackend.exe
python tools/editor-ui-host/tests/test_catalog.py
```


## Отправная точка реализации F-II.0b

Локальный `5324262` поверх замороженного F-I `ce06956`. Штатная сборка теперь использует JBR/JCEF; старые vanilla-измерения выше остаются baseline. [Текущая квалификация](FII0B_GRAPH_ROUNDTRIP.md).

## Закрытие F-II.0b

Батч native sandbox launcher начат от `7402924`. F-II.0b PASS: штатный distribution EXE использует собственный CEF sandbox launcher и JBR/JCEF. Повторены Graph/IPC/catalog, 19 адресных CTest/архитектура, standalone Release smoke, startup lock/restart и changed-TU static checks. Адресный Windows ввод проверен без полного CU. [Текущая квалификация, ограничения и вход F-II.1](FII0B_GRAPH_ROUNDTRIP.md).

# F-II.0b: native Graph roundtrip

Дата: 11 ноября 2026. Отправная точка этого батча: `7402924`, поверх F-II.0a `5324262` и замороженного F-I `ce06956`. Push не выполнялся.

**F-II.0b PASS. F-II.0 PASS. Вход в F-II.1 открыт; реализация F-II.1 не начата.**

## Реализация

- Штатный Windows EXE теперь собственный native launcher. В основном EXE до JVM создаётся настоящий `CefScopedSandboxInfo`, статически связанный с официальным CEF 137 `/MT`. Chromium дочерние процессы запускают тот же EXE и входят в публичный `jcef_helper.dll!execute_subprocess` без JVM. Сохраняется JCEF message router. JNI передаёт настоящий hexadecimal pointer и точную CEF version, принудительно используется in-process JCEF. NativeLauncher проверяет зарегистрированный JNI, а Graph отвергает no_sandbox/remote mode. Unsandboxed исключение для smoke удалено.
- Официальный JBR/JCEF `21.0.10-windows-x64-b1163.108` сохранён. Совместимый SDK: `cef_binary_137.0.17+gf354b0e+chromium-137.0.7151.104_windows64_minimal`. SHA512 архива закреплён в `Build-Launcher.ps1` и проверяется при каждой сборке. После первоначального разрешения JBR, CEF SDK, Maven и npm нормальная упаковка работает из проверенных offline caches. Пустой cache не объявляется offline-ready. Подробности, хеш и исходные контракты: [native launcher](../../tools/editor-ui-host/standalone/native-launcher/README.md).
- JNI явно перечисляет locked JARs, помещает продуктовый epidemic.jar первым для существующих bootstrap corrections DirectoryLock/MainImpl, задаёт system context classloader и прежние JVM opens. Runtime DLL поиск ограничен app/system и bundled bin; используется официальный CEF delay-load list, необходимый для Win32k lockdown. Реальный Platform HttpConfigurable инициализируется до JBCefApp Holder, чтобы повторный профиль не создавал proxy service внутри class initializer. Сохраняются runtime legal, CEF/JCEF BSD, полные Chromium credits (542 компонента), npm notices; исходники launcher/manifest/build script входят в notices продукта. Это не юридическая квалификация публичного распространения.
- Rete остаётся проекцией настоящего GraphDefinition: JSQuery → Java Bridge → private IPC v4 → EditorWorkspace::Execute. Op 38 create/connect/disconnect/move/delete, DTO v1 nodes/ports/edges/layout, stable IDs, generation/revision и Play read-only guards. Отдельного авторитетного JS-графа нет.
- Presentation metadata `graph_layout.version=1` принадлежит EditorDocument; v0 без поля мигрирует к implicit grid. Native MoveGraphNode участвует в Undo/Redo, dirty/Save. Неизвестные версии/узлы, дубликаты и недопустимые координаты отклоняются. Delete/Undo восстанавливает incident edges и позиции.
- Исправлен найденный вводом дефект: Rete останавливает bubbling pointerdown, поэтому сброс отменённого жеста теперь выполняется capture listener. После Escape следующий drag снова работает; откат возвращает координаты без пересоздания узлов во время удержания мыши. Постоянный standalone smoke проверяет реальный preview, удерживаемый drag/Escape/release, восстановление позиции и следующий commit через JCEF input APIs.
- CSP hash bundle, точный локальный origin, запреты external resources/navigation/popup/protocol и allowlist Bridge сохранены. Нет shell/filesystem RPC. Browser/query принадлежат FileEditor и освобождаются Disposer. [Контракты Code Space и managed F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md) окончательные; LSP/Rider/C# Play в этом батче не реализованы.

## Пройденная квалификация

| Проверка | Результат |
|---|---|
| Собственный Release launcher, `/W4 /WX`, штатная offline-упаковка | PASS; штатный `Temp/U/distribution/EpidemicEditor/EpidemicEditor.exe` |
| Настоящий Windows CEF sandbox | PASS; own renderer имеет restricted token, integrity RID 0 (Untrusted), Win32kDisabled=true; `no_sandbox=false`, remote=false. Job membership — дополнительная информация, не единственное доказательство. AppContainer не заявляется. |
| Production Bridge / native Graph roundtrip | PASS; три native-узла, типизированное соединение/reject, stale guard, invalid layout reject, Undo/Redo, exact IDs/coordinates Save/Open во втором companion, delete/Undo |
| Standalone Rete/JSQuery/native | PASS; native DTO отрисован React DOM, create/connect/move/history/Save |
| Отмена удерживаемого drag | PASS; JCEF sendMouseEvent/sendKeyEvent: press/drag/Escape/release не меняет revision; следующий drag даёт ровно один native edit, Save |
| CSP и Bridge | PASS; file/HTTPS/loopback fetch и eval запрещены, open отвергнут без смены revision |
| FileEditor lifecycle / shutdown | PASS; close/reopen освобождает browser/query и создаёт новую view, независимый HWND; после smoke нет owned launcher/CEF процессов в пределах 15 секунд |
| Live owner / stale startup lock | PASS; повторный EXE активирует прежний owner; после аварийного завершения проверенного собственного main повторный запуск с тем же изолированным профилем проходит полный smoke |
| IPC / catalog | PASS; 16 adversarial framing cases, 38 canonical panels + 8 on-demand, 325 tabulated fields |
| Адресные CTest / архитектура | PASS; 19/19 Graph/VM/control/scheduler/time/serialization/schema/renderer, Editor Core/Workspace/SceneView/Headless и архитектура. Layout migration входит в Core. C++ engine не менялся относительно `7402924`. |
| Изменённые native TU, штатный Verify-Changed | PASS; обе новые C++ единицы, clang-analyzer/bugprone, 0 failures |

Адресно проверен настоящий Windows UI в maximized workspace, без полного Computer Use прогона: создание Native Call кнопкой, совместимое соединение мышью, Control→Data reject `graph.connection_kind`, отмена начатого соединения Escape, следующий move, Ctrl+Z/Y/S. Graph вынесен через штатное Open Tab in New Window; там работают клавиатура и Save. После закрытия отдельного окна основной workspace сохраняет граф. Новый Editor-процесс открыл сохранённые 4 узла/2 связи/координаты. Удерживаемый mouse-down→Escape проверен отдельно JCEF input APIs, а не выдан за физический жест. Найденный дефект следующего move после Escape воспроизведён и повторно проверен исправленным UI.

Финальные автоматические проверки: `sandbox-input-final`, `sandbox-regressions`, `sandbox-restart`; runtime подробности в `Temp/U/distribution/EpidemicEditor/app/{smoke-state,qualification-state}/log/platform-status.log`. Успешные Run-Quiet логи могут очищаться согласно штатной политике; runtime состояние временное.

**Блокеров F-II.0b нет.** Не выполнены и не включены в этот gate: полный повтор CTest 161, 500/2000-node performance/stress и полный U/мониторы/DPI прогон. Это не PASS по F-II.1. Известные исторические analyzer diagnostics Scene/Composition/EditorDocument/ThumbnailCache остаются backlog; проверки не ослаблены.

## Повтор

Из корня репозитория:

```powershell
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/editor-ui-host/Build-Standalone.ps1 -JdkRoot C:/Users/Akku/.jdks/ms-21.0.7 -Smoke
powershell.exe -NoProfile -ExecutionPolicy Bypass -File tools/editor-ui-host/tests/Test-StandaloneRestart.ps1
python tools/editor-ui-host/tests/test_protocol.py Temp/b/ui-host/Release/EpidemicEditorBackend.exe
python tools/editor-ui-host/tests/test_catalog.py
```

Native roundtrip: скомпилировать `Bridge.java` и `tests/GraphTests.java` Microsoft JDK 21 в `.ai/graph-tests`, запустить `epidemic.editor.GraphTests` с аргументами `Temp/b/ui-host/Release/EpidemicEditorBackend.exe .ai/graph-tests`. Для первоначального npm cache: `Build-Graph.ps1 -Resolve`; launcher SDK первоначально разрешает `Build-Launcher.ps1` и затем всегда сверяет SHA512.

## Вход в F-II.1

Готов фундамент §4 F-II_Implementation_Spec; следующий батч расширяет текущий adapter, не создаёт новую оболочку или graph owner:

1. Schema-derived catalogue и DTO (schema revision, port multiplicity/defaults, params/diagnostics/feature states), затем подключение существующих Graph Palette и Node Inspector/Selection. Активировать только подтверждённые native операции; Variables/C# без backend остаются disabled.
2. Native UI create/delete/duplicate/connect/disconnect/parameters, bulk actions атомарно через EditRequest; stable IDs и revision/conflict guards. Validate/Compile и Problems→Node ID; выполнение сравнивается с headless Graph VM.
3. Pan/zoom/fit/reset, selection/multi-select, bounded Copy/Paste, comments/groups/minimap и markers согласно spec. UI history лишь transient; `rete-engine` не используется. Camera/selection сохраняются при incremental projection; один commit на drop, rollback Escape/capture loss.
4. Расширить security tests: JSON depth/count/rate и output/HTML injection. Замерить 100/500/2000-node профили, соблюдая существующий native лимит 1024: превышение должно давать bounded reject до отдельно обоснованного изменения контракта. Нет скрытого повышения лимитов ради benchmark.

F-II.1-GATE пока NOT RUN: его DoD требует compile/execute native VM, расширенное редактирование и собственную адресную квалификацию. Graph↔Scene остаётся F-II.3, Code Space/managed — F-II.2.

# Проверки F-II

**F-II.0a PASS. F-II.0b PASS. F-II.0 PASS.**

| Проверка F-II.0b | Статус | Что действительно проверено |
|---|---|---|
| Native Graph / production Java Bridge / IPC v4 | PASS | Три узла, DTO портов/рёбер, совместимое соединение, несовместимые/неизвестные endpoints, stale revision. GraphTests.java повторён. |
| Move/history/Save/Open во втором native-процессе | PASS | Точные stable IDs/coordinates; delete/Undo восстанавливает incident edges и layout. |
| Layout migration | PASS | EditorCoreTests: v0 без поля, стабильная сериализация, v1 coordinates, отказ v2. |
| Rete → JSQuery → Bridge → Execute | PASS | Штатный sandbox EXE создаёт native узлы/ребро, move/history/Save; React DOM отображает snapshot. |
| FileEditor lifecycle | PASS | Browser/query disposed на close; reopen создаёт новую view; отдельный HWND; реальный Windows UI сохраняет данные при закрытии floating view. |
| JCEF shutdown | PASS | Нет owned launcher/cef_server/jcef_helper процессов в пределах 15s после standalone smoke. |
| CSP / ресурсы / Bridge | PASS | File/HTTPS/loopback fetch, eval и Bridge open отклонены; native revision не меняется. |
| Windows CEF sandbox | PASS | Реальный CefScopedSandboxInfo в EXE до JVM; own renderer restricted=true, integrity=0, Win32kDisabled=true. Без no_sandbox/remote. AppContainer не заявляется. |
| Official JBR/JCEF / offline / notices | PASS | JBR и matching CEF SDK SHA512, Maven 353 artifacts, npm offline, полный Chromium credits и launcher sources в продукте. |
| Native mouse/keyboard Graph scenario | PASS адресно | Create, compatible/rejected connection, Escape cancellation, move после Escape, Ctrl+Z/Y/S; отдельное Graph окно и новый процесс Save/Open. Без полного CU прогона. |
| Удерживаемый drag/Escape/release | PASS JCEF input APIs | Настоящие CefBrowser mouse/key события: ноль native edits при отмене, следующий drag — один edit. Не объявляется физическим mouse-down тестом. |
| Startup / lock | PASS | Live-owner activation; аварийно остановлен только подтверждённый own main, stale profile restart прошёл полный Platform/native smoke. |
| Адресные CTest / архитектура | PASS | 19/19; Graph/VM/control/scheduler/time/serialization/schema/renderer, Editor Core/Workspace/SceneView/Headless, permanent architecture checks. |
| IPC framing / catalog | PASS | 16 cases; 38 canonical IDs + 8 on-demand, 325 tabulated fields. IPC envelope v4, Graph DTO v1. |
| Static analysis launcher | PASS | Штатный Verify-Changed, обе C++ TU, 0 failures; MSVC Release W4/WX. |
| Исторические engine analyzers | KNOWN DIAGNOSTICS | Ранее известные Scene signed-bitwise, Composition/EditorDocument exception-escape/unsafe-to-allow-exceptions и ThumbnailCache easily-swappable-parameters; checks не менялись. |
| 500/2000 nodes / idle / расширенный stress | NOT RUN | F-II.1/Q0; текущее native ограничение 1024 сохраняется. |
| Полный CTest | NOT RUN в F-II.0b | Адресная выборка; исторические 161/161 относятся к F-I. |

Технологические EditorFactory/LSP/managed пробы из [baseline](FII_BASELINE.md) не выданы за продуктовый Code Space или C# Play. Их окончательные задачи — [F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md).

[Точный результат и команды](FII0B_GRAPH_ROUNDTRIP.md). F-II.1 подготовлен, реализация и gate пока NOT RUN.

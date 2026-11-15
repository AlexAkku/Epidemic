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

[Точный результат и команды](FII0B_GRAPH_ROUNDTRIP.md). F-II.1 реализован; автоматическая функциональная квалификация PASS, manual gate NOT RUN.

## F-II.1

| Проверка | Статус | Основание |
|---|---|---|
| Native authoring / metadata / atomic fragment / 50-node history | PASS | Production Bridge, typed rejections, точные IDs/positions/settings во втором процессе |
| Compile / native VM / Scheduler | PASS | Math13, overflow Node ID, Branch/Repeat/Wait result7/time6 |
| Graph Inspector / Save / invalid input / Undo | PASS | Настоящее Swing поле, pending draft flush, invalid Save reject, EDT future ordering |
| 64-node JCEF drag/Escape/Undo | PASS input APIs | Сохраняются относительные позиции; физическая мышь — ручная приёмка |
| JCEF100/500 / HTML/depth/size/CSP / lifecycle | PASS | Реальный DOM, kernel renderer peak + CDP heap, UI disposal/reopen/HWND/shutdown |
| CTest / architecture / IPC / catalog | PASS | 19/19 и 16 framing cases; свежие затронутые Editor consumers |
| Static | KNOWN DIAGNOSTICS: Verify-Changed Fast проверил 61 TU; общий результат FAIL из-за предупреждений в неизменённом коде. Два новых unchecked-optional-access исправлены; повтор backend с теми же checks не содержит diagnostics на добавленных/изменённых строках. Sandbox metrics TU чистый PASS. Сохраняются signed-bitwise Scene, exception/unsafe constructors Composition/Editor, ThumbnailCache parameters, narrowing в тестах и float-loop/switch diagnostics native_viewport. Подавлений и ослабления checks нет | Checks не ослаблялись; классификация в отчёте |
| 2000-node render | NOT RUN / native limit1024 | Безопасный reject PASS, лимит не повышался |
| Ручной Windows input/visual acceptance | PASS · владелец 2026-11-14 | Без Computer Use; [инструкция](FII1_MANUAL_QA.md) |

[Отчёт F-II.1](FII1_GRAPH_AUTHORING.md).


F-II.1-GATE **PASS**: владелец подтвердил ручную приёмку 2026-11-14 («Проверил, всё работает»). Исходная реализация `451ba62`; ранее зафиксированные ограничения производительности и исторические static diagnostics сохраняются. F-II.2 начинается от этого коммита.


## F-II.2

| Проверка | Результат |
|---|---|
| Full Debug/native-only CTest, public headers, architecture | PASS 161/161 |
| Scalar metadata + native commands + Undo/Redo + fresh-process Save/Open | PASS; i64/double/bool/string |
| Production Code Space build/register; SDK errors; failed last-good; Directory.Build.targets | PASS standalone smoke |
| Source Save/external conflict/Reload/reopen/Editor disposal | PASS; owner-lifetime race regression retained |
| C# through native VM and Play/Send/Tick; two methods, one worker, Stop and retired handles | PASS automated |
| IPC v4/catalog and existing managed ABI tests | PASS |
| Static | 73 TU checked; new diagnostics corrected, existing-code warnings remain FAIL/backlog |
| Physical multiwindow/Rider/visual qualification | MANUAL NOT RUN |
| LSP | NOT SHIPPED; specification offline fallback, not claimed PASS |

## F-II.3 — текущий implementation batch

Native GraphReference payload v1, typed compiler/IR/VM resolver, atomic command 40 и Graph DTO v4 подключены к одному project-scoped Windows OLE DnD coordinator. [Контракт и фактические проверки](FII3_GRAPH_SCENE.md), [ручная квалификация](FII3_MANUAL_QA.md). F-II.3-GATE ожидает физической межоконной DnD/DPI проверки владельцем; NOT RUN не переименован в PASS.

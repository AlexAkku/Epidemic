# F-II.1: Graph Canvas / native authoring

Дата: 13 ноября 2026. Отправная точка: `8aca4a9` (F-II.0b). Push не выполнялся.

**F-II.1-GATE PASS.** Автоматическая квалификация завершена; владелец подтвердил ручную приёмку 2026-11-14. F-II.2 начат от `451ba62`.

## Что работает

- Существующие Graph Palette / Node Inspector / Variables / References подключены к native snapshot. Каталог происходит из ProjectSchema: Event, Native Call, Wait, Input, Output, Add, Multiply, Entry, Sequence, Branch, Repeat. Необеспеченные Variables/Get/Set/C#/Entity явно недоступны. Новая docking оболочка не создавалась.
- Rete остаётся проекцией GraphDefinition. Private IPC v4 сохранён. Текущий Graph DTO v5 (после F-II.3 и исправлений ручной проверки): каталог, stable node/port/edge IDs, typed ports, reference metadata, координаты, метаданные, compile/run diagnostics и optional i64 port default. Все постоянные изменения идут через EditorWorkspace::Execute / EditRequest и native Undo/Redo. rete-engine и отдельной JS history нет.
- Create/delete/duplicate, Connect/Disconnect/atomic Reconnect, native clipboard с внутренней топологией, групповое перемещение и группировка транзакционны. Лимит выделения/clipboard — 64 узла, внутренних edges — 128, общий native budget — 256 commands; Copy заранее проверяет вместимость Paste. IDs присваивает native owner. Stale generation/revision и неверные endpoints отклоняются без частичного изменения.
- `graph_layout.version=1` сохраняется. Добавлен отдельный optional `graph_authoring.version=1`: preview defaults, comment/group и bookmark/breakpoint markers. Старый архив без раздела мигрирует к пустым defaults, неизвестная версия отклоняется. Сериализация runtime GraphDefinition / ABI VM не менялись.
- Inspector редактирует имя, Input/Event preview default, комментарий, группу и маркеры. Ввод группируется до Apply/Enter/Save, без native mutation на каждую клавишу. Draft участвует в Save/close/selection; invalid/stale input не проходит. Escape отменяет draft. Завершение future происходит после EDT transition, включая отказ, чтобы Save/Undo/Close не гонялись с состоянием поля.
- Pan/zoom/Fit/Reset, Ctrl/box selection, один commit на drop, Escape/focus/capture cancellation, native selection, context menu, minimap, navigation/bookmarks. Snapshot обновляет существующие node views и связи; camera/selection сохраняются. Комментарии видны в Inspector/tooltip, группы — в metadata/подписи и Select Group. Размеры узлов вычисляются renderer по портам/заголовку; произвольный node resize не заявляется.
- Compile и Run Preview используют настоящий GraphInstance, GraphScheduler и отдельные native Time/Simulation services. Scheduler подтверждает Wait по собственному due time; preview двигает изолированный clock, не игровой мир. Budget — 4096 instructions / 256 scheduler steps. NativeCall без gameplay owner явно отклоняется: фиктивная реализация ABI не подставляется. Существующие scoped domain bindings/Play сохранены; Graph↔Scene wiring остаётся F-II.3.
- Подхват существующей связи не удаляет её; Escape сохраняет исходную связь, drop выполняет атомарный reconnect с одним Undo. Для удаления есть явный Disconnect.
- Typed errors попадают в Problems; compile/VM/connect diagnostics содержат Node ID, есть переход к узлу. Find Uses показывает поддерживаемые Scene System → Graph ссылки.
- Сохранены настоящий Windows CEF sandbox, CSP hash/local resources и запрет внешних files/network/eval/general RPC. До JSON parsing действуют size/depth guards (65536 UTF-16 units, depth 6), rate 60 messages/s, argument/count limits. Большое сообщение даёт явный отказ, а не молчаливый drop. При reconnect отсутствующий snapshot безопасно отклоняется.

## Выполненные проверки

| Проверка | Результат |
|---|---|
| Production Bridge / native Graph | PASS; 11-operation catalogue, typed rejects, unique IDs/topology, 50-node move/delete, один Undo, metadata/Save/Open во втором процессе |
| Native compile/VM | PASS; арифметика 7+6=13, overflow с Node ID; Entry→Branch→Repeat→Wait завершён, result=7, game_ticks=6 |
| Metadata migration | PASS; старый архив без optional part, v1 roundtrip, отказ неизвестной authoring version; существующий layout migration сохранён |
| Адресные CTest / архитектура | PASS 19/19, свежие Editor Core/Workspace/SceneView/Headless consumers; Graph/VM/scheduler/serialization/schema/renderer и permanent architecture checks |
| IPC / catalog | PASS; 16 framing cases; 38 canonical + 8 on-demand panels, 325 tabulated fields |
| Standalone Release | PASS; реальные JCEF nodes/links, existing-edge pickup/Escape без edit, 64-node group drag/Escape/Undo, draft Save/invalid rejection, reopen/independent HWND, native crash/reconnect, shutdown без owned CEF процессов в пределах 15s |
| Security | PASS; CSP/file/HTTPS/loopback/eval, forbidden Bridge action, depth/size, literal HTML injection, invalid/2000-node snapshot без native mutation |
| Web UI detector | PASS; mechanical impeccable detect — пустой список. Визуальная проверка не заявляется |
| Static analysis | KNOWN DIAGNOSTICS: Verify-Changed Fast проверил 61 TU; общий результат FAIL из-за предупреждений в неизменённом коде. Два новых unchecked-optional-access исправлены; повтор backend с теми же checks не содержит diagnostics на добавленных/изменённых строках. Sandbox metrics TU чистый PASS. Сохраняются signed-bitwise Scene, exception/unsafe constructors Composition/Editor, ThumbnailCache parameters, narrowing в тестах и float-loop/switch diagnostics native_viewport. Подавлений и ослабления checks нет |
| Computer Use / ручной прогон | Computer Use NOT RUN; ручная приёмка владельцем PASS 2026-11-14 |

Временные команды/журналы: Run-Quiet `fii1-bridge-final`, `fii1-regressions`, `fii1-release-final`, `fii1-static`; успешные ShortOutput логи очищаются штатным wrapper. Runtime smoke: `Temp/U/distribution/EpidemicEditor/app/smoke-state/log/platform-status.log`.

## Замеры профиля

| Native nodes → real Rete DOM | Grow/layout/DOM roundtrip | Renderer peak working set | CDP JS used heap после projection |
|---|---|---|---|
| 100 | 1011 ms | 93.20 MiB | 6.91 MiB |
| 500 | 2017 ms | 193.92 MiB | 15.24 MiB |

Это один локальный прогон Windows, не FPS/idle/multiwindow certification. Roundtrip включает native grow/layout и ожидание DOM; renderMs в runtime log относится только к последнему incremental update. Kernel peak относится к renderer-процессам, не ко всей JVM/utility/GPU совокупности. CDP usedSize — точная текущая выборка, не гарантированный continuous JS peak; coarse `performance.memory` не выдаётся за точное измерение. Расширенный Q0 performance budget остаётся будущей квалификацией.

Native workloads 100/500/1024 прошли. Лимит core **1024** не повышался: 1025-й узел даёт `graph.limit_exceeded`, snapshot из 2000 узлов отклоняется. **2000-node render не выполнен**, поскольку противоречит текущему native контракту; это известное ограничение, а не PASS такого render.

## Артефакт и приёмка

Штатный дистрибутив: `Temp/U/distribution/EpidemicEditor/EpidemicEditor.exe`. JBR/JCEF, SDK SHA512, Maven/npm locks, offline packaging и notices сохранены; новых сетевых frontend dependencies нет.

[Пошаговая ручная инструкция](FII1_MANUAL_QA.md). Владелец подтвердил приёмку 2026-11-14; F-II.1-GATE PASS. Контракты Code Space / managed из F-II.0b сохраняются.


F-II.1-GATE **PASS**: владелец подтвердил ручную приёмку 2026-11-14 («Проверил, всё работает»). Исходная реализация `451ba62`; ранее зафиксированные ограничения производительности и исторические static diagnostics сохраняются. F-II.2 начинается от этого коммита.

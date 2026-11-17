# Приёмка F-II

**F-II.0a PASS. F-II.0b PASS. F-II.0 PASS.**

Сквозной Graph authoring работает в штатном самостоятельном Editor с настоящим Windows CEF sandbox. Native launcher создаёт sandbox до JVM; renderer имеет restricted token, Untrusted integrity и Win32k lockdown. Unsandboxed smoke исключение удалено.

| Обязательный результат F-II.0b | Статус |
|---|---|
| Native GraphDefinition через Rete/Java Bridge/IPC v4/Execute; узлы/порты/связи | PASS |
| Native create/connect/type reject, координаты, миграция, Undo/Redo, Save/Open в другом процессе | PASS |
| Официальный JBR/JCEF, проверяемый offline cache, SHA512, notices | PASS |
| Настоящий sandbox, CSP/file/network/eval/Bridge ограничения | PASS |
| FileEditor close/reopen, независимое Windows окно, keyboard focus, bounded shutdown | PASS |
| Мышь: compatible/rejected connections, Escape; клавиатура Ctrl+S/Z/Y в основной и отдельной view | PASS, адресный Windows сценарий |
| Удерживаемый node drag→Escape→release; следующий drag | PASS через реальные JCEF input APIs: ноль edits, затем ровно один commit |
| Адресные 19 CTest/архитектура, IPC/catalog, два launcher TU static analysis, startup lock regression | PASS |
| Code Space и managed lifecycle | PASS по окончательным контрактам F-II.2; LSP/Rider/C# Play не реализованы |

Полный U/Computer Use прогон не выполнялся. На выходе F-II.0 нагрузочные 500/2000 nodes и расширенное редактирование/compile/VM оставались NOT RUN; текущая квалификация F-II.1 приведена ниже. Исторические анализаторные предупреждения остаются backlog.

[Отчёт и подготовленный вход F-II.1](FII0B_GRAPH_ROUNDTRIP.md) · [Контракты F-II.2](FII_CODESPACE_MANAGED_CONTRACT.md).

## F-II.1

Реализация завершена; функциональная автоматическая квалификация PASS. **F-II.1-GATE PASS: ручная приёмка владельцем 2026-11-14**. [Результат, static diagnostics и ограничения](FII1_GRAPH_AUTHORING.md) · [Пошаговая инструкция](FII1_MANUAL_QA.md). Полный Computer Use не запускался; 2000-node render не заявляется. NativeCall preview требует gameplay scope; C#/Variables остаются F-II.2/3.


F-II.1-GATE **PASS**: владелец подтвердил ручную приёмку 2026-11-14 («Проверил, всё работает»). Исходная реализация `451ba62`; ранее зафиксированные ограничения производительности и исторические static diagnostics сохраняются. F-II.2 начинается от этого коммита.


## F-II.2

Реализован scalar/unary authoring profile от `451ba62`: native persisted schema, C# build/registration/invocation, один generation worker, real Code Space и external-editor workflow. Полный CTest 161/161, IPC/catalog и managed ABI PASS. Реальный production SDK pipeline и close/reopen Editor проверены standalone smoke; обнаруженная owner race исправлена без удаления теста. Новые static diagnostics исправлены; общий анализ сохраняет прежние предупреждения. [Подробности](FII2_SCHEMA_CODESPACE.md).

**F-II.2 manual acceptance NOT RUN. Gate не объявлен PASS по одним автоматическим тестам.** По последнему указанию владельца разработка продолжается до F-II.3 без промежуточной остановки; это не заменяет визуальную приёмку. [Инструкция F-II.2](FII2_MANUAL_QA.md).

## F-II.3 — текущий implementation batch

Native GraphReference payload v1, typed compiler/IR/VM resolver, atomic command 40 и Graph DTO v4 подключены к одному project-scoped Windows OLE DnD coordinator. [Контракт и фактические проверки](FII3_GRAPH_SCENE.md), [ручная квалификация](FII3_MANUAL_QA.md). F-II.3-GATE ожидает физической межоконной DnD/DPI проверки владельцем; NOT RUN не переименован в PASS.

## Ручной прогон владельца 2026-11-16

F-II.0/0b запуск и существующий owner подтверждены; исчезновение caption buttons после активации требует повтора исправленного toolbar. F-II.1 основные arithmetic/control/selection/history/persistence сценарии подтверждены; новые inline/menu/clipboard/Palette DnD изменения ожидают адресного повторного прогона.

F-II.2: source editing, автосохранение, внешний VS Code sync/Open With, native C# invocation и Source navigation, multi-window документы подтверждены. Автосохранение принято как целевое поведение. Rider принят владельцем без установленного Rider; это исключение приёмки, а не доказанный запуск Rider. Schema/defaults/rebuild, Remove Component и Ctrl+Y исправляются по найденным ошибкам и требуют повторной приёмки; F-II.2-GATE пока не PASS.

F-II.3: физические пункты37–48 владелец ещё не прошёл; native automation не заменяет их. F-II.4 не начат. [Отчёт исправлений](FII_REVIEW_FIXES.md) · [Новый адресный прогон](FII_REVIEW_MANUAL_QA.md).

### Подтверждение адресного повтора 2026-11-16

Владелец подтвердил PASS пунктов 1–13 нового адресного прогона: исправленные Graph interactions/inline/clipboard, window controls, source shortcuts/file commands, schema defaults/rebuild и Remove Component приняты. Ручная приёмка исправлений F-II.1 и F-II.2 подтверждена. Пункт 13 подтверждает подготовку Scene/Graph и двух placements, а не весь F-II.3. Пункты 14–21 требуют подробной инструкции и остаются PENDING; частичное «что-то работает» не считается PASS. Пункт 22 понятен владельцу, но результат ещё не сообщён. F-II.3-GATE остаётся PENDING, F-II.4 не начат.

### Последующее подтверждение 2026-11-16

Владелец подтвердил пункты 14–21. Пункт 17 принят функционально с замечаниями о resize, подписях и размещении элементов; 17, 19 и 20 сохранить для последующих регрессий. Пункт 21 принят владельцем без отдельного перечисления конфигураций мониторов/DPI; самостоятельная повторная физическая проверка агентом не заявляется.

**F-II.2-GATE PASS** в реализованном offline scalar/unary профиле: автоматические проверки и ручной повтор исправлений прошли; Rider остаётся принятым исключением без фактического запуска. F-II.1 повтор исправлений также принят.

Замечания UI устранены отдельным батчем: стабильная dock geometry, узкий Graph Inspector, человекочитаемые подписи, закрываемый overlay Graph diagnostics и caption controls у правого края. Адресный standalone smoke PASS; окончательная упаковка фиксируется в [отчёте](FII_REVIEW_FIXES.md). Требуется короткий визуальный повтор этих изменений. Результат пункта 22 ещё не получен, поэтому **F-II.3-GATE PENDING**, а не автоматический PASS. **F-II.4 NOT STARTED**: подготовлен только [порядок входа](FII4_ENTRY.md).

## F-II.3-GATE PASS — подтверждение владельца 2026-11-16

Владелец подтвердил все исправления UI и пункт22 (Save, полное закрытие, новый процесс, Play/Stop и завершение). **F-II.3-GATE PASS.** Принятые ограничения Rider и исторические static diagnostics сохраняются; 17/19/20 остаются в регрессиях. F-II.4 разрешён к реализации с единственным C++ Runtime authority и двумя проверочными прогонами — промежуточным и финальным. Computer Use и push запрещены.


## F-II.4 — code complete, ручной gate ожидается (17 ноября 2026)

Full clean C++ build, architecture self-test и public headers PASS; полный CTest161/161 PASS после актуализации failure-сценариев без удаления проверок. Standalone Release/JCEF smoke и packaged Runtime/Graph/Scene/C#/IPC/architecture/catalog регрессии PASS. Реальные GPU Sprite pixels, native Health100→93→86/Wait/Pause/Step/Restart/Stop, активная нода настоящего JCEF,200 окон и late-open Console history подтверждены автоматически. Java управляет/отображает, не исполняет игру и не задаёт её время.

Deep analysis77 TU/154 checks не назван PASS: inherited diagnostics остаются backlog, новый narrowing исправлен и проходит адресный повтор. Подробности и measured budgets в [FII4_REPORT](FII4_REPORT.md).

**F-II.4-GATE PENDING MANUAL**: владелец ещё не подтвердил физические кадры/фокус/DPI/мониторы и performance budgets. Актуальная совместная инструкция начинается с10 комбинированных регрессий прежних стадий: [FII45_MANUAL_QA](FII45_MANUAL_QA.md). F-II.5 реализован, финальная автоматическая квалификация описана отдельно ниже. Без push.

## F-II.5 — gameplay implementation, ручной gate ожидается

Реализованы три последовательных блока: единый C++ gameplay owner/Schema, native fixed-step механики, managed adapter и пользовательский Arena sample. Публичный IPC v4 и managed C ABI v1 сохранены; private worker RPC v3 делегирует операции тому же owner. Нет второго игрового цикла в Java/C#.

Full native build и CTest162/162 PASS, standalone Release/JCEF smoke PASS; точные packaged/static результаты и ограничения — [FII5_REPORT](FII5_REPORT.md). **F-II.5-GATE PENDING MANUAL**, F-II.4 также PENDING MANUAL, общий F-II-GATE не закрыт. [Контракт](FII5_GAMEPLAY_CONTRACT.md), [пошаговая проверка](FII45_MANUAL_QA.md). F-III не начат.

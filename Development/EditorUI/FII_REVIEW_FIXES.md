# F-II: исправления по ручной проверке 2026-11-16

Отправная точка: `594f776`. Computer Use и push не используются.

## Приоритетные ошибки

- Schema registration больше не считает изменение default несовместимым типом. Dynamic registry заменяет immutable descriptors; compatibility проверяется native project transaction с сохранёнными значениями и Undo. Defaults и добавление scalar fields допускаются; удаление/смена типа существующих fields требует явной миграции и отвергается атомарно.
- Новые C# компоненты наследуют schema defaults без записи копии всех defaults в Prefab. Архив хранит versioned policy `inherit_defaults`: для старых компонентов без неё совпадающие с прежним default значения однократно возвращаются к наследованию. После этой миграции явные authored values сохраняются даже при совпадении со старым default; instance overrides не затрагиваются. Для старого формата совпадение с default является единственным доступным признаком наследования.
- Remove Component доступен с root через явный выбор компонента, а также по выбранной строке компонента. Native command сохраняет generation/revision/selection guards и Undo.
- Code Space Ctrl+Y/Ctrl+Shift+Z назначаются Redo, Delete Line перенесён на Ctrl+Delete. Штатная синхронизация/автосохранение C# сохранены по желанию владельца.
- В merged toolbar добавлены устойчивые действия Minimize / Maximize-Restore / Close с обычной проверкой закрытия dirty-проекта. Активация существующего окна обновляет root pane.

## Запрошенное поведение

Recent Projects открываются двойным кликом. Graph Palette сгруппирована сворачиваемыми категориями и поддерживает drag/drop recipe на Graph. Контекстное Add использует категории; внешний левый клик и Escape закрывают меню. При создании node из меню во время протягивания соединения исходный socket восстанавливается после native commit. Подхваченное соединение удаляется Delete; Escape сохраняет прежнее; Disconnect в меню остаётся.

Graph Input редактируется на узле. Add/Multiply имеют persisted i64 input literals; подключённая связь имеет приоритет. Literal хранится в GraphDefinition port, компилируется через native IR, проходит Undo/Redo/Save/Open. Graph DTO v5; envelope IPC v4 и ограничения прежние. Save запрашивает inline drafts и не должен пропускать invalid значение.

Graph Ctrl+C/V/X/D маршрутизируются к браузеру и native copy/paste/cut/duplicate; текстовое clipboard внутри inline input остаётся текстовым. Scripts имеют Copy/Cut/Paste/Duplicate/Rename/Delete через Explorer с ограничением пути проектным Scripts и без overwrite. Дублирование файла копирует текст, не переименовывает автоматически C# symbols/canonical IDs; перед сборкой дубликата их нужно явно изменить.

[Backlog будущей полировки](FII_UI_POLISH_BACKLOG.md).

## Проверки и статус

- PASS: чистая Release-сборка native host; GraphTests, GraphSceneTests, ScriptTests с настоящим SDK, новым процессом и native VM; IPC и catalogue (453.27 s).
- PASS: Debug-сборка, public headers, полный CTest **161/161**, managed bridge, IPC/catalogue, контрактные и архитектурные проверки (975.29 s).
- Изменённые C++ translation units проверены clang-analyzer/bugprone с warnings-as-errors. На изменённых production-строках новых диагностик нет. Общий static-check остаётся FAIL из-за 10 прежних диагностик (Scene signed-bitwise, Composition/Editor exception/move, ThumbnailCache parameters); подавления и ослабления не добавлены.
- PASS: адресный standalone JCEF smoke (115.10 s): inline draft/Save/invalid/Escape; категории Add и внешний клик; Copy/Paste/Cut с атомарным Undo; подхват ребра настоящим JCEF input → Delete → Undo; Add во время связи → продолжение от прежнего socket → Escape/Undo. Native hit/coordinator/typed DTO, Missing/Restore, закрытие/reopen и shutdown без fatal diagnostics, `surfaces=0`.
- Новые регрессии проверены и исправлены: synthetic outside-click обязан отпускать pointer; diagnostic report не отправляется посреди pending native write. Обязательные проверки selection/topology не удалены и не ослаблены.
- PASS: свежий standalone Release, воспроизводимая offline-упаковка с hash/license checks и полный штатный smoke (188.43 s). Физический cross-HWND DnD, второй монитор и DPI не заявляются автоматически проверенными.
- PASS: GraphTests, GraphSceneTests и ScriptTests повторно на native backend именно из свежего дистрибутива (24.57 s): Graph literals/history/fresh-process persistence, typed references/coordinator/100 cancel cycles и реальный C# SDK/VM/defaults/Remove roundtrip.

EXE для проверки: `Temp/U/verification-images/724bdd048b1d45c784ab02537097add6/EpidemicEditor/EpidemicEditor.exe`. Сборка расположена отдельно от пользовательского экземпляра; обычный повторный запуск активирует прежний editor process, поэтому перед проверкой новой сборки нужно штатно закрыть старый.

F-II.0 / F-II.0b: PASS. F-II.1: прежняя основа принята владельцем, новые изменения ожидают адресного повторного прогона. F-II.2: большинство визуальных сценариев приняты владельцем; schema/Remove/shortcuts требуют повтора после исправлений. Rider принят владельцем без установленного Rider; реальный запуск Rider не заявляется. F-II.3: текущие native/coordinator/JCEF проверки PASS; физические пункты 37–48 ещё не приняты, гейт не закрыт. F-II.4 не начат.

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

EXE для проверки: `Temp/U/verification-images/c50b7725ff2d44f29996dadf9b7d4a7a/EpidemicEditor/EpidemicEditor.exe`. Сборка расположена отдельно от пользовательского экземпляра; обычный повторный запуск активирует прежний editor process, поэтому перед проверкой новой сборки нужно штатно закрыть старый.

Текущий статус: F-II.0/0b и F-II.1 PASS; F-II.2-GATE PASS в принятом offline scalar/unary профиле, Rider принят с исключением без запуска. Владелец принял пункты1–21; окончательный F-II.3-GATE ожидает короткого повтора UI и результата пункта22. F-II.4 NOT STARTED, подготовлен только порядок входа.

## Последний батч: стабилизация компоновки, 2026-11-16

Отправная точка этого батча — `0156236`. Владелец принял пункты 1–21 адресного прогона; F-II.1 и F-II.2 приняты в заявленном профиле. Замечания 17/21 исправлены:

- Dock-панели получают стабильные layout hints, независимые от выбранного объекта/длины подписи. При переключении контекста новые вкладки добавляются до удаления старых: группа не опустошается и не перестраивает splitter. Ручное изменение разделителей и resize окна сохраняются.
- Graph Inspector использует форму, следующую ширине viewport. Все поля/кнопки выровнены слева, действия могут сужаться вместе с панелью, вертикальная прокрутка сохраняет доступ к нижним элементам.
- Explorer/Hierarchy показывают имена документов, типы операций Graph, имена компонентов и пути ресурсов без сырых IDs/generation. Внутренние identities, Selection, архив и native commands не изменены. Диагностическая identity reference остаётся в Inspector для проверки ссылок.
- Диагностика Graph находится в непрозрачном overlay над прежней областью canvas и закрывается крестиком. Закрытие не стирает native ошибку; новая диагностика/следующий Compile может показать её вновь. Canvas сохраняет размер и координаты.
- Кнопки окна перенесены из списка действий toolbar в закреплённую правую область header. Они следуют размеру окна; обработчики Minimize/Maximize-Restore/Close и dirty-close guard сохранены.

Проверки: адресный standalone smoke **PASS (136.74 s)**, включая неизменность bounds при Scene→Graph→Scene, все четыре действия Inspector при ширине220 px, caption controls у правого края максимизированного окна, invalid inline Save и dismiss overlay без изменения canvas. Прежние Graph/SDK/Scene/lifecycle проверки в этом прогоне прошли, shutdown `surfaces=0`. Снимок максимизированного окна просмотрен без Computer Use. IPC v4 adversarial16 cases, каталог и managed architecture **PASS**. Свежая standalone Release упаковка с полным штатным smoke PASS (199.23 s), без fatal diagnostics, surfaces=0. Диагностический detector: [].

Пункты 17,19,20 сохраняются в последующих регрессиях. Результат пункта22 ещё не сообщён, поэтому окончательный F-II.3-GATE остаётся PENDING. Для визуального повтора нужны только стабильность размеров при выборе, узкий Inspector, подписи, overlay ошибки и положение кнопок окна. [Порядок входа в F-II.4](FII4_ENTRY.md) подготовлен; **F-II.4 NOT STARTED**, runtime-код этого шага не добавлялся.

GraphTests, GraphSceneTests и ScriptTests на backend именно из финального дистрибутива — **PASS (23.73 s)**. Native C++ код в этом батче не изменялся; прежний полный CTest161/161 остаётся baseline, новый полный CTest не заявляется. Новая автоматическая проверка ошибок Graph сравнивает bounds до/после invalid Save и после dismiss; проверки native history, CSP, sandbox и disposal сохранены.

# Исправления регрессии F-II.4 / F-II.5

Baseline:17498a7. Запрос владельца19 ноября2026; Computer Use и push не используются. Ручные F-II.4/F-II.5-GATE остаются PENDING MANUAL.

- Compile native Graph больше не запускает preview/managed worker и не требует живого Entity. Binding проверяется настоящим CompileGraph с compile-only factory; вызов такой factory во время исполнения запрещён. Run Preview для native gameplay возвращает информационное предложение Attach Graph/Play без изменения authoring. Play сохраняет единственный native Runtime owner.
- Выбор Prefab Document через IPC selection нормализуется на корневой PrefabNode; Scalar Inspector и component actions сразу доступны из Explorer.
- Remove Sprite снимает существующую native binding. Fit сохраняет пропорции в локальных unit bounds без увеличения маленьких текстур; Reset восстанавливает64px/unit. Asset edits и сохранение используют существующий ProjectFolder/Workspace.
- Prefab component/resource actions переносятся по ширине dock в несколько строк; minimum width не растягивает соседние панели.
- Documents поддерживает Move через общую Rename/Move и Document Actions. Организация сохраняется в пути имени через native RenameDocument с прежними ID/history/Save/Open. Empty folders в Documents, Scripts и корне создаются как обычные project directories; Explorer проецирует их с ограничением256 каталогов/глубины8, без обхода служебных/Content/bin/obj деревьев. Версионированные document blobs не перемещаются.
- Добавлена [типизированная математика](FII_NUMERIC_GRAPH.md): i64/f64, вычитание, деление/целая часть, округления и явные conversions. C# gameplay call ABI сохранён.
- [Основная инструкция](FII45_MANUAL_QA.md), пункт8, теперь содержит точную последовательность Entity/component reference с Alt+drag, Missing/Undo и новым процессом. Подготовлен [адресный ручной повтор](FII45_REVIEW_QA.md).
- Дополнительный smoke поймал потерю Open во время Loading/асинхронного закрытия документа. Запрос теперь проходит через существующую очередь (лимит32), с проверкой bridge/generation/project/document kind; чужой/удалённый документ не открывается. В исходный lifecycle smoke добавлен настоящий Save перед повторным Open: старый вариант отбрасывал запрос, исправленный создаёт свежий Graph FileEditor/Windows HWND в прежний таймаут. Глобальные SDK $Redo/EditorDeleteLine больше не изменяются: source shortcuts используют ActionUtil.wrap с lifetime конкретного Editor.

## Проверки

Адресный native батч4/4 PASS после исправления границы enum в Editor Schema validator. Первый прогон поймал этот дефект, а не был отмечен PASS. Native Graph VM/control, gameplay и workspace прошли. В новой compile-only factory исправлена ошибка квалификации namespace, выявленная настоящей Release-сборкой.

Standalone Release/JCEF smoke после исправления codec PASS (293.70s); все packaged Runtime/Graph↔Scene/Graph/C#/двухоконные Game/IPC/catalog/managed architecture проверки PASS (190.09s). В частности, доказаны Compile/Preview вне Play без изменения Health, настоящее Play100→93→86, выбор Prefab root, Remove/Fit/Reset Sprite, f64 inline/Undo/typed reject и Move Document/Save/Open другим процессом. Первый packaged прогон обнаружил отсутствующий f64 в архивном codec; guard не отключался, список дополнили новым каноническим типом.

200 закрытий/открытий Game: surfaces=0, steady handlesΔ−3 и private bytesΔ≈14.5MiB (конечный повтор), ниже порога плато64MiB. Кадры/фокус/DPI/мониторы вручную агентом не проверялись. Детектор skill по изменённым Java UI-файлам вернул пустой список; это не визуальная приёмка.

Полная native пересборка и **CTest162/162 PASS** (768.41s вместе); публичные header consumers прошли. После lint-only исправлений повторно собран/запущен Graph VM с сохранением всех numeric cases — PASS (138.54s). Алгоритмы расчёта после полного прогона не менялись.

Deep завершён:5headers,52 реальных TU,104checks (1560.07s). Generated one-line header wrappers проверены полной сборкой; в temporary analysis DB остались все реальные потребители. Итог Deep **FAIL**: наследуемые/toolchain diagnostics остаются backlog. Четыре новые позиции (два defaults, clone ветвей switch и соседние double-параметры теста) устранены; адресные clang-tidy и cppcheck с теми же флагами **4/4 PASS** (79.17s), без новых suppressions или отключения проверок.

Конечный standalone Release/JCEF smoke **PASS** (336.50s), включая воспроизводимую очередь Open за Save, новое Graph view/HWND, crash/reconnect, Arena и teardown. В idea.log нет предупреждений `ShortcutSet of global`; surfaces=0. Финальный packaged повтор на этом же EXE — **PASS** (190.77s): Runtime, Graph↔Scene, Graph/f64, C#,200 Game window cycles/Arena в двух HWND, IPC v4, catalog и managed architecture. Source/UI/build/test inputs после этого не изменялись.

Ручная приёмка не считается успешной заранее. Приложение для владельца: `Temp/U/verification-images/3bd1025cae5c4ec68c90220446c6dd3b/EpidemicEditor/EpidemicEditor.exe`.

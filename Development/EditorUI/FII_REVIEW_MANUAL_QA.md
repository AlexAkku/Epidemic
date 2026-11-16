# Повтор исправлений и оставшиеся проверки F-II

Штатно закрыть прежний Editor, затем открыть `Temp/U/verification-images/724bdd048b1d45c784ab02537097add6/EpidemicEditor/EpidemicEditor.exe` и максимизировать окно. При оставленном старом процессе повторный EXE активирует его, поэтому исправления не будут загружены. Полный повтор 48 пунктов не нужен. Source auto-save/sync сохранены по принятому контракту; Rider принят владельцем без фактического запуска.

## Исправления

1. Recent Projects: двойной клик открывает проект. Повторный EXE активирует прежний workspace; проверить кнопки Minimize, Maximize/Restore, Close справа в merged toolbar. Close должен сохранять обычную защиту dirty-проекта.
2. Palette: категории сворачиваются; поиск раскрывает совпадения; Add Node, двойной клик и drag/drop recipe в Graph создают один node в нужном месте.
3. ПКМ Graph: Add → категория → node. ЛКМ вне меню и Escape закрывают меню. Начать связь, ПКМ → Add → node; после создания линия продолжает тянуться от прежнего socket до подключения или Escape.
4. Input: изменить значение прямо на node, Enter применяет, Escape отменяет. Ctrl+S должен сначала применить draft. Неверное число блокирует Save, после исправления сохранение работает.
5. Entry/Input(7)/Add/Output: Input→Add.left; Add.right оставить literal=6 на node. Entry.next→Output.in; Add.result→Output.value. Compile/Run Preview =13. Изменить right=9 →16. Подключить к right второй Input(2) →9, поле должно уступить связи. Disconnect →16 (сохранённый literal=9). Проверить Undo/Redo и Save/Open.
6. Подхватить существующую связь за input: линия полупрозрачная; Escape возвращает, перенос переподключает, Delete удаляет; один Undo возвращает. Disconnect в ПКМ остаётся.
7. Graph Ctrl+C/V/X/D: copy/paste/cut/duplicate выбранных nodes, сохранение внутренней topology, новые IDs копий, один Undo каждой операции. В inline field clipboard остаётся текстовым.
8. C# text: Ctrl+Y и Ctrl+Shift+Z — Redo, Ctrl+Delete — Delete Line. Автосохранение и внешняя синхронизация должны остаться.
9. Scripts Explorer: Copy/Cut/Paste/Duplicate/Rename/Delete через ПКМ и соответствующие C/V/X/D/Delete. Проверить путь с кириллицей, открытый файл и отсутствие overwrite. Duplicate копирует текст: перед Build удалить копию либо изменить class/canonical IDs, чтобы не компилировать дубликат типа.
10. C# Build: создать Turret, собрать; добавить компонент на свежий Prefab. Изменить Range default 150→175 в C#, Build & Register. Тип и компонент не исчезают, незаданный Range становится175. Явно изменить Range в Prefab на123, source default→200: после rebuild остаётся123. Добавить scalar поле — появляется без потери прежних. Удаление/смена типа прежнего поля должны дать migration_required без потери данных.
11. Создать второй C# компонент, добавить рядом с Turret. Remove Component с root предлагает выбор; удалить ровно один, Undo/Redo сохраняют второй. Повторить с выбором строки компонента, Save/Open и после успешного Build.
12. В C# Graph из прежней инструкции Input7 → ApplyDamage → Output: оригинальный метод даёт93, тело amount*2 после Build —86. Синтаксическая ошибка даёт file:line diagnostic, последняя рабочая schema остаётся. Проверить Cancel Build, Rebuild Required, Play/Stop/Build и последующий запуск новой generation.

## Оставшиеся F-II.3 (прежние37–48)

13. Разместить два экземпляра одного Prefab: A/B. Graph и Scene открыть рядом и в отдельных Windows-окнах. Добавить Is Entity Valid.
14. Alt+drag reference-input→A в Hierarchy, затем→B в viewport: native preview, node+edge одной операцией, без spawn; Undo/Redo целиком.
15. Alt+drag из Hierarchy и viewport→пустой Graph, затем→reference-input. При неоднозначности выбрать Entity/Instance/Definition/Component. Проверить разные placement IDs у A/B при одном prefab/entity ID.
16. Точный Component/part → совместимая Is <Component> Valid; wrong component type, i64/control/event или занятый input отклоняются без изменений.
17. Jump to Reference Target/Find Target References. Rename/Reparent не меняют identity. Delete даёт Missing Reference; Undo оживляет тот же node.
18. Graph Entry.next→Output.in, Reference→Is Entity Valid.reference, valid→Output.value: Preview1, после Delete цели0, после Undo1.
19. Отдельный Graph для Prefab-owned scope: явно отличать Definition от placement. Разные Scene scopes не смешиваются.
20. Escape, drop вне окна, закрытие source/target view/window, смена проекта и backend reconnect: нет частичных edits и зависшей подсветки; следующий drag работает.
21. Повторить оба направления между независимыми окнами, на втором мониторе, DPI100/150. Проверить точность socket/entity и фокус после Drop. Нет конфигурации — NOT RUN.
22. Save, полностью закрыть Editor, открыть новым процессом. Сверить связи/IDs/координаты/Component part; несколько Play/Stop не меняют authoring. Закрытие не оставляет owned ScriptWorker/CEF.

Для ответа: номер PASS/FAIL/NOT RUN; для FAIL — действие, ожидаемое/фактическое и ошибка. Физический DnD/DPI по-прежнему закрывается только ручной приёмкой, не headless-тестами.

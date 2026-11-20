# Финальные исправления приёмки F-II.4 / F-II.5

20 ноября 2026. Отправная точка: `50983ed`. Без Computer Use и push. F-II.3-GATE PASS; F-II.4/F-II.5 и общий F-II-GATE **PENDING MANUAL**.

## Что завершено

- ПКМ Rename/Move/Delete папок Content и Documents: одна native история Undo/Redo, стабильные AssetId/DocumentId и ссылки, Save/Open. Непустое удаление сохраняет bytes в приватной корзине; entry Scene и внешние сильные ссылки защищены. Межмонтные перемещения, циклы, конфликтующие пути, ссылки/reparse и слишком длинные результирующие пути отвергаются.
- Общий source-moves journal v2 сохраняет порядок файловых и каталоговых операций; чтение старого v1 поддерживается. Неподтверждённые перемещения восстанавливаются в обратном порядке после нового Open.
- Scripts и обычные папки проекта используют существующие SDK VFS/WriteCommandAction; открытый C# source переоткрывается по новому пути. Удаление сохраняет файлы в `.epidemic/FolderTrash`, исключает их из активных sources. Фиксированные и служебные корни защищены. Единый native Undo для этих файловых операций не заявлен.
- Нативная регистрация явно публикует реальные scalar defaults C++ компонентов. Inspector читает метаданные без создания component/factory. Существующие Arena Prefab получают доступные InputBindings и Collider без пересоздания; авторские значения, параметры и overrides сохраняют приоритет. Типы без объявленных defaults остаются unavailable.
- Восемь native повторов двух Entity: explicit events и autostart, одинаковые tick inputs при разном wall pacing. Проверяется Health обоих объектов на каждом тике, 100→93→86. В этом профиле повреждения происходят на одинаковых тиках без breakpoints. Универсальный replay managed/external inputs этим не заявлен.

## Фактические проверки

- Native Debug build, полный **CTest 162/162 PASS**, ArchitectureFreezeSelfTest PASS (643.05 s).
- После дополнительного preflight результирующих asset paths: Folder Create/Reopen/Recovery **3/3 PASS**. После расширения replay: GameplayTests PASS, восемь explicit/autostart запусков.
- Финальный Release backend/JNI, штатная offline упаковка с pinned JBR/JCEF и standalone smoke PASS. Platform Graph/FileEditor/окна, sandbox/Bridge/CSP, reconnect, C# Editor и новые папки Scripts прошли; shutdown: `surfaces=0`, fatal Platform diagnostics нет.
- Упакованные GraphTests, RuntimeTests, GraphSceneTests, ScriptTests PASS. IPC v4 adversarial, catalog/source coverage и managed architecture PASS. Финальный повтор всего этого набора на готовом EXE: **PASS, 102.00 s**. Время и исполнение остаются в C++ Runtime, Java управляет сессией и отображает DTO.
- Адресный clang-tidy по 8 реальным TU: **FAIL**, 356.24 s, 20 унаследованных диагностик (19 в исходниках, 1 в MSVC xfilesystem_abi). Git blame подтверждает прежние выражения; macro-parentheses на изменённой строке относится к прежнему аргументу V, существовавшему в 17498a78, а не к добавленному default. Новых диагностик от текущей реализации не найдено. Остаток: signed bitwise, exception specifications/escape, легко переставляемые параметры и branch clone; это существующий static backlog. Checks и warnings-as-errors сохранены, suppressions не добавлялись.

Первый Release-запуск с ошибкой адаптера Select(bool) был FAIL; исправлен и повторён. Неуспешные предварительные запуски не названы PASS.

## Игровая семантика

Детерминизм означает одинаковую последовательность состояния при одинаковом начальном состоянии, seed, tick inputs и scheduler budgets. Это не обещание атомарности независимых coroutine. Но неповторимый сдвиг в чистом профиле без debugger недопустим: требуется отдельный ручной повтор на проекте владельца. Конкретная причина наблюдавшегося сдвига не подтверждена.

Текущий ApplyDamage — single-target. Будущая AoE обязана зафиксировать target set, подготовить и применить все записи одной native транзакцией до callbacks/публикации игрового checkpoint. Debug RuntimeSnapshot может показывать частично исполненный tick на breakpoint; это наблюдение отладчика, а не savegame/checkpoint. Editor Save сохраняет authoring.

Три Enemy в стартовой позиции Player ожидаемы в минимальном Arena sample: Graph Spawn копирует transform вызывающего Player. Отдельный WaveSpawner использует свой transform. Пространственный spawn не добавлялся.

## Осталось вручную

Владелец принял прежние исправления 1–7, F-II.4:15/16/24 и F-II.5:1–4,6–7,9–13,16. Остались новые папки, повтор InputBindings/Collider (5/8), чистый replay двух Entity, F-II.4:21 Console/Problems и F-II.5:14/15. Подробные действия и ожидаемые результаты: [финальный раздел инструкции](FII45_REVIEW_QA.md#финальный-ручной-прогон-перед-freeze-f-ii).

После подтверждения этих пунктов можно закрывать F-II.4/F-II.5 и замораживать F-II. Сейчас PASS не выставлен. F-III не начинался.

Актуальный EXE:

```powershell
& 'D:\MyProjects\Epidemic engine v2.0\Temp\U\verification-images\8b28b81e5f534452b8274e111f01d4a1\EpidemicEditor\EpidemicEditor.exe'
```

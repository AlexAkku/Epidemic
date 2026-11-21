# F-II — итоговый freeze-отчёт

21 ноября 2026. **F-II-GATE PASS · F-II FROZEN.** Владелец подтвердил весь финальный ручной прогон на реализации `55ab4bf`: «Всё проверил, работает корректно». Отправная точка последнего implementation batch: `8b27287`. Нового Computer Use или повторного полного прогона не выполнялось; push не выполнялся. Следующая стадия не начата.

## Изменения

- Исправлен реальный scheduler starvation: общий Play budget 16 jobs / 512 VM instructions мог разносить одинаковые активации на разные игровые ticks. Теперь каждый ready Graph получает собственный bounded GraphScheduleOptions quantum перед следующим native tick. Max-active и per-activation instruction/native-call limits сохранены. Clock, Pump и исполнение остаются в C++ Runtime; Java игрового цикла не получила.
- Регрессия расширена с двух простых Entity до пяти с 80 Add-зависимостями перед Damage, восемь explicit/autostart повторов с разным wall pacing. Старый aggregate budget действительно роняет этот тест; исправленный Play даёт одинаковые Health по каждому tick, 100→93→86.
- Scene Viewport/Hierarchy: Ctrl+C/X/V и Delete. Native clipboard хранит authoring subtree; Paste назначает новые IDs, сохраняет имена, subtree/Transform/Prefab/overrides; мировая позиция переносимого корня вычисляется существующим C++ Scene/Transform API, а не из устаревшего derived cache. Same-scene Cut сохраняет IDs и references, корректный no-op не создаёт history; Copy, Cut/Paste и Delete-many проходят существующие EditorWorkspace::Execute. Mutation — один Undo шаг. Copy не меняет revision; Cut переносит при Paste и отвергает устаревшую authoring revision. Clipboard очищается при смене native workspace.
- Project Explorer: Shift/Ctrl multiselect в Tree/List/Grid, Copy/Cut/Paste/Duplicate/Delete и F2; native primary-selection acknowledgement не схлопывает множественное выделение. File/folder move по внутреннему DnD использует существующие C++ команды либо SDK VFS. Старый Prefab/texture resource drag в Scene сохранён.
- Повторный одиночный клик по выбранной строке Tree и F2 включают inline rename. Enter подтверждает через владельца данных, Escape отменяет; локальный tree model не подменяет native результат. Double-click открывает документ. List/Grid используют Rename dialog.
- В Explorer оставлены Import/Watch Content, остальные действия перенесены в ПКМ. New Document, New C# Class и Import Here используют выбранную папку. Content показывает basename с расширением вместо полного path. Protected roots, entry Scene, mounts, conflicts, cycles и stale generations защищены. Asset jobs ожидаются асинхронно, ошибки/отмена не выдаются за успешную копию.
- Scene toolbar уплотнён в одну штатную строку, уменьшены отступы/шрифт, добавлен общий native toolbar-button style.

## Пройденные проверки

- Native Debug build и полный CTest: первоначально **161/162 PASS**, 329.86 s. Единственный FAIL выявил устаревший Stop/Failed fixture, полагавшийся на aggregate job cap. Он изменён на реальный ненулевой Wait у соседних активаций; все прежние проверки cancellation/retention/diagnostics/context release сохранены и добавлено доказательство Waiting до Stop. Адресный повтор **1/1 PASS**. Остальные 161 тест повторно не запускались: их код и production inputs не изменились.
- Контрпример scheduling: расширенный five-Entity тест FAIL с прежним budget и PASS с исправлением; GameplayTests PASS.
- Standalone Release/JBR/JCEF smoke PASS. Pinned SHA512/лицензии/offline упаковка сохранены. Реальные SDK Editor/VFS source copy/move/folder copy/delete и release, Explorer Documents/Content folder copy, Unicode rename/drop move, стабильность source IDs прошли. Graph/JCEF lifecycle, sandbox/Bridge/CSP, отдельные окна и reconnect прошли; shutdown surfaces=0 и owned launcher/CEF processes завершены.
- Packaged UiTests PASS, включая новые multiselect/hotkey/context target/inline rename projection проверки. Найдено и исправлено ещё одно устаревшее ожидание: зарегистрированный Health теперь имеет пятое доступное поле maximum; тест проверяет его editable default=100, а не игнорирует его. GraphTests и RuntimeTests PASS. Scene clipboard адресный Release-прогон PASS (40.01 s): имена, world/local, same-scene IDs, hierarchy Undo, cross-scene и Save/Open. Окончательный **standalone Release + SDK/JCEF smoke + UiTests/GraphTests/RuntimeTests/GraphSceneTests/ScriptTests + IPC v4 adversarial/catalog/managed architecture PASS**, 352.13 s, после всех production исправлений. Логические Documents-папки и корень без physical directory отдельно проверены настоящим SDK сценарием; New Document сохраняет прежний запрет во время pending close.
- Штатный **Verify-Changed Fast**: compiler dependency discovery PASS (54.95 s), 1 header/4 реальных TU. Первоначально обнаружены две новые exception-escape диагностики неявного move SceneClipboard; тип сделан неперемещаемым и размещён под единственным unique_ptr владельцем. Release Scene roundtrip после исправления PASS (38.72 s); повтор backend clang-tidy завершён (77.24 s). Остаток **FAIL: 14 унаследованных diagnostics**, 0 на изменённых строках; Git blame подтверждает 12 прежних source locations плюс один MSVC header diagnostic (два прежних сообщения относятся к одному source location). Ничего не подавлялось и checks/warnings-as-errors не отключались.

Предварительные ошибки запуска проверок (неверное имя build target и интерфейс аргументов UiTests) не названы PASS. Они исправлены без изменения production ради harness.

## Границы профиля

- Native replay проверяет равные входы и ready activations; это не универсальная replay-квалификация асинхронных managed/external inputs. Атомарная AoE-транзакция и committed savegame snapshot по-прежнему будущий контракт; debug projection может быть partial tick.
- Content folder copy требует полного текущего asset snapshot (до 1000 записей). Если snapshot неполон, операция явно отклоняется, не теряя скрытые assets. Для Scripts/обычных папок действуют VFS/file semantics; native atomic Undo заявлен для Scene mutation и существующих Content/Documents move/delete. Explorer batch file operations последовательны и прекращаются на первом отказе.
- Владелец подтвердил drag/rename/fullscreen toolbar и одинаковые ticks у пяти экземпляров. Исторический static backlog: signed bitwise, exception specifications/escape, optional access, похожие параметры и branch clone; отдельно MSVC enum-cast diagnostic. Этот backlog не закрывается изменением checks.

[Регрессионный набор](FII45_MANUAL_QA.md) принят владельцем 21 ноября. F-II.0–F-II.5 и общий gate закрыты в описанном профиле; следующий этап требует отдельной задачи.

Замороженный квалифицированный EXE: `D:/MyProjects/Epidemic engine v2.0/Temp/U/verification-images/2e86f090fd7845cf9f9c9b8ef7bec475/EpidemicEditor/EpidemicEditor.exe`.

## Что сохранено после очистки

Сохранены исходники/тесты/данные движка и редактора, исходные задания, текущие контракты/ADR/матрицы, регрессионные инструкции и измеренные бюджеты. Оставлен один принятый standalone image; pinned JBR/JCEF/CEF SDK, Maven/npm caches, licenses и текущие native build/compilation databases сохранены для offline разработки.

Промежуточные review/entry инструкции и накопленные статусы сведены в этот отчёт, матрицы и регрессионный набор. Удалены устаревшие standalone images, доступные для очистки промежуточные outputs, автоматически созданные smoke-проекты, логи и одноразовые служебные скрипты. Каталоги с проектными данными (`fii0`, `fixtures`, `platform`, `ui-host`, `ui-release-tests`, `ui-tests`) сохранены, чтобы не удалить пользовательские проекты. Контроль SHA256 принятого EXE, backend и JAR подтвердил отсутствие изменений; offline-зависимости и текущие build-каталоги сохранены. История удалённой документации доступна в Git. Очистка не меняет production-код и не запускает следующий этап.

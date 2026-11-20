# Исправления повторной приёмки F-II.4 / F-II.5

20 ноября2026. Baseline:c02355b. Без Computer Use и push. F-II.3-GATE остаётся PASS; F-II.4/F-II.5-GATE — PENDING MANUAL.

## Изменения

- Пустые папки Explorer используют folder icon даже у leaf-узла.
- В Data один Input/Output, в Math один элемент каждой числовой операции. Старые typed TypeId сохранены для открытия прежних графов; отдельные i64/f64 пункты палетки и Add menu скрыты.
- Замки на узле и в Graph Inspector выбирают Auto/Lock i64/Lock f64. Native C++ согласует типы связанных числовых узлов, сохраняет авторские признаки литералов, отвергает несовместимые фиксированные типы и потерю точности. Node/port IDs сохраняются. Правка и изменение типов проходят одним Execute/Undo.
- graph_authoring v2 сохраняет режим и признаки литералов; reader поддерживает v1, неизвестные версии отвергаются. Core/VM арифметика переиспользована; Java/JCEF отображают native DTO. Нового игрового цикла в Java нет.
- Debug gate теперь останавливает на фактической операции, а не повторно на служебных фазах Native Call. Заморозка остальных активаций сохраняется, включая административные фазы. Старая остановка очищается при Step; Go to Node выбирает текущую runtime activation, даже если в авторском Graph выделен первый Native Call.
- Problems закрывается своей вкладкой, сохраняя Debugger/Output, и открывается снова с корректным Content lifetime.
- Smoke-тест замка дожидается native-ответа перед отчётом; JavaScript assertions сообщают причину ошибки. Rate/nonce/revision/security guards сохранены.

## Реально пройденные проверки

- Native Debug build и **CTest4/4 PASS**: EditorCore (включая v1→v2 migration/future rejection), Gameplay, Graph VM, Graph Control (одна логическая breakpoint boundary и полная session freeze). ArchitectureFreezeSelfTest PASS. Батч344.08s.
- **Финальный Release/standalone/JCEF/packaged батч PASS**,299.06s после последних unsigned-mask правок. Штатная offline упаковка, pinned runtime/лицензии, sandbox/CSP/ограничения Bridge, окна/Graph FileEditor lifecycle и shutdown прошли. Новые UI-сценарии: клик Lock i64→Auto, inline1.2500→native1.25→Save→i64, независимое закрытие/повторное открытие Problems. surfaces=0; принадлежащие launcher/CEF-процессы завершились.
- Упакованные GraphTests/RuntimeTests/GraphSceneTests PASS: Auto promotion/demotion/disconnect, строгие locks, fractional literals, rejection без изменения revision, отсутствие потери точности, atomic Undo/Redo, Save/Open другим процессом, stale guards, Damage/Wait/Damage100→93→86, Stop с authoring100. Go to Node проверен через настоящий RuntimeDebugger с намеренно другим авторским выделением.
- IPC v4 adversarial, catalog/source coverage и managed architecture PASS на конечном backend. Полный CTest162 предыдущей итерации не выдаётся за новый полный прогон: текущий CTest адресный4/4.
- Targeted clang-tidy:8 реальных TU с clang-analyzer/bugprone checks. Общий результат **FAIL**:15 унаследованных диагностик (14 выражений подтверждены Git blame,1 в заголовке MSVC). Два новых signed-bitwise замечания исправлены; повтор backend и linear VM завершён, на добавленных/изменённых строках диагностик0. Не добавлялись suppressions и не отключались проверки. Остаток остаётся существующим static backlog.

Первые неуспешные запуски не названы PASS: исправлены ошибки компиляции адаптера, повторный breakpoint и тестовый race с pending Bridge; конечные проверки выполнены после исправлений.

## Статус ручной приёмки

Владелец принял адресные2–4,6–7 и F-II.4:13–15,17–20,22–23,25. Для повторения остаются пустые папки, новый Auto/Lock и16 (Wait/navigation/Problems).21 отложен,24 требует уточнённой проверки Profiler. F-II.5 manual ещё не начат. F-III не начинался.

Реальный FPS уже есть в Profiler — HWND … FPS; новый HUD не вводился. Трёхтиковый сдвиг двух placement сам по себе не ошибка: сравнивается собственная длительность Wait каждой activation. WASD общий, скорость берётся из каждого объекта.

Инструкции15/16/23/24 и продолжение: [FII45_REVIEW_QA](FII45_REVIEW_QA.md). Полный совместный прогон, включая Arena/F-II.5: [FII45_MANUAL_QA](FII45_MANUAL_QA.md).

Конечный EXE:

```powershell
& 'D:\MyProjects\Epidemic engine v2.0\Temp\U\verification-images\31f2a6d5325f4025a84e52185a1c0baa\EpidemicEditor\EpidemicEditor.exe'
```

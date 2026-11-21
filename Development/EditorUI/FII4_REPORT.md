# F-II.4 — Game View / native Runtime / Debugger

Дата начала: 2026-11-16. Отправная точка: F-II.3-GATE PASS `688be74`; первый F-II.4 native батч `695fc4e`. Computer Use и push не использовались.

## Статус

F-II.3-GATE PASS — подтверждён владельцем, включая пункт22. F-II.4 реализован; standalone Release и функциональные автоматические проверки прошли (завершение17 ноября 2026). Статический анализ сохраняет отдельно перечисленные inherited FAIL. **F-II.4-GATE PASS** по последующей ручной приёмке владельца; общий F-II заморожен 21 ноября 2026. Ниже сохранены измерения implementation batch, не новые тесты.

## Результат реализации

- Один C++ PlaySession владеет временем, fixed ticks60Hz, native Scheduler/Graph VM и экземплярами мира. Неблокирующий pipe effect loop продолжает Play без Java запросов. Accumulator/catch-up policy ограничены и описаны в контракте.
- Game показывает реальные Runtime fields/transforms/sprite keys через существующий native Renderer. Native Movement использует переданный клавиатурный/world-pointer input. UI только передаёт команды и рисует immutable projection; отдельного Java игрового цикла нет.
- Play/Pause/Step Tick/Step to Node/Continue/Stop/Restart, active Scene, viewport fit/size и presenter60/120. Restart берёт authoring snapshot; runtime damage/движение не сохраняются обратно.
- Runtime DTO содержит connection/generation/session/tick, полный128-bit EntityId, placement/node, реальные fields/world transforms, resolved references, activation reports/counters/values и bounded trace. Guards отвергают команды старой сессии/соединения.
- Read-only Runtime Inspector, native Graph overlay, cooperative breakpoints/Continue и отдельный ограниченный Step to Node без продвижения GameClock. Wait имеет native deadline; при Pause время заморожено. Stop безопасен при Wait/Breakpoint/Failed и повторном вызове.
- Console/Debugger: bounded history, timestamp/severity/category/source/session/tick, фильтры и Copy; ссылки на Graph/Entity/C#/asset при наличии адресуемой цели. Problems содержит warnings/errors; успешные asset логи остаются в Console.
- Profiler: существующий Diagnostics/Auditor SPI с сохранением предыдущего collector, native submit interval/PSAPI/resource metrics, import/build owners. У метрик source/unit/window; GPU/allocations без provider — Unavailable. Нет нового владельца performance или симуляции.

## Проверки

Выполнены промежуточная и финальная квалификации. При обнаружении ошибок кода и устаревших ожиданий тестов выполнялись исправления и дополнительные повторы затронутых проверок. Поэтому буквальное ограничение владельца двумя прогонами не было соблюдено; это отклонение процесса, а не основание считать результаты непройденных проверок успешными.

Промежуточный PASS: Release/Java build, Debug EditorCore и новый native controls test; production Bridge→C++ Damage/Wait/Damage100/93/86, Pause/Step/Restart/stale session/Stop/fresh process; adversarial IPC и managed architecture. Сохранён исходный EOF contract обрезанного protocol frame.

Перед финальной квалификацией проверена актуальность тестов. Старый smoke ожидал JTextArea для Console/Problems; переведён на JTable с сохранением assertions импорта, отсутствия routine logs в Problems и cancellation. Явные сценарии ScriptLifecycleSmoke сохранили opt-out startup event, чтобы не удваивать события. Проверка2000-node rejection сохраняет native limit1024. Добавлены breakpoint/typed values/input/Failed/stale connection/live-count assertions, load10/100/1000 и1/100 activation, реальные2HWND/100Game dispose cycles.

Фактически завершено: clean full-engine Debug build PASS (984.95s), architecture self-test PASS, public-header self-containment PASS (150.30s). Первый CTest159/161 выявил два устаревших failure-сценария. Demo теперь проверяет Failed→Stop→новый Play от авторских значений; stabilization ставит ограниченную очередь до ошибки, проверяет запрет Tick в Failed, отмену очереди на Stop, сохранение исходной диагностики и retention64. После этих правок полный CTest **161/161 PASS** (380.89s).

Deep analysis выполнен без ослабления:77 translation units /154 checks, итог FAIL. Найдено61 уникальное диагностическое место:60 на неизменённых строках (Scene/RuntimeBridge/старые Editor commands/Renderer и др.) и одно новое narrowing в native frame percentile index. Новое место исправлено явным преобразованием; адресный повтор завершён: оба Graph VM источника PASS (clang-tidy/cppcheck); в PlaySession/backend/viewport6 проверок сохраняют25 диагностик на неизменённых строках, новых на строках F-II.4 —0. Исторические диагностики не объявлены PASS и остаются backlog согласно принятому контракту. Standalone/JCEF и packaged Release roundtrip/load проверки завершились PASS; детали ниже.

## Известные границы профиля

Step to Node — кооперативный переход между native node boundaries в существующем execution budget; это не произвольный breakpoint внутри managed C# и не Step Tick. Native managed calls ограничиваются существующим worker deadline. Runtime polling100ms live/500ms stopped; Game presentation cap60/120 не повышает Runtime60Hz и не гарантирует фактическую FPS. Пресеты Canvas имеют логический размер, physical extent зависит от DPI. Native Graph limit1024 сохранён;2000 узлов квалифицируются как ожидаемый отказ. GPU timing/allocations без provider не выдаются за измеренные значения.

[Контракт](FII4_RUNTIME_CONTRACT.md) · [Пошаговая ручная проверка](FII45_MANUAL_QA.md).


## Финальный Release и функциональные регрессии

Штатный отдельный EXE, официальный JBR/JCEF и native launcher с sandbox собраны offline в отдельный каталог. Full Platform smoke PASS: настоящий Graph FileEditor, native Health93/Wait/Step, активная нода в JCEF, одинаковые session/tick и readonly, crash/reconnect, shutdown без owned launcher/CEF processes. Улучшены устаревшие ожидания smoke: новый браузер ждёт `__ready`; lifecycle сначала сохраняет документ и ждёт фактического закрытия всех FileEditor, не только browser disposal.

Проверки выполнены против **JAR, backend и JBR из готового image**, не из случайных старых classes: RuntimeTests, GraphSceneTests, GraphTests, ScriptTests, RuntimePresentationTests, adversarial IPC v4, catalog/source coverage и managed architecture — PASS. Packaged batch156.14s. Подтверждены UTF-8 boundary, tiny double1e-7 без округления в0, native Failed, Stop live counts0, повтор native session ID в новом процессе и отдельный отказ старого connection даже при подстановке актуальной native generation. Generation native process остаётся уникальной; тест не предполагает её сброс в0.

Native PNG импортирован существующим pipeline и назначен Prefab штатной командой; Game capture проверяет реальные красные GPU pixels. Два настоящих HWND используют одну сессию; Inspector read-only и читает свежую позицию при стабильной модели выбора. Проверены200 последовательных окон: первая сотня surfaces0, Δhandles+2, ΔGDI0; вторая Δhandles−3, Δprivate bytes+5.39MiB. Перед steady-state сравнением разрешён обычный GC/disposer AWT; при каждом отдельном закрытии native surface освобождается сразу и presenter остановлен. Позднее первое открытие Console после нескольких сессий сохраняет их trace и исходный authoring timestamp; Problems не содержит Info.

## Измеренный baseline на этом ПК

Release backend, собственный authoring проект, стандартные Diagnostics providers; значения не являются гарантией для других ПК.

| Runtime load | Play roundtrip,ms | Native tick p50/p95,µs | Backend private,MiB | Stop roundtrip,ms |
|---|---:|---:|---:|---:|
|10Entity /1activation|8.49|9.3 /24.2|2.17|15.24|
|100Entity /100activation|19.03|158.7 /415.9|7.16|16.89|
|1000Entity /100activation|155.85|531.3 /778.8|41.69|19.65|

| Game projection, один HWND | Refresh + native draw p50/p95,ms | UI process private,MiB |
|---|---:|---:|
|10Entity|1.53 /1.90|332.19|
|100Entity|7.80 /9.26|347.38|
|1000Entity|67.98 /75.76|347.92|

Два HWND, presenter caps60/120: фактически32.61 /64.95FPS; frame intervals p50/p95 соответственно30.67/31.63ms и15.40/17.08ms. Это измеренные submit intervals, не обещание60/120FPS и не GPU timestamp. Общий UI refresh p50/p951.05/1.51ms, Profiler refresh0.56/1.69ms. Первоначальный лишний system-wide thread scan занимал около65ms; исправлен без удаления отображаемых счётчиков.

Graph в полном standalone/JCEF:100nodes roundtrip828ms, renderer peak98.34MiB, CDP used heap7.65MiB;500nodes2633ms, peak221.22MiB, CDP used heap19.53MiB. Native Graph workloads100/500/1024nodes36/382/861ms.2000nodes — ожидаемый отказ при сохранённом лимите1024, без мутации. Debug Core numbers не подменяют эти Release измерения.

### Предлагаемые бюджеты для ручного gate

До согласования владельцем это предложения, а не уже принятые требования: native tick p95≤2ms на1000Entity/100activation; Stop roundtrip≤100ms; UI refresh p95≤16ms; Profiler refresh p95≤3ms; Game refresh/draw p95≤4/16/90ms для10/100/1000Entity в текущем диагностическом2Dпрофиле. Для двух HWND native frame interval p95≤40ms на cap60 и≤35ms на cap120. После прогрева в следующей сотне закрытий — surfaces0, Δhandles<40, ΔGDI<20, рост private bytes<64MiB после AWT GC/disposer.

**1000Entity с диагностическими labels сейчас требуют около76ms на кадр. Это явный предел текущего presentation profile, а не60FPS.** Снижение стоимости массовой отрисовки согласовать как отдельную оптимизацию, если такой профиль нужен для ближайшего gameplay. Не достигать FPS отключением обязательных diagnostics.


## Freeze

F-II.4/F-II.5 и общий F-II-GATE PASS. Финальная реализация `55ab4bf` принята владельцем 21 ноября; [итоговый отчёт](FII_FREEZE.md). Измерения выше сохраняются как regression baseline; старые standalone images не являются актуальным artifact.

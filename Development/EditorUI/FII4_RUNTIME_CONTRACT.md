# F-II.4 — native Runtime authority и управление сессией

Отправная точка: `688be74` (F-II.3-GATE PASS), код предыдущего этапа `33e3beb`. Статус реализации CODE COMPLETE; функциональная автоматическая приёмка PASS, ручной F-II.4-GATE ожидается. Полный static PASS не заявляется: inherited diagnostics перечислены отдельно.

Промежуточный прогон: native Release/Java build и Debug EditorCore PASS; EditorCoreTests2.90 s, включая breakpoint/step/Wait. Production Java Bridge → native backend RuntimeTests PASS: автономные часы без запросов Java, Health100/93/86, Pause/Step, Restart/stale session, Stop authoring100 и другой процесс. Adversarial IPC v4 и managed architecture PASS. Найденное различие EOF для обрезанного тела кадра исправлено с сохранением исходного теста. Debugger/Profiler UI и финальная квалификация ещё выполняются; это не F-II.4-GATE PASS.

## Единственный владелец исполнения

Backend обслуживает IPC и Play на одном native-effect потоке. Windows pipe читается без блокирующего ожидания полного следующего запроса; между чтениями C++ pump передаёт монотонный elapsed существующей PlaySession. Java не передаёт elapsed, не вызывает Tick по таймеру и не рассчитывает следующий игровой кадр. UI poll opcode42 читает detached значения; renderer presenter отвечает только за показ последнего полученного кадра.

Профиль Editor Play: существующий Runtime.Time конфигурируется на60 game ticks/sec; один fixed step = GameDuration{1}. Accumulator принадлежит PlaySession, не Java. За один pump разрешено не более8 catch-up steps; входной elapsed ограничен250 ms, оставшийся целый backlog отбрасывается с диагностикой dropped_ticks. Частота запросов UI не изменяет скорость Runtime. Ввод и native graph callbacks применяются тем же владельцем.

## State, шаг и Wait

Stopped → Starting → Running ↔ Paused → Stopping → Stopped. Ошибка запуска/сессии имеет Failed-диагностику; terminal graph failure отдельно виден в trace и не выдаётся за Completed. Stop остаётся безопасным при повторе и отменяет queued/Waiting/paused activations, прежде чем уничтожить Runtime services/managed worker.

Pause сбрасывает real-time backlog и input. Game clock/Wait deadlines не продвигаются. Step Tick разрешён только на паузе и вызывает один native fixed tick. Step to Node использует кооперативный VM gate и существующие instruction/native-call budgets: до следующей границы Node, Wait, завершения или budget yield, без продвижения game clock. Это не StepTick и не обещание одной высокоуровневой операции C#. Недостигнутый Wait deadline требует StepTick/Resume.

Breakpoint наблюдается перед выполнением native VM work item. Продолжение не пересоздаёт GraphInstance и не повторяет уже выполненный native effect. Stop/cancellation проверяется прежде debug gate. Runtime overlay не сохраняется в GraphDefinition.

## Snapshot и данные

IPC envelope остаётся v4; Runtime section version1 и отдельный read-only opcode42. Команды41 используют connection generation и session_id. Старый session_id после Restart отвергается. Snapshot включает session/scene/tick, native fields/world transforms, activation/node/Wait/counters, bounded typed values и trace. Применяются пределы1000 Entity,4096 scalar fields,64 показанных activations/64 values на activation,256 trace events; total counts показываются отдельно.

Game View и Runtime Inspector читают эти значения. Они не берут authoring Health как запасной runtime результат. Authoring-write во время Play остаётся закрытым native guards. Restart создаёт новую сессию из authoring snapshot; Stop не копирует изменения обратно. Ссылки в Runtime не являются сериализованными C++ handles.

В штатном Editor запуске native session один раз посылает события прикреплённых behaviors при старте (payload0). Input defaults копируются в activation binding из authoring metadata только при отсутствии явного behavior input. Программный Play API сохраняет opt-in start_events=false для сценариев с явным Send. Game input использует native fixed_update; renderer и новые окна не создают второй Runtime.

## Наблюдаемость

Tick timings проходят через существующий Diagnostics.ProfileScope/IProfileCollector SPI с bounded consumer, передающим события прежнему collector. Результат не является вторым игровым или performance clock. Метрики содержат source, unit и sampling window; неподключённый GPU/allocation provider отображается как Unavailable. Рабочая память берётся из реального process API, поля и activations — из Runtime services, а не из UI-счётчиков. Историческая диагностика остаётся помеченной session/tick.

## Debugger, input и отображение

DTO runtime section v1 поставляется вместе с согласованными native backend/Java client (IPC envelope остаётся v4). Runtime entity содержит настоящий 128-bit EntityId (High/Low), authoring placement/node и реальные компоненты/WorldTransform. Отдельный индекс в native renderer projection нужен только для нарисованного примитива и не является игровым handle. Java не разрешает references и не хранит mutable GraphDefinition.

Runtime commands проверяют connection+generation в Bridge IO worker и session_id в C++ effect owner. После Restart/project switch старые view commands отвергаются. Ввод сбрасывается при Pause/Stop/focus loss/закрытии Game; keyboard и world pointer доставляются как данные. Native game fixed_update использует существующие Movement/ComponentStore/EntityService; Java не вычисляет движение.

Game использует существующий JNI JAWT Renderer/ScenePreview. У каждого HWND собственные surface/swapchain/preview leases согласно действующему Renderer; cooked resources загружаются существующим preview loader. Surface освобождается в removeNotify; presentation timer останавливается, последняя сессия не сохраняется в canvas. У Runtime одна сессия и отдельные native resources; состояние передаётся только detached DTO. Автоматический тест создаёт два настоящих HWND и100 последующих окон, проверяя возврат surface count и native handles/GDI. Системное масштабирование/фокус глазами остаются ручной приёмкой.

Fit занимает текущую область; пресеты640×360/1280×720/1920×1080 задают логический размер Canvas с прокруткой, не отдельный offscreen framebuffer. Physical extent виден через GetClientRect в Profiler. Настройки60/120 — только presenter cap, не гарантия фактической FPS; Runtime остаётся60Hz. Runtime polling100ms live/500ms stopped; renderer показывает последнюю целостную projection, без Java extrapolation.

Debugger читает реальные activation/report/counter/typed port values; reference values разрешаются native PlayRuntime. Terminal values/history явно относятся к завершённой activation. Node overlay отделён от GraphDefinition. Breakpoints кооперативны; Continue пропускает текущую остановку один раз, Step to Node не продвигает часы. VM instruction/node budgets и cancellation сохранены. Native managed invocation использует существующие bounded worker deadlines, а не прерывание managed кода через UI.

Console/Debugger history ограничена512 display entries, native trace256. Trace фильтруется по Graph, Entity, activation, tick, severity; source links создаются только для известных адресуемых целей. Problems показывает Warning/Error, не Info. C# file links проходят штатный ScriptWorkspace и ограничение Scripts. Stop сохраняет помеченную историю, но live entity/activation counts становятся0.

Profiler использует существующий Diagnostics.ProfileScope/IProfileCollector с передачей прежнему collector, native Renderer/JAWT submit diagnostics, Windows PSAPI, ScenePreview residency, EditorWorkspace import owner и ScriptWorkspace single-build owner. UI лишь отображает значения. Frame p50/p95 — интервалы успешного native submit (last256), а не GPU timestamp; tick p50/p95 — native execution scopes (last256), units us. GPU duration/allocations без подключённого provider — Unavailable. Ни один diagnostic clock не продвигает GameClock. Наблюдаемые p50/p95 и предлагаемые бюджеты фиксируются в итоговом отчёте, ручной gate остаётся PENDING до подтверждения.


## Уточнения финального батча

Connection token создаётся глобально для каждого экземпляра Java Bridge и меняется при закрытии; он не сбрасывается при замене Bridge. При новом backend session ID может повториться, а native process generation остаётся новой. Старый client connection отвергается отдельно, даже если запрос подставляет актуальную native generation/session. Это identity транспорта, не источник игрового времени.

Архив Console хранит только bounded native trace предыдущих4 сессий, без Runtime Entity/VM/World snapshots; display history —512 событий. История собирается и при закрытых Console/Problems, authoring timestamps сохраняются на момент события. Переходы к Source отвергают иной connection/generation и неоднозначные targets. Graph compile reports попадают в Problems без принудительного изменения dock geometry.

Inspector сохраняет модель выбора Entity при неизменных identity и обновляет значения из нового RuntimeSnapshot, не из сохранённого Combo item. Runtime double форматируется shortest round-trip; ограничения UTF-8 строк не разрезают codepoint. Runtime table cells отображают plain text. Mouse drag coalesces в один in-flight + один latest request; focus loss ставит release в очередь раньше ввода другого окна. Ни эта очередь, ни presenter не выполняют игровые шаги.

Profiler не перечисляет все системные потоки на EDT: использует те же surface/process counters без ненужного thread scan. Backend CPU — реальный cumulative GetProcessTimes (ms), а не выдуманный процент. GPU/allocations/per-renderer JCEF RAM без provider — Unavailable; CDP heap измеряется отдельно в standalone smoke. Native draw получает новую projection до кадра; sprites не перекрываются непрозрачной диагностической заливкой. Renderer при teardown освобождает записанные texture commands, отвязывает target и flushes deferred D3D destruction.

## Уточнение отладки20 ноября2026

Flow VM различает логическую границу операции и служебные Execute/Evaluate/Clear фазы. Legacy SetDebugGate вызывается на логической операции; session execution gate дополнительно блокирует служебные фазы невыбранных активаций и всего Runtime в Paused. Breakpoint на Native Call останавливает до побочного эффекта один раз; зависимости не вызывают повторную остановку на Apply. Подтверждённая остановка очищается при Continue/Step, поэтому Waiting snapshot содержит Wait, а не старый Native Call.

Go to Node/Open C# Source используют выбранную runtime activation; авторское выделение имеет приоритет только для Toggle Breakpoint. Runtime breakpoint действует в текущей сессии; авторский флаг Graph Inspector сохраняется в проекте. Ни один callback отладки не создаёт дополнительный clock в Java. Настоящие FPS/frame intervals каждого видимого native viewport уже доступны в Profiler; Present60/120 является настройкой показа.

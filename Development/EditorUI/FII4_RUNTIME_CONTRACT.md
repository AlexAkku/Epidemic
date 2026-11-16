# F-II.4 — native Runtime authority и управление сессией

Отправная точка: `688be74` (F-II.3-GATE PASS), код предыдущего этапа `33e3beb`. Статус реализации IN PROGRESS; этот контракт не объявляет проверку пройденной.

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

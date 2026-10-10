---
aliases: []
type: "guide"
source_snapshot: "2026-10-10"
---

# C++23 по исходникам Epidemic

Цель маршрута: изучить используемые в движке C++23 интерфейсы, ownership и lifecycle. Все приведённые пути относятся к корню репозитория.

## Урок 1. Result и ожидаемая ошибка

Откройте [[11 API/EngineBase/Foundation/result.h]]. Разберите, как выглядит `foundation::Result<T>`, где хранится error и как вызывающий проверяет успешность. Затем изучите [[11 API/EngineBase/Foundation/error.h]]. Упражнение: выписать сценарии, в которых возвращается `Result`, и причины, по которым contract violation не должен маскироваться под recoverable failure.

## Урок 2. Сервисы и lifetime

[[11 API/EngineBase/Core/service_container.h]] и [[11 API/EngineBase/Core/application.h]]. Найдите регистрацию интерфейса, извлечение и управление жизненным циклом. Упражнение: нарисовать граф владения `Application` и сервисов.

## Урок 3. Колбэки и многопоточность

[[11 API/EngineBase/Core/task_scheduler.h]] и [[11 API/EngineBase/Core/main_thread_dispatcher.h]]. Упражнение: объяснить, почему подготовка immutable данных возможна в worker, а коммит authoritative state должен выполняться через владельца и на согласованной фазе.

## Урок 4. Интерфейсы подсистем

[[11 API/EngineRuntime/Resources/resource_services.h]] и [[11 API/EngineRuntime/Support/runtime_support.h]]. Упражнение: найти `ResourceLease`, `EngineRuntimeOptions` и `EngineRuntimeServices`, объяснить RAII/явное освобождение и двухфазную композицию.

## Урок 5. Состояние игрового мира

[[06 EngineFramework/Модуль SaveGame]] и [[06 EngineFramework/Модуль World]]. Упражнение: различить Runtime World и gameplay World, а также authoritative и derived state.

При чтении кода ориентируйтесь на реальные сигнатуры в приложенных header-снимках. Это учебный маршрут, а не набор компилируемых примеров.

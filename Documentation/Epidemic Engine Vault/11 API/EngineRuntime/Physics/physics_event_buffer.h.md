---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h"
---

# Physics: physics_event_buffer.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h`.

**Модуль:** [[05 EngineRuntime/Модуль Physics|Physics]].
**Размер исходного файла:** 478 байт.
**Подключение:** `#include <Epidemic/Runtime/Physics/physics_event_buffer.h>`.

## Объявления типов (лексический индекс)
`IPhysicsEventBuffer`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Physics/physics_types.h"

#include <span>

namespace epidemic::runtime::physics
{
// Event boundary contract for publishing contacts out of Physics without mutating World directly.
class IPhysicsEventBuffer
{
  public:
    virtual ~IPhysicsEventBuffer() = default;

    [[nodiscard]] virtual std::span<const ContactEvent> Contacts() const = 0;
    virtual void Clear() = 0;
};
} // namespace epidemic::runtime::physics
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Physics/physics_types.h`.

### Состав типов и операции

#### `IPhysicsEventBuffer` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsEventBuffer() = default;
[[nodiscard]] virtual std::span<const ContactEvent> Contacts() const = 0;
virtual void Clear() = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Contacts` | `PhysicsRuntime::Contacts` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1007` |
| `Clear` | `PhysicsRuntime::Clear` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1012` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Contacts`: `EngineRuntime/Physics/tests/physics_tests.cpp:313`
- `Clear`: `EngineRuntime/Physics/tests/physics_tests.cpp:452`


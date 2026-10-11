---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h"
---

# Physics: physics_query.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h`.

**Модуль:** [[05 EngineRuntime/Модуль Physics|Physics]].
**Размер исходного файла:** 563 байт.
**Подключение:** `#include <Epidemic/Runtime/Physics/physics_query.h>`.

## Объявления типов (лексический индекс)
`IPhysicsQuery`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Physics/physics_types.h"

namespace epidemic::runtime::physics
{
// Public query contract for deterministic raycast and overlap requests.
class IPhysicsQuery
{
  public:
    virtual ~IPhysicsQuery() = default;

    [[nodiscard]] virtual foundation::Result<RaycastHit> Raycast(const RaycastQuery& query) const = 0;
    [[nodiscard]] virtual foundation::Result<OverlapResult> Overlap(const OverlapQuery& query) const = 0;
};
} // namespace epidemic::runtime::physics
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Physics/physics_types.h`.

### Состав типов и операции

#### `IPhysicsQuery` (строка заголовка 9)

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsQuery() = default;
[[nodiscard]] virtual foundation::Result<RaycastHit> Raycast(const RaycastQuery& query) const = 0;
[[nodiscard]] virtual foundation::Result<OverlapResult> Overlap(const OverlapQuery& query) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Raycast` | `PhysicsRuntime::Raycast` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:874` |
| `Overlap` | `PhysicsRuntime::Overlap` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:986` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Raycast`: `EngineRuntime/Physics/tests/physics_tests.cpp:465`
- `Overlap`: `EngineRuntime/Physics/tests/physics_tests.cpp:1230`


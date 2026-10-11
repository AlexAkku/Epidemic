---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h"
---

# Persistence: tombstone_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 840 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/tombstone_store.h>`.

## Объявления типов (лексический индекс)
`TombstoneRecord`, `ITombstoneStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace epidemic::runtime
{
struct TombstoneRecord
{
    PersistentObjectId persistent_id{};
    GameTimePoint deleted_game_time{};
    foundation::StringId reason{};
    std::uint64_t revision = 0;
};

class ITombstoneStore
{
  public:
    virtual ~ITombstoneStore() = default;

    [[nodiscard]] virtual std::optional<TombstoneRecord> FindTombstone(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual bool IsTombstoned(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<TombstoneRecord> ListTombstones() const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/runtime_time.h`.

### Состав типов и операции

#### `TombstoneRecord` (строка заголовка 13)

Публичные поля и значения по умолчанию:

- `std::uint64_t revision = 0`

#### `ITombstoneStore` (строка заголовка 21)

Публичные сигнатуры операций:

```cpp
virtual ~ITombstoneStore() = default;
[[nodiscard]] virtual std::optional<TombstoneRecord> FindTombstone(PersistentObjectId id) const = 0;
[[nodiscard]] virtual bool IsTombstoned(PersistentObjectId id) const = 0;
[[nodiscard]] virtual std::vector<TombstoneRecord> ListTombstones() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindTombstone` | `InMemoryPersistenceStore::FindTombstone` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:587` |
| `IsTombstoned` | `InMemoryPersistenceStore::IsTombstoned` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:597` |
| `ListTombstones` | `InMemoryPersistenceStore::ListTombstones` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:602` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `FindTombstone`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:425`
- `IsTombstoned`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:427`


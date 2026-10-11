---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/zone_override_store.h"
---

# Persistence: zone_override_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/zone_override_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 946 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/zone_override_store.h>`.

## Объявления типов (лексический индекс)
`ZoneOverrideSnapshot`, `IZoneOverrideStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Persistence/lazy_rule_record.h"
#include "Epidemic/Runtime/Persistence/persistence_location.h"
#include "Epidemic/Runtime/Persistence/tombstone_store.h"

#include <cstdint>
#include <optional>
#include <vector>

namespace epidemic::runtime
{
struct ZoneOverrideSnapshot
{
    PersistenceLocation location{};
    std::vector<PersistentObjectId> record_ids;
    std::vector<TombstoneRecord> tombstones;
    std::vector<LazyRuleRecord> lazy_rules;
    std::uint64_t revision = 0;
};

class IZoneOverrideStore
{
  public:
    virtual ~IZoneOverrideStore() = default;

    [[nodiscard]] virtual std::optional<ZoneOverrideSnapshot> FindZoneOverride(const PersistenceLocation& location) const = 0;
    [[nodiscard]] virtual std::vector<ZoneOverrideSnapshot> ListZoneOverrides() const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Persistence/lazy_rule_record.h`, `Epidemic/Runtime/Persistence/persistence_location.h`, `Epidemic/Runtime/Persistence/tombstone_store.h`.

### Состав типов и операции

#### `ZoneOverrideSnapshot` (строка заголовка 14)

Публичные поля и значения по умолчанию:

- `std::vector<PersistentObjectId> record_ids`
- `std::vector<TombstoneRecord> tombstones`
- `std::vector<LazyRuleRecord> lazy_rules`
- `std::uint64_t revision = 0`

#### `IZoneOverrideStore` (строка заголовка 23)

Публичные сигнатуры операций:

```cpp
virtual ~IZoneOverrideStore() = default;
[[nodiscard]] virtual std::optional<ZoneOverrideSnapshot> FindZoneOverride(const PersistenceLocation& location) const = 0;
[[nodiscard]] virtual std::vector<ZoneOverrideSnapshot> ListZoneOverrides() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindZoneOverride` | `InMemoryPersistenceStore::FindZoneOverride` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:615` |
| `ListZoneOverrides` | `InMemoryPersistenceStore::ListZoneOverrides` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:625` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `FindZoneOverride`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:426`
- `ListZoneOverrides`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:931`


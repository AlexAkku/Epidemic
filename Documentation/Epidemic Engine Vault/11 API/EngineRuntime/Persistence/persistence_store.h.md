---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h"
---

# Persistence: persistence_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 1555 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_store.h>`.

## Объявления типов (лексический индекс)
`IPersistenceQuery`, `IPersistenceStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/dirty_tracker.h"
#include "Epidemic/Runtime/Persistence/lazy_rule_record.h"
#include "Epidemic/Runtime/Persistence/persistent_object_store.h"
#include "Epidemic/Runtime/Persistence/save_transaction.h"
#include "Epidemic/Runtime/Persistence/tombstone_store.h"
#include "Epidemic/Runtime/Persistence/zone_override_store.h"

#include <cstdint>
#include <memory>
#include <optional>
#include <vector>

namespace epidemic::runtime
{
class IPersistenceQuery : public IPersistentObjectStore, public IDirtyTracker, public ITombstoneStore, public IZoneOverrideStore
{
  public:
    virtual ~IPersistenceQuery() = default;

    [[nodiscard]] virtual std::optional<LazyRuleRecord> FindLazyRule(LazyRuleId id) const = 0;
    [[nodiscard]] virtual std::vector<LazyRuleRecord> FindLazyRules(PersistentObjectId target_id) const = 0;
    [[nodiscard]] virtual std::vector<LazyRuleRecord> QueryDueLazyRules(GameTimePoint now) const = 0;
    [[nodiscard]] virtual std::vector<LazyRuleRecord> ListLazyRules() const = 0;
    [[nodiscard]] virtual PersistenceRevision GetRevision() const = 0;
};

class IPersistenceStore : public IPersistenceQuery
{
  public:
    virtual ~IPersistenceStore() = default;

    [[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction() = 0;
    [[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction(PersistenceRevision base_revision) = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Persistence/dirty_tracker.h`, `Epidemic/Runtime/Persistence/lazy_rule_record.h`, `Epidemic/Runtime/Persistence/persistent_object_store.h`, `Epidemic/Runtime/Persistence/save_transaction.h`, `Epidemic/Runtime/Persistence/tombstone_store.h`, `Epidemic/Runtime/Persistence/zone_override_store.h`.

### Состав типов и операции

#### `IPersistenceQuery` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
virtual ~IPersistenceQuery() = default;
[[nodiscard]] virtual std::optional<LazyRuleRecord> FindLazyRule(LazyRuleId id) const = 0;
[[nodiscard]] virtual std::vector<LazyRuleRecord> FindLazyRules(PersistentObjectId target_id) const = 0;
[[nodiscard]] virtual std::vector<LazyRuleRecord> QueryDueLazyRules(GameTimePoint now) const = 0;
[[nodiscard]] virtual std::vector<LazyRuleRecord> ListLazyRules() const = 0;
[[nodiscard]] virtual PersistenceRevision GetRevision() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPersistenceStore` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
virtual ~IPersistenceStore() = default;
[[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction() = 0;
[[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction(PersistenceRevision base_revision) = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindLazyRule` | `InMemoryPersistenceStore::FindLazyRule` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:648` |
| `FindLazyRules` | `InMemoryPersistenceStore::FindLazyRules` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:658` |
| `QueryDueLazyRules` | `InMemoryPersistenceStore::QueryDueLazyRules` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:673` |
| `ListLazyRules` | `InMemoryPersistenceStore::ListLazyRules` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:694` |
| `GetRevision` | `InMemoryPersistenceStore::GetRevision` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:707` |
| `OpenTransaction` | `InMemoryPersistenceStore::OpenTransaction` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:712` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `OpenTransaction`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:118`
- `GetRevision`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:302`
- `FindLazyRule`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:368`
- `FindLazyRules`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:369`
- `QueryDueLazyRules`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:388`
- `ListLazyRules`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:535`


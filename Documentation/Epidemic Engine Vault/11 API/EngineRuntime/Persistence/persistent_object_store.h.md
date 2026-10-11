---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h"
---

# Persistence: persistent_object_store.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 719 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistent_object_store.h>`.

## Объявления типов (лексический индекс)
`IPersistentObjectStore`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Persistence/persistence_location.h"
#include "Epidemic/Runtime/Persistence/persistent_record.h"

#include <optional>
#include <vector>

namespace epidemic::runtime
{
class IPersistentObjectStore
{
  public:
    virtual ~IPersistentObjectStore() = default;

    [[nodiscard]] virtual std::optional<PersistentObjectRecord> FindObject(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<PersistentObjectRecord> FindByLocation(const PersistenceLocation& location) const = 0;
    [[nodiscard]] virtual std::vector<PersistentObjectRecord> ListObjects() const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Persistence/persistence_location.h`, `Epidemic/Runtime/Persistence/persistent_record.h`.

### Состав типов и операции

#### `IPersistentObjectStore` (строка заголовка 12)

Публичные сигнатуры операций:

```cpp
virtual ~IPersistentObjectStore() = default;
[[nodiscard]] virtual std::optional<PersistentObjectRecord> FindObject(PersistentObjectId id) const = 0;
[[nodiscard]] virtual std::vector<PersistentObjectRecord> FindByLocation(const PersistenceLocation& location) const = 0;
[[nodiscard]] virtual std::vector<PersistentObjectRecord> ListObjects() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindObject` | `InMemoryPersistenceStore::FindObject` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:537` |
| `FindByLocation` | `InMemoryPersistenceStore::FindByLocation` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:547` |
| `ListObjects` | `InMemoryPersistenceStore::ListObjects` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:562` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `FindObject`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:299`
- `ListObjects`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:317`


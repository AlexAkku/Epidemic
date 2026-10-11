---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h"
---

# Persistence: dirty_tracker.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 403 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/dirty_tracker.h>`.

## Объявления типов (лексический индекс)
`IDirtyTracker`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"

#include <vector>

namespace epidemic::runtime
{
class IDirtyTracker
{
  public:
    virtual ~IDirtyTracker() = default;

    [[nodiscard]] virtual bool IsDirty(PersistentObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<PersistentObjectId> CollectDirty() const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Состав типов и операции

#### `IDirtyTracker` (строка заголовка 9)

Публичные сигнатуры операций:

```cpp
virtual ~IDirtyTracker() = default;
[[nodiscard]] virtual bool IsDirty(PersistentObjectId id) const = 0;
[[nodiscard]] virtual std::vector<PersistentObjectId> CollectDirty() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `IsDirty` | `InMemoryPersistenceStore::IsDirty` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:575` |
| `CollectDirty` | `InMemoryPersistenceStore::CollectDirty` | `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:580` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `CollectDirty`: `EngineRuntime/Persistence/tests/persistence_tests.cpp:300`


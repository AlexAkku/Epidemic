---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/world_query.h"
---

# World: world_query.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/world_query.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 649 байт.
**Подключение:** `#include <Epidemic/Runtime/World/world_query.h>`.

## Объявления типов (лексический индекс)
`IWorldQuery`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/World/world_object.h"
#include <optional>
#include <vector>

namespace epidemic::runtime
{
class IWorldQuery
{
  public:
    virtual ~IWorldQuery() = default;
    [[nodiscard]] virtual std::optional<WorldObjectRecord> FindObject(RuntimeObjectId id) const = 0;
    [[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInRegion(RegionId region) const = 0;
    [[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInChunk(ChunkId chunk) const = 0;
    [[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsByReality(ObjectRealityLevel reality) const = 0;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/World/world_object.h`.

### Состав типов и операции

#### `IWorldQuery` (строка заголовка 9)

Публичные сигнатуры операций:

```cpp
virtual ~IWorldQuery() = default;
[[nodiscard]] virtual std::optional<WorldObjectRecord> FindObject(RuntimeObjectId id) const = 0;
[[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInRegion(RegionId region) const = 0;
[[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsInChunk(ChunkId chunk) const = 0;
[[nodiscard]] virtual std::vector<WorldObjectRecord> FindObjectsByReality(ObjectRealityLevel reality) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FindObject` | `WorldRuntime::FindObject` | `EngineRuntime/World/src/world_runtime_impl.cpp:773` |
| `FindObjectsInRegion` | `WorldRuntime::FindObjectsInRegion` | `EngineRuntime/World/src/world_runtime_impl.cpp:784` |
| `FindObjectsInChunk` | `WorldRuntime::FindObjectsInChunk` | `EngineRuntime/World/src/world_runtime_impl.cpp:801` |
| `FindObjectsByReality` | `WorldRuntime::FindObjectsByReality` | `EngineRuntime/World/src/world_runtime_impl.cpp:818` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `FindObject`: `EngineRuntime/World/tests/world_tests.cpp:202`
- `FindObjectsInChunk`: `EngineRuntime/World/tests/world_tests.cpp:591`
- `FindObjectsInRegion`: `EngineRuntime/World/tests/world_tests.cpp:642`
- `FindObjectsByReality`: `EngineRuntime/World/tests/world_tests.cpp:662`


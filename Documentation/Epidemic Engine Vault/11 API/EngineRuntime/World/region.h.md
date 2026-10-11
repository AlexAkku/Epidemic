---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/region.h"
---

# World: region.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/region.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 778 байт.
**Подключение:** `#include <Epidemic/Runtime/World/region.h>`.

## Объявления типов (лексический индекс)
`RegionDescriptor`, `IRegionRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include <optional>

namespace epidemic::runtime
{
struct RegionDescriptor
{
    RegionId id{};
    foundation::StringId name{};

    [[nodiscard]] constexpr bool operator==(const RegionDescriptor&) const noexcept = default;
};

class IRegionRegistry
{
  public:
    virtual ~IRegionRegistry() = default;
    [[nodiscard]] virtual foundation::Result<void> RegisterRegion(RegionDescriptor region) = 0;
    [[nodiscard]] virtual std::optional<RegionDescriptor> FindRegion(RegionId id) const = 0;
    virtual void Freeze() noexcept = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Состав типов и операции

#### `RegionDescriptor` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const RegionDescriptor&) const noexcept = default;
```

#### `IRegionRegistry` (строка заголовка 18)

Публичные сигнатуры операций:

```cpp
virtual ~IRegionRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterRegion(RegionDescriptor region) = 0;
[[nodiscard]] virtual std::optional<RegionDescriptor> FindRegion(RegionId id) const = 0;
virtual void Freeze() noexcept = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterRegion` | `WorldRuntime::RegisterRegion` | `EngineRuntime/World/src/world_runtime_impl.cpp:117` |
| `Freeze` | `WorldRuntime::Freeze` | `EngineRuntime/World/src/world_runtime_impl.cpp:139` |
| `IsFrozen` | `WorldRuntime::IsFrozen` | `EngineRuntime/World/src/world_runtime_impl.cpp:144` |
| `FindRegion` | `WorldRuntime::FindRegion` | `EngineRuntime/World/src/world_runtime_impl.cpp:149` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterRegion`: `EngineRuntime/World/tests/world_tests.cpp:87`
- `FindRegion`: `EngineRuntime/World/tests/world_tests.cpp:97`
- `Freeze`: `EngineRuntime/World/tests/world_tests.cpp:887`
- `IsFrozen`: `EngineRuntime/World/tests/world_tests.cpp:892`


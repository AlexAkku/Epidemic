---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_handle.h"
---

# Resources: resource_handle.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_handle.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 816 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_handle.h>`.

## Объявления типов (лексический индекс)
`ResourceHandle`, `ResourceLease`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include <cstdint>

namespace epidemic::runtime
{
struct ResourceHandle
{
    ResourceId id{};
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return id.IsValid() && generation != 0;
    }

    [[nodiscard]] constexpr bool operator==(const ResourceHandle&) const noexcept = default;
};

using ResourceAcquisitionId = std::uint64_t;

struct ResourceLease
{
    ResourceHandle resource{};
    ResourceAcquisitionId acquisition = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return resource.IsValid() && acquisition != 0;
    }

    [[nodiscard]] constexpr bool operator==(const ResourceLease&) const noexcept = default;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`.

### Состав типов и операции

#### `ResourceHandle` (строка заголовка 8)

Публичные поля и значения по умолчанию:

- `std::uint32_t generation = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceHandle&) const noexcept = default;
```

#### `ResourceLease` (строка заголовка 23)

Публичные поля и значения по умолчанию:

- `ResourceAcquisitionId acquisition = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const ResourceLease&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


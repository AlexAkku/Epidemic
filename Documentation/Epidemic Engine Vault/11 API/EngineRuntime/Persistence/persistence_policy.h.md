---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_policy.h"
---

# Persistence: persistence_policy.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_policy.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 1044 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_policy.h>`.

## Объявления типов (лексический индекс)
`ObjectProtectionFlags`, `ObjectProtectionMask`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>

namespace epidemic::runtime
{
enum class ObjectProtectionFlags : std::uint32_t
{
    None = 0,
    PreventTheft = 1u << 0u,
    PreventDecay = 1u << 1u,
    PreventCleanup = 1u << 2u,
    PreserveTransform = 1u << 3u,
    PreserveCondition = 1u << 4u,
};

struct ObjectProtectionMask
{
    std::uint32_t value = 0;

    [[nodiscard]] constexpr bool operator==(const ObjectProtectionMask&) const noexcept = default;
};

[[nodiscard]] constexpr std::uint32_t ToProtectionMask(ObjectProtectionFlags flags) noexcept
{
    return static_cast<std::uint32_t>(flags);
}

[[nodiscard]] constexpr bool HasProtectionFlag(ObjectProtectionMask mask, ObjectProtectionFlags flag) noexcept
{
    return (mask.value & ToProtectionMask(flag)) != 0u;
}

[[nodiscard]] constexpr ObjectProtectionMask AddProtectionFlag(ObjectProtectionMask mask, ObjectProtectionFlags flag) noexcept
{
    return ObjectProtectionMask{mask.value | ToProtectionMask(flag)};
}
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `ObjectProtectionFlags`: `None`, `PreventTheft`, `PreventDecay`, `PreventCleanup`, `PreserveTransform`, `PreserveCondition`

### Состав типов и операции

#### `ObjectProtectionMask` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::uint32_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const ObjectProtectionMask&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


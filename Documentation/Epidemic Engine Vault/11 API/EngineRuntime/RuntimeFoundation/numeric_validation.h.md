---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/numeric_validation.h"
---

# RuntimeFoundation: numeric_validation.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/numeric_validation.h`.

**Модуль:** [[05 EngineRuntime/Модуль RuntimeFoundation|RuntimeFoundation]].
**Размер исходного файла:** 1835 байт.
**Подключение:** `#include <Epidemic/Runtime/Foundation/numeric_validation.h>`.

## Объявления типов (лексический индекс)
_Именованные `class`/`struct`/`enum` в этом заголовке не обнаружены простой лексической проверкой._

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cmath>
#include <type_traits>
#include <limits>
#include <optional>
#include <cstdint>

namespace epidemic::runtime
{
template <typename TValue>
[[nodiscard]] bool IsFinite(TValue value) noexcept
{
    static_assert(std::is_floating_point_v<TValue>);
    return std::isfinite(value);
}

template <typename TValue>
[[nodiscard]] bool IsFinitePositive(TValue value) noexcept
{
    return IsFinite(value) && value > static_cast<TValue>(0);
}

template <typename TValue>
[[nodiscard]] bool IsFiniteNonNegative(TValue value) noexcept
{
    return IsFinite(value) && value >= static_cast<TValue>(0);
}

template <typename TUnsigned>
[[nodiscard]] constexpr std::optional<TUnsigned> CheckedUnsignedIncrement(TUnsigned value) noexcept
{
    static_assert(std::is_integral_v<TUnsigned> && std::is_unsigned_v<TUnsigned>);
    if (value == std::numeric_limits<TUnsigned>::max())
    {
        return std::nullopt;
    }
    return static_cast<TUnsigned>(value + 1);
}

template <typename TUnsigned>
[[nodiscard]] constexpr std::optional<TUnsigned> CheckedUnsignedAdd(TUnsigned left, TUnsigned right) noexcept
{
    static_assert(std::is_integral_v<TUnsigned> && std::is_unsigned_v<TUnsigned>);
    if (right > std::numeric_limits<TUnsigned>::max() - left)
    {
        return std::nullopt;
    }
    return static_cast<TUnsigned>(left + right);
}

template <typename TUnsigned>
[[nodiscard]] constexpr std::optional<TUnsigned> CheckedRevisionIncrement(TUnsigned revision) noexcept
{
    return CheckedUnsignedIncrement(revision);
}

template <typename TUnsigned>
[[nodiscard]] constexpr std::optional<TUnsigned> CheckedGenerationIncrement(TUnsigned generation) noexcept
{
    return CheckedUnsignedIncrement(generation);
}

} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

Этот заголовок преимущественно содержит free-functions, type aliases или implementation helpers. Точные сигнатуры смотрите в исходном блоке выше.

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.


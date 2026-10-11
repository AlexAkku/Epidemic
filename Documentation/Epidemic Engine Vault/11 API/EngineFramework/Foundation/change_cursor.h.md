---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/change_cursor.h"
---

# Foundation: change_cursor.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/change_cursor.h`.

**Модуль:** [[06 EngineFramework/Модуль Foundation|Foundation]].
**Размер исходного файла:** 883 байт.
**Подключение:** `#include <Epidemic/GameFramework/Foundation/change_cursor.h>`.

## Объявления типов (лексический индекс)
`ChangeCursor`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <compare>
#include <cstdint>
#include <limits>
#include <optional>

namespace epidemic::gameplay
{
struct ChangeCursor
{
    std::uint64_t epoch = 0;
    std::uint64_t sequence = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return epoch != 0; }
    [[nodiscard]] constexpr ChangeCursor AtSequence(std::uint64_t value) const noexcept { return {epoch, value}; }
    [[nodiscard]] constexpr bool operator==(const ChangeCursor&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const ChangeCursor&) const noexcept = default;
};

[[nodiscard]] constexpr std::optional<std::uint64_t> CheckedNextChangeEpoch(std::uint64_t epoch) noexcept
{
    if (epoch == std::numeric_limits<std::uint64_t>::max())
    {
        return std::nullopt;
    }
    return epoch + 1;
}
} // namespace epidemic::gameplay
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `ChangeCursor` (строка заголовка 10)

Публичные поля и значения по умолчанию:

- `std::uint64_t epoch = 0`
- `std::uint64_t sequence = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr ChangeCursor AtSequence(std::uint64_t value) const noexcept;
[[nodiscard]] constexpr bool operator==(const ChangeCursor&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const ChangeCursor&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.


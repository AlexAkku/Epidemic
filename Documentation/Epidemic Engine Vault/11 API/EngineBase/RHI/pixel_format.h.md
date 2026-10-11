---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI/include/Epidemic/RHI/pixel_format.h"
---

# RHI: pixel_format.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI/include/Epidemic/RHI/pixel_format.h`.

**Модуль:** [[04 EngineBase/Модуль RHI|RHI]].
**Размер исходного файла:** 837 байт.
**Подключение:** `#include <Epidemic/RHI/pixel_format.h>`.

## Объявления типов (лексический индекс)
`RhiPixelFormat`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstdint>
#include <string_view>

namespace epidemic::rhi
{
// This file defines the minimal pixel formats understood by the EngineBase RHI baseline.
// The enum is intentionally tiny for the current clear-screen and swap-chain scenarios.

enum class RhiPixelFormat : std::uint8_t
{
    Unknown,
    R8G8B8A8_UNorm,
    B8G8R8A8_UNorm,
};

// Converts a pixel format to a stable diagnostic name.
[[nodiscard]] inline std::string_view ToString(RhiPixelFormat pixel_format) noexcept
{
    switch (pixel_format)
    {
    case RhiPixelFormat::Unknown:
        return "Unknown";
    case RhiPixelFormat::R8G8B8A8_UNorm:
        return "R8G8B8A8_UNorm";
    case RhiPixelFormat::B8G8R8A8_UNorm:
        return "B8G8R8A8_UNorm";
    }

    return "Unknown";
}
} // namespace epidemic::rhi
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `RhiPixelFormat`: `Unknown`, `R8G8B8A8_UNorm`, `B8G8R8A8_UNorm`

### Состав типов и операции

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.


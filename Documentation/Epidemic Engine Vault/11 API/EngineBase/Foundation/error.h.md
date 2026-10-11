---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Foundation/include/Epidemic/Foundation/error.h"
---

# Foundation: error.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Foundation/include/Epidemic/Foundation/error.h`.

**Модуль:** [[04 EngineBase/Модуль Foundation|Foundation]].
**Размер исходного файла:** 1862 байт.
**Подключение:** `#include <Epidemic/Foundation/error.h>`.

## Объявления типов (лексический индекс)
`Error`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <string>
#include <string_view>

namespace epidemic::foundation
{
// This file defines the smallest shared error payload used across EngineBase.
// Error is intentionally a plain value type so functions can return it through Result<T>
// without coupling callers to exceptions or subsystem-specific status objects.

// Describes an expected runtime failure in EngineBase.
// Fields:
// - code: stable machine-readable identifier for branching and diagnostics
// - message: human-readable description suitable for logs or UI
// - context: optional extra details such as subsystem or operation name
struct Error
{
    std::string code;
    std::string message;
    std::string context;

    // Builds an Error from borrowed string-like inputs.
    // Input: code/message/context views.
    // Output: a fully owned Error instance.
    // Relationship: this is the main convenience constructor used by Result-producing code.
    [[nodiscard]] static Error Create(std::string_view error_code, std::string_view error_message, std::string_view error_context = {})
    {
        return Error{std::string(error_code), std::string(error_message), std::string(error_context)};
    }

    // Checks whether the stored code matches the requested code exactly.
    // Input: expected code string.
    // Output: true when the codes are equal.
    [[nodiscard]] bool HasCode(std::string_view expected_code) const noexcept
    {
        return code == expected_code;
    }

    // Reports whether the error contains a non-empty message.
    [[nodiscard]] bool HasMessage() const noexcept
    {
        return !message.empty();
    }

    // Reports whether the error contains extra contextual details.
    [[nodiscard]] bool HasContext() const noexcept
    {
        return !context.empty();
    }
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `Error` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::string code`
- `std::string message`
- `std::string context`

Публичные сигнатуры операций:

```cpp
// Checks whether the stored code matches the requested code exactly. // Input: expected code string. // Output: true when the codes are equal. [[nodiscard]] bool HasCode(std::string_view expected_code) const noexcept;
// Reports whether the error contains a non-empty message. [[nodiscard]] bool HasMessage() const noexcept;
// Reports whether the error contains extra contextual details. [[nodiscard]] bool HasContext() const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.


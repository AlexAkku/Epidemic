---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/schema_version.h"
---

# Serialization: schema_version.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/schema_version.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 479 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/schema_version.h>`.

## Объявления типов (лексический индекс)
`SchemaVersion`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <compare>
#include <cstdint>

// Header for runtime contracts or module-local helpers. Comments document how each
// function participates in the module API and what state it observes or mutates.

namespace epidemic::runtime
{
struct SchemaVersion
{
    std::uint32_t major = 0;
    std::uint32_t minor = 0;
    std::uint32_t patch = 0;

    [[nodiscard]] constexpr auto operator<=>(const SchemaVersion&) const noexcept = default;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `SchemaVersion` (строка заголовка 11)

Публичные поля и значения по умолчанию:

- `std::uint32_t major = 0`
- `std::uint32_t minor = 0`
- `std::uint32_t patch = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr auto operator<=>(const SchemaVersion&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


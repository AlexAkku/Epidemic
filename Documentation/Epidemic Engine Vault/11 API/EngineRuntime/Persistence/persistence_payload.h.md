---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_payload.h"
---

# Persistence: persistence_payload.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_payload.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 482 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/persistence_payload.h>`.

## Объявления типов (лексический индекс)
`PersistencePayload`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace epidemic::runtime
{
struct PersistencePayload
{
    foundation::StringId schema_id{};
    std::uint32_t schema_version = 0;
    std::vector<std::byte> bytes;

    [[nodiscard]] bool IsValid() const noexcept
    {
        return schema_id.IsValid() && schema_version != 0 && !bytes.empty();
    }
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`.

### Состав типов и операции

#### `PersistencePayload` (строка заголовка 11)

Публичные поля и значения по умолчанию:

- `std::uint32_t schema_version = 0`
- `std::vector<std::byte> bytes`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] bool IsValid() const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


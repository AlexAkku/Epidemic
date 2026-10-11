---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_type.h"
---

# Assets: asset_type.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Assets/include/Epidemic/Runtime/Assets/asset_type.h`.

**Модуль:** [[05 EngineRuntime/Модуль Assets|Assets]].
**Размер исходного файла:** 527 байт.
**Подключение:** `#include <Epidemic/Runtime/Assets/asset_type.h>`.

## Объявления типов (лексический индекс)
`AssetType`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"

// Header for runtime contracts or module-local helpers. Comments document how each
// function participates in the module API and what state it observes or mutates.

namespace epidemic::runtime
{
struct AssetType
{
    foundation::StringId value{};

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return value.IsValid();
    }

    [[nodiscard]] constexpr bool operator==(const AssetType&) const noexcept = default;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`.

### Состав типов и операции

#### `AssetType` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AssetType&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Environment/include/Epidemic/Runtime/Environment/season_state.h"
---

# Environment: season_state.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Environment/include/Epidemic/Runtime/Environment/season_state.h`.

**Модуль:** [[05 EngineRuntime/Модуль Environment|Environment]].
**Размер исходного файла:** 380 байт.
**Подключение:** `#include <Epidemic/Runtime/Environment/season_state.h>`.

## Объявления типов (лексический индекс)
`SeasonKind`, `SeasonState`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::runtime
{
enum class SeasonKind
{
    Spring,
    Summer,
    Autumn,
    Winter,
    Transitioning,
};

struct SeasonState
{
    SeasonKind kind = SeasonKind::Spring;
    float progress = 0.0f;

    [[nodiscard]] constexpr bool operator==(const SeasonState&) const noexcept = default;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `SeasonKind`: `Spring`, `Summer`, `Autumn`, `Winter`, `Transitioning`

### Состав типов и операции

#### `SeasonState` (строка заголовка 14)

Публичные поля и значения по умолчанию:

- `SeasonKind kind = SeasonKind::Spring`
- `float progress = 0.0f`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const SeasonState&) const noexcept = default;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


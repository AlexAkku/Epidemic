---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/lazy_rule_record.h"
---

# Persistence: lazy_rule_record.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/lazy_rule_record.h`.

**Модуль:** [[05 EngineRuntime/Модуль Persistence|Persistence]].
**Размер исходного файла:** 744 байт.
**Подключение:** `#include <Epidemic/Runtime/Persistence/lazy_rule_record.h>`.

## Объявления типов (лексический индекс)
`LazyRuleKind`, `LazyRuleState`, `LazyRuleRecord`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"

#include <cstdint>

namespace epidemic::runtime
{
enum class LazyRuleKind
{
    Decay,
    Theft,
    Cleanup,
};

enum class LazyRuleState
{
    Pending,
    Evaluated,
    Applied,
    Cancelled,
    Expired,
};

struct LazyRuleRecord
{
    LazyRuleId rule_id{};
    PersistentObjectId target_id{};
    LazyRuleKind kind = LazyRuleKind::Decay;
    LazyRuleState state = LazyRuleState::Pending;
    GameTimePoint created_game_time{};
    GameTimePoint evaluate_after_game_time{};
    std::uint64_t rule_seed = 0;
    std::uint64_t revision = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/runtime_time.h`.

### Перечисления и допустимые категории

- `LazyRuleKind`: `Decay`, `Theft`, `Cleanup`
- `LazyRuleState`: `Pending`, `Evaluated`, `Applied`, `Cancelled`, `Expired`

### Состав типов и операции

#### `LazyRuleRecord` (строка заголовка 26)

Публичные поля и значения по умолчанию:

- `LazyRuleKind kind = LazyRuleKind::Decay`
- `LazyRuleState state = LazyRuleState::Pending`
- `std::uint64_t rule_seed = 0`
- `std::uint64_t revision = 0`

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


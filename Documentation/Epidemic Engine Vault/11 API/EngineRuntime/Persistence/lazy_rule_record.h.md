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

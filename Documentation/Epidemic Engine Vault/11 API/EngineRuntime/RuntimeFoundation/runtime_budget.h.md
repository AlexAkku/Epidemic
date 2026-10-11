---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_budget.h"
---

# RuntimeFoundation: runtime_budget.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_budget.h`.

**Модуль:** [[05 EngineRuntime/Модуль RuntimeFoundation|RuntimeFoundation]].
**Размер исходного файла:** 1770 байт.
**Подключение:** `#include <Epidemic/Runtime/Foundation/runtime_budget.h>`.

## Объявления типов (лексический индекс)
`RuntimeBudget`, `RuntimeBudgetConsumption`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>

#include "Epidemic/Foundation/error.h"
#include "Epidemic/Foundation/result.h"

namespace epidemic::runtime
{
// Zero values mean unlimited/no constraint. RuntimeBudget is a compact value object,
// not a scheduler; individual majors decide how to spend the allowed work.
struct RuntimeBudget
{
    std::chrono::microseconds max_time{};
    std::uint32_t max_items = 0;
    std::uint64_t max_bytes = 0;

    [[nodiscard]] constexpr bool HasTimeLimit() const noexcept { return max_time.count() > 0; }
    [[nodiscard]] constexpr bool HasItemLimit() const noexcept { return max_items != 0; }
    [[nodiscard]] constexpr bool HasByteLimit() const noexcept { return max_bytes != 0; }

    [[nodiscard]] constexpr bool HasTimeBudget() const noexcept { return HasTimeLimit(); }
    [[nodiscard]] constexpr bool HasItemBudget() const noexcept { return HasItemLimit(); }
    [[nodiscard]] constexpr bool HasByteBudget() const noexcept { return HasByteLimit(); }

    [[nodiscard]] constexpr bool IsUnlimited() const noexcept
    {
        return !HasTimeLimit() && !HasItemLimit() && !HasByteLimit();
    }
};


[[nodiscard]] inline foundation::Result<void> ValidateRuntimeBudget(const RuntimeBudget& budget)
{
    if (budget.max_time.count() < 0)
    {
        return foundation::Result<void>::Failure(
            foundation::Error::Create("runtime.invalid_budget", "runtime time budget must not be negative"));
    }
    return foundation::Result<void>::Success();
}

struct RuntimeBudgetConsumption
{
    std::size_t processed_items = 0;
    std::size_t processed_bytes = 0;
    std::chrono::microseconds elapsed{};
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/error.h`, `Epidemic/Foundation/result.h`.

### Состав типов и операции

#### `RuntimeBudget` (строка заголовка 14)

Публичные поля и значения по умолчанию:

- `std::uint32_t max_items = 0`
- `std::uint64_t max_bytes = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool HasTimeLimit() const noexcept;
[[nodiscard]] constexpr bool HasItemLimit() const noexcept;
[[nodiscard]] constexpr bool HasByteLimit() const noexcept;
[[nodiscard]] constexpr bool HasTimeBudget() const noexcept;
[[nodiscard]] constexpr bool HasItemBudget() const noexcept;
[[nodiscard]] constexpr bool HasByteBudget() const noexcept;
[[nodiscard]] constexpr bool IsUnlimited() const noexcept;
```

#### `RuntimeBudgetConsumption` (строка заголовка 45)

Публичные поля и значения по умолчанию:

- `std::size_t processed_items = 0`
- `std::size_t processed_bytes = 0`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `IsUnlimited`: `EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp:95`
- `HasTimeLimit`: `EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp:102`
- `HasTimeBudget`: `EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp:103`


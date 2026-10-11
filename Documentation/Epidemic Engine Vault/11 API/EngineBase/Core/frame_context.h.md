---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Core/include/Epidemic/Core/frame_context.h"
---

# Core: frame_context.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Core/include/Epidemic/Core/frame_context.h`.

**Модуль:** [[04 EngineBase/Модуль Core|Core]].
**Размер исходного файла:** 871 байт.
**Подключение:** `#include <Epidemic/Core/frame_context.h>`.

## Объявления типов (лексический индекс)
`FrameContext`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/time.h>

namespace epidemic::core
{
// This file defines the immutable timing payload passed through one frame execution.
// FrameContext connects Application, modules, and frame-phase handlers through a shared snapshot.

struct FrameContext
{
    foundation::FrameIndex frame_index{};
    foundation::FrameTime delta_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
    foundation::FrameTime absolute_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
    foundation::FrameTime raw_delta_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());

    // Returns true for the very first executed frame.
    [[nodiscard]] bool IsFirstFrame() const noexcept
    {
        return frame_index.Value() == 0;
    }
};
} // namespace epidemic::core
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/time.h`.

### Состав типов и операции

#### `FrameContext` (строка заголовка 10)

Публичные сигнатуры операций:

```cpp
foundation::FrameTime delta_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
foundation::FrameTime absolute_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
foundation::FrameTime raw_delta_time = foundation::FrameTime::FromDuration(foundation::Duration::zero());
// Returns true for the very first executed frame. [[nodiscard]] bool IsFirstFrame() const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `FromDuration` | `FrameTime::FromDuration` | `EngineBase/Core/src/application.cpp:597` |


---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Diagnostics/include/Epidemic/Diagnostics/thread_context.h"
---

# Diagnostics: thread_context.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Diagnostics/include/Epidemic/Diagnostics/thread_context.h`.

**Модуль:** [[04 EngineBase/Модуль Diagnostics|Diagnostics]].
**Размер исходного файла:** 514 байт.
**Подключение:** `#include <Epidemic/Diagnostics/thread_context.h>`.

## Объявления типов (лексический индекс)
_Именованные `class`/`struct`/`enum` в этом заголовке не обнаружены простой лексической проверкой._

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <string>
#include <string_view>

namespace epidemic::diagnostics
{
// Stores a human-readable name for the current thread. The name is thread-local and may be empty.
void SetCurrentThreadName(std::string_view name) noexcept;

// Returns an owning copy of the current thread name. The copy remains valid after a later rename
// and after the originating thread-local string changes storage.
[[nodiscard]] std::string GetCurrentThreadName();
} // namespace epidemic::diagnostics
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

Этот заголовок преимущественно содержит free-functions, type aliases или implementation helpers. Точные сигнатуры смотрите в исходном блоке выше.

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.


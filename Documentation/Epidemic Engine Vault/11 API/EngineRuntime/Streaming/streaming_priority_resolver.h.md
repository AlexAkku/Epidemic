---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h"
---

# Streaming: streaming_priority_resolver.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h`.

**Модуль:** [[05 EngineRuntime/Модуль Streaming|Streaming]].
**Размер исходного файла:** 526 байт.
**Подключение:** `#include <Epidemic/Runtime/Streaming/streaming_priority_resolver.h>`.

## Объявления типов (лексический индекс)
`IStreamingPriorityResolver`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Streaming/streaming_types.h"

namespace epidemic::runtime::streaming
{
// Optional policy interface that lets integrations derive request priority from external
// context such as player position, camera focus or region importance.
class IStreamingPriorityResolver
{
  public:
    virtual ~IStreamingPriorityResolver() = default;

    [[nodiscard]] virtual StreamingPriorityClass ResolvePriority(ChunkId chunk) const = 0;
};
} // namespace epidemic::runtime::streaming
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Streaming/streaming_types.h`.

### Состав типов и операции

#### `IStreamingPriorityResolver` (строка заголовка 9)

Публичные сигнатуры операций:

```cpp
virtual ~IStreamingPriorityResolver() = default;
[[nodiscard]] virtual StreamingPriorityClass ResolvePriority(ChunkId chunk) const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


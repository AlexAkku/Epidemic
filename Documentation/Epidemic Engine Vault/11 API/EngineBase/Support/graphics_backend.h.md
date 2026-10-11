---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Support/include/Epidemic/EngineBase/graphics_backend.h"
---

# Support: graphics_backend.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Support/include/Epidemic/EngineBase/graphics_backend.h`.

**Модуль:** [[04 EngineBase/Модуль Support|Support]].
**Размер исходного файла:** 338 байт.
**Подключение:** `#include <Epidemic/EngineBase/graphics_backend.h>`.

## Объявления типов (лексический индекс)
`GraphicsBackend`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

namespace epidemic::enginebase
{
// This file defines the graphics backend choices exposed by EngineBase support wiring.
// Higher layers choose one of these values during composition and remain backend-agnostic afterward.

enum class GraphicsBackend
{
    Null,
    D3D11,
};
} // namespace epidemic::enginebase
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Перечисления и допустимые категории

- `GraphicsBackend`: `Null`, `D3D11`

### Состав типов и операции

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


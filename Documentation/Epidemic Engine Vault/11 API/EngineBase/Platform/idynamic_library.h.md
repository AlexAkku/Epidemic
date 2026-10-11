---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h"
---

# Platform: idynamic_library.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Platform/include/Epidemic/Platform/idynamic_library.h`.

**Модуль:** [[04 EngineBase/Модуль Platform|Platform]].
**Размер исходного файла:** 860 байт.
**Подключение:** `#include <Epidemic/Platform/idynamic_library.h>`.

## Объявления типов (лексический индекс)
`IDynamicLibrary`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/result.h>

#include <memory>
#include <string_view>

namespace epidemic::platform
{
// This file defines the minimal dynamic-library abstraction returned by the platform runtime.
// The abstraction keeps symbol lookup behind Result-based error reporting instead of exceptions.

class IDynamicLibrary
{
  public:
    virtual ~IDynamicLibrary() = default;

    // Returns the library name or origin label used for diagnostics.
    [[nodiscard]] virtual std::string_view Name() const = 0;

    // Resolves a symbol and returns either its address or an Error describing the failure.
    [[nodiscard]] virtual epidemic::foundation::Result<void *> FindSymbol(std::string_view symbol_name) const = 0;
};

using DynamicLibraryPtr = std::shared_ptr<IDynamicLibrary>;
} // namespace epidemic::platform
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`.

### Состав типов и операции

#### `IDynamicLibrary` (строка заголовка 13)

Публичные сигнатуры операций:

```cpp
virtual ~IDynamicLibrary() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.


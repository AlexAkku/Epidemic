---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Platform/include/Epidemic/Platform/iplatform_runtime.h"
---

# Platform: iplatform_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Platform/include/Epidemic/Platform/iplatform_runtime.h`.

**Модуль:** [[04 EngineBase/Модуль Platform|Platform]].
**Размер исходного файла:** 2200 байт.
**Подключение:** `#include <Epidemic/Platform/iplatform_runtime.h>`.

## Объявления типов (лексический индекс)
`ProcessInfo`, `IPlatformRuntime`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/path.h>
#include <Epidemic/Foundation/result.h>
#include <Epidemic/Foundation/time.h>
#include <Epidemic/Platform/idynamic_library.h>

#include <string>
#include <cstddef>
#include <span>
#include <string_view>
#include <vector>

namespace epidemic::platform
{
// Neutral bounded byte-package I/O. One externally synchronized writer per
// path; successful commit flushes a same-directory temporary then replaces it.
[[nodiscard]] foundation::Result<std::vector<std::byte>> ReadFilePackage(
    const foundation::Path& path, std::size_t max_bytes);
[[nodiscard]] foundation::Result<void> CommitFilePackage(
    const foundation::Path& path, std::span<const std::byte> bytes, std::size_t max_bytes);

// This file defines the abstract runtime services provided by the host platform layer.
// IPlatformRuntime covers process metadata, clock access, dynamic library loading, and event pumping.

struct ProcessInfo
{
    epidemic::foundation::Path executable_path;
    epidemic::foundation::Path working_directory;
    std::vector<std::string> arguments;
};

class IPlatformRuntime
{
  public:
    virtual ~IPlatformRuntime() = default;

    // Returns the human-readable backend name for this runtime implementation.
    [[nodiscard]] virtual std::string_view Name() const = 0;

    // Returns immutable process information captured by the runtime.
    [[nodiscard]] virtual const ProcessInfo &GetProcessInfo() const = 0;

    // Returns the runtime's monotonic steady-clock time for engine timing.
    [[nodiscard]] virtual epidemic::foundation::TimePoint Now() const = 0;

    // Loads a dynamic library from disk and returns either the library object or an Error.
    [[nodiscard]] virtual epidemic::foundation::Result<DynamicLibraryPtr>
    LoadDynamicLibrary(const epidemic::foundation::Path &path) = 0;

    // Pumps native platform messages and updates internal runtime/window state.
    virtual void PumpEvents() = 0;

    // Returns whether the runtime has observed a process-level exit request.
    [[nodiscard]] virtual bool IsExitRequested() const = 0;
};
} // namespace epidemic::platform
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/path.h`, `Epidemic/Foundation/result.h`, `Epidemic/Foundation/time.h`, `Epidemic/Platform/idynamic_library.h`.

### Состав типов и операции

#### `ProcessInfo` (строка заголовка 26)

Публичные поля и значения по умолчанию:

- `epidemic::foundation::Path executable_path`
- `epidemic::foundation::Path working_directory`
- `std::vector<std::string> arguments`

#### `IPlatformRuntime` (строка заголовка 33)

Публичные сигнатуры операций:

```cpp
virtual ~IPlatformRuntime() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.


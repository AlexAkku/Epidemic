---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/RHI/include/Epidemic/RHI/irhi_device.h"
---

# RHI: irhi_device.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/RHI/include/Epidemic/RHI/irhi_device.h`.

**Модуль:** [[04 EngineBase/Модуль RHI|RHI]].
**Размер исходного файла:** 1289 байт.
**Подключение:** `#include <Epidemic/RHI/irhi_device.h>`.

## Объявления типов (лексический индекс)
`IRhiDevice`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Foundation/result.h>
#include <Epidemic/RHI/descriptors.h>
#include <Epidemic/RHI/irhi_command_context.h>
#include <Epidemic/RHI/irhi_swap_chain.h>

#include <memory>
#include <string_view>

namespace epidemic::rhi
{
// This file defines the top-level RHI device abstraction used by EngineBase.
// Devices create command contexts and swap chains while keeping backend selection behind composition.

class IRhiDevice
{
  public:
    virtual ~IRhiDevice() = default;

    // Returns the backend name for diagnostics and logs.
    [[nodiscard]] virtual std::string_view BackendName() const noexcept = 0;

    // Returns the immutable device descriptor used at creation time.
    [[nodiscard]] virtual const RhiDeviceDesc &Descriptor() const noexcept = 0;

    // Creates a command context capable of issuing baseline rendering operations.
    [[nodiscard]] virtual epidemic::foundation::Result<std::shared_ptr<IRhiCommandContext>> CreateCommandContext() = 0;

    // Creates a swap chain for the supplied presentation surface and descriptor.
    [[nodiscard]] virtual epidemic::foundation::Result<std::shared_ptr<IRhiSwapChain>>
    CreateSwapChain(const RhiSwapChainDesc &swap_chain_desc) = 0;
};
} // namespace epidemic::rhi
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/RHI/descriptors.h`, `Epidemic/RHI/irhi_command_context.h`, `Epidemic/RHI/irhi_swap_chain.h`.

### Состав типов и операции

#### `IRhiDevice` (строка заголовка 16)

Публичные сигнатуры операций:

```cpp
virtual ~IRhiDevice() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.


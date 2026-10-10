---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h"
---

# Resources: resource_payload.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h`.

**Модуль:** [[05 EngineRuntime/Модуль Resources|Resources]].
**Размер исходного файла:** 868 байт.
**Подключение:** `#include <Epidemic/Runtime/Resources/resource_payload.h>`.

## Объявления типов (лексический индекс)
`IResourcePayload`, `ByteResourcePayload`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace epidemic::runtime
{
class IResourcePayload
{
  public:
    virtual ~IResourcePayload() = default;

    [[nodiscard]] virtual std::size_t GetSizeBytes() const noexcept = 0;
};

class ByteResourcePayload final : public IResourcePayload
{
  public:
    explicit ByteResourcePayload(std::vector<std::byte> bytes = {}) : bytes_(std::move(bytes))
    {
    }

    [[nodiscard]] std::size_t GetSizeBytes() const noexcept override
    {
        return bytes_.size();
    }

    [[nodiscard]] const std::vector<std::byte>& Bytes() const noexcept
    {
        return bytes_;
    }

  private:
    std::vector<std::byte> bytes_;
};

using ResourcePayloadPtr = std::shared_ptr<const IResourcePayload>;
} // namespace epidemic::runtime
```

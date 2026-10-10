---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_value.h"
---

# Serialization: archive_value.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_value.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 809 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/archive_value.h>`.

## Объявления типов (лексический индекс)
`SerializedDocument`, `Impl`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Serialization/schema_version.h"

#include <cstdint>
#include <memory>

namespace epidemic::runtime
{
class SerializedDocument
{
  public:
    SerializedDocument() = default;

    [[nodiscard]] foundation::StringId GetTypeId() const;
    [[nodiscard]] SchemaVersion GetSchemaVersion() const;
    [[nodiscard]] std::uint32_t GetFormatVersion() const;
    [[nodiscard]] bool IsValid() const noexcept;

  private:
    struct Impl;

    explicit SerializedDocument(std::shared_ptr<const Impl> impl);

    std::shared_ptr<const Impl> impl_;

    friend class InMemoryArchiveReader;
    friend class InMemoryArchiveWriter;
    friend class InMemoryArchiveFactory;
};
} // namespace epidemic::runtime
```

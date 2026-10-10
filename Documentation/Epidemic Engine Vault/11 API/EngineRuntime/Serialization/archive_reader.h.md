---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_reader.h"
---

# Serialization: archive_reader.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_reader.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 1820 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/archive_reader.h>`.

## Объявления типов (лексический индекс)
`IArchiveReader`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/archive_value.h"

#include <cstddef>
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace epidemic::runtime
{
class IArchiveReader
{
  public:
    virtual ~IArchiveReader() = default;

    [[nodiscard]] virtual foundation::StringId GetTypeId() const = 0;
    [[nodiscard]] virtual SchemaVersion GetSchemaVersion() const = 0;
    [[nodiscard]] virtual std::uint32_t GetFormatVersion() const = 0;

    [[nodiscard]] virtual foundation::Result<void> BeginObject(std::string_view name) = 0;
    [[nodiscard]] virtual foundation::Result<void> EndObject() = 0;
    [[nodiscard]] virtual foundation::Result<std::size_t> BeginArray(std::string_view name) = 0;
    [[nodiscard]] virtual foundation::Result<void> BeginArrayElement(std::size_t index) = 0;
    [[nodiscard]] virtual foundation::Result<void> EndArrayElement() = 0;
    [[nodiscard]] virtual foundation::Result<void> EndArray() = 0;

    [[nodiscard]] virtual foundation::Result<std::string> ReadString(std::string_view name) const = 0;
    [[nodiscard]] virtual foundation::Result<std::uint64_t> ReadUInt64(std::string_view name) const = 0;
    [[nodiscard]] virtual foundation::Result<std::int64_t> ReadInt64(std::string_view name) const = 0;
    [[nodiscard]] virtual foundation::Result<double> ReadDouble(std::string_view name) const = 0;
    [[nodiscard]] virtual foundation::Result<bool> ReadBool(std::string_view name) const = 0;
    [[nodiscard]] virtual foundation::Result<std::vector<std::byte>> ReadBytes(std::string_view name) const = 0;
    [[nodiscard]] virtual foundation::Result<bool> IsNull(std::string_view name) const = 0;
};
} // namespace epidemic::runtime
```

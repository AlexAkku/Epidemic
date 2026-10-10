---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "enginebase"]
source_path: "EngineBase/Core/include/Epidemic/Core/basic_configuration.h"
---

# Core: basic_configuration.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineBase/Core/include/Epidemic/Core/basic_configuration.h`.

**Модуль:** [[04 EngineBase/Модуль Core|Core]].
**Размер исходного файла:** 1622 байт.
**Подключение:** `#include <Epidemic/Core/basic_configuration.h>`.

## Объявления типов (лексический индекс)
`BasicConfiguration`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <Epidemic/Core/configuration.h>

#include <mutex>
#include <string>
#include <unordered_map>

namespace epidemic::core::config
{
// This file declares the default in-memory configuration store used by EngineBase.
// BasicConfiguration is intentionally small and thread-safe: it stores typed key/value maps
// without schema validation, persistence, or layered overrides.

class BasicConfiguration final : public IConfiguration
{
  public:
    // Returns the string value for a key, or nullopt when the key is absent.
    [[nodiscard]] std::optional<std::string> GetString(std::string_view key) const override;

    // Stores or replaces a string value for a key.
    void SetString(std::string key, std::string value) override;

    // Returns the integer value for a key, or nullopt when the key is absent.
    [[nodiscard]] std::optional<std::int64_t> GetInt(std::string_view key) const override;

    // Stores or replaces an integer value for a key.
    void SetInt(std::string key, std::int64_t value) override;

    // Returns the boolean value for a key, or nullopt when the key is absent.
    [[nodiscard]] std::optional<bool> GetBool(std::string_view key) const override;

    // Stores or replaces a boolean value for a key.
    void SetBool(std::string key, bool value) override;

  private:
    mutable std::mutex mutex_;
    std::unordered_map<std::string, std::string> string_values_;
    std::unordered_map<std::string, std::int64_t> int_values_;
    std::unordered_map<std::string, bool> bool_values_;
};
} // namespace epidemic::core::config
```

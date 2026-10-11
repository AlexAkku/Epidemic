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

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/Serialization/schema_version.h`.

### Состав типов и операции

#### `SerializedDocument` (строка заголовка 11)

Публичные сигнатуры операций:

```cpp
SerializedDocument() = default;
[[nodiscard]] foundation::StringId GetTypeId() const;
[[nodiscard]] SchemaVersion GetSchemaVersion() const;
[[nodiscard]] std::uint32_t GetFormatVersion() const;
[[nodiscard]] bool IsValid() const noexcept;
```

### Интеграция и безопасность вызовов

- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `GetTypeId` | `InMemoryArchiveReader::GetTypeId` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:423` |
| `GetSchemaVersion` | `InMemoryArchiveReader::GetSchemaVersion` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:424` |
| `GetFormatVersion` | `InMemoryArchiveReader::GetFormatVersion` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:425` |
| `SerializedDocument` | `SerializedDocument::SerializedDocument` | `EngineRuntime/Serialization/src/serialization.cpp:12` |
| `GetTypeId` | `SerializedDocument::GetTypeId` | `EngineRuntime/Serialization/src/serialization.cpp:16` |
| `GetSchemaVersion` | `SerializedDocument::GetSchemaVersion` | `EngineRuntime/Serialization/src/serialization.cpp:21` |
| `GetFormatVersion` | `SerializedDocument::GetFormatVersion` | `EngineRuntime/Serialization/src/serialization.cpp:26` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetTypeId`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:258`
- `GetSchemaVersion`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:885`
- `GetFormatVersion`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:1039`


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

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Serialization/archive_value.h`.

### Состав типов и операции

#### `IArchiveReader` (строка заголовка 15)

Публичные сигнатуры операций:

```cpp
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
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `BeginObject` | `InMemoryArchiveWriter::BeginObject` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:28` |
| `EndObject` | `InMemoryArchiveWriter::EndObject` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:61` |
| `BeginArray` | `InMemoryArchiveWriter::BeginArray` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:71` |
| `BeginArrayElement` | `InMemoryArchiveWriter::BeginArrayElement` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:106` |
| `EndArrayElement` | `InMemoryArchiveWriter::EndArrayElement` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:146` |
| `EndArray` | `InMemoryArchiveWriter::EndArray` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:156` |
| `GetTypeId` | `InMemoryArchiveReader::GetTypeId` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:423` |
| `GetSchemaVersion` | `InMemoryArchiveReader::GetSchemaVersion` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:424` |
| `GetFormatVersion` | `InMemoryArchiveReader::GetFormatVersion` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:425` |
| `BeginObject` | `InMemoryArchiveReader::BeginObject` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:427` |
| `EndObject` | `InMemoryArchiveReader::EndObject` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:443` |
| `BeginArray` | `InMemoryArchiveReader::BeginArray` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:453` |
| `BeginArrayElement` | `InMemoryArchiveReader::BeginArrayElement` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:469` |
| `EndArrayElement` | `InMemoryArchiveReader::EndArrayElement` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:484` |
| `EndArray` | `InMemoryArchiveReader::EndArray` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:494` |
| `ReadString` | `InMemoryArchiveReader::ReadString` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:511` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `GetTypeId`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:258`
- `ReadString`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:264`
- `ReadUInt64`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:269`
- `BeginObject`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:348`
- `EndObject`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:349`
- `BeginArray`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:350`
- `BeginArrayElement`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:351`
- `EndArrayElement`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:352`
- `EndArray`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:353`
- `ReadBytes`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:632`
- `IsNull`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:633`
- `GetSchemaVersion`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:885`


---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_writer.h"
---

# Serialization: archive_writer.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_writer.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 1763 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/archive_writer.h>`.

## Объявления типов (лексический индекс)
`IArchiveWriter`

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
#include <string_view>

namespace epidemic::runtime
{
class IArchiveWriter
{
  public:
    virtual ~IArchiveWriter() = default;

    [[nodiscard]] virtual foundation::Result<void> BeginObject(std::string_view name) = 0;
    [[nodiscard]] virtual foundation::Result<void> EndObject() = 0;
    [[nodiscard]] virtual foundation::Result<void> BeginArray(std::string_view name, std::size_t size) = 0;
    [[nodiscard]] virtual foundation::Result<void> BeginArrayElement(std::size_t index) = 0;
    [[nodiscard]] virtual foundation::Result<void> EndArrayElement() = 0;
    [[nodiscard]] virtual foundation::Result<void> EndArray() = 0;

    [[nodiscard]] virtual foundation::Result<void> WriteString(std::string_view name, std::string_view value) = 0;
    [[nodiscard]] virtual foundation::Result<void> WriteUInt64(std::string_view name, std::uint64_t value) = 0;
    [[nodiscard]] virtual foundation::Result<void> WriteInt64(std::string_view name, std::int64_t value) = 0;
    [[nodiscard]] virtual foundation::Result<void> WriteDouble(std::string_view name, double value) = 0;
    [[nodiscard]] virtual foundation::Result<void> WriteBool(std::string_view name, bool value) = 0;
    [[nodiscard]] virtual foundation::Result<void> WriteBytes(std::string_view name, std::span<const std::byte> value) = 0;
    [[nodiscard]] virtual foundation::Result<void> WriteNull(std::string_view name) = 0;
    [[nodiscard]] virtual foundation::Result<SerializedDocument> Finalize(foundation::StringId type_id, SchemaVersion schema_version) = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Serialization/archive_value.h`.

### Состав типов и операции

#### `IArchiveWriter` (строка заголовка 13)

Публичные сигнатуры операций:

```cpp
virtual ~IArchiveWriter() = default;
[[nodiscard]] virtual foundation::Result<void> BeginObject(std::string_view name) = 0;
[[nodiscard]] virtual foundation::Result<void> EndObject() = 0;
[[nodiscard]] virtual foundation::Result<void> BeginArray(std::string_view name, std::size_t size) = 0;
[[nodiscard]] virtual foundation::Result<void> BeginArrayElement(std::size_t index) = 0;
[[nodiscard]] virtual foundation::Result<void> EndArrayElement() = 0;
[[nodiscard]] virtual foundation::Result<void> EndArray() = 0;
[[nodiscard]] virtual foundation::Result<void> WriteString(std::string_view name, std::string_view value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteUInt64(std::string_view name, std::uint64_t value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteInt64(std::string_view name, std::int64_t value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteDouble(std::string_view name, double value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteBool(std::string_view name, bool value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteBytes(std::string_view name, std::span<const std::byte> value) = 0;
[[nodiscard]] virtual foundation::Result<void> WriteNull(std::string_view name) = 0;
[[nodiscard]] virtual foundation::Result<SerializedDocument> Finalize(foundation::StringId type_id, SchemaVersion schema_version) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
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
| `WriteString` | `InMemoryArchiveWriter::WriteString` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:171` |
| `WriteUInt64` | `InMemoryArchiveWriter::WriteUInt64` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:183` |
| `WriteInt64` | `InMemoryArchiveWriter::WriteInt64` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:188` |
| `WriteDouble` | `InMemoryArchiveWriter::WriteDouble` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:193` |
| `WriteBool` | `InMemoryArchiveWriter::WriteBool` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:198` |
| `WriteBytes` | `InMemoryArchiveWriter::WriteBytes` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:203` |
| `WriteNull` | `InMemoryArchiveWriter::WriteNull` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:215` |
| `Finalize` | `InMemoryArchiveWriter::Finalize` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:220` |
| `BeginObject` | `InMemoryArchiveReader::BeginObject` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:427` |
| `EndObject` | `InMemoryArchiveReader::EndObject` | `EngineRuntime/Serialization/src/in_memory_archive.cpp:443` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `WriteUInt64`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:107`
- `WriteString`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:167`
- `BeginObject`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:348`
- `EndObject`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:349`
- `BeginArray`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:350`
- `BeginArrayElement`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:351`
- `EndArrayElement`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:352`
- `EndArray`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:353`
- `WriteInt64`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:356`
- `WriteDouble`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:357`
- `WriteBool`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:358`
- `WriteBytes`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:359`


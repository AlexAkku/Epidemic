---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration.h"
---

# Serialization: migration.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 1717 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/migration.h>`.

## Объявления типов (лексический индекс)
`MigrationKey`, `IMigration`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/Serialization/archive_reader.h"
#include "Epidemic/Runtime/Serialization/archive_writer.h"
#include "Epidemic/Runtime/Serialization/schema_version.h"

#include <functional>

namespace epidemic::runtime
{
struct MigrationKey
{
    foundation::StringId type_id{};
    SchemaVersion from{};
    SchemaVersion to{};

    [[nodiscard]] constexpr bool operator==(const MigrationKey&) const noexcept = default;
};

class IMigration
{
  public:
    virtual ~IMigration() = default;

    [[nodiscard]] virtual MigrationKey GetKey() const = 0;
    [[nodiscard]] virtual foundation::Result<void> Apply(IArchiveReader& input, IArchiveWriter& output) const = 0;
};
} // namespace epidemic::runtime

namespace std
{
template <> struct hash<epidemic::runtime::MigrationKey>
{
    [[nodiscard]] size_t operator()(const epidemic::runtime::MigrationKey& key) const noexcept
    {
        size_t seed = hash<std::uint64_t>{}(key.type_id.Raw());
        seed ^= hash<std::uint32_t>{}(key.from.major) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash<std::uint32_t>{}(key.from.minor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash<std::uint32_t>{}(key.from.patch) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash<std::uint32_t>{}(key.to.major) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash<std::uint32_t>{}(key.to.minor) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        seed ^= hash<std::uint32_t>{}(key.to.patch) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
        return seed;
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/Serialization/archive_reader.h`, `Epidemic/Runtime/Serialization/archive_writer.h`, `Epidemic/Runtime/Serialization/schema_version.h`.

### Состав типов и операции

#### `MigrationKey` (строка заголовка 13)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const MigrationKey&) const noexcept = default;
```

#### `IMigration` (строка заголовка 22)

Публичные сигнатуры операций:

```cpp
virtual ~IMigration() = default;
[[nodiscard]] virtual MigrationKey GetKey() const = 0;
[[nodiscard]] virtual foundation::Result<void> Apply(IArchiveReader& input, IArchiveWriter& output) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `hash` (строка заголовка 34)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(const epidemic::runtime::MigrationKey& key) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.


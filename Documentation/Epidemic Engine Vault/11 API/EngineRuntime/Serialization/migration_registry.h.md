---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h"
---

# Serialization: migration_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 1106 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/migration_registry.h>`.

## Объявления типов (лексический индекс)
`IMigrationRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/migration.h"

#include <memory>
#include <vector>

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class IMigrationRegistry
{
  public:
    virtual ~IMigrationRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterMigration(std::shared_ptr<const IMigration> migration) = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<const IMigration> FindMigration(const MigrationKey& key) const = 0;
    [[nodiscard]] virtual bool HasMigration(const MigrationKey& key) const = 0;
    [[nodiscard]] virtual foundation::Result<std::vector<std::shared_ptr<const IMigration>>> FindMigrationPath(
        foundation::StringId type_id,
        SchemaVersion from,
        SchemaVersion to) const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Serialization/migration.h`.

### Состав типов и операции

#### `IMigrationRegistry` (строка заголовка 13)

Публичные сигнатуры операций:

```cpp
virtual ~IMigrationRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterMigration(std::shared_ptr<const IMigration> migration) = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
[[nodiscard]] virtual std::shared_ptr<const IMigration> FindMigration(const MigrationKey& key) const = 0;
[[nodiscard]] virtual bool HasMigration(const MigrationKey& key) const = 0;
[[nodiscard]] virtual foundation::Result<std::vector<std::shared_ptr<const IMigration>>> FindMigrationPath( foundation::StringId type_id, SchemaVersion from, SchemaVersion to) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.
- В заголовке задано ограничение потоков: Threading: mutation must occur on the runtime thread..

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Freeze` | `SerializerRegistry::Freeze` | `EngineRuntime/Serialization/src/serializer_registry.cpp:69` |
| `IsFrozen` | `SerializerRegistry::IsFrozen` | `EngineRuntime/Serialization/src/serializer_registry.cpp:75` |
| `RegisterMigration` | `MigrationRegistry::RegisterMigration` | `EngineRuntime/Serialization/src/migration_registry.cpp:32` |
| `Freeze` | `MigrationRegistry::Freeze` | `EngineRuntime/Serialization/src/migration_registry.cpp:82` |
| `IsFrozen` | `MigrationRegistry::IsFrozen` | `EngineRuntime/Serialization/src/migration_registry.cpp:88` |
| `FindMigration` | `MigrationRegistry::FindMigration` | `EngineRuntime/Serialization/src/migration_registry.cpp:93` |
| `HasMigration` | `MigrationRegistry::HasMigration` | `EngineRuntime/Serialization/src/migration_registry.cpp:103` |
| `FindMigrationPath` | `MigrationRegistry::FindMigrationPath` | `EngineRuntime/Serialization/src/migration_registry.cpp:108` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Freeze`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:782`
- `RegisterMigration`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:864`
- `FindMigrationPath`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:897`
- `HasMigration`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:969`
- `FindMigration`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:1209`
- `IsFrozen`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:1412`


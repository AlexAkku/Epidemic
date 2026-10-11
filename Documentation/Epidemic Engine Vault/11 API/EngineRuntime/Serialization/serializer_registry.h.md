---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h"
---

# Serialization: serializer_registry.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h`.

**Модуль:** [[05 EngineRuntime/Модуль Serialization|Serialization]].
**Размер исходного файла:** 887 байт.
**Подключение:** `#include <Epidemic/Runtime/Serialization/serializer_registry.h>`.

## Объявления типов (лексический индекс)
`ISerializerRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Serialization/serializer.h"

#include <memory>

namespace epidemic::runtime
{
// Threading: mutation must occur on the runtime thread.
// Concurrent reads/writes are not supported unless explicitly documented.
class ISerializerRegistry
{
  public:
    virtual ~ISerializerRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterSerializer(std::shared_ptr<const ISerializer> serializer) = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
    [[nodiscard]] virtual std::shared_ptr<const ISerializer> FindSerializer(foundation::StringId type_id) const = 0;
    [[nodiscard]] virtual bool HasSerializer(foundation::StringId type_id) const = 0;
};
} // namespace epidemic::runtime
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Serialization/serializer.h`.

### Состав типов и операции

#### `ISerializerRegistry` (строка заголовка 12)

Публичные сигнатуры операций:

```cpp
virtual ~ISerializerRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterSerializer(std::shared_ptr<const ISerializer> serializer) = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
[[nodiscard]] virtual std::shared_ptr<const ISerializer> FindSerializer(foundation::StringId type_id) const = 0;
[[nodiscard]] virtual bool HasSerializer(foundation::StringId type_id) const = 0;
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
| `RegisterSerializer` | `SerializerRegistry::RegisterSerializer` | `EngineRuntime/Serialization/src/serializer_registry.cpp:18` |
| `Freeze` | `SerializerRegistry::Freeze` | `EngineRuntime/Serialization/src/serializer_registry.cpp:69` |
| `IsFrozen` | `SerializerRegistry::IsFrozen` | `EngineRuntime/Serialization/src/serializer_registry.cpp:75` |
| `FindSerializer` | `SerializerRegistry::FindSerializer` | `EngineRuntime/Serialization/src/serializer_registry.cpp:80` |
| `HasSerializer` | `SerializerRegistry::HasSerializer` | `EngineRuntime/Serialization/src/serializer_registry.cpp:90` |
| `Freeze` | `MigrationRegistry::Freeze` | `EngineRuntime/Serialization/src/migration_registry.cpp:82` |
| `IsFrozen` | `MigrationRegistry::IsFrozen` | `EngineRuntime/Serialization/src/migration_registry.cpp:88` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterSerializer`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:753`
- `HasSerializer`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:760`
- `FindSerializer`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:1192`
- `Freeze`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:1401`
- `IsFrozen`: `EngineRuntime/Serialization/tests/serialization_tests.cpp:1412`


---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h"
---

# World: object_materialization.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/object_materialization.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 2650 байт.
**Подключение:** `#include <Epidemic/Runtime/World/object_materialization.h>`.

## Объявления типов (лексический индекс)
`MaterializationState`, `MaterializationRequest`, `DemotionCommitToken`, `DemotionSnapshot`, `DemotionRequest`, `IObjectMaterializer`, `IDemotionCommitAuthority`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Foundation/string_id.h"
#include "Epidemic/Runtime/World/object_placement.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_states.h"

#include <cstdint>

namespace epidemic::runtime
{
enum class MaterializationState
{
    NotMaterialized,
    Requested,
    Materializing,
    Materialized,
    Demoting,
    Demoted,
    Failed,
};

struct MaterializationRequest
{
    PersistentObjectId persistent_id{};
    ObjectRealityLevel target_reality = ObjectRealityLevel::Physical;

    [[nodiscard]] constexpr bool operator==(const MaterializationRequest&) const noexcept = default;
};

struct DemotionCommitToken
{
    std::uint64_t token_id = 0;
    RuntimeObjectId object{};
    ObjectRealityLevel target_reality = ObjectRealityLevel::Logical;
    ObjectPlacement collapsed_placement{HiddenPlacement{}};
    foundation::StringId collapse_record_id{};
    std::uint64_t source_revision = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return token_id != 0 && object.IsValid() && collapse_record_id.IsValid() && source_revision != 0;
    }

    [[nodiscard]] constexpr bool operator==(const DemotionCommitToken&) const noexcept = default;
};

struct DemotionSnapshot
{
    RuntimeObjectId object{};
    ObjectRealityLevel target_reality = ObjectRealityLevel::Logical;
    ObjectPlacement collapsed_placement{HiddenPlacement{}};
    foundation::StringId collapse_record_id{};
    std::uint64_t source_revision = 0;

    [[nodiscard]] constexpr bool operator==(const DemotionSnapshot&) const noexcept = default;
};

struct DemotionRequest
{
    RuntimeObjectId runtime_id{};
    ObjectRealityLevel target_reality = ObjectRealityLevel::Logical;
    DemotionCommitToken commit_token{};

    [[nodiscard]] constexpr bool operator==(const DemotionRequest&) const noexcept = default;
};

class IObjectMaterializer
{
  public:
    virtual ~IObjectMaterializer() = default;

    [[nodiscard]] virtual foundation::Result<RuntimeObjectId> Materialize(const MaterializationRequest& request) = 0;
    [[nodiscard]] virtual foundation::Result<void> Demote(const DemotionRequest& request) = 0;
};

class IDemotionCommitAuthority
{
  public:
    virtual ~IDemotionCommitAuthority() = default;

    [[nodiscard]] virtual foundation::Result<DemotionCommitToken> IssueDemotionCommitToken(DemotionSnapshot snapshot) = 0;
    [[nodiscard]] virtual foundation::Result<void> RevokeDemotionCommitToken(std::uint64_t token_id) = 0;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Foundation/string_id.h`, `Epidemic/Runtime/World/object_placement.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/runtime_states.h`.

### Перечисления и допустимые категории

- `MaterializationState`: `NotMaterialized`, `Requested`, `Materializing`, `Materialized`, `Demoting`, `Demoted`, `Failed`

### Состав типов и операции

#### `MaterializationRequest` (строка заголовка 24)

Публичные поля и значения по умолчанию:

- `ObjectRealityLevel target_reality = ObjectRealityLevel::Physical`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const MaterializationRequest&) const noexcept = default;
```

#### `DemotionCommitToken` (строка заголовка 32)

Публичные поля и значения по умолчанию:

- `std::uint64_t token_id = 0`
- `ObjectRealityLevel target_reality = ObjectRealityLevel::Logical`
- `std::uint64_t source_revision = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const DemotionCommitToken&) const noexcept = default;
```

#### `DemotionSnapshot` (строка заголовка 49)

Публичные поля и значения по умолчанию:

- `ObjectRealityLevel target_reality = ObjectRealityLevel::Logical`
- `std::uint64_t source_revision = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const DemotionSnapshot&) const noexcept = default;
```

#### `DemotionRequest` (строка заголовка 60)

Публичные поля и значения по умолчанию:

- `ObjectRealityLevel target_reality = ObjectRealityLevel::Logical`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const DemotionRequest&) const noexcept = default;
```

#### `IObjectMaterializer` (строка заголовка 69)

Публичные сигнатуры операций:

```cpp
virtual ~IObjectMaterializer() = default;
[[nodiscard]] virtual foundation::Result<RuntimeObjectId> Materialize(const MaterializationRequest& request) = 0;
[[nodiscard]] virtual foundation::Result<void> Demote(const DemotionRequest& request) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IDemotionCommitAuthority` (строка заголовка 78)

Публичные сигнатуры операций:

```cpp
virtual ~IDemotionCommitAuthority() = default;
[[nodiscard]] virtual foundation::Result<DemotionCommitToken> IssueDemotionCommitToken(DemotionSnapshot snapshot) = 0;
[[nodiscard]] virtual foundation::Result<void> RevokeDemotionCommitToken(std::uint64_t token_id) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Materialize` | `WorldRuntime::Materialize` | `EngineRuntime/World/src/world_runtime_impl.cpp:864` |
| `Demote` | `WorldRuntime::Demote` | `EngineRuntime/World/src/world_runtime_impl.cpp:884` |
| `IssueDemotionCommitToken` | `WorldRuntime::IssueDemotionCommitToken` | `EngineRuntime/World/src/world_runtime_impl.cpp:899` |
| `RevokeDemotionCommitToken` | `WorldRuntime::RevokeDemotionCommitToken` | `EngineRuntime/World/src/world_runtime_impl.cpp:965` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Demote`: `EngineRuntime/World/tests/world_tests.cpp:514`
- `Materialize`: `EngineRuntime/World/tests/world_tests.cpp:539`
- `IssueDemotionCommitToken`: `EngineRuntime/World/tests/world_tests.cpp:817`
- `RevokeDemotionCommitToken`: `EngineRuntime/World/tests/world_tests.cpp:826`


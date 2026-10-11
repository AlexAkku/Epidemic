---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h"
---

# World: chunk.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/World/include/Epidemic/Runtime/World/chunk.h`.

**Модуль:** [[05 EngineRuntime/Модуль World|World]].
**Размер исходного файла:** 2394 байт.
**Подключение:** `#include <Epidemic/Runtime/World/chunk.h>`.

## Объявления типов (лексический индекс)
`ChunkDescriptor`, `ChangeChunkStateCommand`, `ChunkSnapshot`, `IChunkRegistry`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/World/world_state.h"
#include <cstdint>
#include <optional>

namespace epidemic::runtime
{
struct ChunkDescriptor
{
    ChunkId id{};
    RegionId region_id{};
    std::int32_t x = 0;
    std::int32_t y = 0;
    std::int32_t z = 0;

    [[nodiscard]] constexpr bool operator==(const ChunkDescriptor&) const noexcept = default;
};

struct ChangeChunkStateCommand
{
    ChunkId chunk{};
    std::uint64_t expected_revision = 0;
    ChunkState state = ChunkState::Unloaded;
};

struct ChunkSnapshot
{
    ChunkDescriptor descriptor{};
    ChunkState state = ChunkState::Unloaded;
    std::uint64_t revision = 0;
};

[[nodiscard]] constexpr bool CanTransition(ChunkState from, ChunkState to) noexcept
{
    switch (from)
    {
    case ChunkState::Unloaded:
        return to == ChunkState::Unloaded || to == ChunkState::Loading;
    case ChunkState::Loading:
        return to == ChunkState::Loading || to == ChunkState::Resident || to == ChunkState::Unloaded;
    case ChunkState::Resident:
        return to == ChunkState::Resident || to == ChunkState::Active || to == ChunkState::Sleeping ||
               to == ChunkState::Unloading;
    case ChunkState::Active:
        return to == ChunkState::Active || to == ChunkState::Resident || to == ChunkState::Sleeping ||
               to == ChunkState::Unloading;
    case ChunkState::Sleeping:
        return to == ChunkState::Sleeping || to == ChunkState::Active || to == ChunkState::Resident ||
               to == ChunkState::Unloading;
    case ChunkState::Unloading:
        return to == ChunkState::Unloading || to == ChunkState::Unloaded;
    }
    return false;
}

class IChunkRegistry
{
  public:
    virtual ~IChunkRegistry() = default;
    [[nodiscard]] virtual foundation::Result<void> RegisterChunk(ChunkDescriptor chunk) = 0;
    [[nodiscard]] virtual std::optional<ChunkDescriptor> FindChunk(ChunkId id) const = 0;
    [[nodiscard]] virtual foundation::Result<ChunkSnapshot> GetChunkSnapshot(ChunkId id) const = 0;
    [[nodiscard]] virtual foundation::Result<void> SetChunkState(ChangeChunkStateCommand command) = 0;
    virtual void Freeze() noexcept = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};
}
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/World/world_state.h`.

### Состав типов и операции

#### `ChunkDescriptor` (строка заголовка 11)

Публичные поля и значения по умолчанию:

- `std::int32_t x = 0`
- `std::int32_t y = 0`
- `std::int32_t z = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const ChunkDescriptor&) const noexcept = default;
```

#### `ChangeChunkStateCommand` (строка заголовка 22)

Публичные поля и значения по умолчанию:

- `std::uint64_t expected_revision = 0`
- `ChunkState state = ChunkState::Unloaded`

#### `ChunkSnapshot` (строка заголовка 29)

Публичные поля и значения по умолчанию:

- `ChunkState state = ChunkState::Unloaded`
- `std::uint64_t revision = 0`

#### `IChunkRegistry` (строка заголовка 59)

Публичные сигнатуры операций:

```cpp
virtual ~IChunkRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterChunk(ChunkDescriptor chunk) = 0;
[[nodiscard]] virtual std::optional<ChunkDescriptor> FindChunk(ChunkId id) const = 0;
[[nodiscard]] virtual foundation::Result<ChunkSnapshot> GetChunkSnapshot(ChunkId id) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetChunkState(ChangeChunkStateCommand command) = 0;
virtual void Freeze() noexcept = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Freeze` | `WorldRuntime::Freeze` | `EngineRuntime/World/src/world_runtime_impl.cpp:139` |
| `IsFrozen` | `WorldRuntime::IsFrozen` | `EngineRuntime/World/src/world_runtime_impl.cpp:144` |
| `RegisterChunk` | `WorldRuntime::RegisterChunk` | `EngineRuntime/World/src/world_runtime_impl.cpp:160` |
| `FindChunk` | `WorldRuntime::FindChunk` | `EngineRuntime/World/src/world_runtime_impl.cpp:194` |
| `GetChunkSnapshot` | `WorldRuntime::GetChunkSnapshot` | `EngineRuntime/World/src/world_runtime_impl.cpp:205` |
| `SetChunkState` | `WorldRuntime::SetChunkState` | `EngineRuntime/World/src/world_runtime_impl.cpp:216` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterChunk`: `EngineRuntime/World/tests/world_tests.cpp:88`
- `FindChunk`: `EngineRuntime/World/tests/world_tests.cpp:120`
- `GetChunkSnapshot`: `EngineRuntime/World/tests/world_tests.cpp:150`
- `SetChunkState`: `EngineRuntime/World/tests/world_tests.cpp:158`
- `Freeze`: `EngineRuntime/World/tests/world_tests.cpp:887`
- `IsFrozen`: `EngineRuntime/World/tests/world_tests.cpp:892`


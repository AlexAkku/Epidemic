---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/id_generator.h"
---

# Foundation: id_generator.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/id_generator.h`.

**Модуль:** [[06 EngineFramework/Модуль Foundation|Foundation]].
**Размер исходного файла:** 6922 байт.
**Подключение:** `#include <Epidemic/GameFramework/Foundation/id_generator.h>`.

## Объявления типов (лексический индекс)
`MonotonicIdGenerator`, `Snapshot`, `MonotonicIdGeneratorSnapshotStatus`, `MonotonicIdGeneratorSnapshotValidation`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/error.h"
#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Foundation/ids.h"

#include <cstdint>
#include <limits>
#include <string>
#include <string_view>

namespace epidemic::gameplay
{
template <typename TId> class MonotonicIdGenerator
{
  public:
    struct Snapshot
    {
        std::uint64_t scope = 0;
        std::uint64_t next = 1;
    };

    // Scope 1 is reserved for local/session-only IDs. Persistent or globally
    // addressable IDs must use an explicit deterministic scope (numeric,
    // IdScopeId or FromScopeName) and validate restored snapshots with
    // ValidateMonotonicIdGeneratorSnapshot before calling Restore().
    constexpr MonotonicIdGenerator() noexcept = default;
    explicit constexpr MonotonicIdGenerator(std::uint64_t scope) noexcept : scope_(NormalizeScope(scope))
    {
    }
    explicit constexpr MonotonicIdGenerator(IdScopeId scope) noexcept : scope_(NormalizeScope(scope.Raw()))
    {
    }

    [[nodiscard]] static constexpr MonotonicIdGenerator FromScopeName(std::string_view stable_scope_name) noexcept
    {
        return MonotonicIdGenerator(IdScopeId::FromString(stable_scope_name));
    }

    [[nodiscard]] TId Next() noexcept
    {
        if (next_ == 0)
        {
            return {};
        }
        const auto result = TId::FromRaw(scope_, next_);
        if (next_ == std::numeric_limits<std::uint64_t>::max())
        {
            next_ = 0;
        }
        else
        {
            ++next_;
        }
        return result;
    }

    [[nodiscard]] constexpr bool IsExhausted() const noexcept
    {
        return next_ == 0;
    }
    [[nodiscard]] constexpr IdScopeId Scope() const noexcept
    {
        return IdScopeId::FromRaw(scope_);
    }
    [[nodiscard]] Snapshot GetSnapshot() const noexcept
    {
        return Snapshot{scope_, next_};
    }

    // Low-level structural check. State owners restoring persisted IDs should use
    // ValidateMonotonicIdGeneratorSnapshot instead, because only the owner knows
    // the expected persistent scope and the maximum restored low-part for its
    // records.
    [[nodiscard]] static constexpr bool IsValidSnapshot(Snapshot snapshot) noexcept
    {
        return snapshot.scope != 0;
    }

    [[nodiscard]] bool Restore(Snapshot snapshot) noexcept
    {
        if (!IsValidSnapshot(snapshot))
        {
            return false;
        }
        scope_ = snapshot.scope;
        next_ = snapshot.next;
        return true;
    }

  private:
    [[nodiscard]] static constexpr std::uint64_t NormalizeScope(std::uint64_t scope) noexcept
    {
        return scope == 0 ? 1 : scope;
    }

    std::uint64_t scope_ = 1;
    std::uint64_t next_ = 1;
};

enum class MonotonicIdGeneratorSnapshotStatus
{
    Valid,
    InvalidExpectedScope,
    InvalidSnapshotScope,
    UnexpectedScope,
    SequenceNotAheadOfRestoredIds,
};

struct MonotonicIdGeneratorSnapshotValidation
{
    MonotonicIdGeneratorSnapshotStatus status = MonotonicIdGeneratorSnapshotStatus::Valid;
    std::uint64_t expected_scope = 0;
    std::uint64_t snapshot_scope = 0;
    std::uint64_t snapshot_next = 0;
    std::uint64_t max_restored_low_part = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept
    {
        return status == MonotonicIdGeneratorSnapshotStatus::Valid;
    }

    [[nodiscard]] constexpr explicit operator bool() const noexcept
    {
        return IsValid();
    }

    [[nodiscard]] constexpr std::string_view Code() const noexcept
    {
        switch (status)
        {
        case MonotonicIdGeneratorSnapshotStatus::Valid:
            return "gameplay.id_generator_snapshot_valid";
        case MonotonicIdGeneratorSnapshotStatus::InvalidExpectedScope:
            return "gameplay.id_generator_expected_scope_invalid";
        case MonotonicIdGeneratorSnapshotStatus::InvalidSnapshotScope:
            return "gameplay.id_generator_snapshot_scope_invalid";
        case MonotonicIdGeneratorSnapshotStatus::UnexpectedScope:
            return "gameplay.id_generator_snapshot_scope_mismatch";
        case MonotonicIdGeneratorSnapshotStatus::SequenceNotAheadOfRestoredIds:
            return "gameplay.id_generator_snapshot_sequence_stale";
        }
        return "gameplay.id_generator_snapshot_unknown";
    }
};

template <typename TId>
[[nodiscard]] constexpr MonotonicIdGeneratorSnapshotValidation ValidateMonotonicIdGeneratorSnapshot(
    typename MonotonicIdGenerator<TId>::Snapshot snapshot, IdScopeId expected_scope,
    std::uint64_t max_restored_low_part)
{
    MonotonicIdGeneratorSnapshotValidation result;
    result.expected_scope = expected_scope.Raw();
    result.snapshot_scope = snapshot.scope;
    result.snapshot_next = snapshot.next;
    result.max_restored_low_part = max_restored_low_part;

    if (!expected_scope.IsValid())
    {
        result.status = MonotonicIdGeneratorSnapshotStatus::InvalidExpectedScope;
        return result;
    }
    if (snapshot.scope == 0)
    {
        result.status = MonotonicIdGeneratorSnapshotStatus::InvalidSnapshotScope;
        return result;
    }
    if (snapshot.scope != expected_scope.Raw())
    {
        result.status = MonotonicIdGeneratorSnapshotStatus::UnexpectedScope;
        return result;
    }

    // next == 0 is the explicit exhausted state. Otherwise next must be strictly
    // above every restored low-part, including the UINT64_MAX edge case where
    // only exhausted is valid.
    if (snapshot.next != 0 && snapshot.next <= max_restored_low_part)
    {
        result.status = MonotonicIdGeneratorSnapshotStatus::SequenceNotAheadOfRestoredIds;
        return result;
    }

    result.status = MonotonicIdGeneratorSnapshotStatus::Valid;
    return result;
}

template <typename TId>
[[nodiscard]] foundation::Result<void> RestoreMonotonicIdGeneratorSnapshot(
    MonotonicIdGenerator<TId> &generator, typename MonotonicIdGenerator<TId>::Snapshot snapshot,
    IdScopeId expected_scope, std::uint64_t max_restored_low_part)
{
    const auto validation = ValidateMonotonicIdGeneratorSnapshot<TId>(snapshot, expected_scope, max_restored_low_part);
    if (!validation)
    {
        return foundation::Result<void>::Failure(
            foundation::Error::Create(std::string(validation.Code()), "invalid monotonic id generator snapshot"));
    }
    if (!generator.Restore(snapshot))
    {
        return foundation::Result<void>::Failure(foundation::Error::Create("gameplay.id_generator_snapshot_invalid",
                                                                           "invalid monotonic id generator snapshot"));
    }
    return foundation::Result<void>::Success();
}

} // namespace epidemic::gameplay
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/error.h`, `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Foundation/ids.h`.

### Перечисления и допустимые категории

- `MonotonicIdGeneratorSnapshotStatus`: `Valid`, `InvalidExpectedScope`, `InvalidSnapshotScope`, `UnexpectedScope`, `SequenceNotAheadOfRestoredIds`

### Состав типов и операции

#### `MonotonicIdGenerator` (строка заголовка 14)

Публичные сигнатуры операций:

```cpp
explicit constexpr MonotonicIdGenerator(std::uint64_t scope) noexcept : scope_(NormalizeScope(scope));
explicit constexpr MonotonicIdGenerator(IdScopeId scope) noexcept : scope_(NormalizeScope(scope.Raw()));
[[nodiscard]] static constexpr MonotonicIdGenerator FromScopeName(std::string_view stable_scope_name) noexcept;
[[nodiscard]] TId Next() noexcept;
[[nodiscard]] constexpr bool IsExhausted() const noexcept;
[[nodiscard]] constexpr IdScopeId Scope() const noexcept;
[[nodiscard]] Snapshot GetSnapshot() const noexcept;
[[nodiscard]] bool Restore(Snapshot snapshot) noexcept;
```

#### `Snapshot` (строка заголовка 17)

Публичные поля и значения по умолчанию:

- `std::uint64_t scope = 0`
- `std::uint64_t next = 1`

#### `MonotonicIdGeneratorSnapshotValidation` (строка заголовка 110)

Публичные поля и значения по умолчанию:

- `MonotonicIdGeneratorSnapshotStatus status = MonotonicIdGeneratorSnapshotStatus::Valid`
- `std::uint64_t expected_scope = 0`
- `std::uint64_t snapshot_scope = 0`
- `std::uint64_t snapshot_next = 0`
- `std::uint64_t max_restored_low_part = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr std::string_view Code() const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Next`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:129`
- `GetSnapshot`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:130`
- `Restore`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:132`
- `Scope`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:137`
- `Code`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:143`
- `IsExhausted`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:180`


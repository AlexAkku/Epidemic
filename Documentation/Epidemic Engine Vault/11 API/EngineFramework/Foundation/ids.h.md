---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h"
---

# Foundation: ids.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h`.

**Модуль:** [[06 EngineFramework/Модуль Foundation|Foundation]].
**Размер исходного файла:** 8480 байт.
**Подключение:** `#include <Epidemic/GameFramework/Foundation/ids.h>`.

## Объявления типов (лексический индекс)
`StableId128`, `StrongId128`, `StrongTypeId`, `GameplayObjectIdTag`, `OperationIdTag`, `CorrelationIdTag`, `QueryIdTag`, `EventIdTag`, `FactIdTag`, `ScheduleIdTag`, `GameplayDomainIdTag`, `TypeIdTag`, `QueryTypeIdTag`, `EventTypeIdTag`, `FactTypeIdTag`, `ActionTypeIdTag`, `TagIdTag`, `ClockIdTag`, `SubscriberIdTag`, `ProducerIdTag`, `IdScopeIdTag`, `GameplayObjectRef`, `GameplayObjectPartRef`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include <compare>
#include <cstdint>
#include <functional>
#include <string_view>

namespace epidemic::gameplay
{
namespace detail
{
constexpr std::uint64_t Hash64(std::string_view value, std::uint64_t seed) noexcept
{
    std::uint64_t hash = seed;
    for (const char character : value)
    {
        hash ^= static_cast<std::uint8_t>(character);
        hash *= 1099511628211ull;
    }
    return hash;
}

[[nodiscard]] constexpr bool IsCanonicalNameCharacter(char value) noexcept
{
    return (value >= 'a' && value <= 'z') || (value >= '0' && value <= '9') || value == '_';
}

[[nodiscard]] constexpr bool IsCanonicalName(std::string_view value) noexcept
{
    if (value.empty() || value.front() == '.' || value.back() == '.')
    {
        return false;
    }

    bool previous_was_separator = false;
    for (const char character : value)
    {
        if (character == '.')
        {
            if (previous_was_separator)
            {
                return false;
            }
            previous_was_separator = true;
            continue;
        }
        if (!IsCanonicalNameCharacter(character))
        {
            return false;
        }
        previous_was_separator = false;
    }
    return true;
}

struct StableId128
{
    std::uint64_t high = 0;
    std::uint64_t low = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return high != 0 || low != 0; }
    [[nodiscard]] constexpr bool operator==(const StableId128&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const StableId128&) const noexcept = default;
};

template <typename Tag> struct StrongId128
{
    StableId128 value{};

    [[nodiscard]] static constexpr StrongId128 FromRaw(std::uint64_t high, std::uint64_t low) noexcept
    {
        return StrongId128{StableId128{high, low}};
    }

    [[nodiscard]] static constexpr StrongId128 FromString(std::string_view source) noexcept
    {
        if (source.empty())
        {
            return {};
        }
        constexpr std::uint64_t kSeedA = 14695981039346656037ull;
        constexpr std::uint64_t kSeedB = 1099511628211ull ^ 0x9E3779B97F4A7C15ull;
        auto high = Hash64(source, kSeedA);
        auto low = Hash64(source, kSeedB);
        if (high == 0 && low == 0)
        {
            low = 1;
        }
        return FromRaw(high, low);
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr std::uint64_t High() const noexcept { return value.high; }
    [[nodiscard]] constexpr std::uint64_t Low() const noexcept { return value.low; }
    [[nodiscard]] constexpr bool operator==(const StrongId128&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const StrongId128&) const noexcept = default;
};

template <typename Tag> struct StrongTypeId
{
    std::uint64_t value = 0;

    [[nodiscard]] static constexpr StrongTypeId FromRaw(std::uint64_t raw) noexcept { return StrongTypeId{raw}; }

    [[nodiscard]] static constexpr StrongTypeId FromString(std::string_view source) noexcept
    {
        if (source.empty())
        {
            return {};
        }

        auto raw = Hash64(source, 14695981039346656037ull);
        if (raw == 0)
        {
            raw = 1;
        }
        return StrongTypeId{raw};
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr std::uint64_t Raw() const noexcept { return value; }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const StrongTypeId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const StrongTypeId&) const noexcept = default;
};

struct GameplayObjectIdTag {};
struct OperationIdTag {};
struct CorrelationIdTag {};
struct QueryIdTag {};
struct EventIdTag {};
struct FactIdTag {};
struct ScheduleIdTag {};

struct GameplayDomainIdTag {};
struct TypeIdTag {};
struct QueryTypeIdTag {};
struct EventTypeIdTag {};
struct FactTypeIdTag {};
struct ActionTypeIdTag {};
struct TagIdTag {};
struct ClockIdTag {};
struct SubscriberIdTag {};
struct ProducerIdTag {};
struct IdScopeIdTag {};
} // namespace detail

using GameplayObjectId = detail::StrongId128<detail::GameplayObjectIdTag>;
using OperationId = detail::StrongId128<detail::OperationIdTag>;
using CorrelationId = detail::StrongId128<detail::CorrelationIdTag>;
using QueryId = detail::StrongId128<detail::QueryIdTag>;
using EventId = detail::StrongId128<detail::EventIdTag>;
using FactId = detail::StrongId128<detail::FactIdTag>;
using ScheduleId = detail::StrongId128<detail::ScheduleIdTag>;

using GameplayDomainId = detail::StrongTypeId<detail::GameplayDomainIdTag>;
using TypeId = detail::StrongTypeId<detail::TypeIdTag>;
using QueryTypeId = detail::StrongTypeId<detail::QueryTypeIdTag>;
using EventTypeId = detail::StrongTypeId<detail::EventTypeIdTag>;
using FactTypeId = detail::StrongTypeId<detail::FactTypeIdTag>;
using ActionTypeId = detail::StrongTypeId<detail::ActionTypeIdTag>;
using TagId = detail::StrongTypeId<detail::TagIdTag>;
using ClockId = detail::StrongTypeId<detail::ClockIdTag>;
using SubscriberId = detail::StrongTypeId<detail::SubscriberIdTag>;
using ProducerId = detail::StrongTypeId<detail::ProducerIdTag>;
using IdScopeId = detail::StrongTypeId<detail::IdScopeIdTag>;

struct GameplayObjectRef
{
    GameplayDomainId domain{};
    GameplayObjectId id{};

    [[nodiscard]] constexpr bool IsValid() const noexcept { return domain.IsValid() && id.IsValid(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const GameplayObjectRef&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const GameplayObjectRef&) const noexcept = default;
};

// Generic address of a stable semantic part of a gameplay object. The owning major defines
// the meaning and hierarchy of `part`; Foundation only provides a peer-independent address
// usable by bridges, effects and observations without importing the owner's part type.
struct GameplayObjectPartRef
{
    GameplayObjectRef object{};
    TypeId part{};

    [[nodiscard]] constexpr bool IsValid() const noexcept { return object.IsValid() && part.IsValid(); }
    [[nodiscard]] constexpr explicit operator bool() const noexcept { return IsValid(); }
    [[nodiscard]] constexpr bool operator==(const GameplayObjectPartRef&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const GameplayObjectPartRef&) const noexcept = default;
};
} // namespace epidemic::gameplay

namespace std
{
template <typename Tag> struct hash<epidemic::gameplay::detail::StrongId128<Tag>>
{
    [[nodiscard]] size_t operator()(const epidemic::gameplay::detail::StrongId128<Tag>& id) const noexcept
    {
        const auto a = hash<std::uint64_t>{}(id.High());
        const auto b = hash<std::uint64_t>{}(id.Low());
        return a ^ (b + 0x9E3779B97F4A7C15ull + (a << 6u) + (a >> 2u));
    }
};

template <typename Tag> struct hash<epidemic::gameplay::detail::StrongTypeId<Tag>>
{
    [[nodiscard]] size_t operator()(const epidemic::gameplay::detail::StrongTypeId<Tag>& id) const noexcept
    {
        return hash<std::uint64_t>{}(id.Raw());
    }
};

template <> struct hash<epidemic::gameplay::GameplayObjectRef>
{
    [[nodiscard]] size_t operator()(const epidemic::gameplay::GameplayObjectRef& ref) const noexcept
    {
        const auto a = hash<epidemic::gameplay::GameplayDomainId>{}(ref.domain);
        const auto b = hash<epidemic::gameplay::GameplayObjectId>{}(ref.id);
        return a ^ (b + 0x9E3779B97F4A7C15ull + (a << 6u) + (a >> 2u));
    }
};

template <> struct hash<epidemic::gameplay::GameplayObjectPartRef>
{
    [[nodiscard]] size_t operator()(const epidemic::gameplay::GameplayObjectPartRef& ref) const noexcept
    {
        const auto a = hash<epidemic::gameplay::GameplayObjectRef>{}(ref.object);
        const auto b = hash<epidemic::gameplay::TypeId>{}(ref.part);
        return a ^ (b + 0x9E3779B97F4A7C15ull + (a << 6u) + (a >> 2u));
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Состав типов и операции

#### `StableId128` (строка заголовка 56)

Публичные поля и значения по умолчанию:

- `std::uint64_t high = 0`
- `std::uint64_t low = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const StableId128&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const StableId128&) const noexcept = default;
```

#### `StrongId128` (строка заголовка 66)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr StrongId128 FromRaw(std::uint64_t high, std::uint64_t low) noexcept;
[[nodiscard]] static constexpr StrongId128 FromString(std::string_view source) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr std::uint64_t High() const noexcept;
[[nodiscard]] constexpr std::uint64_t Low() const noexcept;
[[nodiscard]] constexpr bool operator==(const StrongId128&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const StrongId128&) const noexcept = default;
```

#### `StrongTypeId` (строка заголовка 100)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr StrongTypeId FromRaw(std::uint64_t raw) noexcept;
[[nodiscard]] static constexpr StrongTypeId FromString(std::string_view source) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const StrongTypeId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const StrongTypeId&) const noexcept = default;
```

#### `GameplayObjectRef` (строка заголовка 169)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const GameplayObjectRef&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const GameplayObjectRef&) const noexcept = default;
```

#### `GameplayObjectPartRef` (строка заголовка 183)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
[[nodiscard]] constexpr bool operator==(const GameplayObjectPartRef&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const GameplayObjectPartRef&) const noexcept = default;
```

#### `hash` (строка заголовка 197)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(const epidemic::gameplay::detail::StrongId128<Tag>& id) const noexcept;
```

#### `hash` (строка заголовка 207)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(const epidemic::gameplay::detail::StrongTypeId<Tag>& id) const noexcept;
```

#### `hash` (строка заголовка 215)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(const epidemic::gameplay::GameplayObjectRef& ref) const noexcept;
```

#### `hash` (строка заголовка 225)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(const epidemic::gameplay::GameplayObjectPartRef& ref) const noexcept;
```

### Интеграция и безопасность вызовов

- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `High`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:133`
- `Low`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:141`
- `Raw`: `EngineFramework/DevelopmentInfrastructure/Tests/foundation_tests.cpp:368`


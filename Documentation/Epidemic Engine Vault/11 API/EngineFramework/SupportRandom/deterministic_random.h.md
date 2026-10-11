---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/BaseInfrastructure/SupportRandom/include/Epidemic/GameFramework/SupportRandom/deterministic_random.h"
---

# SupportRandom: deterministic_random.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/BaseInfrastructure/SupportRandom/include/Epidemic/GameFramework/SupportRandom/deterministic_random.h`.

**Модуль:** [[06 EngineFramework/Модуль SupportRandom|SupportRandom]].
**Размер исходного файла:** 4376 байт.
**Подключение:** `#include <Epidemic/GameFramework/SupportRandom/deterministic_random.h>`.

## Объявления типов (лексический индекс)
`RandomSeed`, `RandomStream`, `RandomSequenceSnapshot`, `RandomSequence`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/GameFramework/Foundation/gameplay_foundation.h"

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
#include <utility>

namespace epidemic::gameplay::random
{
struct RandomSeed
{
    std::uint64_t value = 0;
    [[nodiscard]] constexpr bool operator==(const RandomSeed&) const noexcept = default;
};

struct RandomStream
{
    TypeId id{};
    [[nodiscard]] static constexpr RandomStream FromString(std::string_view name) noexcept
    {
        return RandomStream{TypeId::FromString(name)};
    }
    [[nodiscard]] constexpr bool IsValid() const noexcept { return id.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const RandomStream&) const noexcept = default;
};

struct RandomSequenceSnapshot
{
    RandomSeed seed{};
    RandomStream stream{};
    std::uint64_t sequence = 0;
};

[[nodiscard]] std::uint64_t StableMix(std::uint64_t value) noexcept;
[[nodiscard]] RandomSeed DeriveSeed(RandomSeed root, RandomStream stream, std::uint64_t salt = 0) noexcept;

class RandomSequence
{
  public:
    RandomSequence(RandomSeed seed, RandomStream stream, std::uint64_t sequence = 0);
    [[nodiscard]] static constexpr bool IsValidSnapshot(const RandomSequenceSnapshot& snapshot) noexcept
    {
        return snapshot.stream.IsValid();
    }
    [[nodiscard]] static std::optional<RandomSequence> TryFromSnapshot(RandomSequenceSnapshot snapshot) noexcept;

    [[nodiscard]] std::optional<std::uint64_t> TryNextU64() noexcept;
    [[nodiscard]] std::optional<std::uint64_t> TryUniform(std::uint64_t exclusive_max) noexcept;
    [[nodiscard]] std::optional<std::uint64_t> TryUniformRange(std::uint64_t minimum,
                                                               std::uint64_t maximum_exclusive) noexcept;
    [[nodiscard]] std::optional<double> TryUniform01() noexcept;
    [[nodiscard]] std::optional<double> TryUniformReal(double minimum, double maximum) noexcept;
    [[nodiscard]] std::optional<bool> TryRollMicro(std::uint32_t chance_micro) noexcept;

    // Unchecked convenience methods are only for call sites that have already proven their preconditions.
    // A violated precondition terminates instead of returning a plausible fallback value in release builds.
    [[nodiscard]] std::uint64_t NextU64Unchecked() noexcept;
    [[nodiscard]] std::uint64_t UniformUnchecked(std::uint64_t exclusive_max) noexcept;
    [[nodiscard]] std::uint64_t UniformRangeUnchecked(std::uint64_t minimum,
                                                      std::uint64_t maximum_exclusive) noexcept;
    [[nodiscard]] double Uniform01Unchecked() noexcept;
    [[nodiscard]] double UniformRealUnchecked(double minimum, double maximum) noexcept;
    [[nodiscard]] bool RollMicroUnchecked(std::uint32_t chance_micro) noexcept;
    [[nodiscard]] std::optional<std::size_t> WeightedIndex(std::span<const std::uint64_t> weights) noexcept;

    template <typename T> void ShuffleUnchecked(std::span<T> values) noexcept(std::is_nothrow_swappable_v<T>)
    {
        if (values.size() < 2)
        {
            return;
        }

        for (std::size_t index = values.size() - 1; index > 0; --index)
        {
            const auto swap_index = static_cast<std::size_t>(UniformUnchecked(static_cast<std::uint64_t>(index + 1)));
            using std::swap;
            swap(values[index], values[swap_index]);
        }
    }

    template <typename TContainer> void ShuffleUnchecked(TContainer& values) noexcept(noexcept(ShuffleUnchecked(std::span{values.data(), values.size()})))
    {
        ShuffleUnchecked(std::span{values.data(), values.size()});
    }

    [[nodiscard]] RandomSequenceSnapshot CaptureSnapshot() const noexcept { return {seed_, stream_, sequence_}; }
    [[nodiscard]] bool TryRestoreSnapshot(RandomSequenceSnapshot snapshot) noexcept;

    [[nodiscard]] std::uint64_t Sequence() const noexcept { return sequence_; }
    [[nodiscard]] bool IsExhausted() const noexcept { return sequence_ == std::numeric_limits<std::uint64_t>::max(); }

  private:
    RandomSeed seed_{};
    RandomStream stream_{};
    std::uint64_t sequence_ = 0;
};
} // namespace epidemic::gameplay::random
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/GameFramework/Foundation/gameplay_foundation.h`.

### Состав типов и операции

#### `RandomSeed` (строка заголовка 18)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const RandomSeed&) const noexcept = default;
```

#### `RandomStream` (строка заголовка 24)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] static constexpr RandomStream FromString(std::string_view name) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const RandomStream&) const noexcept = default;
```

#### `RandomSequenceSnapshot` (строка заголовка 35)

Публичные поля и значения по умолчанию:

- `std::uint64_t sequence = 0`

#### `RandomSequence` (строка заголовка 45)

Публичные сигнатуры операций:

```cpp
RandomSequence(RandomSeed seed, RandomStream stream, std::uint64_t sequence = 0);
[[nodiscard]] static constexpr bool IsValidSnapshot(const RandomSequenceSnapshot& snapshot) noexcept;
[[nodiscard]] static std::optional<RandomSequence> TryFromSnapshot(RandomSequenceSnapshot snapshot) noexcept;
[[nodiscard]] std::optional<std::uint64_t> TryNextU64() noexcept;
[[nodiscard]] std::optional<std::uint64_t> TryUniform(std::uint64_t exclusive_max) noexcept;
[[nodiscard]] std::optional<std::uint64_t> TryUniformRange(std::uint64_t minimum, std::uint64_t maximum_exclusive) noexcept;
[[nodiscard]] std::optional<double> TryUniform01() noexcept;
[[nodiscard]] std::optional<double> TryUniformReal(double minimum, double maximum) noexcept;
[[nodiscard]] std::optional<bool> TryRollMicro(std::uint32_t chance_micro) noexcept;
[[nodiscard]] std::uint64_t UniformUnchecked(std::uint64_t exclusive_max) noexcept;
[[nodiscard]] std::uint64_t UniformRangeUnchecked(std::uint64_t minimum, std::uint64_t maximum_exclusive) noexcept;
[[nodiscard]] double Uniform01Unchecked() noexcept;
[[nodiscard]] double UniformRealUnchecked(double minimum, double maximum) noexcept;
[[nodiscard]] bool RollMicroUnchecked(std::uint32_t chance_micro) noexcept;
[[nodiscard]] std::optional<std::size_t> WeightedIndex(std::span<const std::uint64_t> weights) noexcept;
template <typename T> void ShuffleUnchecked(std::span<T> values) noexcept(std::is_nothrow_swappable_v<T>);
template <typename TContainer> void ShuffleUnchecked(TContainer& values) noexcept(noexcept(ShuffleUnchecked(std::span;
[[nodiscard]] RandomSequenceSnapshot CaptureSnapshot() const noexcept;
[[nodiscard]] bool TryRestoreSnapshot(RandomSequenceSnapshot snapshot) noexcept;
[[nodiscard]] std::uint64_t Sequence() const noexcept;
[[nodiscard]] bool IsExhausted() const noexcept;
```

### Интеграция и безопасность вызовов

- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RandomSequence` | `RandomSequence::RandomSequence` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:34` |
| `TryFromSnapshot` | `RandomSequence::TryFromSnapshot` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:43` |
| `TryNextU64` | `RandomSequence::TryNextU64` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:52` |
| `TryUniform` | `RandomSequence::TryUniform` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:63` |
| `TryUniformRange` | `RandomSequence::TryUniformRange` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:89` |
| `TryUniform01` | `RandomSequence::TryUniform01` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:104` |
| `TryUniformReal` | `RandomSequence::TryUniformReal` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:115` |
| `TryRollMicro` | `RandomSequence::TryRollMicro` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:149` |
| `UniformUnchecked` | `RandomSequence::UniformUnchecked` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:176` |
| `UniformRangeUnchecked` | `RandomSequence::UniformRangeUnchecked` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:181` |
| `Uniform01Unchecked` | `RandomSequence::Uniform01Unchecked` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:187` |
| `UniformRealUnchecked` | `RandomSequence::UniformRealUnchecked` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:192` |
| `RollMicroUnchecked` | `RandomSequence::RollMicroUnchecked` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:197` |
| `WeightedIndex` | `RandomSequence::WeightedIndex` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:202` |
| `TryRestoreSnapshot` | `RandomSequence::TryRestoreSnapshot` | `EngineFramework/BaseInfrastructure/SupportRandom/src/deterministic_random.cpp:235` |


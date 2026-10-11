---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h"
---

# Animation: animation_types.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`.

**Модуль:** [[05 EngineRuntime/Модуль Animation|Animation]].
**Размер исходного файла:** 5434 байт.
**Подключение:** `#include <Epidemic/Runtime/Animation/animation_types.h>`.

## Объявления типов (лексический индекс)
`SkeletonId`, `AnimationClipId`, `AnimatorInstanceId`, `AnimatorLifecycle`, `AnimatorReadiness`, `AnimatorPlaybackState`, `PoseState`, `AnimationLodLevel`, `SkeletonDesc`, `AnimationClipDesc`, `AnimatorPlayback`, `CrossfadeState`, `AnimatorDesc`, `AnimationEvent`, `AnimatorHandle`, `PoseSnapshot`, `PoseBuffer`, `AnimatorSnapshot`, `AnimationEvaluationRequest`, `AnimationPlaybackCommand`, `AnimationOptions`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/spatial.h"

#include <chrono>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace epidemic::runtime::animation
{
// Public value types for the Animation major. They model runtime animation data identities,
// animator lifecycle states, generic events and LOD hints without defining higher-level actor behavior.

struct SkeletonId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const SkeletonId&) const noexcept = default;
};

struct AnimationClipId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const AnimationClipId&) const noexcept = default;
};

struct AnimatorInstanceId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const AnimatorInstanceId&) const noexcept = default;
};

using FrameDuration = RuntimeFrameDuration;

enum class AnimatorLifecycle
{
    Uninitialized,
    Ready,
    Disabled
};

enum class AnimatorReadiness
{
    Ready,
    ResourceMissing
};

enum class AnimatorPlaybackState
{
    Stopped,
    Playing,
    Paused,
    Blending,
    Finished
};

enum class PoseState
{
    Clean,
    Dirty,
    Evaluating,
    Ready
};

enum class AnimationLodLevel
{
    Full,
    Reduced,
    Frozen
};

struct SkeletonDesc
{
    SkeletonId id{};
    std::uint32_t joint_count = 0;
};

struct AnimationClipDesc
{
    AnimationClipId id{};
    SkeletonId skeleton{};
    float duration_seconds = 0.0f;
};

struct AnimatorPlayback
{
    AnimationClipId clip{};
    bool loop = false;
    double playback_rate = 1.0;
    FrameDuration local_time{};
    double fractional_microseconds = 0.0;
};

struct CrossfadeState
{
    AnimationClipId source_clip{};
    AnimationClipId target_clip{};
    FrameDuration source_time{};
    FrameDuration target_time{};
    FrameDuration elapsed{};
    FrameDuration duration{};
    float source_weight = 1.0f;
    float target_weight = 0.0f;
};

struct AnimatorDesc
{
    RuntimeObjectId owner{};
    SkeletonId skeleton{};
    AnimationLodLevel lod = AnimationLodLevel::Full;
};

struct AnimationEvent
{
    AnimatorInstanceId animator{};
    std::string name;
    float time = 0.0f;
};

struct AnimatorHandle
{
    AnimatorInstanceId id{};
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return id.IsValid() && generation != 0; }
    [[nodiscard]] constexpr bool operator==(const AnimatorHandle&) const noexcept = default;
};

struct PoseSnapshot
{
    AnimatorInstanceId animator{};
    AnimatorHandle handle{};
    PoseState state = PoseState::Clean;
    AnimationLodLevel lod = AnimationLodLevel::Full;
    std::uint64_t revision = 0;
};

struct PoseBuffer
{
    AnimatorHandle animator{};
    RuntimeObjectId owner{};
    std::vector<Transform> bone_transforms{};
    std::uint64_t revision = 0;
};

struct AnimatorSnapshot
{
    AnimatorHandle handle{};
    AnimatorLifecycle lifecycle = AnimatorLifecycle::Uninitialized;
    AnimatorReadiness readiness = AnimatorReadiness::ResourceMissing;
    AnimatorPlaybackState playback = AnimatorPlaybackState::Stopped;
    AnimationLodLevel lod = AnimationLodLevel::Full;
    AnimationClipId clip{};
    FrameDuration local_time{};
    std::uint64_t revision = 0;
};

struct AnimationEvaluationRequest
{
    AnimatorHandle animator{};
    RuntimeObjectId owner{};
    SkeletonDesc skeleton{};
    AnimationClipDesc source_clip{};
    AnimationClipDesc target_clip{};
    FrameDuration source_time{};
    FrameDuration target_time{};
    float source_weight = 1.0f;
    float target_weight = 0.0f;
    AnimationLodLevel lod = AnimationLodLevel::Full;
    std::uint64_t revision = 0;
};

struct AnimationPlaybackCommand
{
    AnimatorHandle animator{};
    AnimationClipId clip{};
    bool loop = false;
    double playback_rate = 1.0;
};

struct AnimationOptions
{
    bool enable_mock_pose_evaluation = false;
    std::size_t event_capacity = 64;
    std::size_t max_animators = 4096;
    std::uint32_t max_skeleton_joints = 4096;
};
} // namespace epidemic::runtime::animation

namespace std
{
template <> struct hash<epidemic::runtime::animation::SkeletonId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::animation::SkeletonId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::animation::AnimationClipId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::animation::AnimationClipId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::animation::AnimatorInstanceId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::animation::AnimatorInstanceId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};
} // namespace std
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_time.h`, `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/spatial.h`.

### Перечисления и допустимые категории

- `AnimatorLifecycle`: `Uninitialized`, `Ready`, `Disabled`
- `AnimatorReadiness`: `Ready`, `ResourceMissing`
- `AnimatorPlaybackState`: `Stopped`, `Playing`, `Paused`, `Blending`, `Finished`
- `PoseState`: `Clean`, `Dirty`, `Evaluating`, `Ready`
- `AnimationLodLevel`: `Full`, `Reduced`, `Frozen`

### Состав типов и операции

#### `SkeletonId` (строка заголовка 18)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const SkeletonId&) const noexcept = default;
```

#### `AnimationClipId` (строка заголовка 26)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AnimationClipId&) const noexcept = default;
```

#### `AnimatorInstanceId` (строка заголовка 34)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AnimatorInstanceId&) const noexcept = default;
```

#### `SkeletonDesc` (строка заголовка 81)

Публичные поля и значения по умолчанию:

- `std::uint32_t joint_count = 0`

#### `AnimationClipDesc` (строка заголовка 87)

Публичные поля и значения по умолчанию:

- `float duration_seconds = 0.0f`

#### `AnimatorPlayback` (строка заголовка 94)

Публичные поля и значения по умолчанию:

- `bool loop = false`
- `double playback_rate = 1.0`
- `double fractional_microseconds = 0.0`

#### `CrossfadeState` (строка заголовка 103)

Публичные поля и значения по умолчанию:

- `float source_weight = 1.0f`
- `float target_weight = 0.0f`

#### `AnimatorDesc` (строка заголовка 115)

Публичные поля и значения по умолчанию:

- `AnimationLodLevel lod = AnimationLodLevel::Full`

#### `AnimationEvent` (строка заголовка 122)

Публичные поля и значения по умолчанию:

- `std::string name`
- `float time = 0.0f`

#### `AnimatorHandle` (строка заголовка 129)

Публичные поля и значения по умолчанию:

- `std::uint32_t generation = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AnimatorHandle&) const noexcept = default;
```

#### `PoseSnapshot` (строка заголовка 138)

Публичные поля и значения по умолчанию:

- `PoseState state = PoseState::Clean`
- `AnimationLodLevel lod = AnimationLodLevel::Full`
- `std::uint64_t revision = 0`

#### `PoseBuffer` (строка заголовка 147)

Публичные поля и значения по умолчанию:

- `std::uint64_t revision = 0`

#### `AnimatorSnapshot` (строка заголовка 155)

Публичные поля и значения по умолчанию:

- `AnimatorLifecycle lifecycle = AnimatorLifecycle::Uninitialized`
- `AnimatorReadiness readiness = AnimatorReadiness::ResourceMissing`
- `AnimatorPlaybackState playback = AnimatorPlaybackState::Stopped`
- `AnimationLodLevel lod = AnimationLodLevel::Full`
- `std::uint64_t revision = 0`

#### `AnimationEvaluationRequest` (строка заголовка 167)

Публичные поля и значения по умолчанию:

- `float source_weight = 1.0f`
- `float target_weight = 0.0f`
- `AnimationLodLevel lod = AnimationLodLevel::Full`
- `std::uint64_t revision = 0`

#### `AnimationPlaybackCommand` (строка заголовка 182)

Публичные поля и значения по умолчанию:

- `bool loop = false`
- `double playback_rate = 1.0`

#### `AnimationOptions` (строка заголовка 190)

Публичные поля и значения по умолчанию:

- `bool enable_mock_pose_evaluation = false`
- `std::size_t event_capacity = 64`
- `std::size_t max_animators = 4096`
- `std::uint32_t max_skeleton_joints = 4096`

#### `hash` (строка заголовка 201)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::animation::SkeletonId value) const noexcept;
```

#### `hash` (строка заголовка 209)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::animation::AnimationClipId value) const noexcept;
```

#### `hash` (строка заголовка 217)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::animation::AnimatorInstanceId value) const noexcept;
```

### Интеграция и безопасность вызовов

- Перед вызовом проверяйте валидность идентификаторов и соответствие объекта жизненному циклу сервисов; гарантии конкретной операции зафиксированы в заголовке и тестах.


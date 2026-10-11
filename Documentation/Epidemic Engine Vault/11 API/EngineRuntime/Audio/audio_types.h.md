---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h"
---

# Audio: audio_types.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`.

**Модуль:** [[05 EngineRuntime/Модуль Audio|Audio]].
**Размер исходного файла:** 7117 байт.
**Подключение:** `#include <Epidemic/Runtime/Audio/audio_types.h>`.

## Объявления типов (лексический индекс)
`SoundId`, `AudioEmitterId`, `AudioListenerId`, `BackendVoiceHandle`, `MixerGroupId`, `AudioEmitterHandle`, `AudioListenerHandle`, `AudioEventSpace`, `AudioEventOverflowPolicy`, `SoundState`, `EmitterState`, `MixerFadeState`, `SoundDesc`, `AudioClipPayload`, `AudioClipStorage`, `AudioClipFormat`, `IAudioClipResource`, `AudioStreamReadResult`, `IAudioStreamSource`, `AudioBackendOptions`, `AudioSpatialState`, `AudioEmitterDesc`, `AudioVoiceDesc`, `AudioEmitterSnapshot`, `AudioListenerDesc`, `AudioEvent`, `MixerGroupState`, `AudioOptions`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Runtime/Foundation/runtime_ids.h"
#include "Epidemic/Runtime/Foundation/runtime_time.h"
#include "Epidemic/Runtime/Foundation/spatial.h"
#include "Epidemic/Foundation/result.h"

#include <cstdint>
#include <cstddef>
#include <functional>
#include <memory>
#include <span>
#include <string>

namespace epidemic::runtime::audio
{
// Public value types for the Audio major. These records describe sound resources, emitters, listeners,
// mixer state and queued one-shot events without binding to a concrete audio backend.

struct SoundId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const SoundId&) const noexcept = default;
};

struct AudioEmitterId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const AudioEmitterId&) const noexcept = default;
};

struct AudioListenerId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const AudioListenerId&) const noexcept = default;
};

struct BackendVoiceHandle
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const BackendVoiceHandle&) const noexcept = default;
};

struct MixerGroupId
{
    std::uint64_t value = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value != 0; }
    [[nodiscard]] constexpr bool operator==(const MixerGroupId&) const noexcept = default;
};

struct AudioEmitterHandle
{
    AudioEmitterId id{};
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return id.IsValid() && generation != 0; }
    [[nodiscard]] constexpr bool operator==(const AudioEmitterHandle&) const noexcept = default;
};

struct AudioListenerHandle
{
    AudioListenerId id{};
    std::uint32_t generation = 0;

    [[nodiscard]] constexpr bool IsValid() const noexcept { return id.IsValid() && generation != 0; }
    [[nodiscard]] constexpr bool operator==(const AudioListenerHandle&) const noexcept = default;
};

using AudioTransformId = RuntimeObjectId;

enum class AudioEventSpace
{
    NonSpatial,
    WorldPosition
};

enum class AudioEventOverflowPolicy
{
    DropNewest,
    DropOldest,
    FailSubmit
};

enum class SoundState
{
    Missing,
    Loading,
    Ready,
    Failed
};

enum class EmitterState
{
    Stopped,
    Playing,
    Paused,
    FadingIn,
    FadingOut,
    Virtualized,
    Destroyed
};

enum class MixerFadeState
{
    Stable,
    FadingIn,
    FadingOut
};

struct SoundDesc
{
    SoundId id{};
    SoundState state = SoundState::Ready;
};

struct AudioClipPayload
{
    SoundId sound{};
    SoundState state = SoundState::Ready;
    std::shared_ptr<const class IAudioClipResource> resource;
};

enum class AudioClipStorage
{
    InMemoryEncoded,
    Streaming
};

struct AudioClipFormat
{
    std::uint32_t channel_count = 0;
    std::uint32_t sample_rate_hz = 0;
    std::uint32_t bits_per_sample = 0;
    bool compressed = false;
};

class IAudioClipResource
{
public:
    virtual ~IAudioClipResource() = default;

    [[nodiscard]] virtual AudioClipFormat GetFormat() const = 0;
    [[nodiscard]] virtual AudioClipStorage GetStorage() const = 0;
    [[nodiscard]] virtual std::span<const std::byte> GetEncodedData() const = 0;
    [[nodiscard]] virtual std::shared_ptr<class IAudioStreamSource> GetStreamSource() const = 0;
};

struct AudioStreamReadResult
{
    std::size_t bytes_read = 0;
    bool end_of_stream = false;
};

class IAudioStreamSource
{
public:
    virtual ~IAudioStreamSource() = default;

    [[nodiscard]] virtual std::uint64_t GetSizeBytes() const = 0;
    [[nodiscard]] virtual bool IsSeekable() const = 0;
    [[nodiscard]] virtual foundation::Result<AudioStreamReadResult> Read(std::uint64_t offset, std::span<std::byte> output) = 0;
};

struct AudioBackendOptions
{
    bool enabled = true;
};

struct AudioSpatialState
{
    Transform transform{};
    bool spatial = true;
};

struct AudioEmitterDesc
{
    RuntimeObjectId owner{};
    SoundId sound{};
    AudioTransformId transform{};
    bool loop = false;
    MixerGroupId mixer_group{};
    float gain = 1.0f;
    bool spatial = true;
};

struct AudioVoiceDesc
{
    AudioClipPayload clip{};
    bool loop = false;
    MixerGroupId mixer_group{};
    float initial_gain = 1.0f;
    AudioSpatialState spatial{};
};

struct AudioEmitterSnapshot
{
    AudioEmitterId id{};
    AudioEmitterHandle handle{};
    SoundId sound{};
    AudioTransformId transform{};
    EmitterState state = EmitterState::Destroyed;
    RuntimeFrameDuration fade_duration{};
    float fade_progress = 0.0f;
    std::uint64_t revision = 0;
};

struct AudioListenerDesc
{
    AudioTransformId transform{};
};

struct AudioEvent
{
    SoundId sound{};
    Vec3 position{};
    float volume = 1.0f;
    AudioEventSpace space = AudioEventSpace::WorldPosition;
};

struct MixerGroupState
{
    MixerGroupId id{};
    float volume = 1.0f;
    MixerFadeState fade_state = MixerFadeState::Stable;
    MixerGroupId parent{};
    RuntimeFrameDuration fade_duration{};
    float fade_progress = 0.0f;
};

struct AudioOptions
{
    bool enable_mock_backend = true;
    std::uint32_t max_queued_events = 64;
    AudioEventOverflowPolicy event_overflow_policy = AudioEventOverflowPolicy::FailSubmit;
    bool allow_mixer_boost = false;
};
} // namespace epidemic::runtime::audio

namespace std
{
template <> struct hash<epidemic::runtime::audio::SoundId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::audio::SoundId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::audio::AudioEmitterId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::audio::AudioEmitterId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::audio::AudioListenerId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::audio::AudioListenerId value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::audio::BackendVoiceHandle>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::audio::BackendVoiceHandle value) const noexcept
    {
        return hash<std::uint64_t>{}(value.value);
    }
};

template <> struct hash<epidemic::runtime::audio::MixerGroupId>
{
    [[nodiscard]] size_t operator()(epidemic::runtime::audio::MixerGroupId value) const noexcept
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

Заголовок непосредственно подключает: `Epidemic/Runtime/Foundation/runtime_ids.h`, `Epidemic/Runtime/Foundation/runtime_time.h`, `Epidemic/Runtime/Foundation/spatial.h`, `Epidemic/Foundation/result.h`.

### Перечисления и допустимые категории

- `AudioEventSpace`: `NonSpatial`, `WorldPosition`
- `AudioEventOverflowPolicy`: `DropNewest`, `DropOldest`, `FailSubmit`
- `SoundState`: `Missing`, `Loading`, `Ready`, `Failed`
- `EmitterState`: `Stopped`, `Playing`, `Paused`, `FadingIn`, `FadingOut`, `Virtualized`, `Destroyed`
- `MixerFadeState`: `Stable`, `FadingIn`, `FadingOut`
- `AudioClipStorage`: `InMemoryEncoded`, `Streaming`

### Состав типов и операции

#### `SoundId` (строка заголовка 20)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const SoundId&) const noexcept = default;
```

#### `AudioEmitterId` (строка заголовка 28)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AudioEmitterId&) const noexcept = default;
```

#### `AudioListenerId` (строка заголовка 36)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AudioListenerId&) const noexcept = default;
```

#### `BackendVoiceHandle` (строка заголовка 44)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const BackendVoiceHandle&) const noexcept = default;
```

#### `MixerGroupId` (строка заголовка 52)

Публичные поля и значения по умолчанию:

- `std::uint64_t value = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const MixerGroupId&) const noexcept = default;
```

#### `AudioEmitterHandle` (строка заголовка 60)

Публичные поля и значения по умолчанию:

- `std::uint32_t generation = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AudioEmitterHandle&) const noexcept = default;
```

#### `AudioListenerHandle` (строка заголовка 69)

Публичные поля и значения по умолчанию:

- `std::uint32_t generation = 0`

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const AudioListenerHandle&) const noexcept = default;
```

#### `SoundDesc` (строка заголовка 119)

Публичные поля и значения по умолчанию:

- `SoundState state = SoundState::Ready`

#### `AudioClipPayload` (строка заголовка 125)

Публичные поля и значения по умолчанию:

- `SoundState state = SoundState::Ready`
- `std::shared_ptr<const class IAudioClipResource> resource`

#### `AudioClipFormat` (строка заголовка 138)

Публичные поля и значения по умолчанию:

- `std::uint32_t channel_count = 0`
- `std::uint32_t sample_rate_hz = 0`
- `std::uint32_t bits_per_sample = 0`
- `bool compressed = false`

#### `IAudioClipResource` (строка заголовка 146)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioClipResource() = default;
[[nodiscard]] virtual AudioClipFormat GetFormat() const = 0;
[[nodiscard]] virtual AudioClipStorage GetStorage() const = 0;
[[nodiscard]] virtual std::span<const std::byte> GetEncodedData() const = 0;
[[nodiscard]] virtual std::shared_ptr<class IAudioStreamSource> GetStreamSource() const = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `AudioStreamReadResult` (строка заголовка 157)

Публичные поля и значения по умолчанию:

- `std::size_t bytes_read = 0`
- `bool end_of_stream = false`

#### `IAudioStreamSource` (строка заголовка 163)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioStreamSource() = default;
[[nodiscard]] virtual std::uint64_t GetSizeBytes() const = 0;
[[nodiscard]] virtual bool IsSeekable() const = 0;
[[nodiscard]] virtual foundation::Result<AudioStreamReadResult> Read(std::uint64_t offset, std::span<std::byte> output) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `AudioBackendOptions` (строка заголовка 173)

Публичные поля и значения по умолчанию:

- `bool enabled = true`

#### `AudioSpatialState` (строка заголовка 178)

Публичные поля и значения по умолчанию:

- `bool spatial = true`

#### `AudioEmitterDesc` (строка заголовка 184)

Публичные поля и значения по умолчанию:

- `bool loop = false`
- `float gain = 1.0f`
- `bool spatial = true`

#### `AudioVoiceDesc` (строка заголовка 195)

Публичные поля и значения по умолчанию:

- `bool loop = false`
- `float initial_gain = 1.0f`

#### `AudioEmitterSnapshot` (строка заголовка 204)

Публичные поля и значения по умолчанию:

- `EmitterState state = EmitterState::Destroyed`
- `float fade_progress = 0.0f`
- `std::uint64_t revision = 0`

#### `AudioEvent` (строка заголовка 221)

Публичные поля и значения по умолчанию:

- `float volume = 1.0f`
- `AudioEventSpace space = AudioEventSpace::WorldPosition`

#### `MixerGroupState` (строка заголовка 229)

Публичные поля и значения по умолчанию:

- `float volume = 1.0f`
- `MixerFadeState fade_state = MixerFadeState::Stable`
- `float fade_progress = 0.0f`

#### `AudioOptions` (строка заголовка 239)

Публичные поля и значения по умолчанию:

- `bool enable_mock_backend = true`
- `std::uint32_t max_queued_events = 64`
- `AudioEventOverflowPolicy event_overflow_policy = AudioEventOverflowPolicy::FailSubmit`
- `bool allow_mixer_boost = false`

#### `hash` (строка заголовка 250)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::audio::SoundId value) const noexcept;
```

#### `hash` (строка заголовка 258)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::audio::AudioEmitterId value) const noexcept;
```

#### `hash` (строка заголовка 266)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::audio::AudioListenerId value) const noexcept;
```

#### `hash` (строка заголовка 274)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::audio::BackendVoiceHandle value) const noexcept;
```

#### `hash` (строка заголовка 282)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::audio::MixerGroupId value) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Read`: `EngineRuntime/Audio/tests/audio_tests.cpp:759`


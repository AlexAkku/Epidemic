---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h"
---

# Audio: audio_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Audio|Audio]].
**Размер исходного файла:** 6862 байт.
**Подключение:** `#include <Epidemic/Runtime/Audio/audio_runtime.h>`.

## Объявления типов (лексический индекс)
`ISoundRegistry`, `IAudioBackend`, `IAudioResourceSource`, `IAudioTransformSource`, `IAudioRuntime`, `IListenerSystem`, `IAudioEventQueue`, `IMixerSystem`, `AudioDependencies`, `AudioServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Audio/audio_types.h"

#include <memory>
#include <optional>
#include <span>

namespace epidemic::runtime::audio
{
// Public contracts for Audio runtime services. The interfaces expose registry, emitter, listener,
// mixer and event-queue behavior while keeping backend ownership behind the implementation boundary.

class ISoundRegistry
{
public:
    virtual ~ISoundRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterSound(SoundDesc desc) = 0;

    [[nodiscard]] virtual SoundState GetSoundState(SoundId id) const = 0;
};

class IAudioBackend
{
public:
    virtual ~IAudioBackend() = default;

    // Freeze contract for retryable Runtime projection:
    //  * Initialize failure/exception means Audio services were not created.
    //  * CreateVoice/CreateBackendListener success transfers one backend object to the caller; failure/exception
    //    means no object was acquired.
    //  * Setters and Play/Pause/Stop are safe to repeat with the same handle/value. A reported failure/exception
    //    either leaves that command unapplied or permits an exact retry without duplicating a semantic effect.
    //  * DestroyVoice/DestroyBackendListener failure/exception leaves ownership alive and retryable.
    //  * Update(delta) failure/exception means delta was not accepted or temporally advanced. Retrying the same
    //    delta after a failed Update therefore advances backend time exactly once.
    [[nodiscard]] virtual bool IsEnabled() const = 0;
    [[nodiscard]] virtual foundation::Result<void> Initialize(const AudioBackendOptions& options) = 0;
    [[nodiscard]] virtual foundation::Result<BackendVoiceHandle> CreateVoice(const AudioVoiceDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyVoice(BackendVoiceHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> Play(BackendVoiceHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> Pause(BackendVoiceHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> Stop(BackendVoiceHandle handle) = 0;
    [[nodiscard]] virtual bool IsVoiceFinished(BackendVoiceHandle handle) const = 0;
    [[nodiscard]] virtual foundation::Result<void> SetGain(BackendVoiceHandle handle, float gain) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetSpatialState(BackendVoiceHandle handle, const AudioSpatialState& state) = 0;
    [[nodiscard]] virtual foundation::Result<void> CreateBackendListener(AudioListenerHandle handle, const Transform& transform) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyBackendListener(AudioListenerHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> SetBackendListenerTransform(AudioListenerHandle handle, const Transform& transform) = 0;
    [[nodiscard]] virtual foundation::Result<void> Update(RuntimeFrameDuration delta) = 0;
};

class IAudioResourceSource
{
public:
    virtual ~IAudioResourceSource() = default;

    [[nodiscard]] virtual SoundState GetSoundState(SoundId id) const = 0;
    [[nodiscard]] virtual foundation::Result<AudioClipPayload> LoadClip(SoundId id) const = 0;
};

class IAudioTransformSource
{
public:
    virtual ~IAudioTransformSource() = default;

    [[nodiscard]] virtual foundation::Result<Transform> ReadTransform(AudioTransformId id) const = 0;
};

class IAudioRuntime
{
public:
    virtual ~IAudioRuntime() = default;

    [[nodiscard]] virtual foundation::Result<AudioEmitterHandle> CreateEmitterHandle(const AudioEmitterDesc& desc) = 0;

    [[nodiscard]] virtual foundation::Result<void> DestroyEmitter(AudioEmitterHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> Play(AudioEmitterHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> Pause(AudioEmitterHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> Resume(AudioEmitterHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> Stop(AudioEmitterHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> FadeOut(AudioEmitterHandle handle, RuntimeFrameDuration duration) = 0;

    [[nodiscard]] virtual foundation::Result<void> FadeIn(AudioEmitterHandle handle, RuntimeFrameDuration duration) = 0;

    // Virtualization releases the backend voice; calling Play(handle) later restarts playback from the clip beginning.
    [[nodiscard]] virtual foundation::Result<void> Virtualize(AudioEmitterHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<EmitterState> GetEmitterState(AudioEmitterHandle handle) const = 0;

    [[nodiscard]] virtual foundation::Result<AudioEmitterSnapshot> GetEmitterSnapshot(AudioEmitterHandle handle) const = 0;
    [[nodiscard]] virtual foundation::Result<void> Tick(RuntimeFrameDuration delta) = 0;
    [[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
};

class IListenerSystem
{
public:
    virtual ~IListenerSystem() = default;

    [[nodiscard]] virtual foundation::Result<AudioListenerHandle> CreateListenerHandle(const AudioListenerDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyListener(AudioListenerHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> SetMainListener(AudioListenerHandle handle) = 0;

    [[nodiscard]] virtual std::optional<AudioListenerHandle> GetMainListener() const = 0;
};

class IAudioEventQueue
{
public:
    virtual ~IAudioEventQueue() = default;

    [[nodiscard]] virtual foundation::Result<void> SubmitOneShot(const AudioEvent& event) = 0;

    [[nodiscard]] virtual std::span<const AudioEvent> Events() const = 0;

    virtual void Clear() = 0;
};

class IMixerSystem
{
public:
    virtual ~IMixerSystem() = default;

    [[nodiscard]] virtual foundation::Result<void> SetMixerGroup(MixerGroupState state) = 0;

    [[nodiscard]] virtual std::optional<MixerGroupState> GetMixerGroup(MixerGroupId id) const = 0;
};

struct AudioDependencies
{
    std::shared_ptr<IAudioBackend> backend;
    std::shared_ptr<IAudioResourceSource> resources;
    std::shared_ptr<IAudioTransformSource> transforms;
};

struct AudioServices
{
    std::shared_ptr<ISoundRegistry> sounds;
    std::shared_ptr<IAudioRuntime> runtime;
    std::shared_ptr<IListenerSystem> listeners;
    std::shared_ptr<IAudioEventQueue> events;
    std::shared_ptr<IMixerSystem> mixer;
    std::shared_ptr<IAudioBackend> backend;
};

[[nodiscard]] foundation::Result<AudioServices> CreateAudioServices(
    AudioOptions options = {},
    AudioDependencies dependencies = {});
[[nodiscard]] foundation::Result<AudioServices> CreateMockAudioServices(AudioOptions options = {});
} // namespace epidemic::runtime::audio
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Audio/audio_types.h`.

### Состав типов и операции

#### `ISoundRegistry` (строка заголовка 15)

Публичные сигнатуры операций:

```cpp
virtual ~ISoundRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterSound(SoundDesc desc) = 0;
[[nodiscard]] virtual SoundState GetSoundState(SoundId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAudioBackend` (строка заголовка 25)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioBackend() = default;
[[nodiscard]] virtual foundation::Result<void> Initialize(const AudioBackendOptions& options) = 0;
[[nodiscard]] virtual foundation::Result<BackendVoiceHandle> CreateVoice(const AudioVoiceDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyVoice(BackendVoiceHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Play(BackendVoiceHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Pause(BackendVoiceHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Stop(BackendVoiceHandle handle) = 0;
[[nodiscard]] virtual bool IsVoiceFinished(BackendVoiceHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetGain(BackendVoiceHandle handle, float gain) = 0;
[[nodiscard]] virtual foundation::Result<void> SetSpatialState(BackendVoiceHandle handle, const AudioSpatialState& state) = 0;
[[nodiscard]] virtual foundation::Result<void> CreateBackendListener(AudioListenerHandle handle, const Transform& transform) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyBackendListener(AudioListenerHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> SetBackendListenerTransform(AudioListenerHandle handle, const Transform& transform) = 0;
[[nodiscard]] virtual foundation::Result<void> Update(RuntimeFrameDuration delta) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAudioResourceSource` (строка заголовка 55)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioResourceSource() = default;
[[nodiscard]] virtual SoundState GetSoundState(SoundId id) const = 0;
[[nodiscard]] virtual foundation::Result<AudioClipPayload> LoadClip(SoundId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAudioTransformSource` (строка заголовка 64)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioTransformSource() = default;
[[nodiscard]] virtual foundation::Result<Transform> ReadTransform(AudioTransformId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAudioRuntime` (строка заголовка 72)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioRuntime() = default;
[[nodiscard]] virtual foundation::Result<AudioEmitterHandle> CreateEmitterHandle(const AudioEmitterDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyEmitter(AudioEmitterHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Play(AudioEmitterHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Pause(AudioEmitterHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Resume(AudioEmitterHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Stop(AudioEmitterHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> FadeOut(AudioEmitterHandle handle, RuntimeFrameDuration duration) = 0;
[[nodiscard]] virtual foundation::Result<void> FadeIn(AudioEmitterHandle handle, RuntimeFrameDuration duration) = 0;
calling Play(handle) later restarts playback from the clip beginning. [[nodiscard]] virtual foundation::Result<void> Virtualize(AudioEmitterHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<EmitterState> GetEmitterState(AudioEmitterHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<AudioEmitterSnapshot> GetEmitterSnapshot(AudioEmitterHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<void> Tick(RuntimeFrameDuration delta) = 0;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IListenerSystem` (строка заголовка 103)

Публичные сигнатуры операций:

```cpp
virtual ~IListenerSystem() = default;
[[nodiscard]] virtual foundation::Result<AudioListenerHandle> CreateListenerHandle(const AudioListenerDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyListener(AudioListenerHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> SetMainListener(AudioListenerHandle handle) = 0;
[[nodiscard]] virtual std::optional<AudioListenerHandle> GetMainListener() const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAudioEventQueue` (строка заголовка 116)

Публичные сигнатуры операций:

```cpp
virtual ~IAudioEventQueue() = default;
[[nodiscard]] virtual foundation::Result<void> SubmitOneShot(const AudioEvent& event) = 0;
[[nodiscard]] virtual std::span<const AudioEvent> Events() const = 0;
virtual void Clear() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IMixerSystem` (строка заголовка 128)

Публичные сигнатуры операций:

```cpp
virtual ~IMixerSystem() = default;
[[nodiscard]] virtual foundation::Result<void> SetMixerGroup(MixerGroupState state) = 0;
[[nodiscard]] virtual std::optional<MixerGroupState> GetMixerGroup(MixerGroupId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `AudioDependencies` (строка заголовка 138)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IAudioBackend> backend`
- `std::shared_ptr<IAudioResourceSource> resources`
- `std::shared_ptr<IAudioTransformSource> transforms`

#### `AudioServices` (строка заголовка 145)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<ISoundRegistry> sounds`
- `std::shared_ptr<IAudioRuntime> runtime`
- `std::shared_ptr<IListenerSystem> listeners`
- `std::shared_ptr<IAudioEventQueue> events`
- `std::shared_ptr<IMixerSystem> mixer`
- `std::shared_ptr<IAudioBackend> backend`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterSound` | `AudioRuntime::RegisterSound` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:124` |
| `GetSoundState` | `AudioRuntime::GetSoundState` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:158` |
| `Initialize` | `AudioRuntime::Initialize` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:190` |
| `CreateVoice` | `AudioRuntime::CreateVoice` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:201` |
| `DestroyVoice` | `AudioRuntime::DestroyVoice` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:253` |
| `Play` | `AudioRuntime::Play` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:263` |
| `Pause` | `AudioRuntime::Pause` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:275` |
| `Stop` | `AudioRuntime::Stop` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:287` |
| `IsVoiceFinished` | `AudioRuntime::IsVoiceFinished` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:299` |
| `SetGain` | `AudioRuntime::SetGain` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:305` |
| `SetSpatialState` | `AudioRuntime::SetSpatialState` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:322` |
| `CreateBackendListener` | `AudioRuntime::CreateBackendListener` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:339` |
| `DestroyBackendListener` | `AudioRuntime::DestroyBackendListener` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:354` |
| `SetBackendListenerTransform` | `AudioRuntime::SetBackendListenerTransform` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:364` |
| `Update` | `AudioRuntime::Update` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:379` |
| `CreateEmitterHandle` | `AudioRuntime::CreateEmitterHandle` | `EngineRuntime/Audio/src/audio_runtime_impl.cpp:384` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterSound`: `EngineRuntime/Audio/tests/audio_tests.cpp:346`
- `CreateEmitterHandle`: `EngineRuntime/Audio/tests/audio_tests.cpp:358`
- `GetEmitterState`: `EngineRuntime/Audio/tests/audio_tests.cpp:360`
- `GetEmitterSnapshot`: `EngineRuntime/Audio/tests/audio_tests.cpp:361`
- `Play`: `EngineRuntime/Audio/tests/audio_tests.cpp:362`
- `Stop`: `EngineRuntime/Audio/tests/audio_tests.cpp:365`
- `DestroyEmitter`: `EngineRuntime/Audio/tests/audio_tests.cpp:370`
- `CreateListenerHandle`: `EngineRuntime/Audio/tests/audio_tests.cpp:379`
- `SetMainListener`: `EngineRuntime/Audio/tests/audio_tests.cpp:381`
- `GetMainListener`: `EngineRuntime/Audio/tests/audio_tests.cpp:382`
- `SubmitOneShot`: `EngineRuntime/Audio/tests/audio_tests.cpp:396`
- `FadeOut`: `EngineRuntime/Audio/tests/audio_tests.cpp:407`


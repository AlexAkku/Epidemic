---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "audio"]
source_path: "EngineRuntime/Audio"
---

# EngineRuntime / Audio

Модуль: **Audio**. Audio resources, playback и listeners.

**Реализация:** `EngineRuntime/Audio/`.
**CMake targets:** `EpidemicRuntimeAudio`
**Локальные test targets:** `EpidemicRuntimeAudioTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Audio/audio.h|audio.h]]: `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio.h`
- [[11 API/EngineRuntime/Audio/audio_runtime.h|audio_runtime.h]]: `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h`; объявленные типы: `ISoundRegistry`, `IAudioBackend`, `IAudioResourceSource`, `IAudioTransformSource`, `IAudioRuntime`, `IListenerSystem`, `IAudioEventQueue`, `IMixerSystem`, `AudioDependencies`, `AudioServices`
- [[11 API/EngineRuntime/Audio/audio_types.h|audio_types.h]]: `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`; объявленные типы: `SoundId`, `AudioEmitterId`, `AudioListenerId`, `BackendVoiceHandle`, `MixerGroupId`, `AudioEmitterHandle`, `AudioListenerHandle`, `AudioEventSpace`, `AudioEventOverflowPolicy`, `SoundState`, `EmitterState`, `MixerFadeState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/audio|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Audio управляет sound metadata, emitter/listener handles, backend voices, spatial state, mixer hierarchy, fades, one-shots и clip resources.

#### Контракты

`ISoundRegistry` хранит `SoundDesc`. `IAudioRuntime` создает emitter, управляет Play/Pause/Resume/Stop/Fade/Virtualize, выполняет Tick и terminal shutdown. `IListenerSystem` управляет listeners. `IMixerSystem` хранит groups. `IAudioBackend` владеет voices и listener representation. `IAudioResourceSource` возвращает typed `IAudioClipResource`, который может содержать encoded bytes или `IAudioStreamSource`.

Backend boundary имеет freeze-level retry contract. Повтор той же setter/control projection безопасен. Failure/exception setter/control command означает, что exact retry безопасен. Failure/exception `Update(delta)` означает, что `delta` не был принят и temporal state backend не продвинулся. `CreateVoice`/`CreateBackendListener` передают ownership только при success; failed destroy сохраняет ownership для retry.

`AudioClipPayload`, полученный от resource source, обязан относиться к запрошенному `SoundId`. Failure/exception resource или transform source сохраняется как controlled Runtime error и не приводит к backend acquisition.

#### Playback

Base gain, fade multiplier и mixer gain разделены. Pause сохраняет незавершенный fade state, Resume продолжает его. Zero-duration fade завершается синхронно. Virtualize освобождает backend voice; следующий Play начинает clip заново согласно baseline policy.

До любого внешнего `CreateVoice` Runtime заранее резервирует cleanup ownership. Для batch one-shot Tick capacity резервируется на весь batch до первого acquisition. Поэтому failed rollback после successful `CreateVoice` может сохранить `BackendVoiceHandle` и pinned clip resource без новой allocation. One-shot completion проверяется backend. Failed voice/listener cleanup сохраняет handles для retry и не повреждает survivor collections.

#### Shutdown

После начала shutdown новая работа отклоняется, даже если cleanup временно завершился ошибкой. Cleanup voice/listener ownership выполняется best-effort: ошибка одного объекта не блокирует независимые cleanup steps. Повторный shutdown продолжает только незавершенный cleanup; успешный повторный вызов идемпотентен. ID/generation allocators защищены от overflow.

#### Статус

После локального аудита Goal 3 и canonical evidence convergence модуль имеет статус `LOCAL_READY`. Это не означает `FROZEN`: системные persistence/determinism, integration, fault/load qualification и окончательный freeze выполняются в последующих Goals 5–9.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `audio.h`: factory-функции или backend implementation без отдельного публичного типа.
- `audio_runtime.h`: `ISoundRegistry`, `IAudioBackend`, `IAudioResourceSource`, `IAudioTransformSource`, `IAudioRuntime`, `IListenerSystem`, `IAudioEventQueue`, `IMixerSystem`, `AudioDependencies`, `AudioServices`.
- `audio_types.h`: `SoundId`, `AudioEmitterId`, `AudioListenerId`, `BackendVoiceHandle`, `MixerGroupId`, `AudioEmitterHandle`, `AudioListenerHandle`, `AudioEventSpace`, `AudioEventOverflowPolicy`, `SoundState`, `EmitterState`, `MixerFadeState`, `SoundDesc`, `AudioClipPayload`, `IAudioClipResource`, `AudioClipStorage`, `AudioClipFormat`, `IAudioStreamSource`, `AudioStreamReadResult`, `AudioBackendOptions`, `AudioSpatialState`, `AudioEmitterDesc`, `AudioVoiceDesc`, `AudioEmitterSnapshot`, `AudioListenerDesc`, `AudioEvent`, `MixerGroupState`, `AudioOptions`.

Источник контрактов: `Documentation/EngineRuntime/modules/audio.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AudioEventSpace` (`EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`): `NonSpatial`, `WorldPosition`.
- `AudioEventOverflowPolicy` (`EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`): `DropNewest`, `DropOldest`, `FailSubmit`.
- `SoundState` (`EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`): `Missing`, `Loading`, `Ready`, `Failed`.
- `EmitterState` (`EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`): `Stopped`, `Playing`, `Paused`, `FadingIn`, `FadingOut`, `Virtualized`, `Destroyed`.
- `MixerFadeState` (`EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`): `Stable`, `FadingIn`, `FadingOut`.
- `AudioClipStorage` (`EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h`): `InMemoryEncoded`, `Streaming`.

### Операции и хранилища состояния

#### `IAudioBackend`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:25`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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

#### `IAudioRuntime`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:72`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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

#### `IAudioClipResource`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h:146`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAudioClipResource() = default;
[[nodiscard]] virtual AudioClipFormat GetFormat() const = 0;
[[nodiscard]] virtual AudioClipStorage GetStorage() const = 0;
[[nodiscard]] virtual std::span<const std::byte> GetEncodedData() const = 0;
[[nodiscard]] virtual std::shared_ptr<class IAudioStreamSource> GetStreamSource() const = 0;
```

#### `IListenerSystem`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:103`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IListenerSystem() = default;
[[nodiscard]] virtual foundation::Result<AudioListenerHandle> CreateListenerHandle(const AudioListenerDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyListener(AudioListenerHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> SetMainListener(AudioListenerHandle handle) = 0;
[[nodiscard]] virtual std::optional<AudioListenerHandle> GetMainListener() const = 0;
```

#### `IAudioStreamSource`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h:163`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAudioStreamSource() = default;
[[nodiscard]] virtual std::uint64_t GetSizeBytes() const = 0;
[[nodiscard]] virtual bool IsSeekable() const = 0;
[[nodiscard]] virtual foundation::Result<AudioStreamReadResult> Read(std::uint64_t offset, std::span<std::byte> output) = 0;
```

#### `IAudioEventQueue`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:116`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAudioEventQueue() = default;
[[nodiscard]] virtual foundation::Result<void> SubmitOneShot(const AudioEvent& event) = 0;
[[nodiscard]] virtual std::span<const AudioEvent> Events() const = 0;
virtual void Clear() = 0;
```

#### `ISoundRegistry`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:15`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISoundRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterSound(SoundDesc desc) = 0;
[[nodiscard]] virtual SoundState GetSoundState(SoundId id) const = 0;
```

#### `IAudioResourceSource`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:55`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAudioResourceSource() = default;
[[nodiscard]] virtual SoundState GetSoundState(SoundId id) const = 0;
[[nodiscard]] virtual foundation::Result<AudioClipPayload> LoadClip(SoundId id) const = 0;
```

#### `IMixerSystem`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:128`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IMixerSystem() = default;
[[nodiscard]] virtual foundation::Result<void> SetMixerGroup(MixerGroupState state) = 0;
[[nodiscard]] virtual std::optional<MixerGroupState> GetMixerGroup(MixerGroupId id) const = 0;
```

#### `IAudioTransformSource`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:64`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAudioTransformSource() = default;
[[nodiscard]] virtual foundation::Result<Transform> ReadTransform(AudioTransformId id) const = 0;
```

#### `AudioServices`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_runtime.h:145`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<ISoundRegistry> sounds`; `std::shared_ptr<IAudioRuntime> runtime`; `std::shared_ptr<IListenerSystem> listeners`; `std::shared_ptr<IAudioEventQueue> events`; `std::shared_ptr<IMixerSystem> mixer`; `std::shared_ptr<IAudioBackend> backend`.

#### `SoundId`

Объявлен в `EngineRuntime/Audio/include/Epidemic/Runtime/Audio/audio_types.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Audio/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Audio/src/audio_runtime_impl.cpp` (2354 строк): `TResult::Failure`, `Error::Create`, `AudioRuntime::AudioRuntime`, `AudioRuntime::RegisterSound`, `AudioRuntime::GetSoundState`, `AudioRuntime::IsEnabled`, `AudioRuntime::Initialize`, `AudioRuntime::CreateVoice`, `AudioRuntime::DestroyVoice`, `AudioRuntime::Play`
- `EngineRuntime/Audio/src/audio.cpp` (63 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `audio.invalid_sound`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:134`
- `audio.invalid_sound_state`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1715`
- `audio.sound_already_registered`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:144`
- `audio.allocation_failed`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1942`
- `audio.backend_disabled`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:211`
- `audio.sound_not_ready`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1798`
- `audio.invalid_gain`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:409`
- `audio.invalid_spatial_state`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1764`
- `audio.voice_id_overflow`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:231`
- `audio.duplicate_voice_id`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:241`
- `audio.backend_voice_not_found`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:328`
- `audio.invalid_listener`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:369`
- `audio.invalid_listener_transform`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:2171`
- `audio.invalid_owner`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:394`
- `audio.sound_not_found`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1734`
- `audio.invalid_transform`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:404`
- `audio.emitter_id_overflow`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:414`
- `audio.duplicate_emitter_id`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:434`
- `audio.stale_emitter`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:929`
- `audio.backend_exception`: `EngineRuntime/Audio/src/audio.cpp:34`
- `audio.backend_missing`: `EngineRuntime/Audio/src/audio.cpp:15`
- `audio.invalid_transition`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:2080`
- `audio.invalid_fade`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:2070`
- `audio.invalid_delta`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:954`
- `audio.listener_id_overflow`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1349`
- `audio.transform_source_exception`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:2161`
- `audio.duplicate_listener_id`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1410`
- `audio.stale_listener`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:2154`
- `audio.invalid_event_space`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1522`
- `audio.invalid_overflow_policy`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1527`
- `audio.invalid_volume`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1537`
- `audio.invalid_event_position`: `EngineRuntime/Audio/src/audio_runtime_impl.cpp:1812`

### Конкретные тестовые якоря

- `EngineRuntime/Audio/tests/audio_tests.cpp`: `TestCreatePlayStopDestroyEmitter`, `TestListenerCreationAndMainListener`, `TestInvalidSoundBehavior`, `TestFadeVirtualizedMixerAndEvents`, `TestValidationFailures`, `TestBackendContractsHandlesBoundsAndHierarchy`, `TestProductionFactoryBackendRequiredAndFailurePropagates`, `TestListenerGenerationAndUnknownErrors`, `TestFadeProgressTickAndAttachedTransformUpdate`, `TestMixerCycleAndQueueOverflowPolicies`, `TestBackendVoiceDescOneShotListenerAndMixerGain`, `TestPlayRollbackDestroyFailureVirtualizeFadeAndShutdown`, `TestFadeEdgesPauseResumeAndRollbackCleanup`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `audio.invalid_sound` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:134`)

```cpp
132     {
  133         return foundation::Result<void>::Failure(
  134             foundation::Error::Create("audio.invalid_sound", "sound id must be valid before registration"));
  135     }
  136     if (!IsValid(desc.state))
```

#### `audio.invalid_sound_state` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:1715`)

```cpp
1713         {
 1714             return foundation::Result<AudioClipPayload>::Failure(
 1715                 foundation::Error::Create("audio.invalid_sound_state", "audio resource source returned an invalid sound state"));
 1716         }
 1717         if (loaded.Value().sound != id)
```

#### `audio.sound_already_registered` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:144`)

```cpp
142     {
  143         return foundation::Result<void>::Failure(
  144             foundation::Error::Create("audio.sound_already_registered", "sound id is already registered"));
  145     }
  146     try
```

#### `audio.allocation_failed` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:1942`)

```cpp
1940     {
 1941         return foundation::Result<void>::Failure(
 1942             foundation::Error::Create("audio.allocation_failed", "pending voice cleanup allocation failed"));
 1943     }
 1944     for (auto& owned : pending_voice_cleanups_)
```

#### `audio.backend_disabled` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:211`)

```cpp
209     {
  210         return foundation::Result<BackendVoiceHandle>::Failure(
  211             foundation::Error::Create("audio.backend_disabled", "mock audio backend is disabled"));
  212     }
  213     if (!desc.clip.sound.IsValid() || desc.clip.state != SoundState::Ready || desc.clip.resource == nullptr)
```

#### `audio.sound_not_ready` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:1798`)

```cpp
1796     {
 1797         return foundation::Result<VoiceOwnership>::Failure(
 1798             foundation::Error::Create("audio.sound_not_ready", "one-shot sound is not ready"));
 1799     }
 1800     IAudioBackend* backend = Backend();
```

#### `audio.invalid_gain` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:409`)

```cpp
407     {
  408         return foundation::Result<AudioEmitterHandle>::Failure(
  409             foundation::Error::Create("audio.invalid_gain", "audio emitter gain is outside the supported range"));
  410     }
  411     if (next_emitter_value_ == 0 || next_emitter_generation_ == 0)
```

#### `audio.invalid_spatial_state` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:1764`)

```cpp
1762         {
 1763             return foundation::Result<AudioSpatialState>::Failure(
 1764                 foundation::Error::Create("audio.invalid_spatial_state", "audio transform source returned a non-finite transform"));
 1765         }
 1766     }
```

#### `audio.voice_id_overflow` (`EngineRuntime/Audio/src/audio_runtime_impl.cpp:231`)

```cpp
229     {
  230         return foundation::Result<BackendVoiceHandle>::Failure(
  231             foundation::Error::Create("audio.voice_id_overflow", "mock backend voice id allocator is exhausted"));
  232     }
  233
```

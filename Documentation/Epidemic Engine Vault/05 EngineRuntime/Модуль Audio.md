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

---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "animation"]
source_path: "EngineRuntime/Animation"
---

# EngineRuntime / Animation

Модуль: **Animation**. Animation sampling и pose buffers.

**Реализация:** `EngineRuntime/Animation/`.
**CMake targets:** `EpidemicRuntimeAnimation`
**Локальные test targets:** `EpidemicRuntimeAnimationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Animation/animation.h|animation.h]]: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation.h`
- [[11 API/EngineRuntime/Animation/animation_runtime.h|animation_runtime.h]]: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h`; объявленные типы: `ISkeletonRegistry`, `IAnimationClipRegistry`, `IAnimationRuntime`, `IPoseProvider`, `IAnimationResourceSource`, `IAnimationPoseSink`, `IAnimationEvaluatorBackend`, `IAnimationEventBuffer`, `AnimationDependencies`, `AnimationServices`
- [[11 API/EngineRuntime/Animation/animation_types.h|animation_types.h]]: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`; объявленные типы: `SkeletonId`, `AnimationClipId`, `AnimatorInstanceId`, `AnimatorLifecycle`, `AnimatorReadiness`, `AnimatorPlaybackState`, `PoseState`, `AnimationLodLevel`, `SkeletonDesc`, `AnimationClipDesc`, `AnimatorPlayback`, `CrossfadeState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/animation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

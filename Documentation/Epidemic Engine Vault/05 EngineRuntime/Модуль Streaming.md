---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "streaming"]
source_path: "EngineRuntime/Streaming"
---

# EngineRuntime / Streaming

Модуль: **Streaming**. Загрузка и выгрузка chunks по budget.

**Реализация:** `EngineRuntime/Streaming/`.
**CMake targets:** `EpidemicRuntimeStreaming`
**Локальные test targets:** `EpidemicRuntimeStreamingTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Streaming/residency_controller.h|residency_controller.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/residency_controller.h`; объявленные типы: `IResidencyController`
- [[11 API/EngineRuntime/Streaming/streaming_priority_resolver.h|streaming_priority_resolver.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_priority_resolver.h`; объявленные типы: `IStreamingPriorityResolver`
- [[11 API/EngineRuntime/Streaming/streaming_runtime.h|streaming_runtime.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_runtime.h`; объявленные типы: `IStreamingRuntime`, `IStreamingQuery`, `IStreamingController`, `StreamingServices`, `StreamingDependencies`
- [[11 API/EngineRuntime/Streaming/streaming_sources.h|streaming_sources.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_sources.h`; объявленные типы: `IStreamingWorldSource`, `IStreamingPersistenceSource`, `IStreamingResourceSource`, `IStreamingDataSource`, `IStreamingCommitTarget`, `IStreamingPriorityProvider`
- [[11 API/EngineRuntime/Streaming/streaming_types.h|streaming_types.h]]: `EngineRuntime/Streaming/include/Epidemic/Runtime/Streaming/streaming_types.h`; объявленные типы: `StreamingState`, `StreamingPriorityClass`, `StreamingRequestId`, `StreamingRequestHandle`, `StreamingDemandId`, `StreamingDemandHandle`, `ChunkStreamingTarget`, `RegionStreamingTarget`, `AssetStreamingTarget`, `ObjectStreamingTarget`, `ResourceGroupStreamingTarget`, `StreamingPlanStep`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/streaming|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

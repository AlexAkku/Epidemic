---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "support"]
source_path: "EngineRuntime/Support"
---

# EngineRuntime / Support

Модуль: **Support**. Композиция сервисов и adapters.

**Реализация:** `EngineRuntime/Support/`.
**CMake targets:** `EpidemicRuntimeSupport`
**Локальные test targets:** `EpidemicRuntimeSupportTests`, `EpidemicRuntimeSaveGenerationPackageTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Support/runtime_support.h|runtime_support.h]]: `EngineRuntime/Support/include/Epidemic/Runtime/Support/runtime_support.h`; объявленные типы: `RuntimeFoundationRegistration`, `RuntimeProfile`, `RuntimeAdapterKind`, `RuntimeUpdateStep`, `RuntimeShutdownStep`, `ShutdownProposalPolicy`, `ChunkResourceRequirement`, `ChunkStreamingManifest`, `IChunkStreamingManifestSource`, `PreparedChunkDataSnapshot`, `IStreamingPreparedChunkDataQuery`, `IAnimationResourceMapper`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/support|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

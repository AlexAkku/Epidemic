---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "foundation"]
source_path: "EngineFramework/BaseInfrastructure/Foundation"
---

# EngineFramework / Foundation

Модуль: **Foundation**. Базовые типы, Result, errors и ID.

**Реализация:** `EngineFramework/BaseInfrastructure/Foundation/`.
**CMake targets:** `EpidemicGameFrameworkFoundation`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. The module is qualified only for local readiness; this document does not claim whole-engine freeze.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Foundation/change_cursor.h|change_cursor.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/change_cursor.h`; объявленные типы: `ChangeCursor`
- [[11 API/EngineFramework/Foundation/gameplay_context.h|gameplay_context.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_context.h`; объявленные типы: `GameplayContext`
- [[11 API/EngineFramework/Foundation/gameplay_foundation.h|gameplay_foundation.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/gameplay_foundation.h`; объявленные типы: `SnapshotRestoreAccess`
- [[11 API/EngineFramework/Foundation/id_generator.h|id_generator.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/id_generator.h`; объявленные типы: `MonotonicIdGenerator`, `Snapshot`, `MonotonicIdGeneratorSnapshotStatus`, `MonotonicIdGeneratorSnapshotValidation`
- [[11 API/EngineFramework/Foundation/ids.h|ids.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/ids.h`; объявленные типы: `StableId128`, `StrongId128`, `StrongTypeId`, `GameplayObjectIdTag`, `OperationIdTag`, `CorrelationIdTag`, `QueryIdTag`, `EventIdTag`, `FactIdTag`, `ScheduleIdTag`, `GameplayDomainIdTag`, `TypeIdTag`…
- [[11 API/EngineFramework/Foundation/tags.h|tags.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/tags.h`; объявленные типы: `GameplayTagRegistry`, `Entry`, `GameplayTagSet`
- [[11 API/EngineFramework/Foundation/time_types.h|time_types.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/time_types.h`; объявленные типы: `GameplayDuration`, `GameplayTimePoint`, `GameplayTickId`, `Revision`, `GameplayTickContext`, `hash`
- [[11 API/EngineFramework/Foundation/type_registry.h|type_registry.h]]: `EngineFramework/BaseInfrastructure/Foundation/include/Epidemic/GameFramework/Foundation/type_registry.h`; объявленные типы: `StableTypeRegistry`, `Entry`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/foundation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

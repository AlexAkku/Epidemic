---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "persistence"]
source_path: "EngineRuntime/Persistence"
---

# EngineRuntime / Persistence

Модуль: **Persistence**. Durable snapshots, transactions и backend.

**Реализация:** `EngineRuntime/Persistence/`.
**CMake targets:** `EpidemicRuntimePersistence`
**Локальные test targets:** `EpidemicRuntimePersistenceTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Persistence/dirty_tracker.h|dirty_tracker.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h`; объявленные типы: `IDirtyTracker`
- [[11 API/EngineRuntime/Persistence/lazy_rule_record.h|lazy_rule_record.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/lazy_rule_record.h`; объявленные типы: `LazyRuleKind`, `LazyRuleState`, `LazyRuleRecord`
- [[11 API/EngineRuntime/Persistence/persistence_backend.h|persistence_backend.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_backend.h`; объявленные типы: `PersistenceSnapshot`, `PersistenceDurability`, `IPersistenceBackend`
- [[11 API/EngineRuntime/Persistence/persistence_location.h|persistence_location.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_location.h`; объявленные типы: `PersistenceLocation`
- [[11 API/EngineRuntime/Persistence/persistence_operation.h|persistence_operation.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_operation.h`; объявленные типы: `UpsertObjectOperation`, `AdminRemoveObjectOperation`, `DeleteObjectOperation`, `AdminAddTombstoneOperation`, `UpsertLazyRuleOperation`, `UpdateLazyRuleOperation`, `RemoveLazyRuleOperation`, `UpsertZoneOverrideOperation`, `RemoveZoneOverrideOperation`
- [[11 API/EngineRuntime/Persistence/persistence_payload.h|persistence_payload.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_payload.h`; объявленные типы: `PersistencePayload`
- [[11 API/EngineRuntime/Persistence/persistence_policy.h|persistence_policy.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_policy.h`; объявленные типы: `ObjectProtectionFlags`, `ObjectProtectionMask`
- [[11 API/EngineRuntime/Persistence/persistence_services.h|persistence_services.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_services.h`; объявленные типы: `PersistenceOptions`, `PersistenceServices`
- [[11 API/EngineRuntime/Persistence/persistence_state.h|persistence_state.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_state.h`; объявленные типы: `PersistenceState`, `PersistentObjectKind`
- [[11 API/EngineRuntime/Persistence/persistence_store.h|persistence_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h`; объявленные типы: `IPersistenceQuery`, `IPersistenceStore`
- [[11 API/EngineRuntime/Persistence/persistent_object_store.h|persistent_object_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h`; объявленные типы: `IPersistentObjectStore`
- [[11 API/EngineRuntime/Persistence/persistent_record.h|persistent_record.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_record.h`; объявленные типы: `PersistentObjectRecord`
- [[11 API/EngineRuntime/Persistence/save_transaction.h|save_transaction.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h`; объявленные типы: `SaveTransactionState`, `ISaveTransaction`, `IPersistenceAdministrativeTransaction`
- [[11 API/EngineRuntime/Persistence/tombstone_store.h|tombstone_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h`; объявленные типы: `TombstoneRecord`, `ITombstoneStore`
- [[11 API/EngineRuntime/Persistence/zone_override_store.h|zone_override_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/zone_override_store.h`; объявленные типы: `ZoneOverrideSnapshot`, `IZoneOverrideStore`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/persistence|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

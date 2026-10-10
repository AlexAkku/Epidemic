---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "serialization"]
source_path: "EngineRuntime/Serialization"
---

# EngineRuntime / Serialization

Модуль: **Serialization**. Versioned wire format и serialization.

**Реализация:** `EngineRuntime/Serialization/`.
**CMake targets:** `EpidemicRuntimeSerialization`
**Локальные test targets:** `EpidemicRuntimeSerializationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Serialization/archive_reader.h|archive_reader.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_reader.h`; объявленные типы: `IArchiveReader`
- [[11 API/EngineRuntime/Serialization/archive_value.h|archive_value.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_value.h`; объявленные типы: `SerializedDocument`, `Impl`
- [[11 API/EngineRuntime/Serialization/archive_writer.h|archive_writer.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/archive_writer.h`; объявленные типы: `IArchiveWriter`
- [[11 API/EngineRuntime/Serialization/migration.h|migration.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration.h`; объявленные типы: `MigrationKey`, `IMigration`, `hash`
- [[11 API/EngineRuntime/Serialization/migration_executor.h|migration_executor.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_executor.h`; объявленные типы: `IArchiveFactory`, `IMigrationRegistry`
- [[11 API/EngineRuntime/Serialization/migration_registry.h|migration_registry.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/migration_registry.h`; объявленные типы: `IMigrationRegistry`
- [[11 API/EngineRuntime/Serialization/schema_version.h|schema_version.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/schema_version.h`; объявленные типы: `SchemaVersion`
- [[11 API/EngineRuntime/Serialization/serialization_error.h|serialization_error.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_error.h`
- [[11 API/EngineRuntime/Serialization/serialization_services.h|serialization_services.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serialization_services.h`; объявленные типы: `SerializationOptions`, `IArchiveFactory`, `SerializationServices`
- [[11 API/EngineRuntime/Serialization/serializer.h|serializer.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer.h`; объявленные типы: `IArchiveFactory`, `ISerializer`
- [[11 API/EngineRuntime/Serialization/serializer_registry.h|serializer_registry.h]]: `EngineRuntime/Serialization/include/Epidemic/Runtime/Serialization/serializer_registry.h`; объявленные типы: `ISerializerRegistry`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/serialization|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

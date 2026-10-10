---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "resources"]
source_path: "EngineRuntime/Resources"
---

# EngineRuntime / Resources

Модуль: **Resources**. Resource requests, payload и leases.

**Реализация:** `EngineRuntime/Resources/`.
**CMake targets:** `EpidemicRuntimeResources`
**Локальные test targets:** `EpidemicRuntimeResourcesTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Resources/resource_dependency_graph.h|resource_dependency_graph.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_dependency_graph.h`; объявленные типы: `ResourceDependency`, `ResourceDependencySet`
- [[11 API/EngineRuntime/Resources/resource_handle.h|resource_handle.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_handle.h`; объявленные типы: `ResourceHandle`, `ResourceLease`
- [[11 API/EngineRuntime/Resources/resource_loader.h|resource_loader.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader.h`; объявленные типы: `ResourceLoadArtifact`, `IResourceLoader`
- [[11 API/EngineRuntime/Resources/resource_loader_registry.h|resource_loader_registry.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_loader_registry.h`; объявленные типы: `IResourceLoaderRegistry`
- [[11 API/EngineRuntime/Resources/resource_manager.h|resource_manager.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_manager.h`; объявленные типы: `ResourceProcessingStats`, `ResourceMemoryStats`, `IResourceManager`
- [[11 API/EngineRuntime/Resources/resource_payload.h|resource_payload.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_payload.h`; объявленные типы: `IResourcePayload`, `ByteResourcePayload`
- [[11 API/EngineRuntime/Resources/resource_request.h|resource_request.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_request.h`; объявленные типы: `ResourceRequest`
- [[11 API/EngineRuntime/Resources/resource_result.h|resource_result.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_result.h`
- [[11 API/EngineRuntime/Resources/resource_services.h|resource_services.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_services.h`; объявленные типы: `ResourceOptions`, `ResourceServices`
- [[11 API/EngineRuntime/Resources/resource_state.h|resource_state.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_state.h`; объявленные типы: `ResourceState`
- [[11 API/EngineRuntime/Resources/resource_type.h|resource_type.h]]: `EngineRuntime/Resources/include/Epidemic/Runtime/Resources/resource_type.h`; объявленные типы: `ResourceType`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/resources|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

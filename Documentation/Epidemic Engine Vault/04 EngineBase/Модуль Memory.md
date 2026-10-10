---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "memory"]
source_path: "EngineBase/Memory"
---

# EngineBase / Memory

Модуль: **Memory**. Алоцирование, memory tracking и memory contracts.

**Реализация:** `EngineBase/Memory/`.
**CMake targets:** `EpidemicMemory`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/Memory/allocation_tag.h|allocation_tag.h]]: `EngineBase/Memory/include/Epidemic/Memory/allocation_tag.h`; объявленные типы: `AllocationTag`
- [[11 API/EngineBase/Memory/allocator.h|allocator.h]]: `EngineBase/Memory/include/Epidemic/Memory/allocator.h`; объявленные типы: `IAllocator`, `DefaultAllocator`, `TrackingAllocator`, `IFrameAllocator`
- [[11 API/EngineBase/Memory/imemory_tracker.h|imemory_tracker.h]]: `EngineBase/Memory/include/Epidemic/Memory/imemory_tracker.h`; объявленные типы: `MemoryTagStatistics`, `IMemoryTracker`
- [[11 API/EngineBase/Memory/memory_tracker.h|memory_tracker.h]]: `EngineBase/Memory/include/Epidemic/Memory/memory_tracker.h`; объявленные типы: `MemoryTracker`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineBase/modules/memory|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

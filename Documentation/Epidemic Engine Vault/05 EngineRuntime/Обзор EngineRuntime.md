---
aliases: []
type: "index"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "index"]
---

# Обзор EngineRuntime

EngineRuntime предоставляет изолированные engine-neutral majors. Взаимные technical bridges реализованы через Support; прямые cross-major зависимости запрещены.

Практическое руководство: [[90 Первоисточники/Documentation/EngineRuntime/using_engineruntime|Использование EngineRuntime]].

[[02 Архитектура/Runtime и Support]] · [[07 Сценарии/Runtime кадр]]

## Состав (17)

| Модуль | Назначение | Public headers |
| --- | --- | ---: |
| [[05 EngineRuntime/Модуль Animation|Animation]] | Animation sampling и pose buffers | 3 |
| [[05 EngineRuntime/Модуль Assets|Assets]] | Идентичность, metadata и locations ассетов | 9 |
| [[05 EngineRuntime/Модуль Audio|Audio]] | Audio resources, playback и listeners | 3 |
| [[05 EngineRuntime/Модуль Environment|Environment]] | Нейтральное состояние окружения | 9 |
| [[05 EngineRuntime/Модуль Navigation|Navigation]] | Пути, данные навигации и запросы | 3 |
| [[05 EngineRuntime/Модуль Persistence|Persistence]] | Durable snapshots, transactions и backend | 15 |
| [[05 EngineRuntime/Модуль Physics|Physics]] | Physics scene и simulation bodies | 4 |
| [[05 EngineRuntime/Модуль Renderer|Renderer]] | Backend-neutral renderer contracts | 6 |
| [[05 EngineRuntime/Модуль Resources|Resources]] | Resource requests, payload и leases | 11 |
| [[05 EngineRuntime/Модуль RuntimeFoundation|RuntimeFoundation]] | Общие Runtime IDs, budgets и spatial contracts | 10 |
| [[05 EngineRuntime/Модуль Scene|Scene]] | Hierarchy и spatial representation | 9 |
| [[05 EngineRuntime/Модуль Serialization|Serialization]] | Versioned wire format и serialization | 11 |
| [[05 EngineRuntime/Модуль Simulation|Simulation]] | Budgeted proposals и commits | 3 |
| [[05 EngineRuntime/Модуль Streaming|Streaming]] | Загрузка и выгрузка chunks по budget | 5 |
| [[05 EngineRuntime/Модуль Support|Support]] | Композиция сервисов и adapters | 1 |
| [[05 EngineRuntime/Модуль Time|Time]] | Runtime game delta и clock | 7 |
| [[05 EngineRuntime/Модуль World|World]] | World ownership, placement, residency | 12 |

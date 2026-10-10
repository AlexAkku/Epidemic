---
aliases: []
type: "index"
source_snapshot: "2026-10-10"
tags: ["enginebase", "index"]
---

# Обзор EngineBase

EngineBase служит process-level основанием: C++23 types, сервисы, события, Win32, ввод и RHI. Здесь **нет** gameplay, Runtime World, Renderer или Physics.

Практическое руководство: [[90 Первоисточники/Documentation/EngineBase/using_enginebase|Использование EngineBase]].

[[02 Архитектура/Жизненный цикл]] · [[03 Работа с движком/Smoke приложения]]

## Состав (9)

| Модуль | Назначение | Public headers |
| --- | --- | ---: |
| [[04 EngineBase/Модуль Core|Core]] | Application, services, frame lifecycle, scheduler, events | 12 |
| [[04 EngineBase/Модуль Diagnostics|Diagnostics]] | Логирование, counters, profiling | 5 |
| [[04 EngineBase/Модуль Foundation|Foundation]] | Базовые типы, Result, errors и ID | 6 |
| [[04 EngineBase/Модуль Input|Input]] | Raw input, keyboard/mouse snapshots | 8 |
| [[04 EngineBase/Модуль Memory|Memory]] | Алоцирование, memory tracking и memory contracts | 4 |
| [[04 EngineBase/Модуль Platform|Platform]] | Win32 runtime, окна и события | 7 |
| [[04 EngineBase/Модуль RHI|RHI]] | Нейтральная RHI-абстракция | 7 |
| [[04 EngineBase/Модуль RHI_D3D11|RHI_D3D11]] | D3D11 backend и device integration | 1 |
| [[04 EngineBase/Модуль Support|Support]] | Композиция сервисов и adapters | 2 |

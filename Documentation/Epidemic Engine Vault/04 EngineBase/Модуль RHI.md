---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["enginebase", "module", "r_h_i"]
source_path: "EngineBase/RHI"
---

# EngineBase / RHI

Модуль: **RHI**. Нейтральная RHI-абстракция.

**Реализация:** `EngineBase/RHI/`.
**CMake targets:** `EpidemicRHI`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineBase/RHI/descriptors.h|descriptors.h]]: `EngineBase/RHI/include/Epidemic/RHI/descriptors.h`; объявленные типы: `RhiColor`, `RhiClearDesc`, `RhiDeviceDesc`, `RhiSwapChainDesc`
- [[11 API/EngineBase/RHI/irhi_command_context.h|irhi_command_context.h]]: `EngineBase/RHI/include/Epidemic/RHI/irhi_command_context.h`; объявленные типы: `IRhiCommandContext`
- [[11 API/EngineBase/RHI/irhi_device.h|irhi_device.h]]: `EngineBase/RHI/include/Epidemic/RHI/irhi_device.h`; объявленные типы: `IRhiDevice`
- [[11 API/EngineBase/RHI/irhi_swap_chain.h|irhi_swap_chain.h]]: `EngineBase/RHI/include/Epidemic/RHI/irhi_swap_chain.h`; объявленные типы: `IRhiSwapChain`
- [[11 API/EngineBase/RHI/null_rhi_device.h|null_rhi_device.h]]: `EngineBase/RHI/include/Epidemic/RHI/null_rhi_device.h`
- [[11 API/EngineBase/RHI/pixel_format.h|pixel_format.h]]: `EngineBase/RHI/include/Epidemic/RHI/pixel_format.h`; объявленные типы: `RhiPixelFormat`
- [[11 API/EngineBase/RHI/presentation_surface_handle.h|presentation_surface_handle.h]]: `EngineBase/RHI/include/Epidemic/RHI/presentation_surface_handle.h`; объявленные типы: `PresentationSurfaceHandle`

## Углублённое чтение
- Отдельный исходный `.md` для этого модуля не найден. Читайте source headers и тесты.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

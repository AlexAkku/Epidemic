---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "renderer"]
source_path: "EngineRuntime/Renderer"
---

# EngineRuntime / Renderer

Модуль: **Renderer**. Backend-neutral renderer contracts.

**Реализация:** `EngineRuntime/Renderer/`.
**CMake targets:** `EpidemicRuntimeRenderer`
**Локальные test targets:** `EpidemicRuntimeRendererTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Renderer/render_resource_bridge.h|render_resource_bridge.h]]: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h`; объявленные типы: `RenderTransformSnapshot`, `RenderPoseBuffer`, `IRenderPoseSource`, `IRenderResourceBridge`, `IRenderSceneSource`, `RenderProxySubmission`, `RenderFrameContext`, `IRenderCommandSink`
- [[11 API/EngineRuntime/Renderer/render_scene.h|render_scene.h]]: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_scene.h`; объявленные типы: `IRenderScene`
- [[11 API/EngineRuntime/Renderer/render_types.h|render_types.h]]: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`; объявленные типы: `RenderProxyId`, `ViewId`, `IRenderMeshResource`, `IRenderMaterialResource`, `RenderProxyLifecycle`, `RenderProxyReadiness`, `RenderProxyVisibility`, `RenderProxyDirtyFlags`, `RenderLayer`, `RenderFrameState`, `ViewLifecycle`, `RenderProxyDesc`…
- [[11 API/EngineRuntime/Renderer/renderer_runtime.h|renderer_runtime.h]]: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_runtime.h`; объявленные типы: `IRendererRuntime`
- [[11 API/EngineRuntime/Renderer/renderer_services.h|renderer_services.h]]: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_services.h`; объявленные типы: `RendererOptions`, `RendererDependencies`, `RendererServices`
- [[11 API/EngineRuntime/Renderer/view_system.h|view_system.h]]: `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/view_system.h`; объявленные типы: `IViewSystem`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/renderer|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Runtime и Support|Runtime Support]] как место technical cross-major adapters.

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

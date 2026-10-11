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

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Назначение

Renderer хранит render proxies и views, проверяет готовность resources, получает world transforms и pose snapshots, формирует deterministic backend-neutral submissions и владеет lifecycle подготовленного/backend frame. Он не является D3D11 implementation и не знает Scene, Animation или ResourceManager напрямую.

#### Контракты

`IRenderScene` регистрирует proxies и их visibility/dirty state. `IViewSystem` управляет views. `IRendererRuntime::PrepareFrame()` выполняет все fallible transform/pose reads и формирует detached staged frame, не открывая backend frame. Повторный `PrepareFrame()` до `RenderFrame()` безопасно заменяет staged frame. `RenderFrame()` разрешён только после успешного prepare и отправляет уже подготовленные submissions в порядке `RenderLayer`, затем `RenderProxyId`. Dirty flags и staged transform cache коммитятся только после successful `EndFrame()`.

`IRenderResourceBridge` предоставляет mesh/material payloads. Failed/throwing `AcquirePayloads` означает отсутствие нового hold; failed/throwing `ReleasePayloads` сохраняет существующий hold за Renderer для retry. Temporary resource errors оставляют proxy в `Loading`, permanent errors и exceptions переводят readiness в `Failed`; ни один такой path не публикует proxy как `Ready`.

`IRenderSceneSource` предоставляет validated finite world transforms. Для анимированных объектов нейтральный `IRenderPoseSource` возвращает immutable `RenderPoseBuffer` по `RuntimeObjectId owner`; null pose является допустимым static path. Non-null pose обязан принадлежать requested owner и содержать только valid finite transforms. Scene и pose failures/exceptions завершают prepare до `IRenderCommandSink::BeginFrame()`, поэтому partial prepared transform state не публикуется.

#### Backend frame ownership

`IRenderCommandSink::BeginFrame()` failure/exception означает, что backend frame не был открыт. После successful Begin failure/exception из `SubmitProxy()` или `EndFrame()` требует `AbortFrame()`. Successful Abort подтверждает закрытие frame. Failed/throwing Abort сохраняет private recovery ownership в Renderer; новый `BeginFrame()` запрещён, пока следующий `PrepareFrame()`, `RenderFrame()` или `Shutdown()` не выполнит successful reconciliation.

Prepared submissions сохраняются до successful frame completion либо замены новым prepare. Поэтому recovery после failed Abort не повторяет уже завершённый pose/transform staging.

#### Identity, cleanup и shutdown

Proxy/view IDs consume значение только после successful local publication; exhaustion возвращается до mutation. Deferred proxy destroy сохраняет resource hold до confirmed `ReleasePayloads`. Drain выполняется deterministic по `RenderProxyId`; successful prefix удаляется окончательно, failed owner остаётся `DestroyPending` для retry.

Первый `Shutdown()` немедленно переводит Renderer в terminal lifecycle и запрещает новую render mutation. Если существует uncertain backend frame, сначала выполняется reconciliation. После этого resource holds освобождаются best-effort в deterministic proxy-ID order; failed releases остаются owned и повторный `Shutdown()` продолжает cleanup. Resize/recreate contract в Renderer отсутствует; presentation resize принадлежит Base RHI/integration layer.

#### Factory и границы

`CreateRendererServices()` является strict production factory и требует `IRenderResourceBridge`, `IRenderSceneSource` и `IRenderCommandSink`; `IRenderPoseSource` optional. Он не подставляет mock dependencies даже при compatibility option `enable_mock_dependencies`. Reference behavior создаётся отдельно через `CreateMockRendererServices()`.

Scene/Resources/Animation подключаются к Renderer только adapters в Support. Renderer state transient и не имеет snapshot/restore persistence contract.

#### Статус

Этот документ описывает локально проверенный Goal 3 contract Renderer; после canonical evidence convergence модуль имеет статус `LOCAL_READY`. Слово `FROZEN` не применяется до общесистемных Goals 5–9 и финального freeze gate.

Источник контрактов: `Documentation/EngineRuntime/modules/renderer.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `RenderProxyLifecycle` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `Unregistered`, `Registered`, `DestroyPending`, `Destroyed`.
- `RenderProxyReadiness` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `MissingResources`, `Loading`, `Failed`, `Ready`.
- `RenderProxyVisibility` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `Hidden`, `Visible`, `Culled`.
- `RenderProxyDirtyFlags` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `None`, `Transform`, `Material`, `Visibility`.
- `RenderLayer` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `Opaque`, `Transparent`, `Sky`, `Water`, `Debug`.
- `RenderFrameState` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `NotPrepared`, `Preparing`, `ReadyToRender`, `Rendering`, `Submitted`, `Failed`.
- `ViewLifecycle` (`EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h`): `Active`, `DestroyPending`, `Destroyed`.

### Операции и хранилища состояния

#### `IRenderScene`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_scene.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderScene() = default;
[[nodiscard]] virtual foundation::Result<RenderProxyId> RegisterProxy(const RenderProxyDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyProxy(RenderProxyId id) = 0;
a failed release remains DestroyPending for retry. [[nodiscard]] virtual foundation::Result<void> FlushDeferredDestroys() = 0;
[[nodiscard]] virtual foundation::Result<void> MarkTransformDirty(RenderProxyId id) = 0;
[[nodiscard]] virtual foundation::Result<void> MarkMaterialDirty(RenderProxyId id) = 0;
[[nodiscard]] virtual foundation::Result<void> SetProxyVisibility(RenderProxyId id, RenderProxyVisibility visibility) = 0;
[[nodiscard]] virtual RenderProxyLifecycle GetProxyLifecycle(RenderProxyId id) const = 0;
[[nodiscard]] virtual RenderProxyReadiness GetProxyReadiness(RenderProxyId id) const = 0;
[[nodiscard]] virtual RenderProxyVisibility GetProxyVisibility(RenderProxyId id) const = 0;
[[nodiscard]] virtual RenderProxyDirtyMask GetProxyDirtyFlags(RenderProxyId id) const = 0;
```

#### `IViewSystem`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/view_system.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IViewSystem() = default;
[[nodiscard]] virtual foundation::Result<ViewId> CreateView(const ViewDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyView(ViewId view) = 0;
[[nodiscard]] virtual foundation::Result<void> SetMainView(ViewId view) = 0;
[[nodiscard]] virtual ViewId GetMainView() const = 0;
[[nodiscard]] virtual ViewLifecycle GetViewLifecycle(ViewId view) const = 0;
```

#### `IRenderCommandSink`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h:74`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderCommandSink() = default;
[[nodiscard]] virtual foundation::Result<void> SubmitProxy(const RenderProxySubmission& submission) = 0;
[[nodiscard]] virtual foundation::Result<void> EndFrame() = 0;
[[nodiscard]] virtual foundation::Result<void> AbortFrame() = 0;
```

#### `IRenderResourceBridge`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h:37`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderResourceBridge() = default;
[[nodiscard]] virtual foundation::Result<void> ReleasePayloads(ResourceId mesh, ResourceId material) = 0;
[[nodiscard]] virtual foundation::Result<RenderResourcePayloads> GetPayloads(ResourceId mesh, ResourceId material) const = 0;
```

#### `IRendererRuntime`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_runtime.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRendererRuntime() = default;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
[[nodiscard]] virtual RenderFrameState GetFrameState() const = 0;
```

#### `IRenderSceneSource`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h:49`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderSceneSource() = default;
[[nodiscard]] virtual foundation::Result<RenderTransformSnapshot> GetTransformSnapshot(RenderTransformId node) const = 0;
```

#### `IRenderPoseSource`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_resource_bridge.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderPoseSource() = default;
```

#### `IRenderMeshResource`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h:29`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderMeshResource() = default;
```

#### `IRenderMaterialResource`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h:35`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRenderMaterialResource() = default;
```

#### `RendererServices`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/renderer_services.h:28`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IRenderResourceBridge> resource_bridge`; `std::shared_ptr<IRenderSceneSource> scene_source`; `std::shared_ptr<IRenderPoseSource> pose_source`; `std::shared_ptr<IRenderCommandSink> command_sink`; `std::shared_ptr<IRenderScene> scene`; `std::shared_ptr<IViewSystem> views`; `std::shared_ptr<IRendererRuntime> runtime`.

#### `RenderProxyId`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `ViewId`

Объявлен в `EngineRuntime/Renderer/include/Epidemic/Runtime/Renderer/render_types.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Renderer/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp` (1027 строк): `Error::Create`, `RendererRuntime::RendererRuntime`, `RendererRuntime::RegisterProxy`, `RendererRuntime::DestroyProxy`, `RendererRuntime::FlushDeferredDestroys`, `RendererRuntime::MarkTransformDirty`, `RendererRuntime::MarkMaterialDirty`, `RendererRuntime::SetProxyVisibility`, `RendererRuntime::GetProxyLifecycle`, `RendererRuntime::GetProxyReadiness`
- `EngineRuntime/Renderer/src/renderer.cpp` (132 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `renderer.resource_not_ready`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:49`
- `resource.not_ready`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:49`
- `resource.loading`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:50`
- `resource.queued`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:50`
- `renderer.invalid_owner`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:151`
- `renderer.invalid_resource`: `EngineRuntime/Renderer/src/renderer.cpp:49`
- `renderer.invalid_enum`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:341`
- `renderer.proxy_id_exhausted`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:167`
- `renderer.duplicate_proxy_id`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:196`
- `renderer.allocation_failed`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:770`
- `renderer.proxy_not_found`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:346`
- `renderer.invalid_view`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:389`
- `renderer.view_id_exhausted`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:398`
- `renderer.duplicate_view_id`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:409`
- `renderer.view_not_found`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:448`
- `renderer.invalid_main_view`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:515`
- `renderer.stale_transform`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:897`
- `renderer.backend_exception`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:949`
- `renderer.invalid_pose`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:617`
- `renderer.frame_not_prepared`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:656`
- `renderer.command_sink_missing`: `EngineRuntime/Renderer/src/renderer.cpp:97`
- `renderer.main_view_missing`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:664`
- `render.submit`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:691`
- `render.execute`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:693`
- `renderer.shutdown_complete`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:802`
- `renderer.shutdown_in_progress`: `EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:802`
- `renderer.invalid_transform`: `EngineRuntime/Renderer/src/renderer.cpp:65`
- `renderer.scene_source_missing`: `EngineRuntime/Renderer/src/renderer.cpp:92`
- `renderer.resource_bridge_missing`: `EngineRuntime/Renderer/src/renderer.cpp:87`

### Конкретные тестовые якоря

- `EngineRuntime/Renderer/tests/renderer_tests.cpp`: `TestRegisterProxyTracksSplitState`, `TestMissingResourceMarksNotReady`, `TestDirtyAndVisibilityFlags`, `TestDeferredProxyDestroy`, `TestViewLifecycleAndMainView`, `TestViewRejectsNonFiniteCameraParameters`, `TestFrameFlowEndsSubmittedAndClearsDirty`, `TestPrepareFrameRefreshesDirtyTransformBeforeSubmit`, `TestRenderFrameAbortOnSubmitFailure`, `TestPrepareFrameSkipsMissingResourceAndContinues`, `TestPermanentResourceFailureMarksFailed`, `TestShutdownBestEffortReleasesAllProxies`, `TestSubmissionOrderIsDeterministicByLayerThenProxyId`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `renderer.resource_not_ready` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:49`)

```cpp
47 [[nodiscard]] bool IsTemporaryResourceFailure(const foundation::Error& error) noexcept
   48 {
   49     return error.HasCode("renderer.resource_not_ready") || error.HasCode("resource.not_ready") ||
   50            error.HasCode("resource.loading") || error.HasCode("resource.queued");
   51 }
```

#### `resource.not_ready` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:49`)

```cpp
47 [[nodiscard]] bool IsTemporaryResourceFailure(const foundation::Error& error) noexcept
   48 {
   49     return error.HasCode("renderer.resource_not_ready") || error.HasCode("resource.not_ready") ||
   50            error.HasCode("resource.loading") || error.HasCode("resource.queued");
   51 }
```

#### `resource.loading` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:50`)

```cpp
48 {
   49     return error.HasCode("renderer.resource_not_ready") || error.HasCode("resource.not_ready") ||
   50            error.HasCode("resource.loading") || error.HasCode("resource.queued");
   51 }
   52
```

#### `resource.queued` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:50`)

```cpp
48 {
   49     return error.HasCode("renderer.resource_not_ready") || error.HasCode("resource.not_ready") ||
   50            error.HasCode("resource.loading") || error.HasCode("resource.queued");
   51 }
   52
```

#### `renderer.invalid_owner` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:151`)

```cpp
149     if (!desc.owner.IsValid())
  150     {
  151         return RendererFailureValue<RenderProxyId>("renderer.invalid_owner", "render proxy owner must be valid before registration");
  152     }
  153     if (!desc.mesh.IsValid() || !desc.material.IsValid())
```

#### `renderer.invalid_resource` (`EngineRuntime/Renderer/src/renderer.cpp:49`)

```cpp
47         {
   48             return foundation::Result<RenderResourcePayloads>::Failure(
   49                 foundation::Error::Create("renderer.invalid_resource", "mock renderer payload request requires valid resources"));
   50         }
   51         return foundation::Result<RenderResourcePayloads>::Success(RenderResourcePayloads{
```

#### `renderer.invalid_enum` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:341`)

```cpp
339     if (!IsValidProxyVisibility(visibility))
  340     {
  341         return RendererFailure("renderer.invalid_enum", "render proxy visibility enum is outside the supported domain");
  342     }
  343     ProxyRecord* record = FindProxy(id);
```

#### `renderer.proxy_id_exhausted` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:167`)

```cpp
165     }
  166 
  167     const auto proxy_value = PeekRendererId(next_proxy_value_, "renderer.proxy_id_exhausted", "render proxy id allocator is exhausted");
  168     if (!proxy_value)
  169     {
```

#### `renderer.duplicate_proxy_id` (`EngineRuntime/Renderer/src/renderer_runtime_impl.cpp:196`)

```cpp
194         if (!inserted)
  195         {
  196             return RendererFailureValue<RenderProxyId>("renderer.duplicate_proxy_id", "allocated render proxy id already exists");
  197         }
  198         published = &iterator->second;
```

---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "physics"]
source_path: "EngineRuntime/Physics"
---

# EngineRuntime / Physics

Модуль: **Physics**. Physics scene и simulation bodies.

**Реализация:** `EngineRuntime/Physics/`.
**CMake targets:** `EpidemicRuntimePhysics`
**Локальные test targets:** `EpidemicRuntimePhysicsTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Physics/physics_event_buffer.h|physics_event_buffer.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h`; объявленные типы: `IPhysicsEventBuffer`
- [[11 API/EngineRuntime/Physics/physics_query.h|physics_query.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h`; объявленные типы: `IPhysicsQuery`
- [[11 API/EngineRuntime/Physics/physics_scene.h|physics_scene.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h`; объявленные типы: `ICollisionShapeRegistry`, `IPhysicsScene`, `IPhysicsStepper`, `IPhysicsRuntimeLifecycle`, `IPhysicsBackend`, `IPhysicsTransformSource`, `IPhysicsTransformSink`, `PhysicsServices`, `PhysicsDependencies`
- [[11 API/EngineRuntime/Physics/physics_types.h|physics_types.h]]: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h`; объявленные типы: `PhysicsBodyId`, `PhysicsBodyHandle`, `CollisionShapeId`, `BackendShapeHandle`, `BackendBodyHandle`, `PhysicsBodyType`, `PhysicsActivityState`, `PhysicsEventState`, `PhysicsDirtyFlags`, `PhysicsBodyDesc`, `PhysicsBackendOptions`, `PhysicsOptions`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/physics|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Physics управляет collision shapes, bodies, fixed-step simulation, spatial queries и contact events через внешний или reference backend. Runtime handles отделены от backend handles.

#### Контракты

`ICollisionShapeRegistry` регистрирует shapes. `IPhysicsScene` создает и уничтожает bodies. `IPhysicsStepper` выполняет fixed steps. `IPhysicsQuery` предоставляет snapshots, raycast и overlap. `IPhysicsEventBuffer` хранит текущий contact batch. `IPhysicsRuntimeLifecycle` выполняет retryable shutdown.

`IPhysicsBackend` работает только с `BackendBodyHandle` и `BackendShapeHandle`, возвращает `BackendBodySnapshot`, `BackendRaycastHit` и `BackendContactEvent`. Runtime преобразует их в public handles. Successful `CreateShape`/`CreateBody` передает ownership нового backend object с valid handle, уникальным среди live objects этого типа; alias существующего handle является backend contract violation и не уничтожается Runtime как rollback ownership.

#### Step

`Tick(delta)` использует prefix-progress semantics. Ошибки validation и time-arithmetic preflight происходят до acceptance и сохраняют timing state. После успешного preflight `delta` принимается в accumulator ровно один раз. Если поздний fixed step завершается ошибкой, уже завершенный prefix остается committed, а оставшееся принятое время остается в accumulator. Для продолжения принятой работы используется `Tick(0)` либо следующий вызов только с новым frame delta; delta не передается повторно как retry. Если `SimulateFixed` уже завершился успешно, а snapshot/contact synchronization завершилась ошибкой, retry завершает тот же backend step без повторного `SimulateFixed`.

Backend snapshots сначала собираются и валидируются batch-first, затем authoritative body records меняются и только после этого публикуются sink writes. Invalid transform/velocity не повреждает state.

Contact payload проходит validation finite values и handles; invalid events фильтруются. Valid events сохраняют backend order, включая duplicate events: Physics не выполняет implicit deduplication.

Raycast принимает finite direction только если его magnitude больше `float epsilon`. Нормализация выполняется без overflow для всего finite `float` range, поэтому accepted large finite direction не может превратиться в zero или non-finite payload. Public external-backend path и reference `RaycastBackend()` используют одну numeric policy.

#### Shutdown

Bodies уничтожаются раньше связанных с ними shapes. Failure одного body сохраняет его backend handle, запрещает уничтожение используемой им shape и не блокирует best-effort cleanup независимых bodies и свободных shapes. Pending rollback body с неизвестной опубликованной shape relation временно удерживает все shapes до успешного cleanup. Повторный shutdown продолжает оставшийся cleanup и после полного завершения является idempotent.

#### Стабильность

Документ описывает проверенный локальный contract. После прохождения Goal 3 audit и ledger gates модуль имеет статус `LOCAL_READY`; системный `FROZEN` остаётся за Goals 5–9. Production SDK подключается через существующий backend port.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `physics_event_buffer.h`: `IPhysicsEventBuffer`.
- `physics_query.h`: `IPhysicsQuery`.
- `physics_scene.h`: `ICollisionShapeRegistry`, `IPhysicsScene`, `IPhysicsStepper`, `IPhysicsRuntimeLifecycle`, `IPhysicsBackend`, `IPhysicsTransformSource`, `IPhysicsTransformSink`, `PhysicsServices`, `PhysicsDependencies`.
- `physics_types.h`: `PhysicsBodyId`, `PhysicsBodyHandle`, `CollisionShapeId`, `BackendShapeHandle`, `BackendBodyHandle`, `PhysicsBodyType`, `PhysicsActivityState`, `PhysicsEventState`, `PhysicsDirtyFlags`, `PhysicsBodyDesc`, `PhysicsBackendOptions`, `PhysicsOptions`, `CollisionShapeDesc`, `PhysicsMaterial`, `ContactEvent`, `BackendContactEvent`, `RaycastQuery`, `RaycastHit`, `BackendRaycastHit`, `OverlapQuery`, `OverlapResult`, `PhysicsStepResult`, `PhysicsBodySnapshot`, `BackendBodySnapshot`.

Источник контрактов: `Documentation/EngineRuntime/modules/physics.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PhysicsBodyType` (`EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h`): `Static`, `Dynamic`, `Kinematic`.
- `PhysicsActivityState` (`EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h`): `Static`, `Awake`, `Sleeping`, `Disabled`.
- `PhysicsEventState` (`EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h`): `Begin`, `Persist`, `End`.
- `PhysicsDirtyFlags` (`EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h`): `None`, `Transform`, `Shape`, `Material`, `Activity`.

### Операции и хранилища состояния

#### `IPhysicsBackend`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:63`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsBackend() = default;
[[nodiscard]] virtual foundation::Result<void> Initialize(const PhysicsBackendOptions& options) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyShape(BackendShapeHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<BackendBodyHandle> CreateBody(const PhysicsBodyDesc& desc, BackendShapeHandle shape) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyBody(BackendBodyHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> ApplyImpulse(BackendBodyHandle handle, const Vec3& impulse) = 0;
[[nodiscard]] virtual foundation::Result<void> SimulateFixed(RuntimeFrameDuration fixed_delta) = 0;
[[nodiscard]] virtual foundation::Result<BackendBodySnapshot> GetBodySnapshot(BackendBodyHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<BackendRaycastHit> RaycastBackend(const RaycastQuery& query) const = 0;
[[nodiscard]] virtual foundation::Result<std::vector<BackendContactEvent>> ConsumeContactEvents() = 0;
```

#### `IPhysicsScene`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsScene() = default;
[[nodiscard]] virtual foundation::Result<PhysicsBodyHandle> CreateBody(const PhysicsBodyDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyBody(PhysicsBodyHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> ApplyImpulse(PhysicsBodyHandle handle, Vec3 impulse) = 0;
[[nodiscard]] virtual foundation::Result<PhysicsBodySnapshot> GetBodySnapshot(PhysicsBodyHandle handle) const = 0;
```

#### `ICollisionShapeRegistry`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ICollisionShapeRegistry() = default;
unregister is rejected while any body references the shape. [[nodiscard]] virtual foundation::Result<void> RegisterShape(const CollisionShapeDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> UnregisterShape(CollisionShapeId id) = 0;
[[nodiscard]] virtual bool HasShape(CollisionShapeId id) const = 0;
```

#### `IPhysicsEventBuffer`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_event_buffer.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsEventBuffer() = default;
[[nodiscard]] virtual std::span<const ContactEvent> Contacts() const = 0;
virtual void Clear() = 0;
```

#### `IPhysicsQuery`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_query.h:9`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsQuery() = default;
[[nodiscard]] virtual foundation::Result<RaycastHit> Raycast(const RaycastQuery& query) const = 0;
[[nodiscard]] virtual foundation::Result<OverlapResult> Overlap(const OverlapQuery& query) const = 0;
```

#### `IPhysicsStepper`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:37`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsStepper() = default;
[[nodiscard]] virtual foundation::Result<PhysicsStepResult> StepFixed(RuntimeFrameDuration fixed_delta) = 0;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `do not resubmit the failed`.

#### `IPhysicsRuntimeLifecycle`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:55`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsRuntimeLifecycle() = default;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

#### `IPhysicsTransformSource`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:83`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsTransformSource() = default;
[[nodiscard]] virtual foundation::Result<Transform> ReadTransform(PhysicsTransformId id) const = 0;
```

#### `IPhysicsTransformSink`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:91`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPhysicsTransformSink() = default;
[[nodiscard]] virtual foundation::Result<void> WriteTransform(PhysicsTransformId id, const Transform& transform) = 0;
```

#### `PhysicsServices`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h:99`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<ICollisionShapeRegistry> shapes`; `std::shared_ptr<IPhysicsScene> scene`; `std::shared_ptr<IPhysicsStepper> stepper`; `std::shared_ptr<IPhysicsQuery> query`; `std::shared_ptr<IPhysicsEventBuffer> events`; `std::shared_ptr<IPhysicsBackend> backend`; `std::shared_ptr<IPhysicsRuntimeLifecycle> lifecycle`.

#### `PhysicsBodyId`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `PhysicsBodyHandle`

Объявлен в `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_types.h:27`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t generation = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Physics/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Physics/src/physics_runtime_impl.cpp` (1616 строк): `Error::Create`, `TResult::Failure`, `PhysicsRuntime::PhysicsRuntime`, `PhysicsRuntime::DrainPendingBackendCleanup`, `PhysicsRuntime::Shutdown`, `PhysicsRuntime::RegisterShape`, `PhysicsRuntime::HasShape`, `PhysicsRuntime::UnregisterShape`, `PhysicsRuntime::CreateBody`, `PhysicsRuntime::DestroyBody`
- `EngineRuntime/Physics/src/physics.cpp` (66 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `physics.backend_exception`: `EngineRuntime/Physics/src/physics.cpp:45`
- `physics.invalid_shape`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:752`
- `physics.duplicate_shape`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:319`
- `physics.backend_shape_id_overflow`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:756`
- `physics.allocation_failure`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1492`
- `physics.invalid_backend_handle`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1249`
- `physics.cleanup_pending`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:505`
- `physics.duplicate_backend_shape_handle`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:328`
- `physics.shape_not_found`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1603`
- `physics.shape_in_use`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:370`
- `physics.invalid_owner`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:392`
- `physics.invalid_transform`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:437`
- `physics.invalid_body_type`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:782`
- `physics.invalid_mass`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:408`
- `physics.body_id_overflow`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:447`
- `physics.duplicate_backend_body_handle`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:550`
- `physics.duplicate_body_id`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:541`
- `physics.revision_overflow`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1598`
- `physics.invalid_impulse`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:817`
- `physics.impulse_not_allowed`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:618`
- `physics.invalid_backend_snapshot`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1594`
- `physics.invalid_step`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:826`
- `physics.time_overflow`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:731`
- `physics.invalid_handle`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1295`
- `physics.invalid_body`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:787`
- `physics.backend_body_id_overflow`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:791`
- `physics.unknown_handle`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1299`
- `physics.callback`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:865`
- `physics.invalid_query`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:990`
- `physics.invalid_backend_hit`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:898`
- `physics.dispatch`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1513`
- `physics.unknown_backend_handle`: `EngineRuntime/Physics/src/physics_runtime_impl.cpp:1254`

### Конкретные тестовые якоря

- `EngineRuntime/Physics/tests/physics_tests.cpp`: `TestGenerationSnapshotAndDestroyLifecycle`, `TestUnknownHandleAndImpulseRules`, `TestFixedStepAccumulatorAndMaxSubsteps`, `TestDefaultAndConfiguredFixedStep`, `TestInitialTransformFallsBackToDescWithoutSource`, `TestUnregisterShapeRejectsInUseAndDestroysFreeShape`, `TestShutdownBestEffortAndIdempotent`, `TestBackendFailurePropagation`, `TestTransformSourceAndSinkSynchronization`, `TestContactBufferStoresExplicitBeginPersistEnd`, `TestRaycastRespectsDirectionAndMaxDistance`, `TestEqualDistanceRaycastUsesStableBodyOrder`, `TestExternalBackendLifecycleAndSynchronization`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `physics.backend_exception` (`EngineRuntime/Physics/src/physics.cpp:45`)

```cpp
43         {
   44             return foundation::Result<PhysicsServices>::Failure(
   45                 foundation::Error::Create("physics.backend_exception",
   46                                           "physics backend threw during initialization"));
   47         }
```

#### `physics.invalid_shape` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:752`)

```cpp
750     if (!desc.id.IsValid() || !IsValidAabb(desc.local_bounds))
  751     {
  752         return PhysicsFailureValue<BackendShapeHandle>("physics.invalid_shape", "backend shape requires valid id and bounds");
  753     }
  754     const auto allocated = AllocateMonotonicId(
```

#### `physics.duplicate_shape` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:319`)

```cpp
317         {
  318             const auto cleanup = rollback_backend();
  319             return cleanup ? PhysicsFailure("physics.duplicate_shape", "collision shape id is already registered")
  320                            : cleanup;
  321         }
```

#### `physics.backend_shape_id_overflow` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:756`)

```cpp
754     const auto allocated = AllocateMonotonicId(
  755         next_backend_shape_value_,
  756         "physics.backend_shape_id_overflow",
  757         "backend shape id allocator is exhausted");
  758     if (!allocated)
```

#### `physics.allocation_failure` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:1492`)

```cpp
1490     catch (const std::bad_alloc&)
 1491     {
 1492         return PhysicsFailureValue<PhysicsStepResult>("physics.allocation_failure", "could not allocate fixed-step commit buffers");
 1493     }
 1494
```

#### `physics.invalid_backend_handle` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:1249`)

```cpp
1247     if (!handle.IsValid())
 1248     {
 1249         return PhysicsFailureValue<PhysicsBodyHandle>("physics.invalid_backend_handle", "backend body handle must be valid");
 1250     }
 1251     const BodyRecord* body = FindBackendBody(handle);
```

#### `physics.cleanup_pending` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:505`)

```cpp
503         {
  504             pending_backend_body_cleanup_.push_back(backend_handle);
  505             return PhysicsFailure("physics.cleanup_pending", "backend body rollback is pending");
  506         }
  507         return foundation::Result<void>::Success();
```

#### `physics.duplicate_backend_shape_handle` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:328`)

```cpp
326             shapes_.erase(desc.id);
  327             const auto cleanup = rollback_backend();
  328             return cleanup ? PhysicsFailure("physics.duplicate_backend_shape_handle", "physics backend returned a duplicate shape handle")
  329                            : cleanup;
  330         }
```

#### `physics.shape_not_found` (`EngineRuntime/Physics/src/physics_runtime_impl.cpp:1603`)

```cpp
1601     if (shape == shapes_.end())
 1602     {
 1603         return PhysicsFailure("physics.shape_not_found", "physics body references a missing collision shape");
 1604     }
 1605     const Aabb staged_bounds = TransformAabb(snapshot.world_transform, shape->second.desc.local_bounds);
```

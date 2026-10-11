---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h"
---

# Physics: physics_scene.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Physics/include/Epidemic/Runtime/Physics/physics_scene.h`.

**Модуль:** [[05 EngineRuntime/Модуль Physics|Physics]].
**Размер исходного файла:** 5479 байт.
**Подключение:** `#include <Epidemic/Runtime/Physics/physics_scene.h>`.

## Объявления типов (лексический индекс)
`ICollisionShapeRegistry`, `IPhysicsScene`, `IPhysicsStepper`, `IPhysicsRuntimeLifecycle`, `IPhysicsBackend`, `IPhysicsTransformSource`, `IPhysicsTransformSink`, `PhysicsServices`, `PhysicsDependencies`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Physics/physics_event_buffer.h"
#include "Epidemic/Runtime/Physics/physics_query.h"
#include "Epidemic/Runtime/Physics/physics_types.h"

#include <memory>
#include <vector>

namespace epidemic::runtime::physics
{
// Public contracts for creating and managing physics bodies and collision shapes.
class ICollisionShapeRegistry
{
  public:
    virtual ~ICollisionShapeRegistry() = default;

    // Shape registration is intentionally runtime-dynamic. Register/unregister remain
    // legal after bootstrap; unregister is rejected while any body references the shape.
    [[nodiscard]] virtual foundation::Result<void> RegisterShape(const CollisionShapeDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> UnregisterShape(CollisionShapeId id) = 0;
    [[nodiscard]] virtual bool HasShape(CollisionShapeId id) const = 0;
};

class IPhysicsScene
{
  public:
    virtual ~IPhysicsScene() = default;

    [[nodiscard]] virtual foundation::Result<PhysicsBodyHandle> CreateBody(const PhysicsBodyDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyBody(PhysicsBodyHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> ApplyImpulse(PhysicsBodyHandle handle, Vec3 impulse) = 0;
    [[nodiscard]] virtual foundation::Result<PhysicsBodySnapshot> GetBodySnapshot(PhysicsBodyHandle handle) const = 0;
};

class IPhysicsStepper
{
  public:
    virtual ~IPhysicsStepper() = default;

    [[nodiscard]] virtual foundation::Result<PhysicsStepResult> StepFixed(RuntimeFrameDuration fixed_delta) = 0;

    // Tick uses prefix-progress acceptance semantics. Invalid input and arithmetic
    // preflight failures happen before acceptance and leave timing state unchanged.
    // After preflight succeeds, delta is accepted exactly once into the accumulator.
    // A later failure keeps every completed fixed-step prefix committed and leaves the
    // remaining accepted time in the accumulator. Retry that accepted work with
    // Tick(0), or submit only a genuinely new frame delta; do not resubmit the failed
    // call's delta. If backend simulation succeeded but synchronization failed, retry
    // completes that same backend step without calling SimulateFixed again.
    [[nodiscard]] virtual foundation::Result<PhysicsStepResult> Tick(RuntimeFrameDuration delta) = 0;
};

class IPhysicsRuntimeLifecycle
{
  public:
    virtual ~IPhysicsRuntimeLifecycle() = default;

    [[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
};

class IPhysicsBackend
{
  public:
    virtual ~IPhysicsBackend() = default;

    [[nodiscard]] virtual foundation::Result<void> Initialize(const PhysicsBackendOptions& options) = 0;
    // A successful create transfers ownership of one newly-created backend object
    // represented by a valid handle that is unique among currently live objects of
    // that kind. Returning an alias to an existing live handle is a contract violation.
    [[nodiscard]] virtual foundation::Result<BackendShapeHandle> CreateShape(const CollisionShapeDesc& desc) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyShape(BackendShapeHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<BackendBodyHandle> CreateBody(const PhysicsBodyDesc& desc, BackendShapeHandle shape) = 0;
    [[nodiscard]] virtual foundation::Result<void> DestroyBody(BackendBodyHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> ApplyImpulse(BackendBodyHandle handle, const Vec3& impulse) = 0;
    [[nodiscard]] virtual foundation::Result<void> SimulateFixed(RuntimeFrameDuration fixed_delta) = 0;
    [[nodiscard]] virtual foundation::Result<BackendBodySnapshot> GetBodySnapshot(BackendBodyHandle handle) const = 0;
    [[nodiscard]] virtual foundation::Result<BackendRaycastHit> RaycastBackend(const RaycastQuery& query) const = 0;
    [[nodiscard]] virtual foundation::Result<std::vector<BackendContactEvent>> ConsumeContactEvents() = 0;
};

class IPhysicsTransformSource
{
  public:
    virtual ~IPhysicsTransformSource() = default;

    [[nodiscard]] virtual foundation::Result<Transform> ReadTransform(PhysicsTransformId id) const = 0;
};

class IPhysicsTransformSink
{
  public:
    virtual ~IPhysicsTransformSink() = default;

    [[nodiscard]] virtual foundation::Result<void> WriteTransform(PhysicsTransformId id, const Transform& transform) = 0;
};

struct PhysicsServices
{
    std::shared_ptr<ICollisionShapeRegistry> shapes;
    std::shared_ptr<IPhysicsScene> scene;
    std::shared_ptr<IPhysicsStepper> stepper;
    std::shared_ptr<IPhysicsQuery> query;
    std::shared_ptr<IPhysicsEventBuffer> events;
    std::shared_ptr<IPhysicsBackend> backend;
    std::shared_ptr<IPhysicsRuntimeLifecycle> lifecycle;
};

struct PhysicsDependencies
{
    std::shared_ptr<IPhysicsBackend> backend;
    std::shared_ptr<IPhysicsTransformSource> transform_source;
    std::shared_ptr<IPhysicsTransformSink> transform_sink;
    PhysicsOptions options{};
};

[[nodiscard]] foundation::Result<PhysicsServices> CreatePhysicsServices();
[[nodiscard]] foundation::Result<PhysicsServices> CreatePhysicsServices(std::shared_ptr<IPhysicsBackend> backend);
[[nodiscard]] foundation::Result<PhysicsServices> CreatePhysicsServices(PhysicsDependencies dependencies);
} // namespace epidemic::runtime::physics
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Physics/physics_event_buffer.h`, `Epidemic/Runtime/Physics/physics_query.h`, `Epidemic/Runtime/Physics/physics_types.h`.

### Состав типов и операции

#### `ICollisionShapeRegistry` (строка заголовка 14)

Публичные сигнатуры операций:

```cpp
virtual ~ICollisionShapeRegistry() = default;
unregister is rejected while any body references the shape. [[nodiscard]] virtual foundation::Result<void> RegisterShape(const CollisionShapeDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> UnregisterShape(CollisionShapeId id) = 0;
[[nodiscard]] virtual bool HasShape(CollisionShapeId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPhysicsScene` (строка заголовка 26)

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsScene() = default;
[[nodiscard]] virtual foundation::Result<PhysicsBodyHandle> CreateBody(const PhysicsBodyDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyBody(PhysicsBodyHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> ApplyImpulse(PhysicsBodyHandle handle, Vec3 impulse) = 0;
[[nodiscard]] virtual foundation::Result<PhysicsBodySnapshot> GetBodySnapshot(PhysicsBodyHandle handle) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPhysicsStepper` (строка заголовка 37)

Публичные поля и значения по умолчанию:

- `do not resubmit the failed`

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsStepper() = default;
[[nodiscard]] virtual foundation::Result<PhysicsStepResult> StepFixed(RuntimeFrameDuration fixed_delta) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPhysicsRuntimeLifecycle` (строка заголовка 55)

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsRuntimeLifecycle() = default;
[[nodiscard]] virtual foundation::Result<void> Shutdown() = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPhysicsBackend` (строка заголовка 63)

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPhysicsTransformSource` (строка заголовка 83)

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsTransformSource() = default;
[[nodiscard]] virtual foundation::Result<Transform> ReadTransform(PhysicsTransformId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPhysicsTransformSink` (строка заголовка 91)

Публичные сигнатуры операций:

```cpp
virtual ~IPhysicsTransformSink() = default;
[[nodiscard]] virtual foundation::Result<void> WriteTransform(PhysicsTransformId id, const Transform& transform) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `PhysicsServices` (строка заголовка 99)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<ICollisionShapeRegistry> shapes`
- `std::shared_ptr<IPhysicsScene> scene`
- `std::shared_ptr<IPhysicsStepper> stepper`
- `std::shared_ptr<IPhysicsQuery> query`
- `std::shared_ptr<IPhysicsEventBuffer> events`
- `std::shared_ptr<IPhysicsBackend> backend`
- `std::shared_ptr<IPhysicsRuntimeLifecycle> lifecycle`

#### `PhysicsDependencies` (строка заголовка 110)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IPhysicsBackend> backend`
- `std::shared_ptr<IPhysicsTransformSource> transform_source`
- `std::shared_ptr<IPhysicsTransformSink> transform_sink`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `Shutdown` | `PhysicsRuntime::Shutdown` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:145` |
| `RegisterShape` | `PhysicsRuntime::RegisterShape` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:224` |
| `HasShape` | `PhysicsRuntime::HasShape` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:349` |
| `UnregisterShape` | `PhysicsRuntime::UnregisterShape` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:354` |
| `CreateBody` | `PhysicsRuntime::CreateBody` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:388` |
| `DestroyBody` | `PhysicsRuntime::DestroyBody` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:575` |
| `ApplyImpulse` | `PhysicsRuntime::ApplyImpulse` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:605` |
| `GetBodySnapshot` | `PhysicsRuntime::GetBodySnapshot` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:644` |
| `StepFixed` | `PhysicsRuntime::StepFixed` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:673` |
| `Initialize` | `PhysicsRuntime::Initialize` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:743` |
| `DestroyShape` | `PhysicsRuntime::DestroyShape` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:765` |
| `SimulateFixed` | `PhysicsRuntime::SimulateFixed` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:822` |
| `ConsumeContactEvents` | `PhysicsRuntime::ConsumeContactEvents` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:851` |
| `RaycastBackend` | `PhysicsRuntime::RaycastBackend` | `EngineRuntime/Physics/src/physics_runtime_impl.cpp:945` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `RegisterShape`: `EngineRuntime/Physics/tests/physics_tests.cpp:60`
- `CreateBody`: `EngineRuntime/Physics/tests/physics_tests.cpp:275`
- `GetBodySnapshot`: `EngineRuntime/Physics/tests/physics_tests.cpp:280`
- `DestroyBody`: `EngineRuntime/Physics/tests/physics_tests.cpp:290`
- `ApplyImpulse`: `EngineRuntime/Physics/tests/physics_tests.cpp:299`
- `StepFixed`: `EngineRuntime/Physics/tests/physics_tests.cpp:323`
- `RaycastBackend`: `EngineRuntime/Physics/tests/physics_tests.cpp:486`
- `HasShape`: `EngineRuntime/Physics/tests/physics_tests.cpp:800`
- `Shutdown`: `EngineRuntime/Physics/tests/physics_tests.cpp:1028`
- `ConsumeContactEvents`: `EngineRuntime/Physics/tests/physics_tests.cpp:1310`


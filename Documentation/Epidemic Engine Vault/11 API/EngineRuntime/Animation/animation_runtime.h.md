---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineruntime"]
source_path: "EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h"
---

# Animation: animation_runtime.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h`.

**Модуль:** [[05 EngineRuntime/Модуль Animation|Animation]].
**Размер исходного файла:** 4686 байт.
**Подключение:** `#include <Epidemic/Runtime/Animation/animation_runtime.h>`.

## Объявления типов (лексический индекс)
`ISkeletonRegistry`, `IAnimationClipRegistry`, `IAnimationRuntime`, `IPoseProvider`, `IAnimationResourceSource`, `IAnimationPoseSink`, `IAnimationEvaluatorBackend`, `IAnimationEventBuffer`, `AnimationDependencies`, `AnimationServices`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/Runtime/Animation/animation_types.h"

#include <memory>
#include <span>

namespace epidemic::runtime::animation
{
// Public contracts for registering animation resources, owning animator instances and exposing pose/event state.
// Implementations are allowed to be mock-backed while preserving lifecycle semantics.

class ISkeletonRegistry
{
public:
    virtual ~ISkeletonRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterSkeleton(SkeletonDesc desc) = 0;

    [[nodiscard]] virtual bool HasSkeleton(SkeletonId id) const = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};

class IAnimationClipRegistry
{
public:
    virtual ~IAnimationClipRegistry() = default;

    [[nodiscard]] virtual foundation::Result<void> RegisterClip(AnimationClipDesc desc) = 0;

    [[nodiscard]] virtual bool HasClip(AnimationClipId id) const = 0;
    [[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
    [[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
};

class IAnimationRuntime
{
public:
    virtual ~IAnimationRuntime() = default;

    [[nodiscard]] virtual foundation::Result<AnimatorHandle> CreateAnimatorHandle(const AnimatorDesc& desc) = 0;

    [[nodiscard]] virtual foundation::Result<void> DestroyAnimator(AnimatorHandle handle) = 0;

    [[nodiscard]] virtual foundation::Result<void> Play(const AnimationPlaybackCommand& command) = 0;
    [[nodiscard]] virtual foundation::Result<void> Pause(AnimatorHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> Stop(AnimatorHandle handle) = 0;
    [[nodiscard]] virtual foundation::Result<void> Crossfade(AnimatorHandle handle, AnimationClipId clip, FrameDuration duration) = 0;

    [[nodiscard]] virtual foundation::Result<std::size_t> Tick(FrameDuration delta, std::size_t max_animators) = 0;

    [[nodiscard]] virtual foundation::Result<AnimatorSnapshot> GetAnimatorSnapshot(AnimatorHandle handle) const = 0;

    [[nodiscard]] virtual foundation::Result<void> SetLod(AnimatorHandle handle, AnimationLodLevel lod) = 0;
};

class IPoseProvider
{
public:
    virtual ~IPoseProvider() = default;

    [[nodiscard]] virtual foundation::Result<PoseState> GetPoseState(AnimatorHandle handle) const = 0;

    [[nodiscard]] virtual foundation::Result<PoseSnapshot> GetPoseSnapshot(AnimatorHandle handle) const = 0;
    [[nodiscard]] virtual foundation::Result<PoseBuffer> GetPoseBuffer(AnimatorHandle handle) const = 0;
};

class IAnimationResourceSource
{
public:
    virtual ~IAnimationResourceSource() = default;

    [[nodiscard]] virtual foundation::Result<SkeletonDesc> LoadSkeleton(SkeletonId id) const = 0;
    [[nodiscard]] virtual foundation::Result<AnimationClipDesc> LoadClip(AnimationClipId id) const = 0;
};

class IAnimationPoseSink
{
public:
    virtual ~IAnimationPoseSink() = default;

    // Failure or exception means the pose was not externally committed. A sink that can commit
    // before reporting failure must provide its own idempotent/deduplicated publication boundary.
    [[nodiscard]] virtual foundation::Result<void> Publish(std::shared_ptr<const PoseBuffer> pose) = 0;
};

class IAnimationEvaluatorBackend
{
public:
    virtual ~IAnimationEvaluatorBackend() = default;

    [[nodiscard]] virtual foundation::Result<PoseBuffer> EvaluatePose(const AnimationEvaluationRequest& request) const = 0;
};

class IAnimationEventBuffer
{
public:
    virtual ~IAnimationEventBuffer() = default;

    [[nodiscard]] virtual std::span<const AnimationEvent> Events() const = 0;

    virtual void Clear() = 0;
};

struct AnimationDependencies
{
    std::shared_ptr<IAnimationResourceSource> resources;
    std::shared_ptr<IAnimationPoseSink> pose_sink;
    std::shared_ptr<IAnimationEvaluatorBackend> evaluator;
};

struct AnimationServices
{
    std::shared_ptr<ISkeletonRegistry> skeletons;
    std::shared_ptr<IAnimationClipRegistry> clips;
    std::shared_ptr<IAnimationRuntime> runtime;
    std::shared_ptr<IPoseProvider> poses;
    std::shared_ptr<IAnimationEventBuffer> events;
};

[[nodiscard]] foundation::Result<AnimationServices> CreateAnimationServices(
    AnimationOptions options = {},
    AnimationDependencies dependencies = {});
[[nodiscard]] AnimationServices CreateReferenceAnimationServices(AnimationOptions options = {});
[[nodiscard]] AnimationServices CreateMockAnimationServices(AnimationOptions options = {});
} // namespace epidemic::runtime::animation
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/Runtime/Animation/animation_types.h`.

### Состав типов и операции

#### `ISkeletonRegistry` (строка заголовка 14)

Публичные сигнатуры операций:

```cpp
virtual ~ISkeletonRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterSkeleton(SkeletonDesc desc) = 0;
[[nodiscard]] virtual bool HasSkeleton(SkeletonId id) const = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAnimationClipRegistry` (строка заголовка 26)

Публичные сигнатуры операций:

```cpp
virtual ~IAnimationClipRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterClip(AnimationClipDesc desc) = 0;
[[nodiscard]] virtual bool HasClip(AnimationClipId id) const = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAnimationRuntime` (строка заголовка 38)

Публичные сигнатуры операций:

```cpp
virtual ~IAnimationRuntime() = default;
[[nodiscard]] virtual foundation::Result<AnimatorHandle> CreateAnimatorHandle(const AnimatorDesc& desc) = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyAnimator(AnimatorHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Play(const AnimationPlaybackCommand& command) = 0;
[[nodiscard]] virtual foundation::Result<void> Pause(AnimatorHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Stop(AnimatorHandle handle) = 0;
[[nodiscard]] virtual foundation::Result<void> Crossfade(AnimatorHandle handle, AnimationClipId clip, FrameDuration duration) = 0;
[[nodiscard]] virtual foundation::Result<std::size_t> Tick(FrameDuration delta, std::size_t max_animators) = 0;
[[nodiscard]] virtual foundation::Result<AnimatorSnapshot> GetAnimatorSnapshot(AnimatorHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetLod(AnimatorHandle handle, AnimationLodLevel lod) = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IPoseProvider` (строка заголовка 59)

Публичные сигнатуры операций:

```cpp
virtual ~IPoseProvider() = default;
[[nodiscard]] virtual foundation::Result<PoseState> GetPoseState(AnimatorHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<PoseSnapshot> GetPoseSnapshot(AnimatorHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<PoseBuffer> GetPoseBuffer(AnimatorHandle handle) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAnimationResourceSource` (строка заголовка 70)

Публичные сигнатуры операций:

```cpp
virtual ~IAnimationResourceSource() = default;
[[nodiscard]] virtual foundation::Result<SkeletonDesc> LoadSkeleton(SkeletonId id) const = 0;
[[nodiscard]] virtual foundation::Result<AnimationClipDesc> LoadClip(AnimationClipId id) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAnimationPoseSink` (строка заголовка 79)

Публичные сигнатуры операций:

```cpp
virtual ~IAnimationPoseSink() = default;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAnimationEvaluatorBackend` (строка заголовка 89)

Публичные сигнатуры операций:

```cpp
virtual ~IAnimationEvaluatorBackend() = default;
[[nodiscard]] virtual foundation::Result<PoseBuffer> EvaluatePose(const AnimationEvaluationRequest& request) const = 0;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `IAnimationEventBuffer` (строка заголовка 97)

Публичные сигнатуры операций:

```cpp
virtual ~IAnimationEventBuffer() = default;
[[nodiscard]] virtual std::span<const AnimationEvent> Events() const = 0;
virtual void Clear() = 0;
```

Присутствует полиморфный контракт: проверьте реализацию виртуальных методов и семантику времени жизни объекта, переданного потребителю.

#### `AnimationDependencies` (строка заголовка 107)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<IAnimationResourceSource> resources`
- `std::shared_ptr<IAnimationPoseSink> pose_sink`
- `std::shared_ptr<IAnimationEvaluatorBackend> evaluator`

#### `AnimationServices` (строка заголовка 114)

Публичные поля и значения по умолчанию:

- `std::shared_ptr<ISkeletonRegistry> skeletons`
- `std::shared_ptr<IAnimationClipRegistry> clips`
- `std::shared_ptr<IAnimationRuntime> runtime`
- `std::shared_ptr<IPoseProvider> poses`
- `std::shared_ptr<IAnimationEventBuffer> events`

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::span` является невладеющим представлением; вызывающий отвечает за срок жизни исходного буфера.
- Для `std::shared_ptr` отдельно проверяйте циклы владения и контракт передачи внешних объектов.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `RegisterSkeleton` | `AnimationRuntime::RegisterSkeleton` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:363` |
| `HasSkeleton` | `AnimationRuntime::HasSkeleton` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:375` |
| `RegisterClip` | `AnimationRuntime::RegisterClip` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:389` |
| `HasClip` | `AnimationRuntime::HasClip` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:402` |
| `Freeze` | `AnimationRuntime::Freeze` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:415` |
| `IsFrozen` | `AnimationRuntime::IsFrozen` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:421` |
| `CreateAnimatorHandle` | `AnimationRuntime::CreateAnimatorHandle` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:427` |
| `DestroyAnimator` | `AnimationRuntime::DestroyAnimator` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:455` |
| `Play` | `AnimationRuntime::Play` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:468` |
| `Pause` | `AnimationRuntime::Pause` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:483` |
| `Stop` | `AnimationRuntime::Stop` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:492` |
| `Crossfade` | `AnimationRuntime::Crossfade` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:503` |
| `Tick` | `AnimationRuntime::Tick` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:518` |
| `GetAnimatorSnapshot` | `AnimationRuntime::GetAnimatorSnapshot` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:560` |
| `SetLod` | `AnimationRuntime::SetLod` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:572` |
| `GetPoseState` | `AnimationRuntime::GetPoseState` | `EngineRuntime/Animation/src/animation_runtime_impl.cpp:580` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `Tick`: `EngineRuntime/Animation/tests/animation_tests.cpp:148`
- `GetAnimatorSnapshot`: `EngineRuntime/Animation/tests/animation_tests.cpp:162`
- `RegisterSkeleton`: `EngineRuntime/Animation/tests/animation_tests.cpp:175`
- `RegisterClip`: `EngineRuntime/Animation/tests/animation_tests.cpp:176`
- `CreateAnimatorHandle`: `EngineRuntime/Animation/tests/animation_tests.cpp:185`
- `DestroyAnimator`: `EngineRuntime/Animation/tests/animation_tests.cpp:188`
- `Play`: `EngineRuntime/Animation/tests/animation_tests.cpp:199`
- `GetPoseSnapshot`: `EngineRuntime/Animation/tests/animation_tests.cpp:212`
- `Events`: `EngineRuntime/Animation/tests/animation_tests.cpp:214`
- `GetPoseState`: `EngineRuntime/Animation/tests/animation_tests.cpp:217`
- `Clear`: `EngineRuntime/Animation/tests/animation_tests.cpp:222`
- `SetLod`: `EngineRuntime/Animation/tests/animation_tests.cpp:248`


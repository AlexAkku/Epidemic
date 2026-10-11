---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "animation"]
source_path: "EngineRuntime/Animation"
---

# EngineRuntime / Animation

Модуль: **Animation**. Animation sampling и pose buffers.

**Реализация:** `EngineRuntime/Animation/`.
**CMake targets:** `EpidemicRuntimeAnimation`
**Локальные test targets:** `EpidemicRuntimeAnimationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Animation/animation.h|animation.h]]: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation.h`
- [[11 API/EngineRuntime/Animation/animation_runtime.h|animation_runtime.h]]: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h`; объявленные типы: `ISkeletonRegistry`, `IAnimationClipRegistry`, `IAnimationRuntime`, `IPoseProvider`, `IAnimationResourceSource`, `IAnimationPoseSink`, `IAnimationEvaluatorBackend`, `IAnimationEventBuffer`, `AnimationDependencies`, `AnimationServices`
- [[11 API/EngineRuntime/Animation/animation_types.h|animation_types.h]]: `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`; объявленные типы: `SkeletonId`, `AnimationClipId`, `AnimatorInstanceId`, `AnimatorLifecycle`, `AnimatorReadiness`, `AnimatorPlaybackState`, `PoseState`, `AnimationLodLevel`, `SkeletonDesc`, `AnimationClipDesc`, `AnimatorPlayback`, `CrossfadeState`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/animation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Animation управляет skeleton/clip metadata, animator instances, playback, crossfade, LOD, pose publication и events. Реальная оценка pose вынесена в evaluator backend; Animation не знает Renderer.

#### Контракты

`ISkeletonRegistry` и `IAnimationClipRegistry` регистрируют descriptors. `IAnimationRuntime` создает generation handle, выполняет Play/Pause/Stop/Crossfade/Tick и возвращает snapshots. `IAnimationResourceSource` загружает skeleton/clip descriptors, `IAnimationEvaluatorBackend` оценивает pose, а `IAnimationPoseSink` публикует immutable `PoseBuffer`.

`PoseBuffer` содержит `AnimatorHandle`, `RuntimeObjectId owner`, bone transforms и revision. Owner является нейтральной связью с runtime object и позволяет Support передать pose Renderer без прямой зависимости Animation → Renderer.

#### Playback и владение

Frame time выражается `RuntimeFrameDuration`. Crossfade и playback state принадлежат animator. Pose после публикации является immutable snapshot. Resource adapter может временно удерживать ResourceLease во время загрузки, но после копирования готового skeleton/clip descriptor payload больше не обязан оставаться pinned.

#### Граница

Animation не содержит animation graph gameplay rules, actor state, Renderer contracts или конкретный resource manager. Retargeting, IK, motion matching и сложный graph могут появляться выше либо за отдельными backend/extension contracts.

#### Стабильность

После локального аудита Goal 3 и canonical evidence convergence модуль имеет статус `LOCAL_READY`. Это не означает `FROZEN`: общесистемные integration/fault/load gates и финальный freeze выполняются в последующих Goals 5–9. Cross-major pose wiring реализуется только в Support.

Источник контрактов: `Documentation/EngineRuntime/modules/animation.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `AnimatorLifecycle` (`EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`): `Uninitialized`, `Ready`, `Disabled`.
- `AnimatorReadiness` (`EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`): `Ready`, `ResourceMissing`.
- `AnimatorPlaybackState` (`EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`): `Stopped`, `Playing`, `Paused`, `Blending`, `Finished`.
- `PoseState` (`EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`): `Clean`, `Dirty`, `Evaluating`, `Ready`.
- `AnimationLodLevel` (`EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h`): `Full`, `Reduced`, `Frozen`.

### Операции и хранилища состояния

#### `IAnimationRuntime`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:38`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

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

#### `ISkeletonRegistry`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISkeletonRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterSkeleton(SkeletonDesc desc) = 0;
[[nodiscard]] virtual bool HasSkeleton(SkeletonId id) const = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

#### `IAnimationClipRegistry`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationClipRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> RegisterClip(AnimationClipDesc desc) = 0;
[[nodiscard]] virtual bool HasClip(AnimationClipId id) const = 0;
[[nodiscard]] virtual foundation::Result<void> Freeze() = 0;
[[nodiscard]] virtual bool IsFrozen() const noexcept = 0;
```

#### `IPoseProvider`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:59`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPoseProvider() = default;
[[nodiscard]] virtual foundation::Result<PoseState> GetPoseState(AnimatorHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<PoseSnapshot> GetPoseSnapshot(AnimatorHandle handle) const = 0;
[[nodiscard]] virtual foundation::Result<PoseBuffer> GetPoseBuffer(AnimatorHandle handle) const = 0;
```

#### `IAnimationResourceSource`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationResourceSource() = default;
[[nodiscard]] virtual foundation::Result<SkeletonDesc> LoadSkeleton(SkeletonId id) const = 0;
[[nodiscard]] virtual foundation::Result<AnimationClipDesc> LoadClip(AnimationClipId id) const = 0;
```

#### `IAnimationEventBuffer`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:97`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationEventBuffer() = default;
[[nodiscard]] virtual std::span<const AnimationEvent> Events() const = 0;
virtual void Clear() = 0;
```

#### `IAnimationEvaluatorBackend`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:89`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationEvaluatorBackend() = default;
[[nodiscard]] virtual foundation::Result<PoseBuffer> EvaluatePose(const AnimationEvaluationRequest& request) const = 0;
```

#### `IAnimationPoseSink`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:79`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IAnimationPoseSink() = default;
```

#### `AnimationServices`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_runtime.h:114`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<ISkeletonRegistry> skeletons`; `std::shared_ptr<IAnimationClipRegistry> clips`; `std::shared_ptr<IAnimationRuntime> runtime`; `std::shared_ptr<IPoseProvider> poses`; `std::shared_ptr<IAnimationEventBuffer> events`.

#### `SkeletonId`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `AnimationClipId`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `AnimatorInstanceId`

Объявлен в `EngineRuntime/Animation/include/Epidemic/Runtime/Animation/animation_types.h:34`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Animation/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Animation/src/animation_runtime_impl.cpp` (932 строк): `AnimationRuntime::AnimationRuntime`, `AnimationRuntime::RegisterSkeleton`, `Error::Create`, `AnimationRuntime::HasSkeleton`, `AnimationRuntime::RegisterClip`, `AnimationRuntime::HasClip`, `AnimationRuntime::Freeze`, `AnimationRuntime::IsFrozen`, `AnimationRuntime::CreateAnimatorHandle`, `AnimationRuntime::DestroyAnimator`
- `EngineRuntime/Animation/src/animation.cpp` (71 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `animation.registry_frozen`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:391`
- `animation.duplicate_skeleton`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:368`
- `animation.allocation_failed`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:660`
- `animation.skeleton_not_found`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:629`
- `animation.duplicate_clip`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:395`
- `animation.invalid_owner`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:429`
- `animation.invalid_lod`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:574`
- `animation.animator_capacity`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:431`
- `animation.animator_id_exhausted`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:434`
- `animation.animator_generation_exhausted`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:435`
- `animation.duplicate_animator_id`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:447`
- `animation.animator_not_found`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:610`
- `animation.invalid_playback`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:472`
- `animation.skeleton_mismatch`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:509`
- `animation.started`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:479`
- `animation.invalid_playback_transition`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:486`
- `animation.stopped`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:499`
- `animation.invalid_fade`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:505`
- `animation.crossfade`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:514`
- `animation.invalid_delta`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:520`
- `animation.evaluation_failed`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:532`
- `animation.evaluator_exception`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:533`
- `animation.clip_duration_overflow`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:899`
- `animation.looped`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:553`
- `animation.finished`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:554`
- `animation.resource_exception`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:649`
- `animation.invalid_skeleton`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:874`
- `animation.clip_not_found`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:648`
- `animation.invalid_clip`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:894`
- `animation.evaluator_missing`: `EngineRuntime/Animation/src/animation.cpp:38`
- `animation.invalid_pose`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:789`
- `animation.publish_exception`: `EngineRuntime/Animation/src/animation_runtime_impl.cpp:801`

### Конкретные тестовые якоря

- `EngineRuntime/Animation/tests/animation_tests.cpp`: `TestCreateDestroyAnimator`, `TestPlayInvalidClipReturnsError`, `TestStateTransitionsAndEvents`, `TestTickOrderIsDeterministic`, `TestLodPlaceholderChangesPoseState`, `TestValidationFailures`, `TestNonFiniteNumericInputsAreRejected`, `TestHandlePlaybackPoseBufferAndFactory`, `TestPoseQueriesRejectStaleHandle`, `TestPauseRejectsStoppedAndFinished`, `TestCrossfadeWithoutCurrentClipStartsTargetWithoutBlend`, `TestLoopSettingIsolatedPerAnimatorAndClipMetadataImmutable`, `TestResourceSourceFailureAndPoseSinkPublication`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `animation.registry_frozen` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:391`)

```cpp
389 foundation::Result<void> AnimationRuntime::RegisterClip(AnimationClipDesc desc)
  390 {
  391     if (registries_frozen_) return foundation::Result<void>::Failure(foundation::Error::Create("animation.registry_frozen", "animation registries are frozen"));
  392     const auto validation = ValidateClip(desc);
  393     if (!validation) return validation;
```

#### `animation.duplicate_skeleton` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:368`)

```cpp
366     const auto validation = ValidateSkeleton(desc);
  367     if (!validation) return validation;
  368     if (skeletons_.contains(desc.id)) return foundation::Result<void>::Failure(foundation::Error::Create("animation.duplicate_skeleton", "skeleton is already registered"));
  369     try { skeletons_.emplace(desc.id, desc); }
  370     catch (...) { return foundation::Result<void>::Failure(foundation::Error::Create("animation.allocation_failed", "failed to register skeleton")); }
```

#### `animation.allocation_failed` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:660`)

```cpp
658             foundation::Error::Create("animation.invalid_clip", "resource source returned invalid clip descriptor"));
  659     }
  660     if(!registries_frozen_) { try{clips_.emplace(id,loaded.Value());}catch(...){return foundation::Result<AnimationClipDesc>::Failure(foundation::Error::Create("animation.allocation_failed","failed to cache clip"));} } return foundation::Result<AnimationClipDesc>::Success(loaded.Value());
  661 }
  662
```

#### `animation.skeleton_not_found` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:629`)

```cpp
627 {
  628     const auto it=skeletons_.find(id); if(it!=skeletons_.end()) return foundation::Result<SkeletonDesc>::Success(it->second);
  629     if(!dependencies_.resources) return foundation::Result<SkeletonDesc>::Failure(foundation::Error::Create("animation.skeleton_not_found","skeleton was not registered"));
  630     foundation::Result<SkeletonDesc> loaded=foundation::Result<SkeletonDesc>::Failure(foundation::Error::Create("animation.resource_exception","skeleton callback did not run")); try{loaded=dependencies_.resources->LoadSkeleton(i…
```

#### `animation.duplicate_clip` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:395`)

```cpp
393     if (!validation) return validation;
  394     if (!HasSkeleton(desc.skeleton)) return foundation::Result<void>::Failure(foundation::Error::Create("animation.skeleton_not_found", "clip must reference a registered skeleton"));
  395     if (clips_.contains(desc.id)) return foundation::Result<void>::Failure(foundation::Error::Create("animation.duplicate_clip", "animation clip is already registered"));
  396     try { clips_.emplace(desc.id, desc); }
  397     catch (...) { return foundation::Result<void>::Failure(foundation::Error::Create(…
```

#### `animation.invalid_owner` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:429`)

```cpp
427 foundation::Result<AnimatorHandle> AnimationRuntime::CreateAnimatorHandle(const AnimatorDesc& desc)
  428 {
  429     if (!desc.owner.IsValid()) return foundation::Result<AnimatorHandle>::Failure(foundation::Error::Create("animation.invalid_owner", "animator owner must be valid before creation"));
  430     switch (desc.lod) { case AnimationLodLevel::Full: case AnimationLodLevel::Reduced: case AnimationLodLevel::Frozen: break; default: return foundation::Result<AnimatorHandle>::Failure(foundation::Error::Create("animation.invalid_lod", "ani…
```

#### `animation.invalid_lod` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:574`)

```cpp
572 foundation::Result<void> AnimationRuntime::SetLod(AnimatorHandle handle, AnimationLodLevel lod)
  573 {
  574     switch(lod){case AnimationLodLevel::Full:case AnimationLodLevel::Reduced:case AnimationLodLevel::Frozen:break;default:return foundation::Result<void>::Failure(foundation::Error::Create("animation.invalid_lod","animation LOD value is invalid"));}
  575     AnimatorRecord* animator=FindAnimator(handle); if(!animator) return foundation::Result<void>::Failure(foundation::Error::Create("animation.animator_not_found","animator was not…
```

#### `animation.animator_capacity` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:431`)

```cpp
429     if (!desc.owner.IsValid()) return foundation::Result<AnimatorHandle>::Failure(foundation::Error::Create("animation.invalid_owner", "animator owner must be valid before creation"));
  430     switch (desc.lod) { case AnimationLodLevel::Full: case AnimationLodLevel::Reduced: case AnimationLodLevel::Frozen: break; default: return foundation::Result<AnimatorHandle>::Failure(foundation::Error::Create("animation.invalid_lod", "animator LOD value is invalid")); }
  431     if (options_.max_animators != 0 && animators_.size() >= options_.max_an…
```

#### `animation.animator_id_exhausted` (`EngineRuntime/Animation/src/animation_runtime_impl.cpp:434`)

```cpp
432     const auto skeleton = ResolveSkeleton(desc.skeleton);
  433     if (!skeleton) return foundation::Result<AnimatorHandle>::Failure(skeleton.GetError());
  434     const auto id_value = PeekMonotonicId(next_animator_value_, "animation.animator_id_exhausted", "animator id allocator is exhausted");
  435     const auto generation = PeekMonotonicId(next_generation_, "animation.animator_generation_exhausted", "animator generation allocator is exhausted");
  436     if (!id_value) return foundation::Result<AnimatorHandle>::Failure(id_value.Get…
```

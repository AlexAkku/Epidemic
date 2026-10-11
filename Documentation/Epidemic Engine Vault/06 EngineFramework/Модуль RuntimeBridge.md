---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "runtime_bridge"]
source_path: "EngineFramework/RuntimeBoundary/RuntimeBridge"
---

# EngineFramework / RuntimeBridge

Модуль: **RuntimeBridge**. Мост Runtime contracts ↔ Gameplay identity.

**Реализация:** `EngineFramework/RuntimeBoundary/RuntimeBridge/`.
**CMake targets:** `EpidemicGameFrameworkRuntimeBridge`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: B01 Goal 4 review. Local module contract only.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/RuntimeBridge/runtime_bridge.h|runtime_bridge.h]]: `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h`; объявленные типы: `RuntimeObjectHandle`, `RuntimePersistentObjectHandle`, `RuntimePhysicsBodyHandle`, `RuntimeRegionHandle`, `RuntimeVector3`, `RuntimeQuaternion`, `RuntimeWorldPosition`, `RuntimeWorldAabb`, `RuntimeEnvironmentValues`, `RuntimeBindingState`, `RuntimeProjectionPriority`, `RuntimeBindingGeneration`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/modules/runtime_bridge|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Состояния и доменные перечисления

- `RuntimeBindingState` (`EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h`): `Materializing`, `Active`, `Dematerializing`, `Failed`.
- `RuntimeProjectionPriority` (`EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h`): `Critical`, `High`, `Normal`, `Background`.
- `RuntimeNavigationPathState` (`EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h`): `Pending`, `Running`, `PartiallyComplete`, `Completed`, `Failed`, `Cancelled`, `Stale`.

### Операции и хранилища состояния

#### `RuntimeBridgeService`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:390`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
) : backend_(backend), queue_policy_(queue_policy);
[[nodiscard]] foundation::Result<void> Enqueue(RuntimeProjectionRequest request);
[[nodiscard]] std::optional<RuntimeBinding> GetBinding(GameplayObjectRef object) const noexcept;
[[nodiscard]] std::optional<RuntimeBinding> GetBinding(RuntimeObjectHandle object) const noexcept;
[[nodiscard]] foundation::Result<std::vector<SemanticRayHit>> Raycast(const RuntimeRayQuery& query) const;
[[nodiscard]] foundation::Result<std::vector<SemanticOverlapHit>> Overlap(const RuntimeOverlapQuery& query) const;
[[nodiscard]] foundation::Result<bool> Visible(GameplayObjectRef observer, GameplayObjectRef target) const;
[[nodiscard]] foundation::Result<RuntimeTransformObservation> ObserveTransform(GameplayObjectRef object) const;
[[nodiscard]] foundation::Result<void> ForgetObjectIdentity(GameplayObjectRef object);
[[nodiscard]] foundation::Result<void> ForgetEnvironmentProjection(RuntimeRegionHandle region);
[[nodiscard]] foundation::Result<void> ForgetWorldAlterationProjection(GameplayObjectId alteration);
[[nodiscard]] RuntimeBridgeCapabilities GetCapabilities() const noexcept;
[[nodiscard]] foundation::Result<RuntimeNavigationQueryHandle> RequestPath(const RuntimeNavigationPathQuery& query);
[[nodiscard]] foundation::Result<RuntimeNavigationPathState> GetPathState(RuntimeNavigationQueryHandle handle) const;
[[nodiscard]] foundation::Result<RuntimeNavigationPath> GetPathResult(RuntimeNavigationQueryHandle handle) const;
[[nodiscard]] foundation::Result<void> CancelPath(RuntimeNavigationQueryHandle handle);
[[nodiscard]] foundation::Result<void> ReleasePath(RuntimeNavigationQueryHandle handle);
[[nodiscard]] foundation::Result<RuntimeEnvironmentSample> SampleEnvironment(RuntimeRegionHandle region) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `IRuntimeBridgeBackend& backend_`; `std::unordered_map<GameplayObjectRef, RuntimeBinding, RefHash> bindings_`; `std::unordered_map<RuntimeObjectHandle, GameplayObjectRef, RuntimeHash> reverse_`; `std::unordered_map<RuntimePhysicsBodyHandle, BodyOwner, BodyHash> body_reverse_`; `std::unordered_map<GameplayObjectRef, std::uint32_t, RefHash> generations_`; `std::unordered_map<std::uint64_t, Revision> environment_projection_revision_`; `std::unordered_map<GameplayObjectId, Revision> world_projection_revision_`; `std::vector<Queued> queue_`.

#### `IRuntimeBridgeBackend`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:312`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRuntimeBridgeBackend() = default;
[[nodiscard]] virtual foundation::Result<RuntimeMaterializationResult> Materialize(RuntimePersistentObjectHandle persistent_id) = 0;
[[nodiscard]] virtual foundation::Result<void> Dematerialize(RuntimeObjectHandle object) = 0;
[[nodiscard]] virtual foundation::Result<void> Destroy(RuntimeObjectHandle object) = 0;
[[nodiscard]] virtual foundation::Result<void> ApplyImpulse(RuntimePhysicsBodyHandle body, RuntimeVector3 impulse) = 0;
[[nodiscard]] virtual foundation::Result<void> ProjectEnvironment( RuntimeRegionHandle region, const RuntimeEnvironmentValues& values, Revision revision) = 0;
[[nodiscard]] virtual RuntimeBridgeCapabilities GetCapabilities() const noexcept;
[[nodiscard]] virtual bool SupportsWorldAlterationProjection() const noexcept;
[[nodiscard]] virtual foundation::Result<void> ProjectWorldAlteration(const WorldAlterationProjectionRequest&);
[[nodiscard]] virtual std::vector<RuntimeContactObservation> ConsumeContacts() = 0;
[[nodiscard]] virtual foundation::Result<std::vector<RuntimeRayHit>> Raycast(const RuntimeRayQuery&) const;
[[nodiscard]] virtual foundation::Result<std::vector<RuntimePhysicsBodyHandle>> Overlap(const RuntimeOverlapQuery&) const;
[[nodiscard]] virtual foundation::Result<bool> Visible(RuntimeObjectHandle, RuntimeObjectHandle) const;
[[nodiscard]] virtual foundation::Result<RuntimeTransformObservation> ObserveTransform(RuntimeObjectHandle) const;
[[nodiscard]] virtual foundation::Result<bool> IsRepresentationAlive(RuntimeObjectHandle) const;
[[nodiscard]] virtual foundation::Result<RuntimeNavigationQueryHandle> RequestPath(const RuntimeNavigationPathQuery&);
[[nodiscard]] virtual foundation::Result<RuntimeNavigationPathState> GetPathState(RuntimeNavigationQueryHandle) const;
[[nodiscard]] virtual foundation::Result<RuntimeNavigationPath> GetPathResult(RuntimeNavigationQueryHandle) const;
```

#### `RuntimeObjectHandle`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:16`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RuntimePhysicsBodyHandle`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:29`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RuntimePersistentObjectHandle`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RuntimeRegionHandle`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:36`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RuntimeNavigationQueryHandle`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:275`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RuntimeVector3`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:42`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `float x = 0.0f, y = 0.0f, z = 0.0f`.

#### `RuntimeQuaternion`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `float x = 0.0f, y = 0.0f, z = 0.0f, w = 1.0f`.

#### `RuntimeWorldPosition`

Объявлен в `EngineFramework/RuntimeBoundary/RuntimeBridge/include/Epidemic/GameFramework/RuntimeBridge/runtime_bridge.h:52`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t x_mm = 0, y_mm = 0, z_mm = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/RuntimeBoundary/RuntimeBridge/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicRuntimeWorld`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp` (1593 строк): `Error::Create`, `EngineRuntimeBridgeBackend::ImportPersistentObjectId`, `EngineRuntimeBridgeBackend::ImportPhysicsBody`, `EngineRuntimeBridgeBackend::ImportRegion`, `EngineRuntimeBridgeBackend::ReleasePersistentObject`, `EngineRuntimeBridgeBackend::ReleasePhysicsBody`, `EngineRuntimeBridgeBackend::ReleaseRegion`, `EngineRuntimeBridgeBackend::Materialize`, `EngineRuntimeBridgeBackend::Dematerialize`, `StringId::FromString`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.runtime_bridge.invalid_runtime_observation`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:42`
- `gameplay.runtime_bridge.visibility_invalid`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:127`
- `gameplay.runtime_bridge.visibility_range`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:107`
- `gameplay.runtime_bridge.persistent_object_unknown`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:284`
- `gameplay.runtime_bridge.physics_body_unknown`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:406`
- `gameplay.runtime_bridge.region_unknown`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:763`
- `gameplay.runtime_bridge.world_unavailable`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:376`
- `gameplay.runtime_bridge.runtime_handle_exhausted`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:295`
- `gameplay.runtime_bridge.runtime_object_missing`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:605`
- `framework.runtime_bridge.dematerialize`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:348`
- `framework.runtime_bridge.destroy`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:391`
- `gameplay.runtime_bridge.physics_unavailable`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:410`
- `gameplay.runtime_bridge.environment_unavailable`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:425`
- `gameplay.runtime_bridge.raycast_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:482`
- `gameplay.runtime_bridge.overlap_invalid`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:1340`
- `gameplay.runtime_bridge.overlap_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:512`
- `gameplay.runtime_bridge.visibility_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:556`
- `gameplay.runtime_bridge.transform_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:611`
- `gameplay.runtime_bridge.reconciliation_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:645`
- `gameplay.runtime_bridge.navigation_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:655`
- `gameplay.runtime_bridge.navigation_handle_exhausted`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:666`
- `gameplay.runtime_bridge.navigation_handle_invalid`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:747`
- `gameplay.runtime_bridge.environment_sample_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:767`
- `gameplay.runtime_bridge.invalid_request`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:851`
- `gameplay.runtime_bridge.environment_projection_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:843`
- `gameplay.runtime_bridge.world_projection_unsupported`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:856`
- `gameplay.runtime_bridge.queue_full`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:1532`
- `gameplay.runtime_bridge.sequence_exhausted`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:924`
- `gameplay.runtime_bridge.binding_stale`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:1106`
- `gameplay.runtime_bridge.part_invalid`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:969`
- `gameplay.runtime_bridge.body_collision`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:979`
- `gameplay.runtime_bridge.persistent_identity_changed`: `EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:1008`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.runtime_bridge.invalid_runtime_observation` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:42`)

```cpp
40     {
   41         return foundation::Result<Integer>::Failure(
   42             E("gameplay.runtime_bridge.invalid_runtime_observation", "runtime observation is outside the semantic numeric range"));
   43     }
   44     return foundation::Result<Integer>::Success(static_cast<Integer>(rounded));
```

#### `gameplay.runtime_bridge.visibility_invalid` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:127`)

```cpp
125     {
  126         return foundation::Result<void>::Failure(
  127             E("gameplay.runtime_bridge.visibility_invalid", "runtime visibility ray is not representable"));
  128     }
  129     return foundation::Result<void>::Success();
```

#### `gameplay.runtime_bridge.visibility_range` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:107`)

```cpp
105     {
  106         return foundation::Result<void>::Failure(E(
  107             "gameplay.runtime_bridge.visibility_range",
  108             "runtime visibility distance is outside the representable raycast range"));
  109     }
```

#### `gameplay.runtime_bridge.persistent_object_unknown` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:284`)

```cpp
282     {
  283         return foundation::Result<RuntimeMaterializationResult>::Failure(
  284             E("gameplay.runtime_bridge.persistent_object_unknown", "runtime persistent object handle is unknown"));
  285     }
  286     if (!world_.materialization)
```

#### `gameplay.runtime_bridge.physics_body_unknown` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:406`)

```cpp
404     if (actual == physics_bodies_.end())
  405     {
  406         return foundation::Result<void>::Failure(E("gameplay.runtime_bridge.physics_body_unknown", "runtime physics body handle is unknown"));
  407     }
  408     if (!physics_.scene)
```

#### `gameplay.runtime_bridge.region_unknown` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:763`)

```cpp
761     if (actual == regions_.end())
  762     {
  763         return foundation::Result<RuntimeEnvironmentSample>::Failure(E("gameplay.runtime_bridge.region_unknown", "runtime region handle is unknown"));
  764     }
  765     if (!environment_)
```

#### `gameplay.runtime_bridge.world_unavailable` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:376`)

```cpp
374     if (!world_.writer)
  375     {
  376         return foundation::Result<void>::Failure(E("gameplay.runtime_bridge.world_unavailable", "runtime world writer is unavailable"));
  377     }
  378     std::uint64_t revision = 0;
```

#### `gameplay.runtime_bridge.runtime_handle_exhausted` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:295`)

```cpp
293     {
  294         return foundation::Result<RuntimeMaterializationResult>::Failure(
  295             E("gameplay.runtime_bridge.runtime_handle_exhausted", "runtime bridge object handle space is exhausted"));
  296     }
  297     // Allocate both map nodes and bucket capacity before external acceptance.
```

#### `gameplay.runtime_bridge.runtime_object_missing` (`EngineFramework/RuntimeBoundary/RuntimeBridge/src/runtime_bridge.cpp:605`)

```cpp
603     {
  604         return foundation::Result<RuntimeTransformObservation>::Failure(
  605             E("gameplay.runtime_bridge.runtime_object_missing", "runtime object is not present in runtime world"));
  606     }
  607     const auto* placement = std::get_if<runtime::WorldSurfacePlacement>(&record->placement);
```

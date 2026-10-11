---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "scene"]
source_path: "EngineRuntime/Scene"
---

# EngineRuntime / Scene

Модуль: **Scene**. Hierarchy и spatial representation.

**Реализация:** `EngineRuntime/Scene/`.
**CMake targets:** `EpidemicRuntimeScene`
**Локальные test targets:** `EpidemicRuntimeSceneTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Scene/bounds.h|bounds.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/bounds.h`
- [[11 API/EngineRuntime/Scene/scene_node.h|scene_node.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h`; объявленные типы: `SceneNodeId`, `SceneNode`, `hash`
- [[11 API/EngineRuntime/Scene/scene_node_registry.h|scene_node_registry.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h`; объявленные типы: `ReparentMode`, `ISceneNodeRegistry`
- [[11 API/EngineRuntime/Scene/scene_query.h|scene_query.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h`; объявленные типы: `SceneNodeSnapshot`, `SceneSnapshot`, `ISceneQuery`, `ISceneSnapshotProvider`
- [[11 API/EngineRuntime/Scene/scene_services.h|scene_services.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h`; объявленные типы: `SceneOptions`, `SceneServices`
- [[11 API/EngineRuntime/Scene/scene_state.h|scene_state.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`; объявленные типы: `SceneAttachmentState`, `SceneMobility`, `SceneVisibilityState`, `SceneDirtyFlags`
- [[11 API/EngineRuntime/Scene/spatial_index.h|spatial_index.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h`; объявленные типы: `ISpatialIndex`
- [[11 API/EngineRuntime/Scene/transform.h|transform.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform.h`
- [[11 API/EngineRuntime/Scene/transform_registry.h|transform_registry.h]]: `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h`; объявленные типы: `WorldTransformWriteMode`, `ITransformRegistry`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/scene|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Scene хранит пространственное представление runtime-объектов: nodes, hierarchy, local/world transforms, bounds, visibility, mobility и spatial queries. Он не хранит gameplay placement, долговечную историю объекта или authoritative persistence state.

#### Контракты

`ISceneNodeRegistry` создает, связывает и уничтожает nodes. `ITransformRegistry` управляет local и world transform, включая `SetWorldTransform()`. `ISpatialIndex` хранит per-node local bounds и вычисляет world bounds, `ISceneQuery` выполняет spatial queries, а snapshot provider формирует detached value snapshot сцены.

`ReparentMode::KeepWorld` сохраняет observable world TRS в рамках deterministic approximate TRS модели RuntimeFoundation. World-to-local conversion дополнительно проходит roundtrip preflight: повторная композиция должна совпадать с requested world transform с absolute tolerance `0.001` и float-relative tolerance `32 * epsilon * magnitude`. Если parent transform численно плохо обусловлен и это условие не выполняется, операция возвращает `scene.unrepresentable_world_transform` без mutation. `KeepLocal` сохраняет local transform. При уничтожении parent его прямые children отсоединяются с `KeepWorld` semantics, поэтому их observable world transform и world transform их descendants сохраняются. Hierarchy traversal выполняется iteratively.

Invalid/stale node IDs, invalid enum values, non-finite/degenerate transforms и invalid bounds отвергаются до mutation. World-to-local conversion отвергается, если parent scale не является invertible по `kSpatialEpsilon`. Любая hierarchy/transform mutation до commit проверяет candidate world TRS всего затрагиваемого subtree: overflow, underflow до degenerate scale и `Inf/NaN` дают `scene.world_transform_out_of_range`. Для nodes с bounds отдельно проверяются все восемь transformed corners; непредставимые world bounds дают `scene.world_bounds_out_of_range`. Multi-record reparent/detach/destroy paths полностью готовят child lists, subtree state и revision range до публикации live mutations.

#### No-op и revision semantics

Повторный attach к уже текущему parent и detach уже detached node являются no-op и не меняют revision. `SetMobility`/`SetVisibility` с уже установленным значением также являются no-op. `MarkTransformClean` и `MarkBoundsClean` меняют только transient dirty bookkeeping и не повышают authoritative revision.

`SetLocalTransform`, `SetWorldTransform` и `SetLocalBounds` являются explicit authored writes. Успешная повторная запись того же значения считается новой accepted mutation, повышает revision и выставляет соответствующие dirty flags. Это позволяет явно переопубликовать transform/bounds без отдельного invalidate API.

#### Bounds и queries

World AABB вычисляется из всех восьми corners после применения deterministic TRS и корректно поддерживает rotation, non-uniform scale и negative scale. `QueryAabb` и `QuerySphere` возвращают только visible nodes с bounds в возрастающем `SceneNodeId` order. Invalid query geometry возвращает пустой result и не меняет Scene state.

#### Snapshot и persistence scope

`SceneSnapshot` является deterministic detached value snapshot: nodes сортируются по `SceneNodeId`, internal vectors/maps/aliases наружу не публикуются, dirty flags в snapshot не входят. Snapshot предназначен для observation/system-level materialization evidence, а не является локальным persistence checkpoint.

Scene официально является transient runtime materialization state. Локальный `Restore()` для Scene в Goal 3 не требуется и не добавляется. System-level восстановление выполняется через rematerialization/reconciliation в Goals 5 и 7.

#### Scope

Scene локально владеет только nodes, hierarchy, transforms, per-node bounds, visibility/mobility metadata, dirty bookkeeping и spatial query/snapshot behavior. `ISceneProjectionQueue` не принадлежит Scene и проверяется в Runtime Support. Physics, Animation и другие majors взаимодействуют со Scene через Support adapters, а не через peer dependencies.

#### Стабильность

После прохождения Goal 3.7 и canonical evidence convergence модуль имеет статус `LOCAL_READY`. `SYSTEM_READY` и `FROZEN` присваиваются только после обязательных системных gates Goals 5–9.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `bounds.h`: factory-функции или backend implementation без отдельного публичного типа.
- `scene_node.h`: `SceneNodeId`, `SceneNode`.
- `scene_node_registry.h`: `ReparentMode`, `ISceneNodeRegistry`.
- `scene_query.h`: `SceneNodeSnapshot`, `SceneSnapshot`, `ISceneQuery`, `ISceneSnapshotProvider`.
- `scene_services.h`: `SceneOptions`, `SceneServices`.
- `scene_state.h`: `SceneAttachmentState`, `SceneMobility`, `SceneVisibilityState`, `SceneDirtyFlags`.
- `spatial_index.h`: `ISpatialIndex`.
- `transform.h`: factory-функции или backend implementation без отдельного публичного типа.
- `transform_registry.h`: `WorldTransformWriteMode`, `ITransformRegistry`.

Источник контрактов: `Documentation/EngineRuntime/modules/scene.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `WorldTransformWriteMode` (`EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h`): `RejectNonInvertibleParent`.
- `SceneAttachmentState` (`EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`): `Detached`, `Attached`.
- `SceneMobility` (`EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`): `Static`, `Dynamic`.
- `SceneVisibilityState` (`EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`): `Visible`, `Hidden`.
- `SceneDirtyFlags` (`EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_state.h`): `None`, `Transform`, `Bounds`, `Hierarchy`, `Visibility`.
- `ReparentMode` (`EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h`): `KeepWorld`, `KeepLocal`.

### Операции и хранилища состояния

#### `ISceneNodeRegistry`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node_registry.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISceneNodeRegistry() = default;
[[nodiscard]] virtual foundation::Result<SceneNodeId> CreateNode() = 0;
[[nodiscard]] virtual foundation::Result<void> DestroyNode(SceneNodeId node) = 0;
[[nodiscard]] virtual bool Exists(SceneNodeId node) const = 0;
[[nodiscard]] virtual std::optional<SceneNode> GetNode(SceneNodeId node) const = 0;
[[nodiscard]] virtual foundation::Result<void> AttachNode(SceneNodeId child, SceneNodeId parent, ReparentMode mode = ReparentMode::KeepWorld) = 0;
[[nodiscard]] virtual foundation::Result<void> DetachNode(SceneNodeId child, ReparentMode mode = ReparentMode::KeepWorld) = 0;
[[nodiscard]] virtual std::optional<SceneNodeId> GetParent(SceneNodeId child) const = 0;
[[nodiscard]] virtual std::vector<SceneNodeId> GetChildren(SceneNodeId parent) const = 0;
[[nodiscard]] virtual foundation::Result<void> SetMobility(SceneNodeId node, SceneMobility mobility) = 0;
[[nodiscard]] virtual foundation::Result<void> SetVisibility(SceneNodeId node, SceneVisibilityState visibility) = 0;
[[nodiscard]] virtual std::uint64_t GetRevision() const = 0;
```

#### `ITransformRegistry`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/transform_registry.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ITransformRegistry() = default;
[[nodiscard]] virtual foundation::Result<void> SetLocalTransform(SceneNodeId node, const Transform& transform) = 0;
[[nodiscard]] virtual foundation::Result<void> SetWorldTransform( SceneNodeId node, const Transform& transform, WorldTransformWriteMode mode = WorldTransformWriteMode::RejectNonInvertibleParent) = 0;
[[nodiscard]] virtual std::optional<Transform> GetLocalTransform(SceneNodeId node) const = 0;
[[nodiscard]] virtual std::optional<Transform> GetWorldTransform(SceneNodeId node) const = 0;
[[nodiscard]] virtual foundation::Result<void> MarkTransformClean(SceneNodeId node) = 0;
[[nodiscard]] virtual bool IsTransformDirty(SceneNodeId node) const = 0;
```

#### `ISpatialIndex`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/spatial_index.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISpatialIndex() = default;
[[nodiscard]] virtual foundation::Result<void> SetLocalBounds(SceneNodeId node, const Aabb& bounds) = 0;
[[nodiscard]] virtual std::optional<Aabb> GetLocalBounds(SceneNodeId node) const = 0;
[[nodiscard]] virtual std::optional<Aabb> GetWorldBounds(SceneNodeId node) const = 0;
[[nodiscard]] virtual foundation::Result<void> MarkBoundsClean(SceneNodeId node) = 0;
[[nodiscard]] virtual bool IsBoundsDirty(SceneNodeId node) const = 0;
```

#### `ISceneQuery`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISceneQuery() = default;
[[nodiscard]] virtual std::vector<SceneNodeId> QueryAabb(const Aabb& bounds) const = 0;
[[nodiscard]] virtual std::vector<SceneNodeId> QuerySphere(const Vec3& center, float radius) const = 0;
```

#### `ISceneSnapshotProvider`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h:41`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISceneSnapshotProvider() = default;
```

#### `SceneServices`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_services.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<ISceneNodeRegistry> nodes`; `std::shared_ptr<ITransformRegistry> transforms`; `std::shared_ptr<ISpatialIndex> spatial`; `std::shared_ptr<ISceneQuery> queries`; `std::shared_ptr<ISceneSnapshotProvider> snapshots`.

#### `SceneNodeId`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h:10`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `SceneNode`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h:32`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `SceneAttachmentState attachment_state = SceneAttachmentState::Detached`; `SceneMobility mobility = SceneMobility::Dynamic`; `SceneVisibilityState visibility = SceneVisibilityState::Visible`; `SceneDirtyMask dirty_flags = 0`; `std::uint64_t revision = 0`.

#### `hash`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_node.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] size_t operator()(epidemic::runtime::SceneNodeId value) const noexcept;
```

#### `SceneNodeSnapshot`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `SceneAttachmentState attachment = SceneAttachmentState::Detached`; `SceneMobility mobility = SceneMobility::Dynamic`; `SceneVisibilityState visibility = SceneVisibilityState::Visible`; `std::uint64_t revision = 0`.

#### `SceneSnapshot`

Объявлен в `EngineRuntime/Scene/include/Epidemic/Runtime/Scene/scene_query.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t revision = 0`; `std::vector<SceneNodeSnapshot> nodes`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Scene/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Scene/src/scene.cpp` (20 строк)
- `EngineRuntime/Scene/src/scene_runtime_impl.cpp` (1048 строк): `Error::Create`, `SceneRuntime::CreateNode`, `SceneRuntime::DestroyNode`, `SceneRuntime::Exists`, `SceneRuntime::GetNode`, `SceneRuntime::AttachNode`, `SceneRuntime::DetachNode`, `SceneRuntime::GetParent`, `SceneRuntime::GetChildren`, `SceneRuntime::SetMobility`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `scene.node_id_exhausted`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:145`
- `scene.allocation_failed`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:999`
- `scene.duplicate_node_id`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:168`
- `scene.node_not_found`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:840`
- `scene.invalid_hierarchy`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:991`
- `scene.invalid_reparent_mode`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:385`
- `scene.hierarchy_cycle`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:288`
- `scene.invalid_transform`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:911`
- `scene.invalid_mobility`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:474`
- `scene.invalid_visibility`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:500`
- `scene.unsupported_world_transform_write_mode`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:571`
- `scene.invalid_bounds`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:659`
- `scene.world_bounds_out_of_range`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:984`
- `scene.invalid_node`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:836`
- `scene.non_invertible_parent_transform`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:900`
- `scene.unrepresentable_world_transform`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:917`
- `scene.world_transform_out_of_range`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:980`
- `scene.revision_overflow`: `EngineRuntime/Scene/src/scene_runtime_impl.cpp:1013`

### Конкретные тестовые якоря

- `EngineRuntime/Scene/tests/scene_tests.cpp`: `TestSplitStateDefaults`, `TestCreateDestroyAndRevision`, `TestHierarchyAttachDetachAndCycleReject`, `TestLocalAndWorldTransforms`, `TestSetWorldTransformRoot`, `TestSetWorldTransformChildUnderParent`, `TestSetWorldTransformChildUnderRotatedScaledParent`, `TestSetWorldTransformRejectsInvalidWithoutMutation`, `TestAttachDetachKeepWorldByDefault`, `TestAttachDetachKeepLocal`, `TestDestroyParentKeepsChildWorldTransform`, `TestWorldBoundsAndValidation`, `TestRotatedScaledWorldBounds`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `scene.node_id_exhausted` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:145`)

```cpp
143     if (next_node_value_ == 0)
  144     {
  145         return SceneFailureValue<SceneNodeId>("scene.node_id_exhausted", "scene node id allocator is exhausted");
  146     }
  147     const auto revisions = ReserveRevisionRange(1);
```

#### `scene.allocation_failed` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:999`)

```cpp
997     catch (const std::exception&)
  998     {
  999         return SceneFailure("scene.allocation_failed", "scene world-state validation allocation failed");
 1000     }
 1001     return foundation::Result<void>::Success();
```

#### `scene.duplicate_node_id` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:168`)

```cpp
166         if (!inserted)
  167         {
  168             return SceneFailureValue<SceneNodeId>("scene.duplicate_node_id", "allocated scene node id already exists");
  169         }
  170     }
```

#### `scene.node_not_found` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:840`)

```cpp
838     if (!Exists(node))
  839     {
  840         return SceneFailure("scene.node_not_found", message);
  841     }
  842     return foundation::Result<void>::Success();
```

#### `scene.invalid_hierarchy` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:991`)

```cpp
989                 if (child == nullptr)
  990                 {
  991                     return SceneFailure("scene.invalid_hierarchy", "scene subtree references a missing child");
  992                 }
  993                 stack.emplace_back(*iterator, ComposeTransform(world, child->local_transform));
```

#### `scene.invalid_reparent_mode` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:385`)

```cpp
383     if (!IsValid(mode))
  384     {
  385         return SceneFailure("scene.invalid_reparent_mode", "scene reparent mode is outside the declared domain");
  386     }
  387     const auto required = RequireNode(child, "scene node was not found for detach");
```

#### `scene.hierarchy_cycle` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:288`)

```cpp
286     if (WouldCreateCycle(child, parent))
  287     {
  288         return SceneFailure("scene.hierarchy_cycle", "scene node attachment would create a cycle");
  289     }
  290
```

#### `scene.invalid_transform` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:911`)

```cpp
909     if (!IsValidTransform(local))
  910     {
  911         return SceneFailureValue<Transform>("scene.invalid_transform", "scene computed local transform is not finite or invertible");
  912     }
  913     const Transform reconstructed_world = ComposeTransform(parent_world, local);
```

#### `scene.invalid_mobility` (`EngineRuntime/Scene/src/scene_runtime_impl.cpp:474`)

```cpp
472     if (!IsValid(mobility))
  473     {
  474         return SceneFailure("scene.invalid_mobility", "scene mobility is outside the declared domain");
  475     }
  476     SceneNodeRecord* record = FindRecord(node);
```

---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "runtime_foundation"]
source_path: "EngineRuntime/RuntimeFoundation"
---

# EngineRuntime / RuntimeFoundation

Модуль: **RuntimeFoundation**. Общие Runtime IDs, budgets и spatial contracts.

**Реализация:** `EngineRuntime/RuntimeFoundation/`.
**CMake targets:** `EpidemicRuntimeFoundation`
**Локальные test targets:** `EpidemicRuntimeFoundationTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/RuntimeFoundation/checked_id_allocator.h|checked_id_allocator.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/checked_id_allocator.h`; объявленные типы: `MonotonicIdReservation`
- [[11 API/EngineRuntime/RuntimeFoundation/numeric_validation.h|numeric_validation.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/numeric_validation.h`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_budget.h|runtime_budget.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_budget.h`; объявленные типы: `RuntimeBudget`, `RuntimeBudgetConsumption`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_foundation.h|runtime_foundation.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_foundation.h`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_handles.h|runtime_handles.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_handles.h`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_ids.h|runtime_ids.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_ids.h`; объявленные типы: `AssetIdTag`, `ResourceIdTag`, `RuntimeObjectIdTag`, `PersistentObjectIdTag`, `LazyRuleIdTag`, `RegionIdTag`, `ChunkIdTag`, `SurfaceIdTag`, `SimulationZoneIdTag`, `NumericRuntimeId`, `AssetId`, `ResourceId`…
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_operation.h|runtime_operation.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_operation.h`; объявленные типы: `AsyncOperationStatus`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_states.h|runtime_states.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`; объявленные типы: `ResidencyState`, `ObjectRealityLevel`, `PersistenceTier`, `SimulationLod`
- [[11 API/EngineRuntime/RuntimeFoundation/runtime_time.h|runtime_time.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_time.h`; объявленные типы: `RuntimeFrameDuration`, `GameDuration`, `GameTimePoint`
- [[11 API/EngineRuntime/RuntimeFoundation/spatial.h|spatial.h]]: `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h`; объявленные типы: `Vec3`, `Quat`, `Transform`, `Aabb`, `Sphere`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/runtime_foundation|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

RuntimeFoundation содержит общий язык EngineRuntime: типобезопасные IDs, budgets, игровое и кадровое время, spatial values и несколько нейтральных состояний. Он не реализует systems и не знает об Assets, World, Renderer или gameplay.

#### Публичная модель

`AssetId`, `ResourceId`, `RuntimeObjectId`, `PersistentObjectId`, `LazyRuleId`, `RegionId`, `ChunkId`, `SurfaceId` и `SimulationZoneId` являются отдельными типами. `RuntimeBudget` ограничивает число операций, bytes и CPU time. `RuntimeFrameDuration` описывает реальное время кадра, а `GameDuration` и `GameTimePoint` — календарное время мира. `Vec3`, `Quat`, `Transform`, `Aabb` и `Sphere` образуют минимальный spatial contract.

Нейтральные enums `AsyncOperationStatus`, `ResidencyState`, `ObjectRealityLevel`, `PersistenceTier` и `SimulationLod` используются несколькими majors, но не содержат gameplay meaning.

#### Инварианты

Нулевой numeric Runtime ID невалиден; для `AssetId` и `ResourceId` тем же единственным invalid state является raw `StringId == 0`. `RuntimeBudget` является только value contract: нулевые item/byte/time limits означают unlimited, а hard/soft/defer semantics определяет consuming major. Арифметика game time имеет checked и saturating variants и не выполняет signed overflow.

`Normalize()` возвращает identity quaternion для нулевой, non-finite либо численно переполненной длины. Это convenience fallback, а не authoritative validation: перед записью состояния вызывающий код обязан использовать `IsFinite`, `IsNormalized` и `IsValidTransform` по своему boundary contract. `IsNormalized` использует tolerance `0.001` по squared length.

`ComposeTransform(parent, local)` фиксирует deterministic approximate TRS contract: position вычисляется через `TransformPoint(parent, local.position)`, rotation через normalized quaternion multiplication, scale через component-wise multiplication. Такая модель намеренно не представляет shear, возникающий при произвольной комбинации non-uniform parent scale и rotated child, и не обещает эквивалентность общему affine matrix multiplication. Публичного matrix decomposition API в RuntimeFoundation нет. Поэтому результат нельзя трактовать как lossless compose/decompose общего affine transform.

#### Стабильность

После Block A, повторной официальной MSVC Debug/Release qualification и canonical evidence convergence модуль имеет статус `LOCAL_READY`. Это не означает whole-engine `SYSTEM_READY` или `FROZEN`. Добавлять сюда новый тип следует только тогда, когда он действительно нужен нескольким независимым majors и не принадлежит их domain.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `runtime_budget.h`: `RuntimeBudget`, `RuntimeBudgetConsumption`.
- `runtime_foundation.h`: umbrella header, который собирает общий RuntimeFoundation contract.
- `runtime_handles.h`: alias template `RuntimeHandle<Tag>` поверх `foundation::Handle<Tag>`.
- `runtime_ids.h`: `AssetIdTag`, `ResourceIdTag`, `RuntimeObjectIdTag`, `PersistentObjectIdTag`, `LazyRuleIdTag`, `RegionIdTag`, `ChunkIdTag`, `SurfaceIdTag`, `SimulationZoneIdTag`, `NumericRuntimeId`, `AssetId`, `ResourceId`, `RuntimeObjectId`, `PersistentObjectId`, `LazyRuleId`, `RegionId`, `ChunkId`, `SurfaceId`, `SimulationZoneId`.
- `runtime_operation.h`: `AsyncOperationStatus`.
- `runtime_states.h`: `ResidencyState`, `ObjectRealityLevel`, `PersistenceTier`, `SimulationLod`.
- `runtime_time.h`: `RuntimeFrameDuration`, `GameDuration`, `GameTimePoint`.
- `spatial.h`: `Vec3`, `Quat`, `Transform`, `Aabb`, `Sphere`.

Источник контрактов: `Documentation/EngineRuntime/modules/runtime_foundation.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `ResidencyState` (`EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`): `Unloaded`, `Loading`, `Resident`, `Active`, `Sleeping`, `Unloading`.
- `ObjectRealityLevel` (`EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`): `AbstractFact`, `Logical`, `Physical`.
- `PersistenceTier` (`EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`): `Disposable`, `TemporaryObserved`, `PlayerTouched`, `Protected`, `QuestCritical`.
- `SimulationLod` (`EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_states.h`): `Dormant`, `Abstract`, `Scheduled`, `Relevant`, `Observed`, `Active`.
- `AsyncOperationStatus` (`EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_operation.h`): `Pending`, `Running`, `WaitingForMainThread`, `PartiallyComplete`, `Completed`, `Failed`, `Cancelled`.

### Операции и хранилища состояния

#### `RuntimeBudget`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_budget.h:14`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool HasTimeLimit() const noexcept;
[[nodiscard]] constexpr bool HasItemLimit() const noexcept;
[[nodiscard]] constexpr bool HasByteLimit() const noexcept;
[[nodiscard]] constexpr bool HasTimeBudget() const noexcept;
[[nodiscard]] constexpr bool HasItemBudget() const noexcept;
[[nodiscard]] constexpr bool HasByteBudget() const noexcept;
[[nodiscard]] constexpr bool IsUnlimited() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t max_items = 0`; `std::uint64_t max_bytes = 0`.

#### `NumericRuntimeId`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_ids.h:41`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
constexpr NumericRuntimeId() noexcept = default;
constexpr explicit NumericRuntimeId(std::uint64_t raw_value) noexcept : value(raw_value);
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint64_t value = 0`.

#### `RuntimeFrameDuration`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_time.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool IsZero() const noexcept;
[[nodiscard]] constexpr bool IsNegative() const noexcept;
```

#### `RuntimeBudgetConsumption`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_budget.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::size_t processed_items = 0`; `std::size_t processed_bytes = 0`.

#### `AssetId`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_ids.h:69`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
constexpr AssetId() noexcept = default;
constexpr explicit AssetId(foundation::StringId raw_value) noexcept : value(raw_value);
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `ResourceId`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_ids.h:101`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
constexpr ResourceId() noexcept = default;
constexpr explicit ResourceId(foundation::StringId raw_value) noexcept : value(raw_value);
[[nodiscard]] constexpr std::uint64_t Raw() const noexcept;
[[nodiscard]] constexpr explicit operator bool() const noexcept;
```

#### `MonotonicIdReservation`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/checked_id_allocator.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
MonotonicIdReservation() = default;
MonotonicIdReservation(TInteger* owner, TInteger value) noexcept : owner_(owner), value_(value);
[[nodiscard]] TInteger Value() const noexcept;
void Commit() noexcept;
void Rollback() noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `TInteger* owner_ = nullptr`; `TInteger value_ = 0`.

#### `GameDuration`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_time.h:138`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] constexpr bool IsZero() const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t ticks = 0`.

#### `GameTimePoint`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/runtime_time.h:151`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t ticks = 0`.

#### `Vec3`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:13`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `float x = 0.0f`; `float y = 0.0f`; `float z = 0.0f`.

#### `Quat`

Объявлен в `EngineRuntime/RuntimeFoundation/include/Epidemic/Runtime/Foundation/spatial.h:22`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `float x = 0.0f`; `float y = 0.0f`; `float z = 0.0f`; `float w = 1.0f`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/RuntimeFoundation/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/RuntimeFoundation/src/runtime_foundation.cpp` (1 строк)

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Конкретные тестовые якоря

- `EngineRuntime/RuntimeFoundation/tests/runtime_foundation_tests.cpp`: `TestDefaultInvalidIds`, `TestCheckedMonotonicIdAllocatorExhaustion`, `TestEquality`, `TestHashWorksInUnorderedMap`, `TestAssetAndResourceIdsUseStringBackedRuntimeIds`, `TestRuntimeBudgetDefaultsToUnlimited`, `TestRuntimeBudgetTracksLimits`, `TestRuntimeBudgetRejectsNegativeTime`, `TestTransactionalMonotonicReservation`, `TestCheckedArithmeticBoundaries`, `TestRuntimeFrameDurationUsesRealMicroseconds`, `TestGameTimeOperations`, `TestGameTimeOverflowHelpers`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.


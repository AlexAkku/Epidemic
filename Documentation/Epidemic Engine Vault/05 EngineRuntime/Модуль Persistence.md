---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineruntime", "module", "persistence"]
source_path: "EngineRuntime/Persistence"
---

# EngineRuntime / Persistence

Модуль: **Persistence**. Durable snapshots, transactions и backend.

**Реализация:** `EngineRuntime/Persistence/`.
**CMake targets:** `EpidemicRuntimePersistence`
**Локальные test targets:** `EpidemicRuntimePersistenceTests`

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineRuntime/Persistence/dirty_tracker.h|dirty_tracker.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h`; объявленные типы: `IDirtyTracker`
- [[11 API/EngineRuntime/Persistence/lazy_rule_record.h|lazy_rule_record.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/lazy_rule_record.h`; объявленные типы: `LazyRuleKind`, `LazyRuleState`, `LazyRuleRecord`
- [[11 API/EngineRuntime/Persistence/persistence_backend.h|persistence_backend.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_backend.h`; объявленные типы: `PersistenceSnapshot`, `PersistenceDurability`, `IPersistenceBackend`
- [[11 API/EngineRuntime/Persistence/persistence_location.h|persistence_location.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_location.h`; объявленные типы: `PersistenceLocation`
- [[11 API/EngineRuntime/Persistence/persistence_operation.h|persistence_operation.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_operation.h`; объявленные типы: `UpsertObjectOperation`, `AdminRemoveObjectOperation`, `DeleteObjectOperation`, `AdminAddTombstoneOperation`, `UpsertLazyRuleOperation`, `UpdateLazyRuleOperation`, `RemoveLazyRuleOperation`, `UpsertZoneOverrideOperation`, `RemoveZoneOverrideOperation`
- [[11 API/EngineRuntime/Persistence/persistence_payload.h|persistence_payload.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_payload.h`; объявленные типы: `PersistencePayload`
- [[11 API/EngineRuntime/Persistence/persistence_policy.h|persistence_policy.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_policy.h`; объявленные типы: `ObjectProtectionFlags`, `ObjectProtectionMask`
- [[11 API/EngineRuntime/Persistence/persistence_services.h|persistence_services.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_services.h`; объявленные типы: `PersistenceOptions`, `PersistenceServices`
- [[11 API/EngineRuntime/Persistence/persistence_state.h|persistence_state.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_state.h`; объявленные типы: `PersistenceState`, `PersistentObjectKind`
- [[11 API/EngineRuntime/Persistence/persistence_store.h|persistence_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h`; объявленные типы: `IPersistenceQuery`, `IPersistenceStore`
- [[11 API/EngineRuntime/Persistence/persistent_object_store.h|persistent_object_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h`; объявленные типы: `IPersistentObjectStore`
- [[11 API/EngineRuntime/Persistence/persistent_record.h|persistent_record.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_record.h`; объявленные типы: `PersistentObjectRecord`
- [[11 API/EngineRuntime/Persistence/save_transaction.h|save_transaction.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h`; объявленные типы: `SaveTransactionState`, `ISaveTransaction`, `IPersistenceAdministrativeTransaction`
- [[11 API/EngineRuntime/Persistence/tombstone_store.h|tombstone_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h`; объявленные типы: `TombstoneRecord`, `ITombstoneStore`
- [[11 API/EngineRuntime/Persistence/zone_override_store.h|zone_override_store.h]]: `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/zone_override_store.h`; объявленные типы: `ZoneOverrideSnapshot`, `IZoneOverrideStore`

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineRuntime/modules/persistence|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Persistence хранит долговечное состояние объектов, tombstones, lazy rules и zone overrides. Он обеспечивает атомарный commit snapshot и отделяет write transaction от read-only query.

#### Модель

`IPersistenceStore` открывает `ISaveTransaction`. Transaction накапливает ordered operations и строит candidate snapshot. Backend получает только существующий `CommitSnapshot(snapshot, durability)`; отдельные `Save()`/`Flush()` API не являются частью Runtime contract. При Result failure, exception или allocation failure прежний durable snapshot остается authoritative, а in-memory store не публикует candidate.

Успешный commit terminal. Failed commit переводит transaction в `Failed`; повторный commit той же transaction запрещен до terminal `Rollback`. Повторный `Commit()` уже committed transaction является idempotent success. Empty transaction является валидным commit и повышает revision. `PersistenceDurability` имеет закрытый domain из `MemoryOnly`, `SaveRequired`, `SaveAndFlushRequired`; другие значения отклоняются при создании services до обращения к backend.

`IPersistenceQuery` читает records без возможности открыть transaction. `IPersistenceAdministrativeTransaction` предназначен только restore, import и migration code. `IDirtyTracker` отражает unsaved changes.

#### Данные

`PersistentObjectRecord` хранит payload, location, kind, protection и revision. Tombstone фиксирует уничтожение. `LazyRuleRecord` описывает отложенное изменение. `ZoneOverrideSnapshot` хранит локальные records и tombstones региона.

#### Инварианты

Active record не может одновременно быть tombstoned. Lazy rule не ссылается на отсутствующий target. Delete удаляет связанные lazy rules. `UpdateLazyRule` не выполняет upsert. Snapshots сортируются детерминированно и проходят полную validation. Query results и snapshots возвращаются detached copies и не дают mutable aliases во внутреннее состояние.

#### Стабильность

После завершения Goal 3 и общего Runtime admission gate модуль имеет статус `LOCAL_READY`. Backend может быть memory, file или database implementation при сохранении atomic commit contract; системный `FROZEN` остаётся за Goals 5–9.

#### Карта публичных заголовков

Этот раздел служит быстрым индексом объявлений. Семантика и инварианты описаны выше; точные сигнатуры остаются источником истины в public headers.

- `dirty_tracker.h`: `IDirtyTracker`.
- `lazy_rule_record.h`: `LazyRuleKind`, `LazyRuleState`, `LazyRuleRecord`.
- `persistence_backend.h`: `PersistenceSnapshot`, `PersistenceDurability`, `IPersistenceBackend`.
- `persistence_location.h`: `PersistenceLocation`.
- `persistence_operation.h`: `UpsertObjectOperation`, `AdminRemoveObjectOperation`, `DeleteObjectOperation`, `AdminAddTombstoneOperation`, `UpsertLazyRuleOperation`, `UpdateLazyRuleOperation`, `RemoveLazyRuleOperation`, `UpsertZoneOverrideOperation`, `RemoveZoneOverrideOperation`.
- `persistence_payload.h`: `PersistencePayload`.
- `persistence_policy.h`: `ObjectProtectionFlags`, `ObjectProtectionMask`.
- `persistence_services.h`: `PersistenceOptions`, `PersistenceServices`.
- `persistence_state.h`: `PersistenceState`, `PersistentObjectKind`.
- `persistence_store.h`: `IPersistenceQuery`, `IPersistenceStore`.
- `persistent_object_store.h`: `IPersistentObjectStore`.
- `persistent_record.h`: `PersistentObjectRecord`.
- `save_transaction.h`: `SaveTransactionState`, `ISaveTransaction`, `IPersistenceAdministrativeTransaction`.
- `tombstone_store.h`: `TombstoneRecord`, `ITombstoneStore`.
- `zone_override_store.h`: `ZoneOverrideSnapshot`, `IZoneOverrideStore`.

Источник контрактов: `Documentation/EngineRuntime/modules/persistence.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `SaveTransactionState` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h`): `NotStarted`, `Open`, `Committing`, `Committed`, `RolledBack`, `Failed`.
- `PersistenceDurability` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_backend.h`): `MemoryOnly`, `SaveRequired`, `SaveAndFlushRequired`.
- `PersistenceState` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_state.h`): `Clean`, `Dirty`, `PendingSave`, `Saving`, `Saved`, `LoadPending`, `Loaded`, `Deleted`, `Tombstoned`, `Conflict`, `Corrupted`.
- `PersistentObjectKind` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_state.h`): `Unknown`, `Object`, `ContainerEntry`, `SurfaceState`, `ZoneOverride`, `AbstractFact`.
- `LazyRuleKind` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/lazy_rule_record.h`): `Decay`, `Theft`, `Cleanup`.
- `LazyRuleState` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/lazy_rule_record.h`): `Pending`, `Evaluated`, `Applied`, `Cancelled`, `Expired`.
- `ObjectProtectionFlags` (`EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_policy.h`): `None`, `PreventTheft`, `PreventDecay`, `PreventCleanup`, `PreserveTransform`, `PreserveCondition`.

### Операции и хранилища состояния

#### `ISaveTransaction`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h:25`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ISaveTransaction() = default;
[[nodiscard]] virtual SaveTransactionState GetState() const = 0;
[[nodiscard]] virtual PersistenceRevision GetBaseRevision() const = 0;
[[nodiscard]] virtual foundation::Result<void> UpsertObject(PersistentObjectRecord record) = 0;
[[nodiscard]] virtual foundation::Result<void> DeleteObject(TombstoneRecord tombstone) = 0;
[[nodiscard]] virtual foundation::Result<void> UpsertLazyRule(LazyRuleRecord record) = 0;
[[nodiscard]] virtual foundation::Result<void> UpdateLazyRule(LazyRuleRecord record) = 0;
[[nodiscard]] virtual foundation::Result<void> RemoveLazyRule(LazyRuleId id) = 0;
[[nodiscard]] virtual foundation::Result<void> UpsertZoneOverride(ZoneOverrideSnapshot snapshot) = 0;
[[nodiscard]] virtual foundation::Result<void> RemoveZoneOverride(const PersistenceLocation& location) = 0;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `it cannot be retried on the`.

#### `IPersistenceQuery`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPersistenceQuery() = default;
[[nodiscard]] virtual std::optional<LazyRuleRecord> FindLazyRule(LazyRuleId id) const = 0;
[[nodiscard]] virtual std::vector<LazyRuleRecord> FindLazyRules(PersistentObjectId target_id) const = 0;
[[nodiscard]] virtual std::vector<LazyRuleRecord> QueryDueLazyRules(GameTimePoint now) const = 0;
[[nodiscard]] virtual std::vector<LazyRuleRecord> ListLazyRules() const = 0;
[[nodiscard]] virtual PersistenceRevision GetRevision() const = 0;
```

#### `IPersistentObjectStore`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistent_object_store.h:12`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPersistentObjectStore() = default;
[[nodiscard]] virtual std::optional<PersistentObjectRecord> FindObject(PersistentObjectId id) const = 0;
[[nodiscard]] virtual std::vector<PersistentObjectRecord> FindByLocation(const PersistenceLocation& location) const = 0;
[[nodiscard]] virtual std::vector<PersistentObjectRecord> ListObjects() const = 0;
```

#### `ITombstoneStore`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/tombstone_store.h:21`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ITombstoneStore() = default;
[[nodiscard]] virtual std::optional<TombstoneRecord> FindTombstone(PersistentObjectId id) const = 0;
[[nodiscard]] virtual bool IsTombstoned(PersistentObjectId id) const = 0;
[[nodiscard]] virtual std::vector<TombstoneRecord> ListTombstones() const = 0;
```

#### `IPersistenceStore`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_store.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPersistenceStore() = default;
[[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction() = 0;
[[nodiscard]] virtual std::unique_ptr<ISaveTransaction> OpenTransaction(PersistenceRevision base_revision) = 0;
```

#### `IPersistenceAdministrativeTransaction`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/save_transaction.h:48`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPersistenceAdministrativeTransaction() = default;
[[nodiscard]] virtual foundation::Result<void> AdminRemoveObject(PersistentObjectId id) = 0;
[[nodiscard]] virtual foundation::Result<void> AdminAddTombstone(TombstoneRecord tombstone) = 0;
```

#### `IDirtyTracker`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/dirty_tracker.h:9`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IDirtyTracker() = default;
[[nodiscard]] virtual bool IsDirty(PersistentObjectId id) const = 0;
[[nodiscard]] virtual std::vector<PersistentObjectId> CollectDirty() const = 0;
```

#### `IZoneOverrideStore`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/zone_override_store.h:23`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IZoneOverrideStore() = default;
[[nodiscard]] virtual std::optional<ZoneOverrideSnapshot> FindZoneOverride(const PersistenceLocation& location) const = 0;
[[nodiscard]] virtual std::vector<ZoneOverrideSnapshot> ListZoneOverrides() const = 0;
```

#### `IPersistenceBackend`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_backend.h:35`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IPersistenceBackend() = default;
[[nodiscard]] virtual foundation::Result<PersistenceSnapshot> Load() = 0;
```

#### `PersistenceServices`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_services.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::shared_ptr<IPersistenceStore> store`; `std::shared_ptr<IPersistenceQuery> query`.

#### `PersistencePayload`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_payload.h:11`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t schema_version = 0`; `std::vector<std::byte> bytes`.

#### `ObjectProtectionMask`

Объявлен в `EngineRuntime/Persistence/include/Epidemic/Runtime/Persistence/persistence_policy.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::uint32_t value = 0`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineRuntime/Persistence/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicRuntimeFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp` (1145 строк): `Error::Create`, `InMemoryPersistenceStore::InMemoryPersistenceStore`, `InMemorySaveTransaction::InMemorySaveTransaction`, `InMemorySaveTransaction::GetState`, `InMemorySaveTransaction::GetBaseRevision`, `InMemorySaveTransaction::EnsureOpen`, `InMemorySaveTransaction::StageOperation`, `InMemorySaveTransaction::UpsertObject`, `InMemorySaveTransaction::AdminRemoveObject`, `InMemorySaveTransaction::DeleteObject`
- `EngineRuntime/Persistence/src/persistence.cpp` (69 строк): `Error::Create`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `persistence.invalid_id`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:353`
- `persistence.invalid_payload`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:123`
- `persistence.invalid_enum`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:157`
- `persistence.invalid_protection_mask`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:132`
- `persistence.invalid_game_time`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:178`
- `persistence.invalid_lazy_rule_id`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:412`
- `persistence.invalid_lazy_rule_time`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:165`
- `persistence.invalid_location`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:456`
- `persistence.begin`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:288`
- `persistence.transaction_invalid_state`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:469`
- `persistence.allocation_failed`: `EngineRuntime/Persistence/src/persistence.cpp:65`
- `persistence.fail`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:510`
- `persistence.backend_exception`: `EngineRuntime/Persistence/src/persistence.cpp:52`
- `persistence.commit`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:517`
- `persistence.rollback`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:529`
- `persistence.invalid_snapshot`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:866`
- `persistence.conflict`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:885`
- `persistence.revision_overflow`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:896`
- `persistence.record_not_found`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:942`
- `persistence.lazy_rule_not_found`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:992`
- `persistence.override_not_found`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:1013`
- `persistence.backend_missing`: `EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:1082`
- `persistence.invalid_durability`: `EngineRuntime/Persistence/src/persistence.cpp:29`

### Конкретные тестовые якоря

- `EngineRuntime/Persistence/tests/persistence_tests.cpp`: `TestPayloadAndTypedProtectionContracts`, `TestTransactionCommitPersistsObjectsAndRevision`, `TestRollbackDiscardsStagedMutations`, `TestConcurrentTransactionConflict`, `TestFailedCommitIsAtomic`, `TestLazyRuleIdQueries`, `TestLazyRuleUpdateRemoveAndDueQuery`, `TestRichTombstonesAndZoneOverrides`, `TestBackendSaveFailureLeavesStoreUnchanged`, `TestBackendFlushFailureLeavesStoreUnchanged`, `TestSaveRequiredCommitPublishesAfterBackendSave`, `TestSaveAndFlushRequiredCallsSaveThenFlush`, `TestUpdateMissingLazyRuleRejected`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `persistence.invalid_id` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:353`)

```cpp
351     if (!id.IsValid())
  352     {
  353         return PersistenceFailure("persistence.invalid_id", "persistent object id must be valid before removal");
  354     }
  355     return StageOperation(AdminRemoveObjectOperation{id});
```

#### `persistence.invalid_payload` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:123`)

```cpp
121     if (!record.payload.IsValid())
  122     {
  123         return PersistenceFailure("persistence.invalid_payload", "persistent object payload must declare schema and bytes");
  124     }
  125     if (!IsValidPersistentObjectKind(record.kind) || !IsValidPersistenceTierValue(record.tier) ||
```

#### `persistence.invalid_enum` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:157`)

```cpp
155     if (!IsValidLazyRuleKindValue(record.kind) || !IsValidLazyRuleStateValue(record.state))
  156     {
  157         return PersistenceFailure("persistence.invalid_enum", "lazy rule contains an out-of-domain enum value");
  158     }
  159     if (!IsValidGameTime(record.created_game_time) || !IsValidGameTime(record.evaluate_after_game_time))
```

#### `persistence.invalid_protection_mask` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:132`)

```cpp
130     if (!HasOnlyKnownProtectionBits(record.protection_flags))
  131     {
  132         return PersistenceFailure("persistence.invalid_protection_mask", "persistent object protection mask contains unknown bits");
  133     }
  134     if (!IsValidGameTime(record.created_game_time) || !IsValidGameTime(record.last_observed_game_time))
```

#### `persistence.invalid_game_time` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:178`)

```cpp
176     if (!IsValidGameTime(tombstone.deleted_game_time))
  177     {
  178         return PersistenceFailure("persistence.invalid_game_time", "tombstone deletion time must be non-negative");
  179     }
  180     return foundation::Result<void>::Success();
```

#### `persistence.invalid_lazy_rule_id` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:412`)

```cpp
410     if (!id.IsValid())
  411     {
  412         return PersistenceFailure("persistence.invalid_lazy_rule_id", "lazy rule id must be valid before removal");
  413     }
  414     return StageOperation(RemoveLazyRuleOperation{id});
```

#### `persistence.invalid_lazy_rule_time` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:165`)

```cpp
163     if (record.evaluate_after_game_time < record.created_game_time)
  164     {
  165         return PersistenceFailure("persistence.invalid_lazy_rule_time", "lazy rule evaluation time must not precede creation time");
  166     }
  167     return foundation::Result<void>::Success();
```

#### `persistence.invalid_location` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:456`)

```cpp
454     if (!IsValidLocation(location))
  455     {
  456         return PersistenceFailure("persistence.invalid_location", "zone override removal location must contain at least one valid component");
  457     }
  458     return StageOperation(RemoveZoneOverrideOperation{location});
```

#### `persistence.begin` (`EngineRuntime/Persistence/src/in_memory_persistence_support.cpp:288`)

```cpp
286     auditor_context_ = auditor::shadow::child_context();
  287     auditor_transaction_ = auditor::shadow::identity();
  288     auditor_context_.cause = auditor::shadow::emit_causal("persistence.begin", auditor_transaction_.data());
  289 #endif
  290 }
```

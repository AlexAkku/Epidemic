---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "loot"]
source_path: "EngineFramework/GameplayWorldStateOwners/Loot"
---

# EngineFramework / Loot

Модуль: **Loot**. Drops и rewards.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Loot/`.
**CMake targets:** `EpidemicGameFrameworkLoot`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Loot/loot.h|loot.h]]: `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`; объявленные типы: `RewardTypeId`, `RewardDefinitionId`, `LootTableId`, `LootEntryId`, `LootQualityId`, `RewardExecutionId`, `IdHash`, `RegisteredRewardPayload`, `RewardDefinition`, `LootRollPolicy`, `LootEntry`, `LootTableDefinition`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Loot|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility and ownership

Owns loot tables, generated rewards, pending delivery records, delivery lifecycle, explicit random-source results, revisions/generators and the loot journal.

The service remains the single authoritative owner of the state listed below. Derived indexes and journals are not independent semantic owners.

#### Authoritative state and indexes

Admission inventory: 52 public callables, 64 mutation obligations, 1 lifecycle candidates, 15 stale-identity candidates and 0 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Mutations that span several containers must complete all fallible staging before the no-fail authoritative commit point. Query ordering and index-derived views are required to agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B03 coverage projection. Revision, ID-generator and journal publication is part of the mutation contract. Allocation/publication failure must not expose a partially advanced generator, revision, primary record, derived index or journal entry.

B03 allocation evidence uses source-private module seams rather than the shared process-global allocator override. These seams are test-only implementation details and do not expand the public API.

#### Lifecycle and identity

Lifecycle candidates and stale identity candidates from the admission inventory were reviewed. Invalid, removed, duplicate and stale IDs are required to be rejected or treated as the documented no-op without publishing false state. Terminal records are not silently resurrected by retry.

#### Persistence

Snapshot/restore is reviewed as a candidate-state operation: validate first, build off-state, preserve all live state on failure, then publish the candidate. Successful restore preserves the persistent ID-generator/revision/journal boundary represented by the snapshot contract.

#### External boundaries

Reward handler prepare/commit/cancel and reconciliation are explicit ports. Admission emitted no separate external-boundary candidate rows.

External callback/provider failure is converted to the module error/result contract. Retry must not duplicate an already committed semantic side effect, and failed compensation that requires reconciliation stays represented by owned state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B03. These owners are treated as externally serialized/owner-thread services for local correctness evidence. Later Goal 6 performs the engine-wide concurrency qualification.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkLootTests` from `EngineFramework/DevelopmentInfrastructure/Tests/loot_tests.cpp`.

Источник контрактов: `Documentation/EngineFramework/Loot.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `LootRollPolicy` (`EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`): `WeightedOne`, `WeightedMany`, `IndependentChance`, `GuaranteedAll`, `PickNWithoutReplacement`.
- `RewardDeliveryDisposition` (`EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`): `Delivered`, `NoOp`, `Unavailable`, `Rejected`.
- `PendingRewardState` (`EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`): `Generated`, `Available`, `Claimed`, `Expired`, `Cancelled`.
- `LootChangeKind` (`EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`): `Generated`, `Discarded`, `Available`, `Claimed`, `Expired`, `Cancelled`, `DeliveryFailed`.
- `RewardClaimHistoryStatus` (`EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h`): `Unknown`, `Claimed`, `HistoryExpired`.

### Операции и хранилища состояния

#### `LootService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h:299`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
LootService();
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<RewardTypeId> RegisterRewardHandler(std::string canonical_name, IRewardHandler *handler);
[[nodiscard]] foundation::Result<RewardDefinitionId> RegisterRewardDefinition(RewardDefinition definition);
[[nodiscard]] foundation::Result<LootTableId> RegisterLootTable(LootTableDefinition definition);
[[nodiscard]] foundation::Result<void> Freeze();
void SetConditionProvider(const ILootConditionProvider *provider) noexcept;
[[nodiscard]] foundation::Result<RewardBundle> GenerateTracked(LootTableId table, LootContext context);
// Compatibility alias for authoritative tracked generation. [[nodiscard]] foundation::Result<RewardBundle> Generate(LootTableId table, LootContext context);
[[nodiscard]] const RewardBundle* FindGenerated(RewardExecutionId reward) const noexcept;
[[nodiscard]] foundation::Result<void> BindSchedule(RewardExecutionId reward, ScheduleId schedule);
[[nodiscard]] const PendingReward *FindPending(RewardExecutionId reward) const noexcept;
[[nodiscard]] std::vector<PendingReward> PendingFor(GameplayObjectRef recipient) const;
[[nodiscard]] RewardClaimHistoryStatus ClaimHistoryStatus(RewardExecutionId reward) const noexcept;
[[nodiscard]] bool WasClaimed(RewardExecutionId reward) const noexcept;
[[nodiscard]] LootChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
[[nodiscard]] LootSnapshot CaptureSnapshot() const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<RewardTypeId, IRewardHandler *, IdHash> handlers_`; `std::unordered_map<RewardDefinitionId, RewardDefinition, IdHash> rewards_`; `std::unordered_map<LootTableId, LootTableDefinition, IdHash> tables_`; `std::unordered_map<RewardExecutionId, RewardBundle, IdHash> generated_`; `std::unordered_map<RewardExecutionId, PendingReward, IdHash> pending_`; `std::unordered_set<RewardExecutionId, IdHash> claimed_`; `std::deque<RewardExecutionId> claimed_order_`; `std::uint64_t claimed_history_floor_low_ = 0`.

#### `IRewardHandler`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h:216`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IRewardHandler() = default;
[[nodiscard]] virtual RewardTypeId Type() const noexcept = 0;
virtual void Commit(RewardDeliveryStage &stage) noexcept = 0;
virtual void Cancel(RewardDeliveryStage &stage) noexcept = 0;
```

#### `ILootConditionProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h:179`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~ILootConditionProvider() = default;
[[nodiscard]] virtual bool Evaluate(TypeId condition, const LootContext &context) const = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h:96`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `RegisteredRewardPayload`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h:107`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] static RegisteredRewardPayload FromTrivial(TypeId t, const T &v);
template <class T> [[nodiscard]] std::optional<T> AsTrivial(TypeId t) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<std::byte> bytes`.

#### `T`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Loot/include/Epidemic/GameFramework/Loot/loot.h:111`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `RegisteredRewardPayload p`; `return p`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Loot/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp` (744 строк): `Error::Create`, `LootService::LootService`, `GameplayObjectId::FromString`, `LootService::RegisterRewardHandler`, `RewardTypeId::FromString`, `LootService::RegisterRewardDefinition`, `RewardDefinitionId::FromString`, `LootService::RegisterLootTable`, `LootTableId::FromString`, `LootService::ValidateNestedTables`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.loot.executions`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:642`
- `gameplay.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:62`
- `gameplay.loot.reward_handler_invalid`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:36`
- `gameplay.loot.reward_invalid`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:54`
- `gameplay.loot.table_invalid`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:72`
- `gameplay.loot.entry_invalid`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:79`
- `gameplay.loot.table_cycle`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:113`
- `gameplay.loot.reward_handler_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:127`
- `gameplay.loot.condition_provider_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:162`
- `gameplay.loot.weight_overflow`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:145`
- `gameplay.loot.zero_weight_table`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:151`
- `gameplay.loot.condition_exception`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:170`
- `gameplay.loot.reward_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:180`
- `gameplay.loot.depth_exceeded`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:192`
- `gameplay.loot.table_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:195`
- `gameplay.loot.not_frozen`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:288`
- `loot.table`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:292`
- `gameplay.registry_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:302`
- `gameplay.loot.generated_capacity`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:304`
- `gameplay.loot.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:313`
- `gameplay.loot.execution_duplicate`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:319`
- `gameplay.loot.generated_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:375`
- `gameplay.loot.pending_invalid`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:369`
- `gameplay.loot.bundle_untrusted`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:347`
- `gameplay.loot.schedule_invalid`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:390`
- `gameplay.loot.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:394`
- `gameplay.loot.reward_unavailable`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:405`
- `gameplay.loot.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:482`
- `gameplay.loot.handler_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:449`
- `gameplay.loot.prepare_exception`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:456`
- `gameplay.loot.delivery_rejected`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:469`
- `gameplay.loot.pending_missing`: `EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:519`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/loot_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.loot.executions` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:642`)

```cpp
640         snapshot.journal.size() > kChangeJournalCapacity)
  641         return foundation::Result<void>::Failure(Error("gameplay.loot.restore_invalid", "snapshot capacity or sequence is invalid"));
  642     const auto expected_scope = GameplayObjectId::FromString("framework.loot.executions").High();
  643     if (!MonotonicIdGenerator<GameplayObjectId>::IsValidSnapshot(snapshot.execution_ids) ||
  644         snapshot.execution_ids.scope != expected_scope)
```

#### `gameplay.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:62`)

```cpp
60 {
   61     if (frozen_)
   62         return foundation::Result<LootTableId>::Failure(Error("gameplay.registry_frozen", "loot registry frozen"));
   63     if (d.canonical_name.empty() || d.entries.empty())
   64         return foundation::Result<LootTableId>::Failure(
```

#### `gameplay.loot.reward_handler_invalid` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:36`)

```cpp
34     if (name.empty() || !handler || handler->Type() != id || handlers_.contains(id))
   35         return foundation::Result<RewardTypeId>::Failure(
   36             Error("gameplay.loot.reward_handler_invalid", "invalid or duplicate reward handler"));
   37     handlers_[id] = handler;
   38     return foundation::Result<RewardTypeId>::Success(id);
```

#### `gameplay.loot.reward_invalid` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:54`)

```cpp
52         rewards_.contains(d.id))
   53         return foundation::Result<RewardDefinitionId>::Failure(
   54             Error("gameplay.loot.reward_invalid", "invalid or duplicate reward definition"));
   55     const auto id = d.id;
   56     rewards_.emplace(id, std::move(d));
```

#### `gameplay.loot.table_invalid` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:72`)

```cpp
70         !EnumInRange(d.policy, LootRollPolicy::PickNWithoutReplacement))
   71         return foundation::Result<LootTableId>::Failure(
   72             Error("gameplay.loot.table_invalid", "invalid or duplicate loot table"));
   73     std::unordered_set<LootEntryId, IdHash> ids;
   74     for (const auto &entry : d.entries)
```

#### `gameplay.loot.entry_invalid` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:79`)

```cpp
77             (entry.reward.IsValid() && entry.nested_table.IsValid()) || entry.chance_micro > 1'000'000 ||
   78             !ids.insert(entry.id).second)
   79             return foundation::Result<LootTableId>::Failure(Error("gameplay.loot.entry_invalid", "invalid loot entry"));
   80     }
   81     const auto id = d.id;
```

#### `gameplay.loot.table_cycle` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:113`)

```cpp
111         if (!visit(visit, id))
  112             return foundation::Result<void>::Failure(
  113                 Error("gameplay.loot.table_cycle", "loot table has cycle or unknown reference"));
  114     }
  115     return foundation::Result<void>::Success();
```

#### `gameplay.loot.reward_handler_missing` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:127`)

```cpp
125         if (!handlers_.contains(r.type))
  126             return foundation::Result<void>::Failure(
  127                 Error("gameplay.loot.reward_handler_missing", "reward definition has no handler"));
  128     }
  129     for (const auto &[id, table] : tables_)
```

#### `gameplay.loot.condition_provider_missing` (`EngineFramework/GameplayWorldStateOwners/Loot/src/loot.cpp:162`)

```cpp
160     if (!conditions_)
  161         return foundation::Result<bool>::Failure(
  162             Error("gameplay.loot.condition_provider_missing", "conditional loot requires a condition provider"));
  163     try
  164     {
```

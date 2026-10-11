---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "knowledge"]
source_path: "EngineFramework/GameplayWorldStateOwners/Knowledge"
---

# EngineFramework / Knowledge

Модуль: **Knowledge**. Знания сущностей и источники.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Knowledge/`.
**CMake targets:** `EpidemicGameFrameworkKnowledge`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Status: block-local `LOCAL_READY` projection for Goal 4. This is not a whole-Framework `FROZEN` claim.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Knowledge/knowledge.h|knowledge.h]]: `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`; объявленные типы: `KnowledgeRecordId`, `MemoryRecordId`, `BeliefTypeId`, `KnowledgeSourceId`, `KnowledgeTopicId`, `MemoryTypeId`, `MemoryDecayRuleId`, `IdHash`, `RefHash`, `KnowledgeAssertionValue`, `KnowledgeEpistemicState`, `KnowledgeConfidence`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Knowledge|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Owns knowledge profile definitions, knowledge/memory records, relations/contradictions, indexes, generators, revisions and the bounded knowledge change journal.

The service remains the single authoritative owner of that state. Derived indexes, journals and diagnostic/read models are not independent semantic owners.

#### Authoritative state and indexes

Admission/generated B05 inventory: 40 public callables, 48 mutation obligations, 1 lifecycle candidates, 12 stale-identity candidates and 1 external-boundary candidates.

Primary records, secondary indexes, ID generators, revision/change sequence and bounded journal state are reviewed together. Multi-container mutations must complete fallible staging before authoritative publication. Deterministic query/order semantics must agree with primary state.

#### Public contracts and failure atomicity

Every admission callable below is classified and reviewed. Mutators have success, no-op, invalid/precondition and failure decisions in the B05 coverage projection. Revision, identity-generator and journal publication is part of the mutation contract. Allocation/publication failure may not expose partially advanced primary state, index state, generator, revision or journal.

B05 allocation evidence uses source-private module seams rather than the shared process-global allocator override. The seams are implementation/test details and do not expand the public API.

#### Lifecycle and identity

All emitted lifecycle and stale-identity candidates were reviewed. Invalid, removed, duplicate and stale identities are rejected or handled as the documented no-op without false publication. Retry does not silently resurrect terminal state.

#### Persistence

Snapshot/restore is reviewed as candidate-state publication: validate first, build off-state, preserve the current service on failure, then atomically publish the candidate. Successful restore preserves generator/revision/cursor continuity required by the module snapshot contract.

#### External boundaries

The admission inventory contains the block’s single external-boundary candidate in Knowledge. Provider/callback failure is reviewed as a local Result/error boundary and may not partially publish graph/index state.

#### Threading contract

No additional internal synchronization guarantee is introduced by B05. These gameplay owners are treated as externally serialized/owner-thread services for Goal 4 local correctness. Engine-wide concurrency qualification remains Goal 6.

#### Regression evidence

Registered module target: `EpidemicGameFrameworkKnowledgeTests` from `EngineFramework/DevelopmentInfrastructure/Tests/knowledge_memory_tests.cpp`.

Goal 4 replaces process-global restore allocation injection with deterministic source-private Knowledge restore fault points. Existing lifecycle, contradiction, decay, graph/index and snapshot regressions remain green.

Источник контрактов: `Documentation/EngineFramework/Knowledge.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `KnowledgeAssertionValue` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `Unknown`, `Affirmed`, `Denied`.
- `KnowledgeEpistemicState` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `Known`, `Suspected`, `Rumor`, `Contradicted`, `Outdated`.
- `KnowledgeConfidence` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `None`, `Low`, `Medium`, `High`, `Certain`.
- `MemoryImportance` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `Low`, `Normal`, `High`, `Critical`.
- `MemoryPersistencePolicy` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `Transient`, `Session`, `Persistent`, `Timed`.
- `KnowledgeChangeKind` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `Learned`, `Updated`, `Forgotten`, `Shared`, `Contradicted`, `MemoryCreated`, `KnowledgeDecayed`, `MemoryDecayed`, `MemoryCompacted`, `MemoryForgotten`, `ProfileCreated`, `ProfileRemoved`.
- `KnowledgeShareMode` (`EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h`): `Tell`, `Report`, `Rumor`, `Broadcast`, `GroupSync`.

### Операции и хранилища состояния

#### `KnowledgeService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h:110`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
explicit KnowledgeService(std::size_t change_capacity=4096);
[[nodiscard]] static constexpr GameplayDomainId Domain()noexcept;
[[nodiscard]] foundation::Result<void> RegisterDecayRule(MemoryDecayRule rule);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen()const noexcept;
[[nodiscard]] foundation::Result<void> CreateProfile(KnowledgeProfile profile);
[[nodiscard]] foundation::Result<KnowledgeRecordId> Learn(LearnKnowledgeRequest request);
[[nodiscard]] foundation::Result<MemoryRecordId> CreateMemory(CreateMemoryRequest request);
[[nodiscard]] foundation::Result<KnowledgeRecordId> Share(ShareKnowledgeRequest request);
[[nodiscard]] foundation::Result<KnowledgeRecordId> Contradict(ContradictKnowledgeRequest request);
[[nodiscard]] foundation::Result<void> Decay(GameplayTimePoint now);
[[nodiscard]] const KnowledgeProfile* FindProfile(GameplayObjectRef subject)const noexcept;
[[nodiscard]] const KnowledgeRecord* FindKnowledge(KnowledgeRecordId id)const noexcept;
[[nodiscard]] const MemoryRecord* FindMemory(MemoryRecordId id)const noexcept;
[[nodiscard]] const MemoryDecayRule* FindDecayRule(MemoryDecayRuleId id)const noexcept;
[[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeByOwner(GameplayObjectRef owner)const;
[[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeByTopic(GameplayObjectRef owner,KnowledgeTopicId topic)const;
[[nodiscard]] std::vector<KnowledgeRecord> FindKnowledgeAboutSubject(GameplayObjectRef owner,GameplayObjectRef subject)const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::unordered_map<MemoryDecayRuleId,MemoryDecayRule,IdHash> decay_rules_`; `bool definitions_frozen_=false`; `std::unordered_map<GameplayObjectRef,KnowledgeProfile,RefHash> profiles_`; `std::unordered_map<KnowledgeRecordId,KnowledgeRecord,IdHash> knowledge_`; `std::unordered_map<MemoryRecordId,MemoryRecord,IdHash> memories_`; `std::unordered_map<GameplayObjectRef,std::vector<KnowledgeRecordId>,RefHash> knowledge_by_owner_`; `std::unordered_map<GameplayObjectRef,std::unordered_map<KnowledgeTopicId,std::vector<KnowledgeRecordId>,IdHash>,RefHash>`; `std::unordered_map<GameplayObjectRef,std::unordered_map<GameplayObjectRef,std::vector<KnowledgeRecordId>,RefHash>,RefHas`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h:26`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template<class T> [[nodiscard]] std::size_t operator()(const T& id)const noexcept;
```

#### `KnowledgeRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h:19`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr KnowledgeRecordId FromRaw(std::uint64_t h,std::uint64_t l) noexcept;
```

#### `MemoryRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h:20`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr MemoryRecordId FromRaw(std::uint64_t h,std::uint64_t l) noexcept;
```

#### `RefHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h:27`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef r)const noexcept;
```

#### `KnowledgeTopic`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Knowledge/include/Epidemic/GameFramework/Knowledge/knowledge.h:37`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `GameplayTagSet tags`; `std::vector<std::byte> payload`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Knowledge/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp` (428 строк): `Error::Create`, `KnowledgeSourceId::FromString`, `KnowledgeService::KnowledgeService`, `KnowledgeService::PrepareRevision`, `KnowledgeService::CanAdvanceRevisionBy`, `KnowledgeService::RegisterDecayRule`, `KnowledgeService::FreezeDefinitions`, `KnowledgeService::CreateProfile`, `KnowledgeService::RemoveProfile`, `KnowledgeService::Degrade`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `framework.knowledge.tell`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:44`
- `framework.knowledge.report`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:50`
- `framework.knowledge.rumor`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:46`
- `framework.knowledge.broadcast`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:47`
- `framework.knowledge.group_sync`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:48`
- `gameplay.knowledge.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:279`
- `gameplay.knowledge.registry_frozen`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:71`
- `gameplay.knowledge.invalid_decay_rule`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:77`
- `gameplay.knowledge.invalid_profile`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:95`
- `gameplay.knowledge.missing_profile`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:204`
- `gameplay.knowledge.invalid_learn`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:142`
- `gameplay.knowledge.invalid_provenance`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:143`
- `gameplay.knowledge.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:263`
- `gameplay.knowledge.id_collision`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:267`
- `gameplay.knowledge.invalid_memory`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:189`
- `gameplay.knowledge.invalid_compaction`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:205`
- `gameplay.knowledge.invalid_share`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:230`
- `gameplay.knowledge.share_depth_exhausted`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:231`
- `gameplay.knowledge.missing_record`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:240`
- `gameplay.knowledge.invalid_contradiction`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:249`
- `gameplay.knowledge.provenance_depth_exhausted`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:250`
- `framework.knowledge.last_known_position`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:318`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:358`
- `gameplay.knowledge.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:394`
- `gameplay.knowledge.restore_invalid`: `EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:391`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `framework.knowledge.tell` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:44`)

```cpp
42     switch(mode)
   43     {
   44     case KnowledgeShareMode::Tell:return KnowledgeSourceId::FromString("framework.knowledge.tell");
   45     case KnowledgeShareMode::Report:return KnowledgeSourceId::FromString("framework.knowledge.report");
   46     case KnowledgeShareMode::Rumor:return KnowledgeSourceId::FromString("framework.knowledge.rumor");
```

#### `framework.knowledge.report` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:50`)

```cpp
48     case KnowledgeShareMode::GroupSync:return KnowledgeSourceId::FromString("framework.knowledge.group_sync");
   49     }
   50     return KnowledgeSourceId::FromString("framework.knowledge.report");
   51 }
   52 } // namespace
```

#### `framework.knowledge.rumor` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:46`)

```cpp
44     case KnowledgeShareMode::Tell:return KnowledgeSourceId::FromString("framework.knowledge.tell");
   45     case KnowledgeShareMode::Report:return KnowledgeSourceId::FromString("framework.knowledge.report");
   46     case KnowledgeShareMode::Rumor:return KnowledgeSourceId::FromString("framework.knowledge.rumor");
   47     case KnowledgeShareMode::Broadcast:return KnowledgeSourceId::FromString("framework.knowledge.broadcast");
   48     case KnowledgeShareMode::GroupSync:return KnowledgeSourceId::FromString("framework.knowledge.group_sync…
```

#### `framework.knowledge.broadcast` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:47`)

```cpp
45     case KnowledgeShareMode::Report:return KnowledgeSourceId::FromString("framework.knowledge.report");
   46     case KnowledgeShareMode::Rumor:return KnowledgeSourceId::FromString("framework.knowledge.rumor");
   47     case KnowledgeShareMode::Broadcast:return KnowledgeSourceId::FromString("framework.knowledge.broadcast");
   48     case KnowledgeShareMode::GroupSync:return KnowledgeSourceId::FromString("framework.knowledge.group_sync");
   49     }
```

#### `framework.knowledge.group_sync` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:48`)

```cpp
46     case KnowledgeShareMode::Rumor:return KnowledgeSourceId::FromString("framework.knowledge.rumor");
   47     case KnowledgeShareMode::Broadcast:return KnowledgeSourceId::FromString("framework.knowledge.broadcast");
   48     case KnowledgeShareMode::GroupSync:return KnowledgeSourceId::FromString("framework.knowledge.group_sync");
   49     }
   50     return KnowledgeSourceId::FromString("framework.knowledge.report");
```

#### `gameplay.knowledge.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:279`)

```cpp
277 {
  278     if(!CanAdvanceRevisionBy(memories_.size()+knowledge_.size()))
  279         return foundation::Result<void>::Failure(Error("gameplay.knowledge.revision_exhausted","knowledge revision is exhausted"));
  280     std::vector<MemoryRecordId> forget_mem;for(const auto& [id,m]:memories_)if(m.persistence==MemoryPersistencePolicy::Timed&&m.decay_after.ticks>0&&m.importance!=MemoryImportance::Critical&&now>=SaturatingAdd(m.time,m.decay_after))forget_mem.push_back(id);
  281     std::sort(forget_mem.begin(),forget_mem.end());for(auto id:f…
```

#### `gameplay.knowledge.registry_frozen` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:71`)

```cpp
69 foundation::Result<void> KnowledgeService::RegisterDecayRule(MemoryDecayRule rule)
   70 {
   71     if(definitions_frozen_)return foundation::Result<void>::Failure(Error("gameplay.knowledge.registry_frozen","knowledge definitions are frozen"));
   72     if(!rule.id.IsValid()||rule.canonical_name.empty()||rule.interval.ticks<=0||rule.confidence_steps==0||decay_rules_.contains(rule.id))
   73         return foundation::Result<void>::Failure(Error("gameplay.knowledge.invalid_decay_rule","invalid or duplicate memory decay rule"));
```

#### `gameplay.knowledge.invalid_decay_rule` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:77`)

```cpp
75         return foundation::Result<void>::Failure(Error("gameplay.knowledge.invalid_decay_rule","invalid outdated confidence threshold"));
   76     if(rule.forget_at_or_below&&!EnumInRange(*rule.forget_at_or_below,KnowledgeConfidence::Certain))
   77         return foundation::Result<void>::Failure(Error("gameplay.knowledge.invalid_decay_rule","invalid forget confidence threshold"));
   78     decay_rules_.emplace(rule.id,std::move(rule)); return foundation::Result<void>::Success();
   79 }
```

#### `gameplay.knowledge.invalid_profile` (`EngineFramework/GameplayWorldStateOwners/Knowledge/src/knowledge.cpp:95`)

```cpp
93     (void)it;
   94     if (!inserted)
   95         return foundation::Result<void>::Failure(Error("gameplay.knowledge.invalid_profile","duplicate knowledge profile"));
   96     revision_ = next.Value();
   97     definitions_frozen_=true;
```

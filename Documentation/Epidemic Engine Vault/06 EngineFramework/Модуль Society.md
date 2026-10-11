---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "society"]
source_path: "EngineFramework/GameplayWorldStateOwners/Society"
---

# EngineFramework / Society

Модуль: **Society**. Группы и социальные отношения.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Society/`.
**CMake targets:** `EpidemicGameFrameworkSociety`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Society/society.h|society.h]]: `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h`; объявленные типы: `MembershipId`, `RelationshipId`, `SocialRoleId`, `SocialGroupTypeId`, `RelationshipTypeId`, `ReputationTrackId`, `IdHash`, `RefHash`, `PairHash`, `MembershipState`, `RelationshipState`, `SocietyChangeKind`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Society|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Scope

Freeze unit: `EngineFramework/GameplayWorldStateOwners/Society`. Admission: 62 public callables, 56 mutation obligations, 1 lifecycle candidates, 14 stale-identity candidates, 0 external-boundary candidates.

#### Public contract review

- Social group, relationship-type and reputation-track definitions.
- Membership lifecycle and member/group derived indexes.
- Relationship semantic-key uniqueness and derived relationship state.
- Reputation records, standings and effective-attitude queries.
- Revision/generator exhaustion is rejected before mutation.
- Multi-container publication and journal publication are failure-atomic; G4-SOC-001 and G4-SOC-002 are regressed.

#### Failure atomicity and boundary review

All revision-bearing mutations were reviewed for revision/generator exhaustion before authoritative mutation. Multi-container mutations were reviewed for rollback or staged publication. Snapshot restore builds/validates candidate state before live-state replacement. Allocation-failure evidence for this block no longer depends on the process-global Framework allocator helper; owned tests use module-local failure seams where allocation failure is evidence.

#### Persistence and deterministic reads

The module snapshot surface is treated as local in-memory persistence evidence for Goal 4. Non-empty roundtrip, invalid restore, generator/revision continuity, journal epoch/cursor behavior and failed-restore pre-state preservation are covered by the module test executable. Whole-engine ordered restore and replay remain Goal 5.

#### Test evidence

Primary regression source: `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp`.
Registered target: `EpidemicGameFrameworkSocietyTests`.
Local Linux verification in the worker environment used direct C++23 compilation with `-Wall -Wextra -Wpedantic -Werror` and executed the module test binary. The repository top-level CMake configure is Windows-only and therefore cannot be used in this Linux worker environment.

#### Goal 4 result

Block-local review result: LOCAL_READY candidate for serial integration. This document does not claim whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN`.

Источник контрактов: `Documentation/EngineFramework/Society.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `MembershipState` (`EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h`): `Active`, `Suspended`, `Former`, `Banned`, `Applicant`, `Honorary`.
- `RelationshipState` (`EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h`): `Neutral`, `Friendly`, `Hostile`, `Allied`, `Feared`, `Trusted`, `Unknown`.
- `SocietyChangeKind` (`EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h`): `GroupCreated`, `MembershipAdded`, `MembershipRemoved`, `MembershipChanged`, `RelationshipChanged`, `ReputationChanged`, `SocialRoleAssigned`, `SocialRoleRemoved`, `FactionRelationChanged`.

### Операции и хранилища состояния

#### `SocietyService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h:316`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterGroup(SocialGroupDefinition group);
[[nodiscard]] foundation::Result<void> RegisterRelationshipType(RelationshipTypeDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterReputationTrack(ReputationTrackDefinition definition);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] foundation::Result<MembershipId> AddMembership(MembershipRecord record);
[[nodiscard]] foundation::Result<void> ApplySocialChange(SocialChangeRequest request);
[[nodiscard]] foundation::Result<void> ApplyReputationChange(ReputationChangeRequest request);
[[nodiscard]] const SocialGroupDefinition *GetGroup(GameplayObjectRef group) const noexcept;
[[nodiscard]] const RelationshipTypeDefinition *GetRelationshipType(RelationshipTypeId id) const noexcept;
[[nodiscard]] const ReputationTrackDefinition *GetReputationTrack(ReputationTrackId id) const noexcept;
[[nodiscard]] const MembershipRecord *GetMembership(MembershipId id) const noexcept;
[[nodiscard]] std::vector<MembershipRecord> FindGroupsOf(GameplayObjectRef member) const;
[[nodiscard]] std::vector<MembershipRecord> FindMembersOf(GameplayObjectRef group) const;
[[nodiscard]] std::optional<RelationshipRecord> GetRelationship(GameplayObjectRef subject, GameplayObjectRef target, RelationshipTypeId type) const;
[[nodiscard]] std::int64_t GetEffectiveAttitude(AttitudeQuery query) const;
[[nodiscard]] std::optional<ReputationRecord> GetReputation(GameplayObjectRef subject, GameplayObjectRef scope, ReputationTrackId track) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::size_t kChangeJournalCapacity = 4096`; `std::unordered_map<GameplayObjectRef, SocialGroupDefinition, RefHash> groups_`; `std::unordered_map<RelationshipTypeId, RelationshipTypeDefinition, IdHash> relationship_types_`; `std::unordered_map<ReputationTrackId, ReputationTrackDefinition, IdHash> reputation_tracks_`; `bool definitions_frozen_ = false`; `std::unordered_map<MembershipId, MembershipRecord, IdHash> memberships_`; `std::unordered_map<MembershipKey, MembershipId, MembershipKeyHash> membership_by_key_`; `std::unordered_map<GameplayObjectRef, std::vector<MembershipId>, RefHash> memberships_by_member_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h:82`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `MembershipId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h:17`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr MembershipId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `RelationshipId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr RelationshipId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `RefHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h:90`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef r) const noexcept;
```

#### `PairHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Society/include/Epidemic/GameFramework/Society/society.h:98`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const std::pair<GameplayObjectRef, GameplayObjectRef> &p) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Society/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp` (1207 строк): `Error::Create`, `SocietyService::RegisterGroup`, `SocietyService::RegisterRelationshipType`, `SocietyService::RegisterReputationTrack`, `SocietyService::FreezeDefinitions`, `SocietyService::AddMembership`, `SocietyService::SetMembershipState`, `SocietyService::SetMembershipRole`, `SocietyService::RemoveMembership`, `SocietyService::SetRelationship`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.society.definitions_frozen`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:262`
- `gameplay.society.invalid_group`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:189`
- `gameplay.society.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:608`
- `gameplay.society.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:942`
- `gameplay.society.invalid_relationship_definition`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:222`
- `gameplay.society.relationship_threshold_gap`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:228`
- `gameplay.society.invalid_relationship_threshold`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:237`
- `gameplay.society.invalid_reputation_definition`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:270`
- `gameplay.society.invalid_reputation_threshold`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:282`
- `gameplay.society.definitions_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:653`
- `gameplay.society.invalid_membership`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:317`
- `gameplay.society.membership_conflict`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:329`
- `gameplay.society.duplicate_membership`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:349`
- `gameplay.society.membership_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:341`
- `gameplay.society.membership_missing`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:417`
- `gameplay.society.invalid_membership_transition`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:383`
- `gameplay.society.invalid_relationship`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:434`
- `gameplay.society.relationship_type_missing`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:556`
- `gameplay.society.relationship_value_out_of_range`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:442`
- `gameplay.society.relationship_key_conflict`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:450`
- `gameplay.society.duplicate_relationship`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:481`
- `gameplay.society.relationship_id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:473`
- `gameplay.society.invalid_change`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:552`
- `gameplay.society.invalid_reputation`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:655`
- `gameplay.society.reputation_track_missing`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:659`
- `gameplay.society.reputation_value_out_of_range`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:604`
- `gameplay.change_journal.epoch_exhausted`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:948`
- `gameplay.society.restore_invalid_relationship_definition`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:973`
- `gameplay.society.restore_invalid_relationship_threshold`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:987`
- `gameplay.society.restore_invalid_reputation_definition`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:1001`
- `gameplay.society.restore_invalid_reputation_threshold`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:1013`
- `gameplay.society.restore_invalid_group`: `EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:1023`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/society_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.society.definitions_frozen` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:262`)

```cpp
260     if (definitions_frozen_)
  261         return foundation::Result<void>::Failure(
  262             Error("gameplay.society.definitions_frozen", "society definitions are frozen"));
  263     if (!definition.id.IsValid() || reputation_tracks_.contains(definition.id) ||
  264         definition.minimum_value_micro > definition.maximum_value_micro ||
```

#### `gameplay.society.invalid_group` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:189`)

```cpp
187             Error("gameplay.society.definitions_frozen", "society definitions are frozen"));
  188     if (!group.group.IsValid() || !group.type.IsValid() || groups_.contains(group.group))
  189         return foundation::Result<void>::Failure(Error("gameplay.society.invalid_group", "invalid or duplicate group"));
  190     if (revision_.value == std::numeric_limits<std::uint64_t>::max())
  191         return foundation::Result<void>::Failure(Error("gameplay.society.revision_exhausted", "society revision exhausted"));
```

#### `gameplay.society.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:608`)

```cpp
606     const ReputationKey key{record.subject, record.scope, record.track};
  607     if (revision_.value == std::numeric_limits<std::uint64_t>::max())
  608         return foundation::Result<void>::Failure(Error("gameplay.society.revision_exhausted", "society revision exhausted"));
  609     const Revision next_revision{revision_.value + 1};
  610     if (auto it = reputations_.find(key); it != reputations_.end())
```

#### `gameplay.society.allocation_failed` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:942`)

```cpp
940     if (testing::ConsumeLocalAllocationFailureForTest())
  941         return foundation::Result<void>::Failure(
  942             Error("gameplay.society.allocation_failed", "module-local injected allocation failure"));
  943 
  944     const auto next_journal_epoch = CheckedNextChangeEpoch(snapshot.change_epoch > journal_epoch_ ? snapshot.change_epoch : journal_epoch_);
```

#### `gameplay.society.invalid_relationship_definition` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:222`)

```cpp
220     {
  221         return foundation::Result<void>::Failure(
  222             Error("gameplay.society.invalid_relationship_definition", "invalid relationship type definition"));
  223     }
  224     std::sort(definition.state_thresholds.begin(), definition.state_thresholds.end(), [](const auto &a, const auto &b) {
```

#### `gameplay.society.relationship_threshold_gap` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:228`)

```cpp
226     });
  227     if (definition.state_thresholds.front().minimum_value_micro > definition.minimum_value_micro)
  228         return foundation::Result<void>::Failure(Error("gameplay.society.relationship_threshold_gap",
  229                                                        "relationship thresholds do not cover minimum value"));
  230     for (std::size_t i = 0; i < definition.state_thresholds.size(); ++i)
```

#### `gameplay.society.invalid_relationship_threshold` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:237`)

```cpp
235             (i > 0 && threshold == definition.state_thresholds[i - 1].minimum_value_micro))
  236         {
  237             return foundation::Result<void>::Failure(Error("gameplay.society.invalid_relationship_threshold",
  238                                                            "invalid or duplicate relationship threshold"));
  239         }
```

#### `gameplay.society.invalid_reputation_definition` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:270`)

```cpp
268     {
  269         return foundation::Result<void>::Failure(
  270             Error("gameplay.society.invalid_reputation_definition", "invalid reputation track definition"));
  271     }
  272     std::sort(definition.standing_thresholds.begin(), definition.standing_thresholds.end(), [](const auto &a, const auto &b) {
```

#### `gameplay.society.invalid_reputation_threshold` (`EngineFramework/GameplayWorldStateOwners/Society/src/society.cpp:282`)

```cpp
280             (i > 0 && threshold.minimum_value_micro == definition.standing_thresholds[i - 1].minimum_value_micro))
  281         {
  282             return foundation::Result<void>::Failure(Error("gameplay.society.invalid_reputation_threshold",
  283                                                            "invalid or duplicate reputation standing threshold"));
  284         }
```

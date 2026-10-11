---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "ownership"]
source_path: "EngineFramework/GameplayWorldStateOwners/Ownership"
---

# EngineFramework / Ownership

Модуль: **Ownership**. Gameplay ownership.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Ownership/`.
**CMake targets:** `EpidemicGameFrameworkOwnership`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Ownership/ownership.h|ownership.h]]: `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`; объявленные типы: `OwnershipRecordId`, `PropertyClaimId`, `AccessRuleId`, `PermissionGrantId`, `PropertyRightId`, `PropertyDomainId`, `IdHash`, `RefHash`, `OwnershipStrength`, `PermissionStrength`, `AccessDecision`, `PermissionDecision`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Ownership|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Responsibility

Owns semantic ownership records, permissions, access rules and property claims, plus all indexes, identity generators, revision and ownership journal.

#### Public headers and types

Public surface is rooted at `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`.

#### Dependency list

Direct production links are `EpidemicGameFrameworkFoundation` and `EpidemicFoundation`. No peer GameplayWorldStateOwner or Runtime module dependency is added by this delta.

#### External ports/callbacks/providers/backends

IRoleProvider is a non-owning authorization read port. No external durable transaction/backend is called by this module.

#### Authoritative state

Ownership, grant, rule and claim maps; canonical owner and per-property/per-owner/per-subject indexes; four ID generators; revision; bounded journal; role provider pointer.

#### Derived/cache/index state

canonical_owners_, ownership_by_property_, ownership_by_owner_, grants_by_subject_, rules_by_property_ and claims_by_property_ are derived indexes over their authoritative record maps.

#### ID spaces, generations, revisions and cursors

OwnershipRecordId, PermissionGrantId, AccessRuleId and PropertyClaimId have independent service scopes. Expected Revision on transfer prevents stale replacement; journal cursor is monotonic and prunable.

#### State machines

No registry Freeze state. Records are created/updated/transferred/removed; grants expire/revoke, rules update/remove, claims create/resolve. Terminal removal invalidates the corresponding typed ID.

Источник контрактов: `Documentation/EngineFramework/Ownership.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `OwnershipStrength` (`EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`): `Owned`, `Claimed`, `Borrowed`, `Leased`, `Reserved`, `Stolen`, `Contested`, `Abandoned`, `Public`, `Unowned`.
- `PermissionStrength` (`EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`): `Weak`, `Normal`, `Strong`, `Absolute`.
- `AccessDecision` (`EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`): `Allow`, `Deny`, `RequirePermission`, `RequireRole`, `RequireOwner`, `Public`, `Private`, `TrespassIfViolated`, `CrimeIfViolated`.
- `PermissionDecision` (`EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`): `Allowed`, `Denied`, `Trespass`, `CrimeCandidate`, `Unknown`, `NotApplicable`.
- `TransferReason` (`EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`): `Trade`, `Gift`, `Reward`, `Loot`, `Theft`, `Confiscation`, `Inheritance`, `Abandonment`, `ConstructionCreated`, `GameSpecific`.
- `OwnershipChangeKind` (`EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h`): `OwnershipAssigned`, `OwnershipTransferred`, `OwnershipRemoved`, `PermissionGranted`, `PermissionRevoked`, `AccessRuleAdded`, `AccessRuleChanged`, `AccessRuleRemoved`, `PropertyClaimCreated`, `PropertyClaimResolved`.

### Операции и хранилища состояния

#### `OwnershipService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:279`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<OwnershipRecordId> AssignOwnership(OwnershipRecord record);
[[nodiscard]] foundation::Result<void> TransferOwnership(TransferOwnershipRequest request);
[[nodiscard]] foundation::Result<PermissionGrantId> GrantPermission(PermissionGrant grant);
[[nodiscard]] foundation::Result<AccessRuleId> AddAccessRule(AccessRule rule);
[[nodiscard]] foundation::Result<PropertyClaimId> CreateClaim(PropertyClaim claim);
[[nodiscard]] OwnershipPermissionResult EvaluatePermission(OwnershipPermissionQuery query) const;
[[nodiscard]] const OwnershipRecord *GetOwner(GameplayObjectRef property) const noexcept;
[[nodiscard]] const OwnershipRecord *GetOwner(GameplayObjectRef property, PropertyDomainId domain) const noexcept;
[[nodiscard]] std::vector<OwnershipRecord> GetOwnershipRecords(GameplayObjectRef property) const;
[[nodiscard]] std::vector<OwnershipRecord> FindPropertiesOwnedBy(GameplayObjectRef owner) const;
[[nodiscard]] std::vector<PermissionGrant> FindPermissions(GameplayObjectRef subject) const;
[[nodiscard]] std::vector<PropertyClaim> FindClaims(GameplayObjectRef property) const;
[[nodiscard]] std::vector<AccessRule> FindAccessRules(GameplayObjectRef property) const;
[[nodiscard]] OwnershipChangeBatch ReadChangesSince(ChangeCursor cursor) const;
[[nodiscard]] ChangeCursor LatestChangeCursor() const noexcept;
void PruneChangesThrough(std::uint64_t sequence);
[[nodiscard]] OwnershipSnapshot CaptureSnapshot() const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `static constexpr std::size_t kChangeRetention = 4096`; `std::unordered_map<OwnershipRecordId, OwnershipRecord, IdHash> records_`; `std::unordered_map<PermissionGrantId, PermissionGrant, IdHash> grants_`; `std::unordered_map<AccessRuleId, AccessRule, IdHash> rules_`; `std::unordered_map<PropertyClaimId, PropertyClaim, IdHash> claims_`; `std::unordered_map<OwnerKey, OwnershipRecordId, OwnerKeyHash> canonical_owners_`; `std::unordered_map<GameplayObjectRef, std::vector<OwnershipRecordId>, RefHash> ownership_by_property_`; `std::unordered_map<GameplayObjectRef, std::vector<OwnershipRecordId>, RefHash> ownership_by_owner_`.

#### `IOwnershipRoleProvider`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:271`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
virtual ~IOwnershipRoleProvider() = default;
[[nodiscard]] virtual bool HasRole(GameplayObjectRef subject, TypeId role, const GameplayContext &context) const noexcept = 0;
```

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:70`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `OwnershipRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:18`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr OwnershipRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PropertyClaimId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:27`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PropertyClaimId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `AccessRuleId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:36`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr AccessRuleId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `PermissionGrantId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:45`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr PermissionGrantId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `RefHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Ownership/include/Epidemic/GameFramework/Ownership/ownership.h:77`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(GameplayObjectRef r) const noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Ownership/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp` (1077 строк): `Error::Create`, `OwnershipService::PrepareRevision`, `OwnershipService::CanRecordChanges`, `OwnershipService::FindMutableOwnership`, `OwnershipService::IndexOwnership`, `OwnershipService::UnindexOwnership`, `OwnershipService::IndexGrant`, `OwnershipService::UnindexGrant`, `OwnershipService::IndexRule`, `OwnershipService::UnindexRule`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.ownership.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:172`
- `gameplay.ownership.invalid_record`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:271`
- `gameplay.ownership.owner_conflict`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:274`
- `gameplay.ownership.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:537`
- `gameplay.ownership.duplicate_record`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:286`
- `gameplay.ownership.change_sequence_exhausted`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:553`
- `gameplay.ownership.publication_failed`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:565`
- `gameplay.ownership.record_missing`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:381`
- `gameplay.ownership.invalid_transfer`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:350`
- `gameplay.ownership.transfer_stale`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:356`
- `gameplay.ownership.transfer_owner_mismatch`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:368`
- `gameplay.ownership.transfer_stale_revision`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:370`
- `gameplay.ownership.invalid_grant`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:423`
- `gameplay.ownership.duplicate_grant`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:432`
- `gameplay.ownership.grant_missing`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:457`
- `gameplay.ownership.invalid_rule`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:507`
- `gameplay.ownership.duplicate_rule`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:495`
- `gameplay.ownership.rule_missing`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:526`
- `gameplay.ownership.rule_stale`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:509`
- `gameplay.ownership.invalid_claim`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:536`
- `gameplay.ownership.duplicate_claim`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:538`
- `gameplay.ownership.claim_missing`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:550`
- `framework.ownership.reason.owner`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:626`
- `framework.ownership.reason.absolute_permission`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:635`
- `framework.ownership.reason.rule_allow`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:662`
- `framework.ownership.reason.rule_deny`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:668`
- `framework.ownership.reason.trespass`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:673`
- `framework.ownership.reason.crime`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:678`
- `framework.ownership.reason.permission_grant`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:725`
- `framework.ownership.reason.permission_required`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:691`
- `framework.ownership.reason.owner_required`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:697`
- `framework.ownership.reason.role_provider_unavailable`: `EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:703`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/ownership_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.ownership.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:172`)

```cpp
170     if (!next)
  171         return foundation::Result<Revision>::Failure(
  172             Error("gameplay.ownership.revision_exhausted", "ownership revision is exhausted"));
  173     return foundation::Result<Revision>::Success(*next);
  174 }
```

#### `gameplay.ownership.invalid_record` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:271`)

```cpp
269     if (!r.property.IsValid() || !r.owner.IsValid() || !IsValidOwnershipStrength(r.strength))
  270         return foundation::Result<OwnershipRecordId>::Failure(
  271             Error("gameplay.ownership.invalid_record", "invalid ownership record"));
  272     if (r.strength == OwnershipStrength::Owned && canonical_owners_.contains(OwnerKey{r.property, r.domain}))
  273         return foundation::Result<OwnershipRecordId>::Failure(
```

#### `gameplay.ownership.owner_conflict` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:274`)

```cpp
272     if (r.strength == OwnershipStrength::Owned && canonical_owners_.contains(OwnerKey{r.property, r.domain}))
  273         return foundation::Result<OwnershipRecordId>::Failure(
  274             Error("gameplay.ownership.owner_conflict", "canonical owner already exists for property/domain"));
  275 
  276     auto staged_ids = ownership_ids_;
```

#### `gameplay.ownership.id_exhausted` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:537`)

```cpp
535 {
  536     if(!c.property.IsValid()||!c.claimant.IsValid()) return foundation::Result<PropertyClaimId>::Failure(Error("gameplay.ownership.invalid_claim","invalid property claim"));
  537     auto staged_ids=claim_ids_; if(!c.id.IsValid()){c.id=PropertyClaimId{staged_ids.Next()};if(!c.id.IsValid())return foundation::Result<PropertyClaimId>::Failure(Error("gameplay.ownership.id_exhausted","property claim id generator exhausted"));}
  538     if(claims_.contains(c.id))return foundation::Result<PropertyClaimId>::Failure(Error("gameplay.ownersh…
```

#### `gameplay.ownership.duplicate_record` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:286`)

```cpp
284     if (records_.contains(r.id))
  285         return foundation::Result<OwnershipRecordId>::Failure(
  286             Error("gameplay.ownership.duplicate_record", "duplicate ownership record"));
  287     AdvanceGeneratorPastExplicitId(staged_ids, r.id);
  288
```

#### `gameplay.ownership.change_sequence_exhausted` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:553`)

```cpp
551     if (!CanRecordChanges())
  552         return foundation::Result<void>::Failure(
  553             Error("gameplay.ownership.change_sequence_exhausted", "ownership change sequence is exhausted"));
  554     auto rev = PrepareRevision();
  555     if (!rev)
```

#### `gameplay.ownership.publication_failed` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:565`)

```cpp
563     {
  564         return foundation::Result<void>::Failure(
  565             Error("gameplay.ownership.publication_failed", "claim resolution journal publication failed"));
  566     }
  567     UnindexClaim(copy);
```

#### `gameplay.ownership.record_missing` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:381`)

```cpp
379     auto *current = FindMutableOwnership(found->id);
  380     if (!current)
  381         return foundation::Result<void>::Failure(Error("gameplay.ownership.record_missing", "canonical ownership record disappeared"));
  382     const auto previous = current->owner;
  383     bool staged_new_index = false;
```

#### `gameplay.ownership.invalid_transfer` (`EngineFramework/GameplayWorldStateOwners/Ownership/src/ownership.cpp:350`)

```cpp
348 {
  349     if (!q.property.IsValid() || !q.to_owner.IsValid() || !IsValidTransferReason(q.reason))
  350         return foundation::Result<void>::Failure(Error("gameplay.ownership.invalid_transfer", "invalid transfer"));
  351 
  352     const auto *found = GetOwner(q.property, q.domain);
```

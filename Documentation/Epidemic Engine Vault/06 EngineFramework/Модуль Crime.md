---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "crime"]
source_path: "EngineFramework/GameplayWorldStateOwners/Crime"
---

# EngineFramework / Crime

Модуль: **Crime**. Gameplay crime/legal systems.

**Реализация:** `EngineFramework/GameplayWorldStateOwners/Crime/`.
**CMake targets:** `EpidemicGameFrameworkCrime`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Описание составлено по дереву исходников и именам контрактов. Для поведения читайте первоисточник и типы ниже.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/Crime/crime.h|crime.h]]: `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`; объявленные типы: `LawId`, `CrimeTypeId`, `JurisdictionId`, `CrimeRecordId`, `EvidenceId`, `WitnessRecordId`, `BountyRecordId`, `AuthorityId`, `LawResponseId`, `LawResponseTypeId`, `EvidenceTypeId`, `WitnessTypeId`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/Crime|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
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

Freeze unit: `EngineFramework/GameplayWorldStateOwners/Crime`. Admission: 100 public callables, 80 mutation obligations, 1 lifecycle candidates, 19 stale-identity candidates, 0 external-boundary candidates.

#### Public contract review

- Law/jurisdiction/authority registration.
- Crime candidate evaluation and crime-record lifecycle.
- Evidence/witness duplicate and stale-reference behavior.
- Proof-state, bounty and law-response lifecycle.
- Revision/generator/journal boundary handling and multi-index consistency.
- Snapshot/restore validates all legal records before publication.

#### Failure atomicity and boundary review

All revision-bearing mutations were reviewed for revision/generator exhaustion before authoritative mutation. Multi-container mutations were reviewed for rollback or staged publication. Snapshot restore builds/validates candidate state before live-state replacement. Allocation-failure evidence for this block no longer depends on the process-global Framework allocator helper; owned tests use module-local failure seams where allocation failure is evidence.

#### Persistence and deterministic reads

The module snapshot surface is treated as local in-memory persistence evidence for Goal 4. Non-empty roundtrip, invalid restore, generator/revision continuity, journal epoch/cursor behavior and failed-restore pre-state preservation are covered by the module test executable. Whole-engine ordered restore and replay remain Goal 5.

#### Test evidence

Primary regression source: `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp`.
Registered target: `EpidemicGameFrameworkCrimeTests`.
Local Linux verification in the worker environment used direct C++23 compilation with `-Wall -Wextra -Wpedantic -Werror` and executed the module test binary. The repository top-level CMake configure is Windows-only and therefore cannot be used in this Linux worker environment.

#### Goal 4 result

Block-local review result: LOCAL_READY candidate for serial integration. This document does not claim whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN`.

Источник контрактов: `Documentation/EngineFramework/Crime.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `CrimeProofState` (`EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`): `Unknown`, `Suspected`, `Witnessed`, `Reported`, `EvidenceFound`, `Confirmed`, `FalseAccusation`, `Resolved`.
- `CrimeCaseState` (`EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`): `Open`, `Reported`, `UnderInvestigation`, `Wanted`, `Punished`, `Forgiven`, `Expired`, `Dismissed`, `Resolved`.
- `BountyState` (`EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`): `Active`, `Paid`, `Forgiven`, `Expired`, `Transferred`.
- `CrimeChangeKind` (`EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`): `CrimeCandidateCreated`, `CrimeRecorded`, `CrimeReported`, `WitnessAdded`, `WitnessRemoved`, `EvidenceAdded`, `EvidenceRemoved`, `ProofStateChanged`, `CaseStateChanged`, `BountyCreated`, `BountyChanged`, `BountyResolved`, `LawResponseGenerated`, `CrimeExpired` и другие значения.
- `CrimeCandidatePolicy` (`EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h`): `RecordAlways`, `RecordIfWitnessed`, `IgnoreWithoutJurisdiction`.

### Операции и хранилища состояния

#### `CrimeService`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:472`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] static constexpr GameplayDomainId Domain() noexcept;
[[nodiscard]] foundation::Result<void> RegisterLaw(LawDefinition law);
[[nodiscard]] foundation::Result<void> RegisterLawResponseDefinition(LawResponseDefinition definition);
[[nodiscard]] foundation::Result<void> RegisterJurisdiction(JurisdictionRecord jurisdiction);
[[nodiscard]] foundation::Result<AuthorityId> RegisterAuthority(AuthorityRecord authority);
[[nodiscard]] foundation::Result<void> FreezeDefinitions();
[[nodiscard]] bool DefinitionsFrozen() const noexcept;
[[nodiscard]] std::optional<JurisdictionRecord> GetApplicableJurisdiction(GameplayObjectRef area) const;
[[nodiscard]] foundation::Result<CrimeEvaluationResult> EvaluateCrimeCandidate( CrimeCandidate candidate, CrimeCandidatePolicy policy = CrimeCandidatePolicy::RecordAlways);
[[nodiscard]] foundation::Result<BountyRecordId> CreateBounty(BountyRecord bounty);
[[nodiscard]] foundation::Result<LawResponseId> GenerateLawResponse(LawResponseRequest request);
[[nodiscard]] LegalityPreview PreviewLegality(GameplayObjectRef offender, CrimeTypeId type, GameplayObjectRef area) const;
[[nodiscard]] std::vector<CrimeRecord> FindCrimesByOffender(GameplayObjectRef offender) const;
[[nodiscard]] std::vector<CrimeRecord> FindCrimesByVictim(GameplayObjectRef victim) const;
[[nodiscard]] std::vector<CrimeRecord> FindCrimesByJurisdiction(JurisdictionId jurisdiction) const;
[[nodiscard]] std::vector<CrimeRecord> FindOpenCases() const;
[[nodiscard]] std::vector<WitnessRecord> FindWitnesses(CrimeRecordId crime) const;
[[nodiscard]] std::vector<EvidenceRecord> FindEvidence(CrimeRecordId crime) const;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `bool definitions_frozen_ = false`; `std::unordered_map<LawId, LawDefinition, IdHash> laws_`; `std::unordered_map<LawResponseTypeId, LawResponseDefinition, IdHash> response_definitions_`; `std::unordered_map<JurisdictionId, JurisdictionRecord, IdHash> jurisdictions_`; `std::unordered_map<AuthorityId, AuthorityRecord, IdHash> authorities_`; `std::unordered_map<CrimeRecordId, CrimeRecord, IdHash> crimes_`; `std::unordered_map<WitnessRecordId, WitnessRecord, IdHash> witnesses_`; `std::unordered_map<EvidenceId, EvidenceRecord, IdHash> evidence_`.

#### `IdHash`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:215`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
template <class T> [[nodiscard]] std::size_t operator()(const T &id) const noexcept;
```

#### `JurisdictionId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:47`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr JurisdictionId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `CrimeRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:65`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr CrimeRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `EvidenceId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:83`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr EvidenceId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `WitnessRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:101`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr WitnessRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `BountyRecordId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:119`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr BountyRecordId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `AuthorityId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:137`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr AuthorityId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

#### `LawResponseId`

Объявлен в `EngineFramework/GameplayWorldStateOwners/Crime/include/Epidemic/GameFramework/Crime/crime.h:155`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
static constexpr LawResponseId FromRaw(std::uint64_t h, std::uint64_t l) noexcept;
```

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/GameplayWorldStateOwners/Crime/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkFoundation`, `EpidemicFoundation`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp` (1541 строк): `Error::Create`, `CrimeService::RegisterLaw`, `CrimeService::RegisterLawResponseDefinition`, `CrimeService::RegisterJurisdiction`, `CrimeService::RegisterAuthority`, `CrimeService::FreezeDefinitions`, `CrimeService::GetApplicableJurisdiction`, `CrimeService::FindLawFor`, `CrimeService::FindResponseDefinition`, `CrimeService::EvaluateCrimeCandidate`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.crime.definitions_frozen`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:230`
- `gameplay.crime.invalid_law`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:161`
- `gameplay.revision_exhausted`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:911`
- `gameplay.crime.allocation_failed`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:796`
- `gameplay.crime.invalid_response_definition`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:186`
- `gameplay.crime.invalid_jurisdiction`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:217`
- `gameplay.crime.duplicate_law`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:206`
- `gameplay.crime.unknown_law`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:280`
- `gameplay.crime.invalid_authority`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:288`
- `gameplay.crime.duplicate_authority`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:258`
- `gameplay.crime.id_exhausted`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:783`
- `gameplay.crime.definitions_not_frozen`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:754`
- `gameplay.crime.invalid_candidate`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:341`
- `gameplay.crime.invalid_witness`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:513`
- `gameplay.crime.no_jurisdiction`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:357`
- `gameplay.crime.no_law`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:361`
- `framework.crime.reason.witness_required`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:368`
- `gameplay.crime.duplicate_crime`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:416`
- `gameplay.crime.duplicate_witness`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:539`
- `gameplay.crime.witness_missing`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:559`
- `gameplay.crime.crime_missing`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:900`
- `gameplay.crime.invalid_evidence`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:581`
- `gameplay.crime.duplicate_evidence`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:607`
- `gameplay.crime.evidence_missing`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:627`
- `gameplay.crime.invalid_bounty`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:650`
- `gameplay.crime.invalid_bounty_source`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:657`
- `gameplay.crime.active_bounty_exists`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:697`
- `gameplay.crime.duplicate_bounty`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:675`
- `gameplay.crime.invalid_bounty_transition`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:733`
- `gameplay.crime.bounty_missing`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:729`
- `gameplay.crime.invalid_response`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:758`
- `gameplay.crime.case_terminal`: `EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:857`

### Конкретные тестовые якоря

- `EngineFramework/DevelopmentInfrastructure/Tests/crime_tests.cpp`

Тесты позволяют проверить предпосылку о контракте только после реального запуска соответствующей CTest цели; путь к файлу не заменяет результат теста.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.crime.definitions_frozen` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:230`)

```cpp
228 {
  229     if (definitions_frozen_)
  230         return foundation::Result<AuthorityId>::Failure(Error("gameplay.crime.definitions_frozen", "crime definitions frozen"));
  231     if (!authority.authority_group.IsValid() || !jurisdictions_.contains(authority.jurisdiction))
  232         return foundation::Result<AuthorityId>::Failure(Error("gameplay.crime.invalid_authority", "invalid authority"));
```

#### `gameplay.crime.invalid_law` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:161`)

```cpp
159     {
  160         if (!laws_.emplace(law.id, std::move(law)).second)
  161             return foundation::Result<void>::Failure(Error("gameplay.crime.invalid_law", "invalid or duplicate law"));
  162     }
  163     catch (...)
```

#### `gameplay.revision_exhausted` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:911`)

```cpp
909     const auto next = NextRevision();
  910     if (!next)
  911         return foundation::Result<void>::Failure(Error("gameplay.revision_exhausted", "crime revision is exhausted"));
  912     const auto offender = it->second.offender;
  913     const auto victim = it->second.victim;
```

#### `gameplay.crime.allocation_failed` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:796`)

```cpp
794     catch (...)
  795     {
  796         return foundation::Result<LawResponseId>::Failure(Error("gameplay.crime.allocation_failed", "failed to create law response"));
  797     }
  798     if (definition->resulting_case_state && crime->state != *definition->resulting_case_state)
```

#### `gameplay.crime.invalid_response_definition` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:186`)

```cpp
184         if (!response_definitions_.emplace(definition.id, std::move(definition)).second)
  185             return foundation::Result<void>::Failure(
  186                 Error("gameplay.crime.invalid_response_definition", "invalid or duplicate law response definition"));
  187     }
  188     catch (...)
```

#### `gameplay.crime.invalid_jurisdiction` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:217`)

```cpp
215     {
  216         if (!jurisdictions_.emplace(jurisdiction.id, std::move(jurisdiction)).second)
  217             return foundation::Result<void>::Failure(Error("gameplay.crime.invalid_jurisdiction", "duplicate jurisdiction"));
  218     }
  219     catch (...)
```

#### `gameplay.crime.duplicate_law` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:206`)

```cpp
204     std::sort(jurisdiction.active_laws.begin(), jurisdiction.active_laws.end());
  205     if (std::adjacent_find(jurisdiction.active_laws.begin(), jurisdiction.active_laws.end()) != jurisdiction.active_laws.end())
  206         return foundation::Result<void>::Failure(Error("gameplay.crime.duplicate_law", "jurisdiction contains duplicate law"));
  207     for (auto law : jurisdiction.active_laws)
  208         if (!laws_.contains(law))
```

#### `gameplay.crime.unknown_law` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:280`)

```cpp
278             if (!laws_.contains(law))
  279                 return foundation::Result<void>::Failure(
  280                     Error("gameplay.crime.unknown_law", "jurisdiction references unknown law"));
  281         }
  282     }
```

#### `gameplay.crime.invalid_authority` (`EngineFramework/GameplayWorldStateOwners/Crime/src/crime.cpp:288`)

```cpp
286         if (!jurisdictions_.contains(authority.jurisdiction))
  287             return foundation::Result<void>::Failure(
  288                 Error("gameplay.crime.invalid_authority", "authority references unknown jurisdiction"));
  289     }
  290     definitions_frozen_ = true;
```

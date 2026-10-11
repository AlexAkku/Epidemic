---
aliases: []
type: "module"
source_snapshot: "2026-10-10"
tags: ["engineframework", "module", "social_legal_integration"]
source_path: "EngineFramework/IntegrationLayer/SocialLegalIntegration"
---

# EngineFramework / SocialLegalIntegration

Модуль: **SocialLegalIntegration**. Связь социальных и юридических систем.

**Реализация:** `EngineFramework/IntegrationLayer/SocialLegalIntegration/`.
**CMake targets:** `EpidemicGameFrameworkSocialLegalIntegration`
**Локальные test targets:** выделенного локального executable в подкаталоге `tests/` не найдено; тесты могут быть в общих suites

## Назначение и контракт
Goal 4 block: `b08_integration_adapters`. Freeze unit: `EngineFramework/IntegrationLayer/SocialLegalIntegration`. Admission: 28 public API, 40 mutation obligations, 1 lifecycle, 10 stale-identity, 0 external-boundary candidates.

## Публичные заголовки
Под каждым заголовком открывается снимок **точного содержимого** `.h` из переданного архива (без привязки к скомпилированной конфигурации).

- [[11 API/EngineFramework/SocialLegalIntegration/social_legal_adapters.h|social_legal_adapters.h]]: `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h`; объявленные типы: `SocialLegalMappingId`, `MappingIdHash`, `PropertyCrimeVictimPolicy`, `PropertyCrimeMapping`, `CrimeRelationshipConsequenceMapping`, `AuthorityResponseMapping`, `PropertyCrimeDisposition`, `TheftResolution`, `RelationshipPenaltyDelivery`, `RelationshipPenaltyDeliveryHash`, `SocialLegalCheckpoint`, `SocialLegalMappings`…

## Углублённое чтение
- [[90 Первоисточники/Documentation/EngineFramework/SocialLegalIntegration|Исходный подробный контракт модуля]]. Не трактуйте даты Goal 3/4 и статус LOCAL_READY как системный FULL FROZEN.
- [[02 Архитектура/Правила архитектурного freeze|Границы зависимостей]] и [[09 Качество и статус/Матрица проверок|Проверки]].
- [[02 Архитектура/Граница игрового кода|Gameplay state owners и IntegrationLayer]].

> [!note] Проверка актуальности
> Содержимое этого модуля подтверждено наличием исходников, CMake и public headers в архиве. Текущее runtime поведение и прохождение тестов здесь отдельно не квалифицировались.

---

## Подробное техническое описание (срез исходников)

В этом разделе сведения получены непосредственно из исходного C++ дерева и контрактной документации того же снимка. Наличие проверки в тестовом файле не означает, что она была выполнена при сборке Vault.

### Контракты и существенные инварианты

Ниже изложены конкретные формулировки исходного контракта модуля. Англоязычные фрагменты оставлены в исходной терминологии, чтобы не исказить условия атомарности, владения и восстановления.

#### Public contract review

- Ownership -> Crime theft resolution.
- Crime -> Society relationship consequence.
- Authority response mapping.
- Duplicate crime input.
- Victim policy.
- Failed consequence delivery.
- Checkpoint restore без duplicate penalty.

#### Failure atomicity and accepted-work review

Every local adapter mutation was reviewed for success, invalid input, retry/duplicate behavior, stale identities/cursors and publication ordering. Fallible local bookkeeping must complete before an externally owned side effect is accepted, or a durable reconciliation record must already exist.

#### Persistence and reconciliation

The module exposes local snapshot/checkpoint state. Capture/restore validation and continuation are covered by the owned integration test; whole-engine ordered persistence remains Goal 5.

#### Test evidence

- Source: `EngineFramework/DevelopmentInfrastructure/Tests/social_legal_integration_tests.cpp`.
- Target: `EpidemicGameFrameworkSocialLegalIntegrationTests`.
- Reviewed paths include happy path, invalid/stale input, duplicate/retry behavior and local failure/reconciliation behavior applicable to this module.
- The B08 worker compiled and executed this target under C++23 with Clang warnings-as-errors in the available Linux environment. Required Windows MSVC Debug/Release qualification remains a serial-integration gate.

#### Goal 4 result

`LOCAL_READY_CANDIDATE` for block-local evidence. This is not a whole-Framework `LOCAL_READY`, `SYSTEM_READY` or `FROZEN` claim.

<!-- goal4-exact-evidence -->

Источник контрактов: `Documentation/EngineFramework/SocialLegalIntegration.md`; при расхождении фактического поведения с описанием необходима проверка тестов и реализации.

### Состояния и доменные перечисления

- `PropertyCrimeVictimPolicy` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h`): `PropertyOwnerRequired`, `PropertyOwnerOptional`, `PublicSubjectRequired`, `Victimless`.
- `PropertyCrimeDisposition` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h`): `NoViolation`, `MissingRequiredVictim`, `CrimeRecorded`.

### Операции и хранилища состояния

#### `SocialLegalMappings`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:112`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
SocialLegalMappings(const crime::CrimeService& crime, const society::SocietyService& society) noexcept;
[[nodiscard]] foundation::Result<void> RegisterPropertyCrimeMapping(PropertyCrimeMapping mapping);
[[nodiscard]] foundation::Result<void> RegisterCrimeRelationshipMapping(CrimeRelationshipConsequenceMapping mapping);
[[nodiscard]] foundation::Result<void> RegisterAuthorityResponseMapping(AuthorityResponseMapping mapping);
[[nodiscard]] foundation::Result<void> Freeze();
[[nodiscard]] bool IsFrozen() const noexcept;
[[nodiscard]] Revision CurrentRevision() const noexcept;
[[nodiscard]] const PropertyCrimeMapping* FindPropertyCrimeMapping(SocialLegalMappingId id) const noexcept;
[[nodiscard]] const CrimeRelationshipConsequenceMapping* FindCrimeRelationshipMapping(SocialLegalMappingId id) const noexcept;
[[nodiscard]] const AuthorityResponseMapping* FindAuthorityResponseMapping(SocialLegalMappingId id) const noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `const crime::CrimeService& crime_`; `const society::SocietyService& society_`; `std::unordered_map<SocialLegalMappingId, PropertyCrimeMapping, MappingIdHash> property_crime_`; `std::unordered_map<SocialLegalMappingId, CrimeRelationshipConsequenceMapping, MappingIdHash> crime_relationship_`; `std::unordered_map<SocialLegalMappingId, AuthorityResponseMapping, MappingIdHash> authority_response_`; `bool frozen_ = false`.

#### `CrimeSocietyAdapter`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:161`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
CrimeSocietyAdapter(crime::CrimeService& crime, society::SocietyService& society, const SocialLegalMappings& mappings) noexcept;
[[nodiscard]] foundation::Result<void> ProcessCrimeLifecycle();
[[nodiscard]] SocialLegalCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(SocialLegalCheckpoint checkpoint);
[[nodiscard]] bool WasRelationshipPenaltyApplied(crime::CrimeRecordId crime_id, SocialLegalMappingId mapping) const noexcept;
std::uint64_t PruneDeliveriesForCrime(crime::CrimeRecordId crime_id) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `crime::CrimeService& crime_`; `society::SocietyService& society_`; `const SocialLegalMappings& mappings_`; `std::unordered_set<RelationshipPenaltyDelivery, RelationshipPenaltyDeliveryHash> applied_relationship_penalties_`.

#### `MappingIdHash`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:30`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(SocialLegalMappingId id) const noexcept;
```

#### `RelationshipPenaltyDeliveryHash`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:95`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
[[nodiscard]] std::size_t operator()(const RelationshipPenaltyDelivery& value) const noexcept;
```

#### `OwnershipCrimeAdapter`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:142`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
OwnershipCrimeAdapter(ownership::OwnershipService& ownership, crime::CrimeService& crime, const SocialLegalMappings& mappings) noexcept;
```

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ownership::OwnershipService& ownership_`; `crime::CrimeService& crime_`; `const SocialLegalMappings& mappings_`.

#### `hash`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:199`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Основные объявленные операции:

```cpp
size_t operator()(const epidemic::gameplay::social_legal_integration::SocialLegalMappingId& value) const noexcept;
```

#### `PropertyCrimeMapping`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:46`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `PropertyCrimeVictimPolicy victim_policy = PropertyCrimeVictimPolicy::PropertyOwnerRequired`; `bool denied_is_violation = false`; `bool trespass_is_violation = true`; `bool crime_candidate_is_violation = true`.

#### `CrimeRelationshipConsequenceMapping`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:58`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::int64_t delta_micro = 0`.

#### `TheftResolution`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:80`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `ownership::OwnershipPermissionResult permission`; `PropertyCrimeDisposition disposition = PropertyCrimeDisposition::NoViolation`; `std::optional<crime::CrimeRecordId> crime`.

#### `SocialLegalCheckpoint`

Объявлен в `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h:105`. Это опубликованная C++ поверхность, а не перечень всех внутренних helper-функций.

Объявленные поля/параметры состояния (публичность зависит от секции класса): `std::vector<RelationshipPenaltyDelivery> applied_relationship_penalties`.

### Сборка и межмодульные зависимости

- CMake-контракт: `EngineFramework/IntegrationLayer/SocialLegalIntegration/CMakeLists.txt`.
- Явно упомянутые linked targets: `EpidemicGameFrameworkOwnership`, `EpidemicFoundation`, `EpidemicGameFrameworkSaveGame`.
- Условия `target_link_libraries`, запрещённые связи и ограничения конфигураций проверяются по CMake и архитектурным тестам; список выше сам по себе не является полным графом транзитивных зависимостей.

### Карта реализации и отладочные точки

- `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp` (374 строк): `Error::Create`, `SocialLegalMappings::SocialLegalMappings`, `SocialLegalMappings::RegisterPropertyCrimeMapping`, `SocialLegalMappings::RegisterCrimeRelationshipMapping`, `SocialLegalMappings::RegisterAuthorityResponseMapping`, `SocialLegalMappings::CrimeTypeRegistered`, `SocialLegalMappings::RelationshipTypeRegistered`, `SocialLegalMappings::Freeze`, `SocialLegalMappings::FindPropertyCrimeMapping`, `SocialLegalMappings::FindCrimeRelationshipMapping`
- `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/save_participants.cpp` (149 строк): `SaveParticipantId::FromString`

При расследовании ошибки сначала сопоставьте вызываемый метод публичного заголовка с реализацией в этом списке, затем с проверкой, устанавливающей его инвариант.

### Диагностические коды ошибок

Коды ниже обнаружены в исходных строковых литералах реализации; для точного условия возврата и recovery изучите ветку вокруг указанной строки.

- `gameplay.social_legal.required_property_owner_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:44`
- `gameplay.social_legal.public_subject_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:54`
- `gameplay.social_legal.victim_policy_invalid`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:62`
- `gameplay.social_legal.mappings_frozen`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:119`
- `gameplay.social_legal.mapping_invalid`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:121`
- `gameplay.social_legal.mapping_duplicate`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:123`
- `gameplay.social_legal.crime_type_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:164`
- `gameplay.social_legal.relationship_type_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:158`
- `gameplay.social_legal.mappings_not_frozen`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:344`
- `gameplay.social_legal.mapping_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:282`
- `gameplay.social_legal.crime_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:285`
- `gameplay.social_legal.crime_type_mismatch`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:287`
- `gameplay.social_legal.victim_missing`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:262`
- `framework.crime.relationship_penalty`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:266`
- `gameplay.social_legal.mapping_revision_mismatch`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:346`
- `gameplay.social_legal.checkpoint_invalid`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:353`
- `gameplay.social_legal.checkpoint_duplicate`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:355`
- `framework.crime`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/save_participants.cpp:147`
- `framework.ownership`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/save_participants.cpp:139`
- `framework.society`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/save_participants.cpp:147`
- `integration.social_legal`: `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/save_participants.cpp:147`

### Проверка поведения

Внутри каталога модуля адресного `*.cpp`-теста не обнаружено. Проверяйте регистрацию общих тестов через CMake и общие suites, прежде чем делать вывод об отсутствии coverage.



### Контекст возврата ошибок в коде

Ниже приведены буквальные соседние строки исходника, а не пересказ предпосылок. Читайте полный enclosing method перед изменением обработки отказа.

#### `gameplay.social_legal.required_property_owner_missing` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:44`)

```cpp
42         {
   43             return foundation::Result<GameplayObjectRef>::Failure(
   44                 Error("gameplay.social_legal.required_property_owner_missing",
   45                       "property crime mapping requires an effective property owner"));
   46         }
```

#### `gameplay.social_legal.public_subject_missing` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:54`)

```cpp
52         {
   53             return foundation::Result<GameplayObjectRef>::Failure(
   54                 Error("gameplay.social_legal.public_subject_missing",
   55                       "public-subject victim policy requires a valid public subject"));
   56         }
```

#### `gameplay.social_legal.victim_policy_invalid` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:62`)

```cpp
60     }
   61     return foundation::Result<GameplayObjectRef>::Failure(
   62         Error("gameplay.social_legal.victim_policy_invalid", "unsupported property crime victim policy"));
   63 }
   64
```

#### `gameplay.social_legal.mappings_frozen` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:119`)

```cpp
117 {
  118     if (frozen_)
  119         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mappings_frozen", "mappings are frozen"));
  120     if (!mapping.id.IsValid() || !mapping.crime_type.IsValid() || !mapping.response_type.IsValid())
  121         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mapping_invalid", "authority response mapping IDs must be valid"));
```

#### `gameplay.social_legal.mapping_invalid` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:121`)

```cpp
119         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mappings_frozen", "mappings are frozen"));
  120     if (!mapping.id.IsValid() || !mapping.crime_type.IsValid() || !mapping.response_type.IsValid())
  121         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mapping_invalid", "authority response mapping IDs must be valid"));
  122     if (property_crime_.contains(mapping.id) || crime_relationship_.contains(mapping.id) || authority_response_.contains(mapping.id))
  123         return founda…
```

#### `gameplay.social_legal.mapping_duplicate` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:123`)

```cpp
121         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mapping_invalid", "authority response mapping IDs must be valid"));
  122     if (property_crime_.contains(mapping.id) || crime_relationship_.contains(mapping.id) || authority_response_.contains(mapping.id))
  123         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mapping_duplicate", "mapping id already registered"));
  124     authority_response_.emplace(mapping.id, std::move(mapping));
  125     Bump();
```

#### `gameplay.social_legal.crime_type_missing` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:164`)

```cpp
162         (void)id;
  163         if (!CrimeTypeRegistered(mapping.crime_type))
  164             return foundation::Result<void>::Failure(Error("gameplay.social_legal.crime_type_missing", "authority response mapping references an unregistered crime type"));
  165     }
  166
```

#### `gameplay.social_legal.relationship_type_missing` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:158`)

```cpp
156             return foundation::Result<void>::Failure(Error("gameplay.social_legal.crime_type_missing", "relationship consequence mapping references an unregistered crime type"));
  157         if (!RelationshipTypeRegistered(mapping.relationship_type))
  158             return foundation::Result<void>::Failure(Error("gameplay.social_legal.relationship_type_missing", "relationship consequence mapping references an unregistered relationship type"));
  159     }
  160     for (const auto& [id, mapping] : authority_response_)
```

#### `gameplay.social_legal.mappings_not_frozen` (`EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:344`)

```cpp
342 {
  343     if (!mappings_.IsFrozen())
  344         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mappings_not_frozen", "social/legal mappings must be frozen before checkpoint restore"));
  345     if (checkpoint.mapping_revision != mappings_.CurrentRevision())
  346         return foundation::Result<void>::Failure(Error("gameplay.social_legal.mapping_revision_mismatch", "checkpoint mapping revision does not match current frozen mappings"));
```

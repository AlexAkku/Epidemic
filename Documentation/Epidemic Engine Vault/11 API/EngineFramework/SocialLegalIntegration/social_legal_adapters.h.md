---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h"
---

# SocialLegalIntegration: social_legal_adapters.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/SocialLegalIntegration/include/Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h`.

**Модуль:** [[06 EngineFramework/Модуль SocialLegalIntegration|SocialLegalIntegration]].
**Размер исходного файла:** 8492 байт.
**Подключение:** `#include <Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h>`.

## Объявления типов (лексический индекс)
`SocialLegalMappingId`, `MappingIdHash`, `PropertyCrimeVictimPolicy`, `PropertyCrimeMapping`, `CrimeRelationshipConsequenceMapping`, `AuthorityResponseMapping`, `PropertyCrimeDisposition`, `TheftResolution`, `RelationshipPenaltyDelivery`, `RelationshipPenaltyDeliveryHash`, `SocialLegalCheckpoint`, `SocialLegalMappings`, `OwnershipCrimeAdapter`, `CrimeSocietyAdapter`, `hash`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Crime/crime.h"
#include "Epidemic/GameFramework/Ownership/ownership.h"
#include "Epidemic/GameFramework/Society/society.h"

#include <cstddef>
#include <cstdint>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace epidemic::gameplay::social_legal_integration
{
struct SocialLegalMappingId
{
    TypeId value{};

    static constexpr SocialLegalMappingId FromString(std::string_view value) noexcept
    {
        return {TypeId::FromString(value)};
    }

    [[nodiscard]] constexpr bool IsValid() const noexcept { return value.IsValid(); }
    [[nodiscard]] constexpr bool operator==(const SocialLegalMappingId&) const noexcept = default;
    [[nodiscard]] constexpr auto operator<=>(const SocialLegalMappingId&) const noexcept = default;
};

struct MappingIdHash
{
    [[nodiscard]] std::size_t operator()(SocialLegalMappingId id) const noexcept
    {
        return std::hash<TypeId>{}(id.value);
    }
};

enum class PropertyCrimeVictimPolicy
{
    PropertyOwnerRequired,
    PropertyOwnerOptional,
    PublicSubjectRequired,
    Victimless
};

struct PropertyCrimeMapping
{
    SocialLegalMappingId id{};
    ownership::PropertyRightId right{};
    crime::CrimeTypeId crime_type{};
    PropertyCrimeVictimPolicy victim_policy = PropertyCrimeVictimPolicy::PropertyOwnerRequired;
    GameplayObjectRef public_subject{};
    bool denied_is_violation = false;
    bool trespass_is_violation = true;
    bool crime_candidate_is_violation = true;
};

struct CrimeRelationshipConsequenceMapping
{
    SocialLegalMappingId id{};
    crime::CrimeTypeId crime_type{};
    society::RelationshipTypeId relationship_type{};
    std::int64_t delta_micro = 0;
};

struct AuthorityResponseMapping
{
    SocialLegalMappingId id{};
    crime::CrimeTypeId crime_type{};
    crime::LawResponseTypeId response_type{};
};

enum class PropertyCrimeDisposition
{
    NoViolation,
    MissingRequiredVictim,
    CrimeRecorded
};

struct TheftResolution
{
    ownership::OwnershipPermissionResult permission;
    PropertyCrimeDisposition disposition = PropertyCrimeDisposition::NoViolation;
    std::optional<crime::CrimeRecordId> crime;
};

struct RelationshipPenaltyDelivery
{
    crime::CrimeRecordId crime{};
    SocialLegalMappingId mapping{};

    [[nodiscard]] constexpr bool operator==(const RelationshipPenaltyDelivery&) const noexcept = default;
};

struct RelationshipPenaltyDeliveryHash
{
    [[nodiscard]] std::size_t operator()(const RelationshipPenaltyDelivery& value) const noexcept
    {
        const auto a = std::hash<crime::CrimeRecordId>{}(value.crime);
        const auto b = std::hash<TypeId>{}(value.mapping.value);
        return a ^ (b + 0x9e3779b97f4a7c15ULL + (a << 6U) + (a >> 2U));
    }
};

struct SocialLegalCheckpoint
{
    Revision mapping_revision{};
    ChangeCursor crime_cursor{};
    std::vector<RelationshipPenaltyDelivery> applied_relationship_penalties;
};

class SocialLegalMappings
{
public:
    SocialLegalMappings(const crime::CrimeService& crime, const society::SocietyService& society) noexcept;

    [[nodiscard]] foundation::Result<void> RegisterPropertyCrimeMapping(PropertyCrimeMapping mapping);
    [[nodiscard]] foundation::Result<void> RegisterCrimeRelationshipMapping(CrimeRelationshipConsequenceMapping mapping);
    [[nodiscard]] foundation::Result<void> RegisterAuthorityResponseMapping(AuthorityResponseMapping mapping);
    [[nodiscard]] foundation::Result<void> Freeze();

    [[nodiscard]] bool IsFrozen() const noexcept { return frozen_; }
    [[nodiscard]] Revision CurrentRevision() const noexcept { return revision_; }
    [[nodiscard]] const PropertyCrimeMapping* FindPropertyCrimeMapping(SocialLegalMappingId id) const noexcept;
    [[nodiscard]] const CrimeRelationshipConsequenceMapping* FindCrimeRelationshipMapping(SocialLegalMappingId id) const noexcept;
    [[nodiscard]] const AuthorityResponseMapping* FindAuthorityResponseMapping(SocialLegalMappingId id) const noexcept;

private:
    [[nodiscard]] bool CrimeTypeRegistered(crime::CrimeTypeId id) const;
    [[nodiscard]] bool RelationshipTypeRegistered(society::RelationshipTypeId id) const;
    void Bump() noexcept { ++revision_.value; }

    const crime::CrimeService& crime_;
    const society::SocietyService& society_;
    std::unordered_map<SocialLegalMappingId, PropertyCrimeMapping, MappingIdHash> property_crime_;
    std::unordered_map<SocialLegalMappingId, CrimeRelationshipConsequenceMapping, MappingIdHash> crime_relationship_;
    std::unordered_map<SocialLegalMappingId, AuthorityResponseMapping, MappingIdHash> authority_response_;
    Revision revision_{};
    bool frozen_ = false;
};

class OwnershipCrimeAdapter
{
public:
    OwnershipCrimeAdapter(ownership::OwnershipService& ownership, crime::CrimeService& crime,
                          const SocialLegalMappings& mappings) noexcept;

    [[nodiscard]] foundation::Result<TheftResolution> TryCreateCrimeFromDeniedRight(
        GameplayObjectRef actor,
        GameplayObjectRef property,
        SocialLegalMappingId mapping,
        GameplayObjectRef area,
        GameplayContext context = {});

private:
    ownership::OwnershipService& ownership_;
    crime::CrimeService& crime_;
    const SocialLegalMappings& mappings_;
};

class CrimeSocietyAdapter
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<CrimeSocietyAdapter>;

public:
    CrimeSocietyAdapter(crime::CrimeService& crime, society::SocietyService& society,
                        const SocialLegalMappings& mappings) noexcept;

    [[nodiscard]] foundation::Result<void> ApplyRelationshipPenalty(
        crime::CrimeRecordId crime_id,
        SocialLegalMappingId mapping,
        GameplayContext context = {});

    [[nodiscard]] foundation::Result<crime::LawResponseId> RequestAuthorityResponse(
        crime::CrimeRecordId crime_id,
        SocialLegalMappingId mapping,
        GameplayObjectRef authority,
        GameplayObjectRef target = {},
        GameplayContext context = {});

    [[nodiscard]] foundation::Result<void> ProcessCrimeLifecycle();
    [[nodiscard]] SocialLegalCheckpoint CaptureCheckpoint() const;
    [[nodiscard]] foundation::Result<void> RestoreCheckpoint(SocialLegalCheckpoint checkpoint);
    [[nodiscard]] bool WasRelationshipPenaltyApplied(crime::CrimeRecordId crime_id,
                                                      SocialLegalMappingId mapping) const noexcept;
    std::uint64_t PruneDeliveriesForCrime(crime::CrimeRecordId crime_id) noexcept;

private:
    crime::CrimeService& crime_;
    society::SocietyService& society_;
    const SocialLegalMappings& mappings_;
    std::unordered_set<RelationshipPenaltyDelivery, RelationshipPenaltyDeliveryHash> applied_relationship_penalties_;
    ChangeCursor crime_cursor_{};
};
} // namespace epidemic::gameplay::social_legal_integration

namespace std
{
template <> struct hash<epidemic::gameplay::social_legal_integration::SocialLegalMappingId>
{
    size_t operator()(const epidemic::gameplay::social_legal_integration::SocialLegalMappingId& value) const noexcept
    {
        return hash<epidemic::gameplay::TypeId>{}(value.value);
    }
};
} // namespace std

// BEGIN GENERATED GOAL5 PARTICIPANTS
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/Crime/crime.h"
#include "Epidemic/GameFramework/Ownership/ownership.h"
#include "Epidemic/GameFramework/Society/society.h"
namespace epidemic::gameplay::savegame
{
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateCrimeServiceSaveParticipant(epidemic::gameplay::crime::CrimeService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateOwnershipServiceSaveParticipant(epidemic::gameplay::ownership::OwnershipService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateSocietyServiceSaveParticipant(epidemic::gameplay::society::SocietyService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateCrimeSocietyAdapterSaveParticipant(epidemic::gameplay::social_legal_integration::CrimeSocietyAdapter& owner, SaveRestoreWorkspace& workspace);
}
// END GENERATED GOAL5 PARTICIPANTS
```

---

## Разбор опубликованного API

Интерфейс ниже извлечён из объявлений заголовка. Это навигационный индекс сигнатур и полей, не замена компиляторному AST или проверки вызовов. Все значения по умолчанию и типы необходимо читать вместе с полным заголовком выше.

### Прямые типовые зависимости

Заголовок непосредственно подключает: `Epidemic/Foundation/result.h`, `Epidemic/GameFramework/Crime/crime.h`, `Epidemic/GameFramework/Ownership/ownership.h`, `Epidemic/GameFramework/Society/society.h`, `Epidemic/GameFramework/SaveGame/save_game.h`, `Epidemic/GameFramework/Crime/crime.h`, `Epidemic/GameFramework/Ownership/ownership.h`, `Epidemic/GameFramework/Society/society.h`.

### Перечисления и допустимые категории

- `PropertyCrimeVictimPolicy`: `PropertyOwnerRequired`, `PropertyOwnerOptional`, `PublicSubjectRequired`, `Victimless`
- `PropertyCrimeDisposition`: `NoViolation`, `MissingRequiredVictim`, `CrimeRecorded`

### Состав типов и операции

#### `SocialLegalMappingId` (строка заголовка 16)

Публичные сигнатуры операций:

```cpp
static constexpr SocialLegalMappingId FromString(std::string_view value) noexcept;
[[nodiscard]] constexpr bool IsValid() const noexcept;
[[nodiscard]] constexpr bool operator==(const SocialLegalMappingId&) const noexcept = default;
[[nodiscard]] constexpr auto operator<=>(const SocialLegalMappingId&) const noexcept = default;
```

#### `MappingIdHash` (строка заголовка 30)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(SocialLegalMappingId id) const noexcept;
```

#### `PropertyCrimeMapping` (строка заголовка 46)

Публичные поля и значения по умолчанию:

- `PropertyCrimeVictimPolicy victim_policy = PropertyCrimeVictimPolicy::PropertyOwnerRequired`
- `bool denied_is_violation = false`
- `bool trespass_is_violation = true`
- `bool crime_candidate_is_violation = true`

#### `CrimeRelationshipConsequenceMapping` (строка заголовка 58)

Публичные поля и значения по умолчанию:

- `std::int64_t delta_micro = 0`

#### `TheftResolution` (строка заголовка 80)

Публичные поля и значения по умолчанию:

- `ownership::OwnershipPermissionResult permission`
- `PropertyCrimeDisposition disposition = PropertyCrimeDisposition::NoViolation`
- `std::optional<crime::CrimeRecordId> crime`

#### `RelationshipPenaltyDelivery` (строка заголовка 87)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] constexpr bool operator==(const RelationshipPenaltyDelivery&) const noexcept = default;
```

#### `RelationshipPenaltyDeliveryHash` (строка заголовка 95)

Публичные сигнатуры операций:

```cpp
[[nodiscard]] std::size_t operator()(const RelationshipPenaltyDelivery& value) const noexcept;
```

#### `SocialLegalCheckpoint` (строка заголовка 105)

Публичные поля и значения по умолчанию:

- `std::vector<RelationshipPenaltyDelivery> applied_relationship_penalties`

#### `SocialLegalMappings` (строка заголовка 112)

Публичные сигнатуры операций:

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

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `OwnershipCrimeAdapter` (строка заголовка 142)

Публичные сигнатуры операций:

```cpp
OwnershipCrimeAdapter(ownership::OwnershipService& ownership, crime::CrimeService& crime, const SocialLegalMappings& mappings) noexcept;
```

#### `CrimeSocietyAdapter` (строка заголовка 161)

Публичные сигнатуры операций:

```cpp
CrimeSocietyAdapter(crime::CrimeService& crime, society::SocietyService& society, const SocialLegalMappings& mappings) noexcept;
[[nodiscard]] foundation::Result<void> ProcessCrimeLifecycle();
[[nodiscard]] SocialLegalCheckpoint CaptureCheckpoint() const;
[[nodiscard]] foundation::Result<void> RestoreCheckpoint(SocialLegalCheckpoint checkpoint);
[[nodiscard]] bool WasRelationshipPenaltyApplied(crime::CrimeRecordId crime_id, SocialLegalMappingId mapping) const noexcept;
std::uint64_t PruneDeliveriesForCrime(crime::CrimeRecordId crime_id) noexcept;
```

Возвраты `foundation::Result<T>` необходимо проверять: после ошибки нельзя обращаться к `Value()`. Код отказа и отсутствие частичной mutation уточняются по реализации и модульному контракту.

#### `hash` (строка заголовка 199)

Публичные сигнатуры операций:

```cpp
size_t operator()(const epidemic::gameplay::social_legal_integration::SocialLegalMappingId& value) const noexcept;
```

### Интеграция и безопасность вызовов

- Ошибки ожидаемых операций передаются через `Result`; проверяйте ветвь ошибки и `GetError()`.
- `std::optional<T>` означает возможное отсутствие значения; до разыменования требуется проверить наличие.
- `std::string_view` не владеет памятью; не сохраняйте его после уничтожения исходной строки без явного копирования.

### Навигация от вызова к коду и тесту

Это точные файловые позиции из того же архива. Совпадение имён методов между разными классами или overloads требует проверки через clangd: таблица не является окончательно разрешённым call graph.

| Операция | Обнаруженная реализация | Строка |
| --- | --- | --- |
| `SocialLegalMappings` | `SocialLegalMappings::SocialLegalMappings` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:81` |
| `RegisterPropertyCrimeMapping` | `SocialLegalMappings::RegisterPropertyCrimeMapping` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:86` |
| `RegisterCrimeRelationshipMapping` | `SocialLegalMappings::RegisterCrimeRelationshipMapping` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:103` |
| `RegisterAuthorityResponseMapping` | `SocialLegalMappings::RegisterAuthorityResponseMapping` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:116` |
| `Freeze` | `SocialLegalMappings::Freeze` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:141` |
| `FindPropertyCrimeMapping` | `SocialLegalMappings::FindPropertyCrimeMapping` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:172` |
| `FindCrimeRelationshipMapping` | `SocialLegalMappings::FindCrimeRelationshipMapping` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:178` |
| `FindAuthorityResponseMapping` | `SocialLegalMappings::FindAuthorityResponseMapping` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:184` |
| `OwnershipCrimeAdapter` | `OwnershipCrimeAdapter::OwnershipCrimeAdapter` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:190` |
| `CrimeSocietyAdapter` | `CrimeSocietyAdapter::CrimeSocietyAdapter` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:236` |
| `ProcessCrimeLifecycle` | `CrimeSocietyAdapter::ProcessCrimeLifecycle` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:293` |
| `PruneDeliveriesForCrime` | `CrimeSocietyAdapter::PruneDeliveriesForCrime` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:315` |
| `CaptureCheckpoint` | `CrimeSocietyAdapter::CaptureCheckpoint` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:324` |
| `RestoreCheckpoint` | `CrimeSocietyAdapter::RestoreCheckpoint` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:341` |
| `WasRelationshipPenaltyApplied` | `CrimeSocietyAdapter::WasRelationshipPenaltyApplied` | `EngineFramework/IntegrationLayer/SocialLegalIntegration/src/social_legal_adapters.cpp:369` |

Первые наблюдаемые вызовы операций из тестовых исходников:

- `ProcessCrimeLifecycle`: `EngineFramework/DevelopmentInfrastructure/Tests/sociallegalintegration_save_participant_tests.cpp:81`
- `CaptureCheckpoint`: `EngineFramework/DevelopmentInfrastructure/Tests/sociallegalintegration_save_participant_tests.cpp:87`
- `RestoreCheckpoint`: `EngineFramework/DevelopmentInfrastructure/Tests/sociallegalintegration_save_participant_tests.cpp:89`


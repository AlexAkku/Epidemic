---
aliases: []
type: "api-header"
source_snapshot: "2026-10-10"
tags: ["api", "engineframework"]
source_path: "EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h"
---

# PopulationSimulationIntegration: population_simulation_adapters.h

Этот файл является точным текстовым снимком публичного заголовка из `Epidemic(1).zip`. Исходный путь: `EngineFramework/IntegrationLayer/PopulationSimulationIntegration/include/Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h`.

**Модуль:** [[06 EngineFramework/Модуль PopulationSimulationIntegration|PopulationSimulationIntegration]].
**Размер исходного файла:** 7429 байт.
**Подключение:** `#include <Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h>`.

## Объявления типов (лексический индекс)
`PopulationEncounterPlanState`, `PopulationBackedEncounterSlot`, `PopulationBackedEncounterPlan`, `PopulationEncounterAdapterSnapshot`, `PopulationBackedSpawnResult`, `PopulationEncounterAdapter`, `RolesNeedsAdapter`, `PopulationLifecycleReconciliation`, `PopulationLifecycleAdapterSnapshot`, `PopulationLifecycleAdapter`, `CityMorningResult`, `CityLifeAdapter`

> [!note] Навигация API
> Список выше получен лексическим сканированием, не является полноценным C++ AST: вложенные/templated декларации и нераспознанные функции требуют просмотра полного заголовка или clangd.

## Точный текст заголовка

```cpp
#pragma once

#include "Epidemic/Foundation/result.h"
#include "Epidemic/GameFramework/Encounters/encounters.h"
#include "Epidemic/GameFramework/NeedsLife/needs_life.h"
#include "Epidemic/GameFramework/Population/population.h"
#include "Epidemic/GameFramework/RolesJobs/roles_jobs.h"

#include <cstddef>
#include <cstdint>
#include <vector>

namespace epidemic::gameplay::population_simulation
{
enum class PopulationEncounterPlanState
{
    AwaitingEntityBindings,
    RollbackPending,
    Completed,
    Failed
};

struct PopulationBackedEncounterSlot
{
    population::PopulationAllocationId allocation{};
    population::PopulationUnitId unit{};
    population::PopulationTemplateId population_template{};
    TypeId archetype{};
    encounters::SpawnedEntityRecordId spawned_record{};
    GameplayObjectRef entity{};
};

struct PopulationBackedEncounterPlan
{
    encounters::SpawnRequestId request{};
    encounters::EncounterDefinitionId encounter{};
    encounters::EncounterInstanceId encounter_instance{};
    population::PopulationGroupId group{};
    GameplayObjectRef area{};
    random::RandomSeed seed{};
    PopulationEncounterPlanState state = PopulationEncounterPlanState::AwaitingEntityBindings;
    std::vector<PopulationBackedEncounterSlot> slots;
};

struct PopulationEncounterAdapterSnapshot
{
    std::vector<PopulationBackedEncounterPlan> plans;
};

struct PopulationBackedSpawnResult
{
    encounters::SpawnResult spawn;
    std::vector<population::PopulationUnitId> allocated_units;
};

class PopulationEncounterAdapter
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<PopulationEncounterAdapter>;

public:
    [[nodiscard]] foundation::Result<PopulationBackedSpawnResult> SpawnFromPopulation(
        population::PopulationService& population_service,
        encounters::EncountersService& encounter_service,
        population::PopulationGroupId group,
        encounters::SpawnRequest request,
        std::size_t max_units);

    [[nodiscard]] foundation::Result<void> BindSpawnedEntity(
        population::PopulationService& population_service,
        encounters::EncountersService& encounter_service,
        encounters::SpawnRequestId request,
        encounters::SpawnedEntityRecordId spawned_record,
        GameplayObjectRef entity,
        GameplayContext context = {});

    [[nodiscard]] foundation::Result<void> CancelPendingSpawn(
        population::PopulationService& population_service,
        encounters::EncountersService& encounter_service,
        encounters::SpawnRequestId request,
        GameplayContext context = {});

    [[nodiscard]] const PopulationBackedEncounterPlan* FindPlan(encounters::SpawnRequestId request) const noexcept;
    [[nodiscard]] PopulationEncounterAdapterSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(
        PopulationEncounterAdapterSnapshot snapshot,
        const population::PopulationService& population_service,
        const encounters::EncountersService& encounter_service);

private:
    [[nodiscard]] PopulationBackedEncounterPlan* FindMutablePlan(encounters::SpawnRequestId request) noexcept;
    std::vector<PopulationBackedEncounterPlan> plans_;
};

class RolesNeedsAdapter
{
public:
    [[nodiscard]] foundation::Result<void> CreateWorkPressureForDuty(
        const roles_jobs::Duty& duty,
        needs_life::NeedsLifeService& needs_service,
        std::int64_t urgency,
        GameplayContext context) const;

    [[nodiscard]] foundation::Result<void> SatisfyWorkNeedFromDuty(
        const roles_jobs::Duty& duty,
        needs_life::NeedsLifeService& needs_service,
        needs_life::NeedTypeId work_need,
        std::int64_t amount,
        GameplayContext context) const;
};

struct PopulationLifecycleReconciliation
{
    population::PopulationUnitId unit{};
    Revision death_revision{};
    bool roles_cleaned = false;
    bool needs_cleaned = false;

    [[nodiscard]] bool Complete() const noexcept
    {
        return roles_cleaned && needs_cleaned;
    }
};

struct PopulationLifecycleAdapterSnapshot
{
    std::vector<PopulationLifecycleReconciliation> reconciliations;
};

class PopulationLifecycleAdapter
{
    friend struct epidemic::gameplay::SnapshotRestoreAccess<PopulationLifecycleAdapter>;

public:
    [[nodiscard]] foundation::Result<void> MarkResidentDead(
        population::PopulationService& population_service,
        roles_jobs::RolesJobsService& roles_service,
        needs_life::NeedsLifeService& needs_service,
        population::PopulationUnitId unit,
        GameplayContext context);

    [[nodiscard]] PopulationLifecycleAdapterSnapshot CaptureSnapshot() const;
    [[nodiscard]] foundation::Result<void> RestoreSnapshot(PopulationLifecycleAdapterSnapshot snapshot);
    void PruneCompleted();

private:
    [[nodiscard]] PopulationLifecycleReconciliation* FindReconciliation(population::PopulationUnitId unit,
                                                                        Revision death_revision) noexcept;
    std::vector<PopulationLifecycleReconciliation> reconciliations_;
};

struct CityMorningResult
{
    std::vector<roles_jobs::DutyId> activated_duties;
    std::vector<population::PopulationUnit> materialization_candidates;
    std::vector<needs_life::NeedState> critical_needs;
};

class CityLifeAdapter
{
public:
    [[nodiscard]] CityMorningResult RunMorningStep(
        population::PopulationService& population_service,
        roles_jobs::RolesJobsService& roles_service,
        needs_life::NeedsLifeService& needs_service,
        GameplayObjectRef area,
        GameplayTimePoint now,
        std::size_t materialization_limit,
        GameplayContext context) const;
};
}

// BEGIN GENERATED GOAL5 PARTICIPANTS
#include "Epidemic/GameFramework/SaveGame/save_game.h"
#include "Epidemic/GameFramework/Encounters/encounters.h"
#include "Epidemic/GameFramework/NeedsLife/needs_life.h"
#include "Epidemic/GameFramework/Population/population.h"
#include "Epidemic/GameFramework/RolesJobs/roles_jobs.h"
namespace epidemic::gameplay::savegame
{
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateEncountersServiceSaveParticipant(epidemic::gameplay::encounters::EncountersService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateNeedsLifeServiceSaveParticipant(epidemic::gameplay::needs_life::NeedsLifeService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreatePopulationServiceSaveParticipant(epidemic::gameplay::population::PopulationService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreateRolesJobsServiceSaveParticipant(epidemic::gameplay::roles_jobs::RolesJobsService& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreatePopulationEncounterAdapterSaveParticipant(epidemic::gameplay::population_simulation::PopulationEncounterAdapter& owner, SaveRestoreWorkspace& workspace);
[[nodiscard]] std::unique_ptr<ISaveParticipant> CreatePopulationLifecycleAdapterSaveParticipant(epidemic::gameplay::population_simulation::PopulationLifecycleAdapter& owner, SaveRestoreWorkspace& workspace);
}
// END GENERATED GOAL5 PARTICIPANTS
```

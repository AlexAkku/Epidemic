// Generated from the reviewed participant mapping and explicit composition fixture.
#include "Epidemic/GameFramework/ExtendedGameplayIntegration/extended_gameplay_adapters.h"
#include "Epidemic/GameFramework/GameplayIntegration/gameplay_adapters.h"
#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/GameFramework/InteractionEffectsIntegration/interaction_effects_adapter.h"
#include "Epidemic/GameFramework/NarrativeIntegration/narrative_adapters.h"
#include "Epidemic/GameFramework/PerceptionKnowledgeAIIntegration/perception_knowledge_ai_adapters.h"
#include "Epidemic/GameFramework/PopulationSimulationIntegration/population_simulation_adapters.h"
#include "Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h"
#include "Epidemic/GameFramework/SocialLegalIntegration/social_legal_adapters.h"
#include "Epidemic/GameFramework/StateIntegration/state_adapters.h"
#include "Epidemic/GameFramework/TraversalNavigationConstructionIntegration/traversal_navigation_construction_adapters.h"
#include "Epidemic/GameFramework/WorldIntegration/world_adapters.h"
#include "participant_test_contract.h"
#include <filesystem>
#include <fstream>
#include <iterator>
using namespace epidemic::gameplay;
void TestFrameworkSaveComposition(const std::filesystem::path& recording = {})
{
    epidemic::gameplay::facts::GameplayFactsService facts;
    epidemic::gameplay::time::GameplayTimeService time;
    epidemic::gameplay::abilities::AbilityService abilities;
    epidemic::gameplay::ai::AIService ai;
    epidemic::gameplay::combat::CombatService combat;
    epidemic::gameplay::conditions::ConditionService conditions;
    epidemic::gameplay::construction::ConstructionService construction;
    epidemic::gameplay::crime::CrimeService crime;
    epidemic::gameplay::dialogue::DialogueService dialogue;
    epidemic::gameplay::economy::EconomyService economy;
    epidemic::gameplay::effects::EffectService effects;
    epidemic::gameplay::encounters::EncountersService encounters;
    epidemic::gameplay::entities::EntityService entities;
    epidemic::gameplay::environment::EnvironmentService environment;
    epidemic::gameplay::equipment::EquipmentService equipment;
    epidemic::gameplay::interaction::InteractionService interaction;
    epidemic::gameplay::items::ItemsInventoryService items;
    epidemic::gameplay::knowledge::KnowledgeService knowledge;
    epidemic::gameplay::loot::LootService loot;
    epidemic::gameplay::materials::MaterialService materials;
    epidemic::gameplay::narrative::NarrativeService narrative;
    epidemic::gameplay::navigation_semantics::NavigationSemanticsService navigation_semantics;
    epidemic::gameplay::needs_life::NeedsLifeService needs_life;
    epidemic::gameplay::ownership::OwnershipService ownership;
    epidemic::gameplay::perception::PerceptionService perception;
    epidemic::gameplay::population::PopulationService population;
    epidemic::gameplay::processes::ProcessesService processes;
    epidemic::gameplay::progression::ProgressionService progression;
    epidemic::gameplay::resources::ResourcesProductionService resources;
    epidemic::gameplay::roles_jobs::RolesJobsService roles_jobs;
    epidemic::gameplay::simulation::SimulationService simulation;
    epidemic::gameplay::society::SocietyService society;
    epidemic::gameplay::traversal::TraversalService traversal;
    epidemic::gameplay::world::WorldService world;
    // Shared authoritative owners, not independently constructed adapter fixtures.
    integration::TradeCoordinator trade(economy, items, ownership);
    integration::AbilityEffectsDispatcher ability_effects(effects);
    participant_tests::Check(bool(ability_effects.FreezeMappings()), "freeze ability/effect mappings");
    const auto clock = time.RegisterClock("composition.clock");
    const auto action = time.RegisterAction("composition.ability", abilities::AbilityService::Domain());
    participant_tests::Check(bool(clock) && bool(action), "composition time contracts");
    integration::AbilityTimeAdapter ability_time(time, abilities, clock.Value(), action.Value());
    integration::ConditionProgressionAdapter condition_progression(conditions, progression);
    participant_tests::Check(bool(condition_progression.FreezeMappings()), "freeze condition/progression mappings");
    integration::DeathRewardAdapter death_reward(combat, loot);
    participant_tests::Check(bool(death_reward.FreezeMappings()), "freeze death/reward mappings");
    population_simulation::PopulationEncounterAdapter population_encounter;
    population_simulation::PopulationLifecycleAdapter population_lifecycle;
    social_legal_integration::SocialLegalMappings social_mappings(crime, society);
    participant_tests::Check(bool(social_mappings.Freeze()), "freeze social/legal mappings");
    social_legal_integration::CrimeSocietyAdapter social_legal(crime, society, social_mappings);
    state_integration::StateFactsAdapter state_facts(entities, materials, conditions, effects, facts);
    state_integration::StateLifecycleAdapter state_lifecycle(entities, materials, conditions, effects);
    state_integration::ConditionEffectsAdapter state_delivery(conditions, effects);
    state_integration::StateTimeAdapter state_time(conditions, effects, time);
    participant_tests::Check(bool(state_facts.RegisterContracts()) && bool(state_time.RegisterContracts()), "state contracts");
    state_integration::StateIntegrationPersistence state(state_facts, state_lifecycle, state_delivery, state_time);
    world_integration::WorldFactsAdapter world_facts(world, environment, interaction, facts);
    participant_tests::Check(bool(world_facts.RegisterContracts()), "world/facts contracts");
    integration::ScheduledTriggerDispatcher scheduled(time);
    narrative_integration::NarrativeExternalConsequenceOutbox narrative_outbox;
    // A real immutable effect handler/definition/mapping is required even for an empty inbox.
    class NoopHandler final : public effects::IEffectHandler
    {
      public:
        effects::EffectTypeId Type() const noexcept override { return effects::EffectTypeId::FromString("composition.noop"); }
        effects::EffectHandlerCapabilities Capabilities() const noexcept override { return {true, false, false, true}; }
        epidemic::foundation::Result<effects::EffectPrepareResult> Prepare(const effects::EffectOperation&) const override
        { return epidemic::foundation::Result<effects::EffectPrepareResult>::Success({effects::EffectPrepareDisposition::Accepted, {}}); }
        epidemic::foundation::Result<effects::EffectCommitResult> Commit(const effects::EffectOperation&, const effects::RegisteredEffectPayload&) noexcept override
        { return epidemic::foundation::Result<effects::EffectCommitResult>::Success({effects::EffectCommitDisposition::Applied, {}}); }
    };
    auto handler = std::make_shared<NoopHandler>();
    participant_tests::Check(bool(effects.RegisterHandler("composition.noop", handler)), "composition effect handler");
    effects::EffectDefinition bundle; bundle.canonical_name = "composition.bundle";
    bundle.steps.push_back({handler->Type(), effects::EffectTargetSelector::AllTargets, 1, {}});
    const auto effect = effects.RegisterDefinition(std::move(bundle));
    participant_tests::Check(bool(effect), "composition effect bundle");
    const auto interaction_type = interaction::InteractionTypeId::FromString("composition.interaction");
    interaction::InteractionDefinition definition; definition.type = interaction_type; definition.canonical_name = "composition.interaction";
    participant_tests::Check(bool(interaction.RegisterDefinition(std::move(definition))), "composition interaction definition");
    interaction_effects_integration::InteractionEffectExecutor interaction_effects(effects, interaction_type);
    participant_tests::Check(bool(interaction_effects.RegisterEffect(effect.Value())), "register interaction/effect mapping");
    participant_tests::FreezeOwner(facts);
    participant_tests::FreezeOwner(time);
    participant_tests::FreezeOwner(abilities);
    participant_tests::FreezeOwner(ai);
    participant_tests::FreezeOwner(combat);
    participant_tests::FreezeOwner(conditions);
    participant_tests::FreezeOwner(construction);
    participant_tests::FreezeOwner(crime);
    participant_tests::FreezeOwner(dialogue);
    participant_tests::FreezeOwner(economy);
    participant_tests::FreezeOwner(effects);
    participant_tests::FreezeOwner(encounters);
    participant_tests::FreezeOwner(entities);
    participant_tests::FreezeOwner(environment);
    participant_tests::FreezeOwner(equipment);
    participant_tests::FreezeOwner(interaction);
    participant_tests::FreezeOwner(items);
    participant_tests::FreezeOwner(knowledge);
    participant_tests::FreezeOwner(loot);
    participant_tests::FreezeOwner(materials);
    participant_tests::FreezeOwner(narrative);
    participant_tests::FreezeOwner(navigation_semantics);
    participant_tests::FreezeOwner(needs_life);
    participant_tests::FreezeOwner(ownership);
    participant_tests::FreezeOwner(perception);
    participant_tests::FreezeOwner(population);
    participant_tests::FreezeOwner(processes);
    participant_tests::FreezeOwner(progression);
    participant_tests::FreezeOwner(resources);
    participant_tests::FreezeOwner(roles_jobs);
    participant_tests::FreezeOwner(simulation);
    participant_tests::FreezeOwner(society);
    participant_tests::FreezeOwner(traversal);
    participant_tests::FreezeOwner(world);
    participant_tests::Check(bool(interaction_effects.Freeze(interaction)), "freeze interaction/effect mapping");
    participant_tests::Check(bool(scheduled.Freeze()), "freeze scheduled dispatcher");
    death_reward.RestoreCursor(combat.LatestChangeCursor());
    participant_tests::Check(bool(state_facts.PublishPendingChanges()) && bool(state_lifecycle.ProcessEntityChanges()) &&
        bool(state_delivery.ProcessPending()) && bool(state_time.SynchronizeConditionSchedules()) && bool(state_time.SynchronizeDeferredEffects()), "initialize state cursors");
    world_facts.ResetCursorsToLatest();
    // Rebind adapter cursors to the shared bootstrap owners before capturing them.
    auto social_checkpoint = social_legal.CaptureCheckpoint(); social_checkpoint.crime_cursor = crime.LatestChangeCursor();
    participant_tests::Check(bool(social_legal.RestoreCheckpoint(std::move(social_checkpoint))), "initialize social cursor");
    savegame::SaveRestoreWorkspace workspace;
    std::vector<std::unique_ptr<savegame::ISaveParticipant>> owned;
    owned.push_back(savegame::CreateGameplayFactsServiceSaveParticipant(facts, workspace));
    owned.push_back(savegame::CreateGameplayTimeServiceSaveParticipant(time, workspace));
    owned.push_back(savegame::CreateAbilityServiceSaveParticipant(abilities, workspace));
    owned.push_back(savegame::CreateAIServiceSaveParticipant(ai, workspace));
    owned.push_back(savegame::CreateCombatServiceSaveParticipant(combat, workspace));
    owned.push_back(savegame::CreateConditionServiceSaveParticipant(conditions, workspace));
    owned.push_back(savegame::CreateConstructionServiceSaveParticipant(construction, workspace));
    owned.push_back(savegame::CreateCrimeServiceSaveParticipant(crime, workspace));
    owned.push_back(savegame::CreateDialogueServiceSaveParticipant(dialogue, workspace));
    owned.push_back(savegame::CreateEconomyServiceSaveParticipant(economy, workspace));
    owned.push_back(savegame::CreateEffectServiceSaveParticipant(effects, workspace));
    owned.push_back(savegame::CreateEncountersServiceSaveParticipant(encounters, workspace));
    owned.push_back(savegame::CreateEntityServiceSaveParticipant(entities, workspace));
    owned.push_back(savegame::CreateEnvironmentServiceSaveParticipant(environment, workspace));
    owned.push_back(savegame::CreateEquipmentServiceSaveParticipant(equipment, workspace));
    owned.push_back(savegame::CreateInteractionServiceSaveParticipant(interaction, workspace));
    owned.push_back(savegame::CreateItemsInventoryServiceSaveParticipant(items, workspace));
    owned.push_back(savegame::CreateKnowledgeServiceSaveParticipant(knowledge, workspace));
    owned.push_back(savegame::CreateLootServiceSaveParticipant(loot, workspace));
    owned.push_back(savegame::CreateMaterialServiceSaveParticipant(materials, workspace));
    owned.push_back(savegame::CreateNarrativeServiceSaveParticipant(narrative, workspace));
    owned.push_back(savegame::CreateNavigationSemanticsServiceSaveParticipant(navigation_semantics, workspace));
    owned.push_back(savegame::CreateNeedsLifeServiceSaveParticipant(needs_life, workspace));
    owned.push_back(savegame::CreateOwnershipServiceSaveParticipant(ownership, workspace));
    owned.push_back(savegame::CreatePerceptionServiceSaveParticipant(perception, workspace));
    owned.push_back(savegame::CreatePopulationServiceSaveParticipant(population, workspace));
    owned.push_back(savegame::CreateProcessesServiceSaveParticipant(processes, workspace));
    owned.push_back(savegame::CreateProgressionServiceSaveParticipant(progression, workspace));
    owned.push_back(savegame::CreateResourcesProductionServiceSaveParticipant(resources, workspace));
    owned.push_back(savegame::CreateRolesJobsServiceSaveParticipant(roles_jobs, workspace));
    owned.push_back(savegame::CreateSimulationServiceSaveParticipant(simulation, workspace));
    owned.push_back(savegame::CreateSocietyServiceSaveParticipant(society, workspace));
    owned.push_back(savegame::CreateTraversalServiceSaveParticipant(traversal, workspace));
    owned.push_back(savegame::CreateWorldServiceSaveParticipant(world, workspace));
    owned.push_back(std::make_unique<integration::ScheduledTriggerDispatcherSaveParticipant>(scheduled));
    owned.push_back(std::make_unique<narrative_integration::NarrativeExternalConsequenceSaveParticipant>(narrative_outbox));
    owned.push_back(savegame::CreateTradeCoordinatorSaveParticipant(trade, workspace));
    owned.push_back(savegame::CreateAbilityEffectsDispatcherSaveParticipant(ability_effects, workspace));
    owned.push_back(savegame::CreateAbilityTimeAdapterSaveParticipant(ability_time, workspace));
    owned.push_back(savegame::CreateConditionProgressionAdapterSaveParticipant(condition_progression, workspace));
    owned.push_back(savegame::CreateDeathRewardAdapterSaveParticipant(death_reward, workspace));
    owned.push_back(savegame::CreateInteractionEffectExecutorSaveParticipant(interaction_effects, workspace));
    owned.push_back(savegame::CreatePopulationEncounterAdapterSaveParticipant(population_encounter, workspace));
    owned.push_back(savegame::CreatePopulationLifecycleAdapterSaveParticipant(population_lifecycle, workspace));
    owned.push_back(savegame::CreateCrimeSocietyAdapterSaveParticipant(social_legal, workspace));
    owned.push_back(savegame::CreateStateIntegrationPersistenceSaveParticipant(state, workspace));
    owned.push_back(savegame::CreateWorldFactsAdapterSaveParticipant(world_facts, workspace));
    std::vector<savegame::ISaveParticipant*> participants;
    for (auto& participant : owned) participants.push_back(participant.get());
    const std::vector<savegame::SaveParticipantManifest> expected{
        {savegame::SaveParticipantId::FromString("framework.facts"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.time"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.abilities"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.ai"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.combat"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.conditions"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.construction"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.crime"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.dialogue"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.economy"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.effects"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.encounters"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.entities"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.environment"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.equipment"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.interaction"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.items"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.knowledge"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.loot"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.materials"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.narrative"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.navigation_semantics"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.needs_life"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.ownership"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.perception"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.population"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.processes"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.progression"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.resources"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.roles_jobs"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.simulation"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.society"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.traversal"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("framework.world"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("integration.scheduled_trigger_dispatcher"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("integration.narrative.external_consequence_delivery"), 1, 0, savegame::SaveParticipantRequirement::Required, {}},
        {savegame::SaveParticipantId::FromString("integration.trade"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.economy"), savegame::SaveParticipantId::FromString("framework.items"), savegame::SaveParticipantId::FromString("framework.ownership")}},
        {savegame::SaveParticipantId::FromString("integration.ability_effects"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.abilities"), savegame::SaveParticipantId::FromString("framework.effects")}},
        {savegame::SaveParticipantId::FromString("integration.ability_time"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.abilities"), savegame::SaveParticipantId::FromString("framework.time")}},
        {savegame::SaveParticipantId::FromString("integration.condition_progression"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.conditions"), savegame::SaveParticipantId::FromString("framework.progression")}},
        {savegame::SaveParticipantId::FromString("integration.death_reward"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.combat"), savegame::SaveParticipantId::FromString("framework.loot")}},
        {savegame::SaveParticipantId::FromString("integration.interaction_effects"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.interaction"), savegame::SaveParticipantId::FromString("framework.effects")}},
        {savegame::SaveParticipantId::FromString("integration.population_encounter"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.population"), savegame::SaveParticipantId::FromString("framework.encounters")}},
        {savegame::SaveParticipantId::FromString("integration.population_lifecycle"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.population"), savegame::SaveParticipantId::FromString("framework.roles_jobs"), savegame::SaveParticipantId::FromString("framework.needs_life")}},
        {savegame::SaveParticipantId::FromString("integration.social_legal"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.crime"), savegame::SaveParticipantId::FromString("framework.society")}},
        {savegame::SaveParticipantId::FromString("integration.state"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.entities"), savegame::SaveParticipantId::FromString("framework.materials"), savegame::SaveParticipantId::FromString("framework.conditions"), savegame::SaveParticipantId::FromString("framework.effects"), savegame::SaveParticipantId::FromString("framework.facts"), savegame::SaveParticipantId::FromString("framework.time")}},
        {savegame::SaveParticipantId::FromString("integration.world_facts"), 1, 0, savegame::SaveParticipantRequirement::Required, {savegame::SaveParticipantId::FromString("framework.world"), savegame::SaveParticipantId::FromString("framework.environment"), savegame::SaveParticipantId::FromString("framework.interaction"), savegame::SaveParticipantId::FromString("framework.facts")}},
    };
    const std::vector<std::string> fixture_names{
        "framework.facts",
        "framework.time",
        "framework.abilities",
        "framework.ai",
        "framework.combat",
        "framework.conditions",
        "framework.construction",
        "framework.crime",
        "framework.dialogue",
        "framework.economy",
        "framework.effects",
        "framework.encounters",
        "framework.entities",
        "framework.environment",
        "framework.equipment",
        "framework.interaction",
        "framework.items",
        "framework.knowledge",
        "framework.loot",
        "framework.materials",
        "framework.narrative",
        "framework.navigation_semantics",
        "framework.needs_life",
        "framework.ownership",
        "framework.perception",
        "framework.population",
        "framework.processes",
        "framework.progression",
        "framework.resources",
        "framework.roles_jobs",
        "framework.simulation",
        "framework.society",
        "framework.traversal",
        "framework.world",
        "integration.scheduled_trigger_dispatcher",
        "integration.narrative.external_consequence_delivery",
        "integration.trade",
        "integration.ability_effects",
        "integration.ability_time",
        "integration.condition_progression",
        "integration.death_reward",
        "integration.interaction_effects",
        "integration.population_encounter",
        "integration.population_lifecycle",
        "integration.social_legal",
        "integration.state",
        "integration.world_facts",
    };
    savegame::SaveGameOrchestrator orchestrator;
    auto missing = participants; missing.pop_back();
    participant_tests::Check(!orchestrator.RegisterComposition(missing, {}, expected), "real composition detects missing participant");
    participant_tests::Check(bool(orchestrator.RegisterComposition(participants, {}, expected)), "all reviewed real participants register and freeze");
    const auto order = orchestrator.ResolveOrder();
    participant_tests::Check(orchestrator.RegistryFrozen() && bool(order) && order.Value().size() == expected.size(), "exact participant set closes registry");
    for (std::size_t i = 0; i < participants.size(); ++i) {
        auto& participant = *participants[i];
        participant_tests::Check(!orchestrator.RegisterParticipant(participant), "late registration rejected for every real participant");
        auto snapshot = participant.CaptureSnapshot({});
        participant_tests::Check(bool(snapshot) && bool(participant.ValidateSnapshot(snapshot.Value(), {})), "every registered real participant captures a valid bootstrap checkpoint");
        const auto path = (recording.empty() ? std::filesystem::path(EPIDEMIC_SAVE_FIXTURES) : recording) / "participant" / fixture_names[i] / "v1.bin";
        if (!recording.empty()) {
            participant_tests::Check(!std::filesystem::exists(path), "never overwrite historical participant fixture");
            std::filesystem::create_directories(path.parent_path());
            std::ofstream file(path, std::ios::binary); const auto& bytes = snapshot.Value().payload;
            file.write(reinterpret_cast<const char*>(bytes.data()), static_cast<std::streamsize>(bytes.size()));
            participant_tests::Check(bool(file), "record explicit participant fixture");
        } else {
            std::ifstream file(path, std::ios::binary); participant_tests::Check(bool(file), "historical participant fixture exists");
            std::vector<char> chars((std::istreambuf_iterator<char>(file)), {}); auto historical = snapshot.Value(); historical.payload.clear();
            for (char c : chars) historical.payload.push_back(std::byte(static_cast<unsigned char>(c)));
            historical.payload_hash = savegame::SaveGameOrchestrator::HashBytes(historical.payload);
            participant_tests::Check(bool(participant.ValidateSnapshot(historical, {})), "all real participants validate historical schema v1 fixture");
            participant_tests::Check(historical.payload == snapshot.Value().payload, "schema-v1 bootstrap bytes remain stable");
        }
    }
}
int main(int argc, char** argv) {
    if (argc == 3 && std::string_view(argv[1]) == "--record-participant-fixtures") TestFrameworkSaveComposition(argv[2]);
    else TestFrameworkSaveComposition();
    return 0;
}

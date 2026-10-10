#pragma once
#include "goal5_replay.h"
#include "Epidemic/GameFramework/ProcessResourceSimulationIntegration/process_resource_simulation_adapters.h"

namespace goal5_test_utility
{
inline fw::GameplayObjectRef ScenarioPlayer()
{
    return {fw::combat::CombatService::Domain(), fw::GameplayObjectId::FromString("ms5.player")};
}
inline fw::GameplayObjectRef ScenarioEnemy()
{
    return {fw::combat::CombatService::Domain(), fw::GameplayObjectId::FromString("whole.reward.enemy")};
}
inline fw::GameplayTimePoint ScenarioNow(WholeComposition& engine)
{
    const auto clock = engine.framework.time.GetClock(engine.framework.clock); TRUTH_REQUIRE(clock);
    return clock->now;
}
struct RequiredScenarioServices
{
    std::unique_ptr<ig::CombatAbilityResourceProvider> ability_resources;
    std::unique_ptr<ig::ResourceProcessInputProvider> process_inputs;
    std::unique_ptr<ig::ResourceProcessOutputHandler> process_outputs;
    fw::resources::ResourceStockpileId stockpile{};
    void Bootstrap(FrameworkCompositionView& view, std::uint64_t seed)
    {
        fw::progression::AttributeDefinition attack; attack.canonical_name = "ms5.attack";
        attack.min_micro = 0; attack.max_micro = 100'000'000;
        const auto attribute = view.progression.RegisterAttribute(attack); TRUTH_REQUIRE(attribute);
        fw::conditions::ConditionDefinition condition; condition.canonical_name = "ms5.condition";
        condition.persistence = fw::conditions::ConditionPersistencePolicy::Persistent;
        condition.stacking = fw::conditions::ConditionStackingPolicy::ReplaceExisting;
        const auto condition_id = view.conditions.RegisterCondition(condition); TRUTH_REQUIRE(condition_id);
        TRUTH_REQUIRE(view.condition_progression.AddMapping({condition_id.Value(), attribute.Value(),
            fw::progression::ModifierOperation::FinalAdd, fw::TypeId::FromString("ms5.condition.modifier"), 0, 1'000'000}));

        fw::combat::CombatResourceDefinition health; health.canonical_name = "ms5.health";
        health.default_maximum_micro = 100'000'000; health.depletion_effect = fw::combat::CombatResourceDepletionEffect::Dead;
        fw::combat::CombatResourceDefinition energy; energy.canonical_name = "ms5.energy"; energy.default_maximum_micro = 50'000'000;
        const auto health_id = view.combat.RegisterResource(health); TRUTH_REQUIRE(health_id);
        const auto energy_id = view.combat.RegisterResource(energy); TRUTH_REQUIRE(energy_id);
        const auto damage = view.combat.RegisterDamageType("ms5.damage"); TRUTH_REQUIRE(damage);
        fw::combat::DamageProfile profile; profile.canonical_name = "ms5.damage.profile";
        profile.target_resource = health_id.Value(); profile.can_miss = profile.can_evade = profile.can_block = profile.can_critical = false;
        const auto profile_id = view.combat.RegisterDamageProfile(profile); TRUTH_REQUIRE(profile_id);
        auto damage_handler = std::make_shared<ig::CombatDamageEffectHandler>(view.combat);
        TRUTH_REQUIRE(view.effects.RegisterHandler("framework.combat.damage", damage_handler,
            ig::CombatDamageEffectHandler::PayloadType(), sizeof(ig::CombatDamageEffectPayload)));
        fw::savegame::detail::SnapshotWriter payload;
        auto damage_value = damage.Value(); auto profile_value = profile_id.Value();
        fw::savegame::detail::TransferSnapshot(payload, damage_value);
        fw::savegame::detail::TransferSnapshot(payload, profile_value);
        fw::savegame::detail::TransferSnapshot(payload, seed);
        TRUTH_REQUIRE(payload.bytes.size() == sizeof(ig::CombatDamageEffectPayload));
        fw::effects::EffectDefinition effect; effect.canonical_name = "ms5.damage.effect";
        fw::effects::EffectStepDefinition step; step.type = ig::CombatDamageEffectHandler::StaticType();
        step.magnitude_micro = 20'000'000; step.payload = {ig::CombatDamageEffectHandler::PayloadType(), std::move(payload.bytes)};
        effect.steps.push_back(std::move(step));
        const auto effect_id = view.effects.RegisterDefinition(std::move(effect)); TRUTH_REQUIRE(effect_id);
        const auto mana = fw::abilities::AbilityResourceTypeId::FromString("ms5.mana");
        ability_resources = std::make_unique<ig::CombatAbilityResourceProvider>(view.combat);
        TRUTH_REQUIRE(ability_resources->AddMapping({mana, energy_id.Value()}));
        TRUTH_REQUIRE(ability_resources->FreezeMappings()); view.abilities.SetResourceProvider(ability_resources.get());
        const auto action = fw::ActionTypeId::FromString("ms5.attack.effect");
        fw::abilities::AbilityDefinition ability; ability.canonical_name = "ms5.ability";
        ability.targeting = fw::abilities::AbilityTargetPolicy::SingleTarget;
        ability.timing.kind = fw::abilities::AbilityTimingKind::CastTime; ability.timing.cast_duration = {5};
        ability.costs.push_back({mana, 10'000'000, fw::abilities::AbilityCostPolicy::ReserveThenCommit});
        ability.outputs.push_back({action, 1'000'000, {}});
        TRUTH_REQUIRE(view.abilities.RegisterDefinition(ability));
        TRUTH_REQUIRE(view.ability_effects.Map(action, effect_id.Value()));

        fw::perception::SenseDefinition hearing; hearing.id = fw::perception::SenseTypeId::FromString("ms5.hearing");
        hearing.canonical_name = "ms5.hearing"; hearing.evaluation_model = fw::perception::SenseEvaluationModel::Hearing;
        hearing.base_range_mm = 20'000; TRUTH_REQUIRE(view.perception.RegisterSense(hearing));
        fw::perception::PerceiverProfileDefinition perceiver; perceiver.id = fw::perception::PerceiverProfileId::FromString("ms5.perceiver");
        perceiver.canonical_name = "ms5.perceiver"; perceiver.senses = {hearing.id};
        TRUTH_REQUIRE(view.perception.RegisterProfileDefinition(perceiver));
        view.perception.SetTemporalPolicy({fw::GameplayDuration{5}, fw::GameplayDuration{1}, 100'000});
        fw::ai::AIGoalDefinition goal; goal.id = fw::ai::AIGoalId::FromString("ms5.known.target");
        goal.canonical_name = "ms5.known.target"; goal.intent_type = fw::ai::AIIntentTypeId::FromString("ms5.intent");
        goal.target_policy = fw::ai::AITargetPolicy::Required; goal.base_priority_micro = 5000;
        fw::ai::AIConsideration known; known.id = fw::ai::AIConsiderationId::FromString("ms5.known");
        known.evaluator = fw::ai::AIService::InputEvaluatorId();
        ig::PerceptionKnowledgeMapping mapping;
        known.input_key = fw::ai::AIInputKeyId::FromType(fw::TypeId{mapping.observed_topic.value.Raw()});
        known.scope = fw::ai::AIConsiderationScope::Target; goal.considerations = {known}; TRUTH_REQUIRE(view.ai.RegisterGoal(goal));
        fw::ai::AIGoalDefinition idle; idle.id = fw::ai::AIGoalId::FromString("ms5.idle");
        idle.canonical_name = "ms5.idle"; idle.intent_type = fw::ai::AIIntentTypeId::FromString("ms5.idle.intent");
        idle.target_policy = fw::ai::AITargetPolicy::Targetless; idle.base_priority_micro = 1; TRUTH_REQUIRE(view.ai.RegisterGoal(idle));
        fw::ai::AIProfile ai; ai.id = fw::ai::AIProfileId::FromString("ms5.ai"); ai.canonical_name = "ms5.ai";
        ai.default_goals = {goal.id, idle.id}; ai.think_interval = {1}; TRUTH_REQUIRE(view.ai.RegisterProfile(ai));

        fw::resources::ResourceType iron; iron.canonical_name = "ms5.iron";
        fw::resources::ResourceType product; product.canonical_name = "ms5.product";
        const auto iron_id = view.resources.RegisterResourceType(iron); TRUTH_REQUIRE(iron_id);
        const auto product_id = view.resources.RegisterResourceType(product); TRUTH_REQUIRE(product_id);
        const auto stock = view.resources.CreateStockpile({{}, ScenarioPlayer(), {}, {}, fw::resources::StockpileState::Active, {}});
        TRUTH_REQUIRE(stock); stockpile = stock.Value(); TRUTH_REQUIRE(view.resources.Add(stockpile, {iron_id.Value(), 10}));
        process_inputs = std::make_unique<ig::ResourceProcessInputProvider>(view.resources);
        process_outputs = std::make_unique<ig::ResourceProcessOutputHandler>(view.resources);
        view.processes.SetInputProvider(process_inputs.get()); view.processes.AddOutputHandler(process_outputs.get());
        fw::processes::ProcessDefinition process; process.canonical_name = "ms5.process";
        process.kind = fw::processes::ProcessKindId::FromString("ms5.process.kind"); process.timing = fw::processes::ProcessTimingPolicy::Timed;
        process.steps.push_back({fw::processes::ProcessStepId::FromString("ms5.process.step"), fw::TypeId::FromString("ms5.process.step"), {5}, {}, {}});
        const auto process_id = view.processes.RegisterDefinition(process); TRUTH_REQUIRE(process_id);
        fw::processes::ProcessRecipe recipe; recipe.canonical_name = "ms5.recipe"; recipe.process = process_id.Value();
        recipe.inputs.push_back({fw::processes::ProcessInputId::FromString("ms5.input"), fw::processes::ProcessInputTypeId::FromString("framework.input.resource"),
            2, fw::processes::InputConsumptionPolicy::ReserveThenConsume, {}, ig::EncodeProcessResourcePayload({stockpile, iron_id.Value()})});
        recipe.outputs.push_back({fw::processes::ProcessOutputId::FromString("ms5.output"), ig::ResourceProcessOutputHandler::OutputType(),
            1, fw::processes::OutputDeliveryPolicy::OnCompletion, ig::EncodeProcessResourcePayload({stockpile, product_id.Value()})});
        TRUTH_REQUIRE(view.processes.RegisterRecipe(recipe));
    }
};
template <typename Use>
decltype(auto) WithRequiredScenarioComposition(const ScenarioDescriptor& descriptor, const std::filesystem::path& path, Use&& use)
{
    RequiredScenarioServices services;
    return WithWholeComposition(path, [&](FrameworkCompositionView& view) { services.Bootstrap(view, descriptor.seed_set.front()); },
        [&](WholeComposition& engine) -> decltype(auto) { return use(engine, services); });
}
}

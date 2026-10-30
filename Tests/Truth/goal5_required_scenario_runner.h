#pragma once
#include "goal5_required_scenarios.h"

namespace goal5_test_utility {
inline ScenarioDescriptor RequiredScenario(char name, std::uint64_t seed = 20261005) {
    auto descriptor = MiniScenario(seed);
    descriptor.scenario_id = std::string("scenario.") + name;
    descriptor.config_id = "whole.required.v1";
    descriptor.initial_state_id = "whole.required.empty.v1";
    descriptor.steps.clear();
    const auto add = [&](ScenarioStepKind kind, std::string operation, std::int64_t value = 0) {
        descriptor.steps.push_back(
            {kind, descriptor.steps.size() + 1, 0, 0, value, std::move(operation)});
    };

    add(ScenarioStepKind::SubmitCommand, "whole.seed");
    if (name == 'D' || name == 'E' || name == 'F' || name == 'G')
        add(ScenarioStepKind::SubmitCommand, std::string(1, name) + ".begin");
    if (name == 'C')
        add(ScenarioStepKind::StreamOut, "stream.out");
    add(ScenarioStepKind::AdvanceGameTime, "time.advance", 200'000);
    if (name == 'C')
        add(ScenarioStepKind::StreamIn, "stream.in");
    add(ScenarioStepKind::SaveReload, "checkpoint.1");
    if (name == 'E' || name == 'F' || name == 'G')
        add(ScenarioStepKind::SubmitCommand, std::string(1, name) + ".finish");
    add(ScenarioStepKind::CompleteExternalOperation, "scheduled.progression", 1);
    if (name == 'D') {
        add(ScenarioStepKind::ApplyBudget, "work.small", 1);
        add(ScenarioStepKind::ApplyBudget, "work.small", 1);
        add(ScenarioStepKind::ApplyBudget, "work.complete", 1);
    } else
        add(ScenarioStepKind::Tick, "scheduled.dispatch", 16);
    add(ScenarioStepKind::Reconcile, "integration.reconcile");
    add(ScenarioStepKind::Materialize, "runtime.materialize");
    return descriptor;
}

class RequiredScenarioPort {
  public:
    RequiredScenarioPort(WholeComposition &engine, RequiredScenarioServices &services,
                         bool checkpoint)
        : engine_(engine), services_(services), whole_(engine, checkpoint) {
    }

    std::string Digest() {
        return whole_.Digest();
    }

    void Apply(const ScenarioStep &step, SemanticTrace &trace) {
        if (step.kind != ScenarioStepKind::SubmitCommand || step.operation_id == "whole.seed") {
            const auto pending = engine_.framework.scheduled.CaptureSnapshot().pending.size();
            whole_.Apply(step, trace);
            if (step.kind == ScenarioStepKind::ApplyBudget && step.operation_id == "work.small") {
                TRUTH_REQUIRE(pending > 1);
                TRUTH_REQUIRE(engine_.framework.scheduled.CaptureSnapshot().pending.size() ==
                              pending - 1);
            }

            if (step.operation_id == "work.complete")
                TRUTH_REQUIRE(engine_.framework.scheduled.CaptureSnapshot().pending.empty());
            return;
        }

        const auto before = Digest();
        if (step.operation_id == "D.begin")
            BudgetBegin();
        else if (step.operation_id == "E.begin")
            CombatBegin();
        else if (step.operation_id == "E.finish")
            CombatFinish();
        else if (step.operation_id == "F.begin")
            ObservationBegin();
        else if (step.operation_id == "F.finish")
            ObservationFinish();
        else if (step.operation_id == "G.begin")
            ProductionBegin();
        else if (step.operation_id == "G.finish")
            ProductionFinish();
        else
            throw std::runtime_error("unknown required scenario input");
        const auto after = Digest();
        TRUTH_REQUIRE(before != after);
        const auto player = ScenarioPlayer().id;
        trace.Commit({step.operation_id, player.High(), player.Low(), 0, 0, before + "->" + after,
                      step.logical_order * 1024 + 1});
    }

  private:
    void BudgetBegin() {
        auto &f = engine_.framework;
        const auto entities = f.entities.CaptureSnapshot().records;
        TRUTH_REQUIRE(entities.size() == 3);
        for (std::size_t index = 1; index != 3; ++index)
            TRUTH_REQUIRE(f.time.Schedule(f.clock, ScenarioNow(engine_),
                                          fw::entities::EntityService::ToGameplayObjectRef(
                                              entities[index].id),
                                          f.action));
        TRUTH_REQUIRE(f.scheduled.CollectDue(f.clock, {}));
        TRUTH_REQUIRE(f.scheduled.CaptureSnapshot().pending.size() == 3);
    }

    void CombatBegin() {
        auto &f = engine_.framework;
        const auto health = fw::combat::CombatResourceTypeId::FromString("ms5.health");
        const auto energy = fw::combat::CombatResourceTypeId::FromString("ms5.energy");
        TRUTH_REQUIRE(
            f.combat.RegisterCombatant(ScenarioPlayer(), {{health, 100'000'000, 100'000'000},
                                                          {energy, 50'000'000, 50'000'000}}));
        TRUTH_REQUIRE(
            f.combat.RegisterCombatant(ScenarioEnemy(), {{health, 20'000'000, 100'000'000}}));
        TRUTH_REQUIRE(f.progression.EnsureProfile(ScenarioPlayer()));
        TRUTH_REQUIRE(f.progression.SetBaseAttribute(ScenarioPlayer(),
                                                     fw::progression::AttributeTypeId::FromString(
                                                         "ms5.attack"),
                                                     5'000'000));
        const auto grant =
            f.abilities.Grant(ScenarioPlayer(),
                              fw::abilities::AbilityDefinitionId::FromString("ms5.ability"));
        TRUTH_REQUIRE(grant);
        fw::abilities::AbilityTargetSet targets;
        targets.primary = ScenarioEnemy();
        fw::GameplayContext context;
        context.actor = context.instigator = context.source = ScenarioPlayer();
        context.operation = fw::OperationId::FromString("ms5.cast");
        context.correlation = fw::CorrelationId::FromString("ms5.combat.chain");
        TRUTH_REQUIRE(
            f.abilities.BeginActivation({grant.Value(), targets, ScenarioNow(engine_), context}));
        const auto resource = f.combat.GetResource(ScenarioPlayer(), energy);
        TRUTH_REQUIRE(resource && resource.Value().current_micro == 40'000'000);
        TRUTH_REQUIRE(f.combat.CaptureSnapshot().resource_reservations.size() == 1);
        TRUTH_REQUIRE(f.abilities.CaptureSnapshot().executions.front().reservations.size() == 1);
    }

    void CombatFinish() {
        auto &f = engine_.framework;
        const auto executions = f.abilities.CaptureSnapshot().executions;
        TRUTH_REQUIRE(executions.size() == 1);
        const auto output =
            f.abilities.CompleteExecution(executions.front().id, ScenarioNow(engine_));
        if (!output)
            throw std::runtime_error("E.complete: " + output.GetError().code + ": " +
                                     output.GetError().message);
        TRUTH_REQUIRE(output && output.Value().size() == 1);
        const auto delivered = f.ability_effects.Dispatch(output.Value());
        TRUTH_REQUIRE(delivered && delivered.Value().size() == 1);
        const auto *enemy = f.combat.FindCombatant(ScenarioEnemy());
        TRUTH_REQUIRE(enemy && enemy->life_state == fw::combat::CombatLifeState::Dead);
        const auto effect_count = f.effects.GetDiagnostics().executions;
        const auto duplicate = f.ability_effects.Dispatch(output.Value());
        TRUTH_REQUIRE(duplicate &&
                      duplicate.Value().front().execution == delivered.Value().front().execution);
        TRUTH_REQUIRE(f.effects.GetDiagnostics().executions == effect_count);
        fw::conditions::ApplyConditionRequest condition;
        condition.type = fw::conditions::ConditionTypeId::FromString("ms5.condition");
        condition.subject = ScenarioPlayer();
        condition.magnitude_micro = 2'000'000;
        TRUTH_REQUIRE(f.conditions.Apply(condition));
        TRUTH_REQUIRE(f.condition_progression.ProcessChanges());
        const auto attack =
            f.progression.GetAttribute(ScenarioPlayer(),
                                       fw::progression::AttributeTypeId::FromString("ms5.attack"));
        TRUTH_REQUIRE(attack && attack.Value() == 7'000'000);
        const auto rewards = f.death_reward.ProcessChanges();
        TRUTH_REQUIRE(rewards && rewards.Value().size() == 1);
        TRUTH_REQUIRE(f.loot.FindGenerated(rewards.Value().front()) ||
                      f.loot.FindPending(rewards.Value().front()));
        const auto settled = Digest();
        const auto duplicate_rewards = f.death_reward.ProcessChanges();
        TRUTH_REQUIRE(duplicate_rewards && duplicate_rewards.Value().empty());
        TRUTH_REQUIRE(f.condition_progression.ProcessChanges());
        TRUTH_REQUIRE(Digest() == settled);
    }

    void CommitIntent(const fw::ai::AIContextSnapshot &context, fw::ai::AIIntentTypeId type) {
        auto &ai = engine_.framework.ai;
        const auto think = ai.Think(ScenarioPlayer(), context, {});
        TRUTH_REQUIRE(think && think.Value().intent && think.Value().intent->type == type);
        ig::AIIntentExecutionRecorder recorder{1};

        TRUTH_REQUIRE(recorder.Accept(*think.Value().intent, ai));
        TRUTH_REQUIRE(recorder.Succeed(think.Value().intent->id, ai));
    }

    void ObservationBegin() {
        auto &f = engine_.framework;
        TRUTH_REQUIRE(f.perception.RegisterPerceiver(ScenarioPlayer(),
                                                     fw::perception::PerceiverProfileId::FromString(
                                                         "ms5.perceiver")));
        fw::knowledge::KnowledgeProfile profile;
        profile.subject = ScenarioPlayer();
        TRUTH_REQUIRE(f.knowledge.CreateProfile(profile));
        TRUTH_REQUIRE(
            f.ai.RegisterAgent(ScenarioPlayer(), fw::ai::AIProfileId::FromString("ms5.ai")));
        fw::perception::PerceptionStimulus stimulus;
        stimulus.sense = fw::perception::SenseTypeId::FromString("ms5.hearing");
        stimulus.source = ScenarioEnemy();
        stimulus.position = {1000, 0, 0};

        stimulus.strength_micro = 1'000'000;
        const auto id = f.perception.CreateStimulus(stimulus);
        TRUTH_REQUIRE(id);
        fw::perception::PerceiverEvaluationSample sample;
        sample.subject = ScenarioPlayer();
        sample.materialized = sample.runtime_projection_available = true;
        const auto observed =
            f.perception
                .ProcessStimulus(id.Value(),
                                 fw::perception::PerceptionProcessingContext{fw::GameplayTickId{1},
                                                                             ScenarioNow(engine_),
                                                                             {},
                                                                             {sample}});
        TRUTH_REQUIRE(observed && observed.Value().size() == 1);
        ig::PerceptionKnowledgeAdapter adapter{f.knowledge, {}};

        const auto learned = adapter.LearnFromObservation(observed.Value().front());
        TRUTH_REQUIRE(learned && learned.Value().knowledge.IsValid());
        ig::KnowledgeAIAdapter inputs{f.knowledge, {}};

        CommitIntent(inputs.BuildContext(ScenarioPlayer(), {true, true}, &f.perception,
                                         ScenarioNow(engine_)),
                     fw::ai::AIIntentTypeId::FromString("ms5.intent"));
    }

    void ObservationFinish() {
        auto &f = engine_.framework;
        const auto knowledge = f.knowledge.CaptureSnapshot().knowledge;
        TRUTH_REQUIRE(knowledge.size() == 1 && f.knowledge.Forget(knowledge.front().id));
        TRUTH_REQUIRE(f.perception.AdvanceTime(ScenarioNow(engine_)));
        TRUTH_REQUIRE(f.perception.FindPerceiver(ScenarioPlayer()));
        TRUTH_REQUIRE(
            f.ai.RequestReplan(ScenarioPlayer(),
                               fw::ai::AIReplanReasonId::FromString("ms5.target.disappeared"),
                               ScenarioNow(engine_)));
        ig::KnowledgeAIAdapter inputs{f.knowledge, {}};

        const auto context = inputs.BuildContext(ScenarioPlayer(), {true, true}, &f.perception,
                                                 ScenarioNow(engine_));
        TRUTH_REQUIRE(context.targets.empty());
        CommitIntent(context, fw::ai::AIIntentTypeId::FromString("ms5.idle.intent"));
    }

    void ProductionBegin() {
        fw::processes::StartProcessRequest request;
        request.recipe = fw::processes::ProcessRecipeId::FromString("ms5.recipe");
        request.actor = ScenarioPlayer();
        request.now = ScenarioNow(engine_);
        request.seed = 20261005;
        TRUTH_REQUIRE(engine_.framework.processes.StartProcess(request));
        TRUTH_REQUIRE(
            engine_.framework.resources.GetAmount(services_.stockpile,
                                                  fw::resources::ResourceTypeId::FromString(
                                                      "ms5.iron")) == 10);
    }

    void ProductionFinish() {
        auto &f = engine_.framework;
        const auto processes = f.processes.FindProcessesByActor(ScenarioPlayer());
        TRUTH_REQUIRE(processes.size() == 1);
        const auto complete = f.processes.Complete(processes.front().id, ScenarioNow(engine_));
        TRUTH_REQUIRE(complete);
        TRUTH_REQUIRE(
            f.resources.GetAmount(services_.stockpile,
                                  fw::resources::ResourceTypeId::FromString("ms5.iron")) == 8);
        TRUTH_REQUIRE(
            f.resources.GetAmount(services_.stockpile,
                                  fw::resources::ResourceTypeId::FromString("ms5.product")) == 1);
        const auto settled = Digest();
        (void)f.processes.Complete(processes.front().id, ScenarioNow(engine_));
        TRUTH_REQUIRE(Digest() == settled);
    }

    WholeComposition &engine_;
    RequiredScenarioServices &services_;
    WholeScenarioPort whole_;
};

inline ScenarioResult ReplayRequiredScenario(const ScenarioDescriptor &descriptor, bool reload,
                                             const std::filesystem::path &storage) {
    SemanticTrace trace;
    ScenarioResult result;
    std::size_t begin = 0, completed = 0;
    bool fresh = false;
    rb::RuntimeBinding old_binding{};

    fw::entities::EntityHandle old_handle{};

    fw::ChangeCursor old_cursor{};

    ValidateScenario(descriptor);
    try {
        while (begin < descriptor.steps.size()) {
            auto end = descriptor.steps.size();
            if (reload) {
                end = begin;
                while (end < descriptor.steps.size() &&
                       descriptor.steps[end].kind != ScenarioStepKind::SaveReload)
                    ++end;
                if (end < descriptor.steps.size())
                    ++end;
            }

            WithRequiredScenarioComposition(
                descriptor, storage,
                [&](WholeComposition &engine, RequiredScenarioServices &services) {
                    if (fresh) {
                        if (descriptor.scenario_id == "scenario.H") {
                            engine.fault_once = "materialize.queued";
                            TRUTH_REQUIRE(!engine.Restore());
                            TRUTH_REQUIRE(engine.restorer.Phase() ==
                                          ig::WholeRestorePhase::ReconciliationRequired);
                            TRUTH_REQUIRE(!engine.restorer.CanResume() && engine.quiesced);
                            TRUTH_REQUIRE(!engine.Advance(std::chrono::microseconds{1}));
                            TRUTH_REQUIRE(engine.restorer.RetryReconciliation());
                        } else
                            TRUTH_REQUIRE(engine.Restore());
                        TRUTH_REQUIRE(engine.restorer.CanResume());
                        TRUTH_REQUIRE(engine.SemanticState().Digest() == result.semantic_digest);
                        if (descriptor.scenario_id == "scenario.H") {
                            TRUTH_REQUIRE(!engine.framework.entities.Resolve(old_handle));
                            TRUTH_REQUIRE(engine.framework.entities.ReadChangesSince(old_cursor)
                                              .snapshot_required);
                            const auto binding =
                                engine.bridge->GetBinding(old_binding.gameplay_object);
                            TRUTH_REQUIRE(binding &&
                                          binding->runtime_object != old_binding.runtime_object);
                            TRUTH_REQUIRE(binding->generation != old_binding.generation);
                            TRUTH_REQUIRE(
                                !engine.backend->ObserveTransform(old_binding.runtime_object));
                            TRUTH_REQUIRE(!engine.bridge->GetBinding(old_binding.runtime_object));
                            const auto before_retry = engine.SemanticState().Digest();
                            engine.MaterializePersistentObjects();
                            engine.MaterializePersistentObjects();
                            TRUTH_REQUIRE(engine.SemanticState().Digest() == before_retry);
                            TRUTH_REQUIRE(engine.bridge->GetDiagnostics().active_bindings == 1);
                        }

                    }

                    RequiredScenarioPort port(engine, services, reload);
                    result = RunScenario(descriptor, port, trace, begin, end);
                    if (reload && end < descriptor.steps.size() &&
                        descriptor.scenario_id == "scenario.H") {
                        const auto objects = rt::CaptureRuntimePersistenceSnapshot(
                            *engine.runtime->services.persistence->query);
                        TRUTH_REQUIRE(objects && objects.Value().objects.size() == 1);
                        const auto entity = DecodeEntityIdentity(objects.Value().objects.front());
                        const auto subject =
                            fw::entities::EntityService::ToGameplayObjectRef(entity);
                        old_handle = engine.framework.entities.HandleOf(entity);
                        old_cursor = engine.framework.entities.LatestChangeCursor();
                        const auto binding = engine.bridge->GetBinding(subject);
                        TRUTH_REQUIRE(binding);
                        old_binding = *binding;
                    }

                });
            if (result.failure)
                return result;
            completed += result.step_count;
            result.step_count = completed;
            begin = end;
            fresh = true;
        }

    } catch (const std::exception &error) {
        result.failure = ScenarioFailure{descriptor.scenario_id,
                                         descriptor.seed_set,
                                         begin,
                                         result.semantic_digest,
                                         "<failed>",
                                         "<required scenario>",
                                         error.what()};

    }

    return result;
}

} // namespace goal5_test_utility

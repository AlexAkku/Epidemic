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
// BEGIN OWNER FREEZE
// END OWNER FREEZE
participant_tests::Check(bool(interaction_effects.Freeze(interaction)), "freeze interaction/effect mapping");
participant_tests::Check(bool(scheduled.Freeze()), "freeze scheduled dispatcher");
death_reward.RestoreCursor(combat.LatestChangeCursor());
participant_tests::Check(bool(state_facts.PublishPendingChanges()) && bool(state_lifecycle.ProcessEntityChanges()) &&
    bool(state_delivery.ProcessPending()) && bool(state_time.SynchronizeConditionSchedules()) && bool(state_time.SynchronizeDeferredEffects()), "initialize state cursors");
world_facts.ResetCursorsToLatest();
// Rebind adapter cursors to the shared bootstrap owners before capturing them.
auto social_checkpoint = social_legal.CaptureCheckpoint(); social_checkpoint.crime_cursor = crime.LatestChangeCursor();
participant_tests::Check(bool(social_legal.RestoreCheckpoint(std::move(social_checkpoint))), "initialize social cursor");

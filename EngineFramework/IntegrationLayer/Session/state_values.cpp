#include <Epidemic/GameFramework/Session/state_values.h>
#include <Epidemic/GameFramework/Entities/entities.h>
#include <Epidemic/GameFramework/Combat/combat.h>
#include <Epidemic/GameFramework/Economy/economy.h>
#include <Epidemic/GameFramework/ItemsInventory/items_inventory.h>
#include <Epidemic/GameFramework/Progression/progression.h>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace epidemic::gameplay::session {
namespace {
template <class T> T Take(foundation::Result<T> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
    return std::move(result).Value();
}
void Check(foundation::Result<void> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
}
std::int64_t Micro(double value) {
    if (!std::isfinite(value) || std::abs(value) > 1e12)
        throw std::invalid_argument("Invalid combat value");
    return static_cast<std::int64_t>(std::llround(value * 1'000'000));
}
struct Actors {
    entities::EntityService entities;
    combat::CombatService combat;
    entities::EntityArchetypeId archetype;
    combat::CombatResourceTypeId health;
    combat::DamageTypeId damage;
    combat::DamageProfileId profile;
    Actors() {
        archetype = Take(entities.RegisterArchetype({.canonical_name = "session.actor"}));
        entities.Freeze();
        health = Take(combat.RegisterResource(
            {.canonical_name = "session.health", .depletion_effect = combat::CombatResourceDepletionEffect::Dead}));
        damage = Take(combat.RegisterDamageType("session.damage"));
        profile = Take(combat.RegisterDamageProfile({.canonical_name = "session.direct_damage",
                                                     .target_resource = health,
                                                     .can_miss = false,
                                                     .can_evade = false,
                                                     .can_block = false,
                                                     .can_critical = false}));
        combat.Freeze();
    }
};
std::shared_ptr<Actors> ActorOwners() {
    // Runtime-thread ownership, as required by the Framework owner contracts.
    static std::weak_ptr<Actors> current;
    auto result = current.lock();
    if (!result) {
        result = std::make_shared<Actors>();
        current = result;
    }
    return result;
}
}
struct HealthValue::State {
    std::shared_ptr<Actors> owners = ActorOwners();
    entities::EntityId entity;
    GameplayObjectRef subject;
    explicit State(double initial) {
        const auto initialMicro = Micro(initial);
        if (initialMicro < 0)
            throw std::invalid_argument("Negative initial health");
        entity = Take(owners->entities.Create({.archetype = owners->archetype})).id;
        subject = entities::EntityService::ToGameplayObjectRef(entity);
        try {
            Check(owners->entities.Activate(entity));
            Check(owners->combat.RegisterCombatant(subject, {{owners->health, initialMicro, initialMicro}}));
        } catch (...) {
            static_cast<void>(owners->entities.RequestDestroy(entity, {}));
            static_cast<void>(owners->entities.CommitPendingDestruction());
            static_cast<void>(owners->entities.Remove(entity));
            throw;
        }
    }
    ~State() {
        // Entities requires destruction to be committed before a slot can be removed.
        // Destructors must also remain safe while a failed command is unwinding.
        try {
            Check(owners->combat.RemoveCombatant(subject));
            Check(owners->entities.RequestDestroy(entity, {}));
            static_cast<void>(Take(owners->entities.CommitPendingDestruction()));
            Check(owners->entities.Remove(entity));
            owners->entities.PruneChangesBefore(owners->entities.LatestChangeCursor().sequence);
        } catch (...) {
        }
    }
};
HealthValue::HealthValue(double initial) : state_(std::make_shared<State>(initial)) {
}
HealthValue::HealthValue(std::shared_ptr<State> state, bool maximum) : state_(std::move(state)), maximum_(maximum) {
}
HealthValue HealthValue::Maximum() const {
    return {state_, true};
}
std::uint64_t HealthValue::EntityId() const {
    return state_->entity.Low();
}
std::uint64_t HealthValue::LiveEntityCount() const {
    return state_->owners->entities.GetDiagnostics().entity_count;
}
HealthValue::operator double() const {
    const auto value = Take(state_->owners->combat.GetResource(state_->subject, state_->owners->health));
    return static_cast<double>(maximum_ ? value.maximum_micro : value.current_micro) / 1'000'000;
}
HealthValue &HealthValue::operator=(const HealthValue &other) {
    return *this = static_cast<double>(other);
}
HealthValue &HealthValue::operator=(double value) {
    auto &owner = state_->owners->combat;
    if (maximum_)
        Check(owner.SetResourceMaximum(state_->subject, state_->owners->health, Micro(value), false));
    else {
        const auto old = Take(owner.GetResource(state_->subject, state_->owners->health)).current_micro;
        Check(owner.ModifyResource(state_->subject, state_->owners->health, Micro(value) - old));
        // Debug/profile resets may revive a combatant; ordinary damage cannot.
        if (value > 0 && owner.FindCombatant(state_->subject)->life_state == combat::CombatLifeState::Dead) {
            const auto resource = Take(owner.GetResource(state_->subject, state_->owners->health));
            Check(owner.RemoveCombatant(state_->subject));
            Check(owner.RegisterCombatant(state_->subject, {resource}));
        }
    }
    return *this;
}
HealthValue &HealthValue::operator+=(double delta) {
    return *this = static_cast<double>(*this) + delta;
}
HealthValue &HealthValue::operator-=(double delta) {
    if (maximum_ || delta <= 0)
        return *this += -delta;
    auto &actors = *state_->owners;
    auto plan = Take(actors.combat.PrepareDamage({.target = state_->subject,
                                                  .damage_type = actors.damage,
                                                  .profile = actors.profile,
                                                  .base_amount_micro = Micro(delta)}));
    static_cast<void>(Take(actors.combat.CommitDamage(plan, {})));
    return *this;
}
HealthValue &HealthValue::operator*=(double scale) {
    return *this = static_cast<double>(*this) * scale;
}
struct CoinValue::State {
    economy::EconomyService owner;
    economy::EconomicAccountId account;
    State() {
        const auto currency = Take(owner.RegisterCurrency({.canonical_name = "session.coin"}));
        owner.Freeze();
        account = Take(owner.CreateAccount(
            {.owner = {economy::EconomyService::Domain(), GameplayObjectId::FromString("session.wallet")},
             .currency = currency}));
    }
};
CoinValue::CoinValue() : state_(std::make_shared<State>()) {
}
CoinValue::operator int() const {
    return static_cast<int>(state_->owner.GetBalance(state_->account));
}
CoinValue &CoinValue::operator=(const CoinValue &other) {
    return *this = static_cast<int>(other);
}
CoinValue &CoinValue::operator=(int value) {
    const auto delta = static_cast<std::int64_t>(value) - static_cast<int>(*this);
    if (delta > 0)
        Check(state_->owner.Credit(state_->account, delta));
    if (delta < 0)
        Check(state_->owner.Debit(state_->account, -delta));
    return *this;
}
CoinValue &CoinValue::operator+=(int delta) {
    return *this = static_cast<int>(*this) + delta;
}
CoinValue &CoinValue::operator-=(int delta) {
    return *this = static_cast<int>(*this) - delta;
}
struct ItemValue::State {
    items::ItemsInventoryService owner;
    items::ContainerId container;
    items::ItemInstanceId item;
    std::vector<items::ItemDefinitionId> definitions;
    State() {
        for (int i = 0; i < 256; ++i) {
            definitions.push_back(
                Take(owner.RegisterDefinition({.canonical_name = "session.item." + std::to_string(i)})));
        }
        owner.Freeze();
        container = Take(owner.CreateContainer({.max_slots = 1}));
    }
};
ItemValue::ItemValue(int definition) : state_(std::make_shared<State>()) {
    *this = definition;
}
ItemValue &ItemValue::operator=(const ItemValue &other) {
    return *this = static_cast<int>(other);
}
ItemValue &ItemValue::operator=(int definition) {
    if (definition < -1 || definition > 255)
        throw std::invalid_argument("Invalid item definition index");
    if (state_->item.IsValid()) {
        Check(state_->owner.DestroyItem(state_->item));
        state_->item = {};
    }
    if (definition >= 0) {
        state_->item = Take(state_->owner.CreateItem(items::ItemInstance{
            .definition = state_->definitions[static_cast<std::size_t>(definition)],
            .location = {.kind = items::ItemLocationKind::Container, .container = state_->container}}));
    }
    return *this;
}
ItemValue::operator int() const {
    const auto *item = state_->owner.FindItem(state_->item);
    if (!item)
        return -1;
    for (std::size_t i = 0; i < state_->definitions.size(); ++i)
        if (state_->definitions[i] == item->definition)
            return static_cast<int>(i);
    throw std::logic_error("Inventory definition lost");
}
struct ProgressValue::State {
    progression::ProgressionService owner;
    progression::ProgressionTrackId track;
    GameplayObjectRef subject{progression::ProgressionService::Domain(),
                              GameplayObjectId::FromString("session.progression.subject")};
    State() {
        track = Take(owner.RegisterTrack({.canonical_name = "session.progression.track",
                                          .rank_thresholds_micro = {0},
                                          .policy = {.allow_decrease = true, .allow_direct_set = true}}));
        Check(owner.Freeze());
        Check(owner.EnsureProfile(subject));
    }
};
ProgressValue::ProgressValue(double initial) : state_(std::make_shared<State>()) {
    *this = initial;
}
ProgressValue::operator double() const {
    return static_cast<double>(Take(state_->owner.GetTrack(state_->subject, state_->track)).progress_micro) / 1'000'000;
}
ProgressValue &ProgressValue::operator=(const ProgressValue &other) {
    return *this = static_cast<double>(other);
}
ProgressValue &ProgressValue::operator=(double value) {
    static_cast<void>(Take(state_->owner.SetProgress(state_->subject, state_->track, Micro(value))));
    return *this;
}
ProgressValue &ProgressValue::operator+=(double delta) {
    if (delta < 0)
        return *this = static_cast<double>(*this) + delta;
    static_cast<void>(Take(state_->owner.GrantProgress(state_->subject, state_->track, Micro(delta))));
    return *this;
}
ProgressValue &ProgressValue::operator-=(double delta) {
    return *this += -delta;
}
}

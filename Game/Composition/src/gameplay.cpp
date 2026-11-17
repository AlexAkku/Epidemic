#include "Epidemic/Game/Composition/gameplay.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>

namespace epidemic::game::composition::arena {
namespace {
bool Nonnegative(const double &v) {
    return std::isfinite(v) && v >= 0 && v <= 10000;
}

bool Finite(const double &v) {
    return std::isfinite(v) && std::abs(v) <= 10000;
}

bool Count(const std::int64_t &v) {
    return v >= 0 && v <= 10000;
}

bool Key(const std::int64_t &v) {
    return v >= 1 && v <= 255;
}

bool Shape(const std::int64_t &v) {
    return v == 0 || v == 1;
}

bool Mask(const std::int64_t &v) {
    return v >= 0 && v <= 65535;
}

using R = foundation::Result<void>;
using V = foundation::Result<std::int64_t>;
constexpr Operation catalogue[] =
    {{"gameplay.health.get.v1", "Get Health", "Gameplay/Health", "unused:i64", "health:i64", false},
     {"gameplay.health.damage.v1", "Apply Damage", "Gameplay/Health", "amount:i64", "applied:i64",
      true},
     {"gameplay.health.heal.v1", "Heal", "Gameplay/Health", "amount:i64", "applied:i64", true},
     {"gameplay.turret.range.v1", "Set Turret Range", "Gameplay/Targeting", "range:centiunits",
      "range:centiunits", true},
     {"gameplay.turret.cooldown.v1", "Set Turret Cooldown", "Gameplay/Targeting", "ticks:i64",
      "ticks:i64", true},
     {"gameplay.turret.damage.v1", "Set Turret Damage", "Gameplay/Targeting", "damage:i64",
      "damage:i64", true},
     {"gameplay.entity.destroy.v1", "Destroy Entity", "Gameplay/Entity", "unused:i64",
      "accepted:bool", true},
     {"gameplay.entity.spawn.v1", "Spawn Prefab", "Gameplay/Entity", "prefab:asset-id-bits",
      "spawned:bool", true},
     {"gameplay.entity.find.v1", "Find Target", "Gameplay/Entity", "range:centiunits", "found:bool",
      false},
     {"gameplay.transform.x.v1", "Get Position X", "Gameplay/Transform", "unused:i64",
      "x:centiunits", false},
     {"gameplay.transform.y.v1", "Get Position Y", "Gameplay/Transform", "unused:i64",
      "y:centiunits", false},
     {"gameplay.transform.setx.v1", "Set Position X", "Gameplay/Transform", "x:centiunits",
      "x:centiunits", true},
     {"gameplay.transform.sety.v1", "Set Position Y", "Gameplay/Transform", "y:centiunits",
      "y:centiunits", true},
     {"gameplay.movement.stop.v1", "Stop Movement", "Gameplay/Movement", "unused:i64",
      "accepted:bool", true},
     {"gameplay.time.tick.v1", "Fixed Tick", "Gameplay/Time", "unused:i64", "tick:i64", false},
     {"gameplay.time.delta.v1", "Delta Microseconds", "Gameplay/Time", "unused:i64",
      "microseconds:i64", false},
     {"gameplay.time.timer.v1", "Start Timer", "Gameplay/Time", "ticks:i64", "deadline:i64", true},
     {"gameplay.input.pressed.v1", "Attack Pressed", "Gameplay/Input", "unused:i64", "pressed:bool",
      false},
     {"gameplay.projectile.fire.v1", "Fire Projectile", "Gameplay/Projectile", "unused:i64",
      "accepted:bool", true},
     {"gameplay.wave.start.v1", "Start Wave", "Gameplay/Waves", "unused:i64", "accepted:bool",
      true},
     {"gameplay.wave.cancel.v1", "Cancel Wave", "Gameplay/Waves", "unused:i64", "accepted:bool",
      true},
     {"gameplay.economy.get.v1", "Get Currency", "Gameplay/State", "unused:i64", "currency:i64",
      false},
     {"gameplay.economy.award.v1", "Award Currency", "Gameplay/State", "amount:i64", "currency:i64",
      true},
     {"gameplay.economy.spend.v1", "Spend Currency", "Gameplay/State", "amount:i64",
      "remaining:i64", true},
#define EVENT(N)                                                                                   \
    {                                                                                              \
        "gameplay.event." #N ".v1",                                                                \
        #N,                                                                                        \
        "Gameplay/Events",                                                                         \
        "event:i64",                                                                               \
        "payload:i64",                                                                             \
        false,                                                                                     \
        runtime::graph::Operation::EventEntry}
     EVENT(Damaged),
     EVENT(Died),
     EVENT(Healed),
     EVENT(CollisionEnter),
     EVENT(CollisionStay),
     EVENT(CollisionExit),
     EVENT(TriggerEnter),
     EVENT(TriggerStay),
     EVENT(TriggerExit),
     EVENT(Timer),
     EVENT(Spawned),
     EVENT(WaveCompleted),
     EVENT(WaveCanceled),
     EVENT(ProjectileHit),
     EVENT(ActionStarted),
     EVENT(ActionReleased)
#undef EVENT
};
} // namespace

std::span<const Operation> Operations() {
    return catalogue;
}

foundation::Result<void> Register(comp::ComponentRegistry &r) {
// All defaults and validators also govern Inspector edits and deserialization.
#define TYPE(T, N)                                                                                 \
    auto type = r.Register<T>(N);                                                                  \
    if (!type)                                                                                     \
    return R::Failure(type.GetError())
#define FIELD(T, F, V)                                                                             \
    {                                                                                              \
        auto field = r.RegisterProperty<T, decltype(T::F)>(type.Value(), #F, &T::F, V);            \
        if (!field)                                                                                \
            return R::Failure(field.GetError());                                                   \
    }
    {
        TYPE(Collider, "gameplay.collider2d");
        FIELD(Collider, radius, &Nonnegative);
        FIELD(Collider, half_x, &Nonnegative);
        FIELD(Collider, half_y, &Nonnegative);
        FIELD(Collider, shape, &Shape);
        FIELD(Collider, layer, &Mask);
        FIELD(Collider, mask, &Mask);
        FIELD(Collider, trigger, nullptr);
    }
    {
        TYPE(Projectile, "gameplay.projectile");
        FIELD(Projectile, lifetime, &Count);
        FIELD(Projectile, damage, &Count);
        FIELD(Projectile, speed, &Nonnegative);
    }
    {
        TYPE(Turret, "gameplay.turret");
        FIELD(Turret, range, &Nonnegative);
        FIELD(Turret, cooldown, &Count);
        FIELD(Turret, damage, &Count);
        FIELD(Turret, projectile, nullptr);
        FIELD(Turret, enabled, nullptr);
    }
    {
        TYPE(Wave, "gameplay.wavespawner");
        FIELD(Wave, prefab, nullptr);
        FIELD(Wave, count, &Count);
        FIELD(Wave, interval, &Count);
        FIELD(Wave, waves, &Count);
        FIELD(Wave, enabled, nullptr);
    }
    {
        TYPE(State, "gameplay.state");
        FIELD(State, currency, &Count);
        FIELD(State, wave, &Count);
    }
    {
        TYPE(Motion, "gameplay.motion2d");
        FIELD(Motion, x, &Finite);
        FIELD(Motion, y, &Finite);
        FIELD(Motion, player, nullptr);
        FIELD(Motion, enabled, nullptr);
    }
    {
        TYPE(InputBindings, "gameplay.inputbindings");
        FIELD(InputBindings, left, &Key);
        FIELD(InputBindings, right, &Key);
        FIELD(InputBindings, up, &Key);
        FIELD(InputBindings, down, &Key);
        FIELD(InputBindings, attack, &Key);
    }
#undef FIELD
#undef TYPE
    return R::Success();
}

bool Gameplay::Alive(Entity e) const {
    auto v = services_.entities.Resolve(e);
    return v && v->lifecycle == gameplay::entities::EntityLifecycleState::Alive &&
           !pending_destroy_.contains(e);
}

bool Gameplay::Has(Entity e, std::string_view t) const {
    auto v = services_.entities.Resolve(e);
    return v && services_.components.Find(v->id, gameplay::TypeId::FromString(t)).has_value();
}

std::vector<Entity> Gameplay::Entities() const {
    std::vector<Entity> out;
    for (const auto &e : services_.entities.AllEntities()) {
        auto h = services_.entities.HandleOf(e.id);
        if (Alive(h))
            out.push_back(h);
    }
    std::sort(out.begin(), out.end(), EntityLess{});
    return out;
}

void Gameplay::Emit(std::string name, Entity subject, Entity other, std::int64_t value) {
    if (++events_ <= 512 && services_.event)
        services_.event({tick_, subject, other, std::move(name), value});
}

V Gameplay::Damage(Entity e, std::int64_t amount, bool heal) {
    if (!Alive(e))
        return V::Failure(Error("gameplay.stale_entity"));
    if (amount < 0 || amount > 10000)
        return V::Failure(Error("gameplay.damage_range"));
    if (dead_.contains(e))
        return V::Success(0);
    auto old = Read<std::int64_t>(e, {"editor.example.health", "points"}, -1);
    if (old < 0)
        return V::Failure(Error("gameplay.health_missing"));
    auto maximum = Read<std::int64_t>(e, {"editor.example.health", "maximum"}, 10000);
    if (maximum < old)
        maximum = old;
    auto next = heal ? std::min(maximum, old + amount) : std::max<std::int64_t>(0, old - amount);
    auto changed = Write(e, {"editor.example.health", "points"}, next);
    if (!changed)
        return V::Failure(changed.GetError());
    Emit(heal ? "Healed" : "Damaged", e, {}, std::abs(next - old));
    if (next == 0 && old > 0) {
        dead_.insert(e);
        Emit("Died", e);
    }
    return V::Success(std::abs(next - old));
}

Entity Gameplay::Target(Entity self, double range) const {
    auto world = GetEntityWorldTransform(services_.scene, services_.entities, self);
    if (!world)
        return {};
    Entity target{};
    double best = range * range;
    for (auto e : Entities())
        if (e != self && Has(e, "editor.example.health") && !dead_.contains(e) &&
            Read<std::int64_t>(e, {"editor.example.health", "points"}, 0) > 0 &&
            !Has(e, "gameplay.projectile")) {
            auto other = GetEntityWorldTransform(services_.scene, services_.entities, e);
            if (!other)
                continue;
            auto dx = world.Value().position.x - other.Value().position.x,
                 dy = world.Value().position.y - other.Value().position.y;
            double distance = static_cast<double>(dx) * dx + static_cast<double>(dy) * dy;
            if (distance <= best) {
                best = distance;
                target = e;
            }
        }
    return target;
}

V Gameplay::Execute(std::string_view op, Entity e, std::int64_t a) {
    auto found = std::find_if(std::begin(catalogue), std::end(catalogue),
                              [&](const auto &o) { return o.id == op; });
    if (found == std::end(catalogue) || found->kind != runtime::graph::Operation::NativeCall)
        return V::Failure(Error("gameplay.operation_unknown"));
    if (!Alive(e))
        return V::Failure(Error("gameplay.stale_entity"));
    if (found->mutates && ++mutations_ > 256)
        return V::Failure(Error("gameplay.tick_budget"));
    if (op == "gameplay.health.damage.v1" || op == "gameplay.health.heal.v1")
        return Damage(e, a, op == "gameplay.health.heal.v1");
    if (op == "gameplay.health.get.v1") {
        auto hp = Read<std::int64_t>(e, {"editor.example.health", "points"}, -1);
        return hp < 0 ? V::Failure(Error("gameplay.health_missing")) : V::Success(hp);
    }
    if (op == "gameplay.time.tick.v1")
        return V::Success(static_cast<std::int64_t>(tick_));
    if (op.starts_with("gameplay.turret.")) {
        if (a < 0 || a > 1000000)
            return V::Failure(Error("gameplay.turret_range"));
        R changed = op == "gameplay.turret.range.v1"
                        ? Write(e, {"gameplay.turret", "range"}, static_cast<double>(a) / 100)
                        : Write(e,
                                {"gameplay.turret",
                                 op == "gameplay.turret.cooldown.v1" ? "cooldown" : "damage"},
                                a);
        return changed ? V::Success(a) : V::Failure(changed.GetError());
    }
    if (op == "gameplay.time.delta.v1")
        return V::Success(16667);
    if (op == "gameplay.input.pressed.v1")
        return V::Success(input_.attack ? 1 : 0);
    if (op == "gameplay.entity.destroy.v1") {
        pending_destroy_.insert(e);
        return V::Success(1);
    }
    if (op == "gameplay.entity.find.v1") {
        if (a < 0 || a > 1000000)
            return V::Failure(Error("gameplay.range"));
        return V::Success(Alive(Target(e, static_cast<double>(a) / 100)) ? 1 : 0);
    }
    if (op.starts_with("gameplay.transform.")) {
        auto world = GetEntityWorldTransform(services_.scene, services_.entities, e);
        if (!world)
            return V::Failure(world.GetError());
        bool x = op == "gameplay.transform.x.v1" || op == "gameplay.transform.setx.v1";
        auto &value = x ? world.Value().position.x : world.Value().position.y;
        if (op == "gameplay.transform.setx.v1" || op == "gameplay.transform.sety.v1") {
            if (std::abs(static_cast<double>(a)) > 100000000)
                return V::Failure(Error("gameplay.position_range"));
            value = static_cast<float>(static_cast<double>(a) / 100);
            auto changed =
                SetEntityWorldTransform(services_.scene, services_.entities, e, world.Value());
            if (!changed)
                return V::Failure(changed.GetError());
        }
        return V::Success(
            static_cast<std::int64_t>(std::llround(static_cast<double>(value) * 100)));
    }
    if (op == "gameplay.movement.stop.v1") {
        auto changed = Write(e, {"gameplay.motion2d", "enabled"}, false);
        return changed ? V::Success(1) : V::Failure(changed.GetError());
    }
    if (op == "gameplay.time.timer.v1") {
        if (a < 1 || a > 360000)
            return V::Failure(Error("gameplay.timer_range"));
        timers_[e] = tick_ + static_cast<std::uint64_t>(a);
        return V::Success(static_cast<std::int64_t>(timers_[e]));
    }
    if (op.starts_with("gameplay.economy.")) {
        auto value = Read<std::int64_t>(e, {"gameplay.state", "currency"}, -1);
        if (value < 0)
            return V::Failure(Error("gameplay.state_missing"));
        if (op == "gameplay.economy.get.v1")
            return V::Success(value);
        if (a < 0 || a > 10000)
            return V::Failure(Error("gameplay.currency_range"));
        auto next = op == "gameplay.economy.spend.v1" ? value - a : value + a;
        if (next < 0 || next > 10000)
            return V::Failure(Error("gameplay.insufficient_or_excess_funds"));
        auto changed = Write(e, {"gameplay.state", "currency"}, next);
        return changed ? V::Success(next) : V::Failure(changed.GetError());
    }
    if (op == "gameplay.entity.spawn.v1") {
        auto spawned =
            Spawn(e, runtime::AssetId{foundation::StringId{std::bit_cast<std::uint64_t>(a)}});
        return spawned ? V::Success(1) : V::Failure(spawned.GetError());
    }
    if (op == "gameplay.projectile.fire.v1") {
        auto fired = Fire(e);
        return fired ? V::Success(1) : V::Failure(fired.GetError());
    }
    if (op == "gameplay.wave.cancel.v1") {
        waves_.erase(e);
        wave_done_.insert(e);
        Emit("WaveCanceled", e);
        return V::Success(1);
    }
    if (op == "gameplay.wave.start.v1") {
        if (!Has(e, "gameplay.wavespawner"))
            return V::Failure(Error("gameplay.wave_missing"));
        auto count = Read<std::int64_t>(e, {"gameplay.wavespawner", "count"}, 0),
             waves = Read<std::int64_t>(e, {"gameplay.wavespawner", "waves"}, 0),
             interval = Read<std::int64_t>(e, {"gameplay.wavespawner", "interval"}, 0);
        if (count < 1 || waves < 1 || count * waves > 10000 || interval < 1 ||
            Read<std::int64_t>(e, {"gameplay.wavespawner", "prefab"}, 0) == 0)
            return V::Failure(Error("gameplay.wave_range"));
        wave_done_.erase(e);
        waves_[e] = {tick_, 0, 1};
        return V::Success(1);
    }
    return V::Failure(Error("gameplay.operation_unknown"));
}

R Gameplay::Begin(std::uint64_t tick, const Input &input) {
    if (tick < tick_)
        return R::Failure(Error("gameplay.clock_reversed"));
    const double delta = static_cast<double>(tick - tick_) / 60;
    tick_ = tick;
    const bool attack_before = input_.attack;
    input_ = input;
    mutations_ = 0;
    events_ = 0;
    if (attack_before != input.attack)
        for (auto e : Entities())
            if (Read(e, {"gameplay.motion2d", "player"}, false))
                Emit(input.attack ? "ActionStarted" : "ActionReleased", e, {},
                     input.attack ? 1 : 0);
    auto entities = Entities();
    for (auto e : entities) {
        if (dead_.contains(e))
            continue;
        if (Has(e, "gameplay.wavespawner") && Read(e, {"gameplay.wavespawner", "enabled"}, false) &&
            !waves_.contains(e) && !wave_done_.contains(e)) {
            auto started = Execute("gameplay.wave.start.v1", e, 0);
            if (!started)
                return R::Failure(started.GetError());
        }
        if (!Has(e, "gameplay.motion2d") || !Read(e, {"gameplay.motion2d", "enabled"}, true) ||
            dead_.contains(e) || shots_.contains(e))
            continue;
        double x = Read(e, {"gameplay.motion2d", "x"}, 0.),
               y = Read(e, {"gameplay.motion2d", "y"}, 0.);
        if (Read(e, {"gameplay.motion2d", "player"}, false)) {
            x = input.x;
            y = input.y;
        }
        double length = std::hypot(x, y);
        if (length > 1) {
            x /= length;
            y /= length;
        }
        double speed = Read(e, {"editor.example.movement", "speed"}, 1.);
        if (!Nonnegative(speed) || !Finite(x) || !Finite(y))
            return R::Failure(Error("gameplay.motion_invalid"));
        auto world = GetEntityWorldTransform(services_.scene, services_.entities, e);
        if (!world)
            continue;
        world.Value().position.x += static_cast<float>(x * speed * delta);
        world.Value().position.y += static_cast<float>(y * speed * delta);
        auto moved = SetEntityWorldTransform(services_.scene, services_.entities, e, world.Value());
        if (!moved)
            return moved;
    }
    for (auto it = timers_.begin(); it != timers_.end();) {
        if (!Alive(it->first) || dead_.contains(it->first)) {
            it = timers_.erase(it);
            continue;
        }
        if (tick_ >= it->second) {
            Emit("Timer", it->first);
            it = timers_.erase(it);
        } else
            ++it;
    }
    for (auto it = waves_.begin(); it != waves_.end();) {
        auto e = it->first;
        auto &job = it->second;
        if (!Alive(e) || dead_.contains(e)) {
            it = waves_.erase(it);
            continue;
        }
        auto count = Read<std::int64_t>(e, {"gameplay.wavespawner", "count"}, 0),
             waves = Read<std::int64_t>(e, {"gameplay.wavespawner", "waves"}, 0);
        if (tick_ >= job.next) {
            auto prefab = Read<std::int64_t>(e, {"gameplay.wavespawner", "prefab"}, 0);
            auto spawned =
                Spawn(e,
                      runtime::AssetId{foundation::StringId{std::bit_cast<std::uint64_t>(prefab)}});
            if (!spawned)
                return R::Failure(spawned.GetError());
            ++job.spawned;
            job.next =
                tick_ + static_cast<std::uint64_t>(
                            std::max<std::int64_t>(1, Read<std::int64_t>(e,
                                                                         {"gameplay.wavespawner",
                                                                          "interval"},
                                                                         1)));
            Emit("Spawned", e, spawned.Value(), job.spawned);
            if (job.spawned >= count) {
                Emit("WaveCompleted", e, {}, job.wave);
                if (Has(e, "gameplay.state")) {
                    auto changed = Write(e, {"gameplay.state", "wave"}, job.wave);
                    if (!changed)
                        return changed;
                }
                if (job.wave >= waves) {
                    wave_done_.insert(e);
                    it = waves_.erase(it);
                    continue;
                }
                ++job.wave;
                job.spawned = 0;
            }
        }
        ++it;
    }
    for (auto e : entities)
        if (Alive(e) && !dead_.contains(e) && Has(e, "gameplay.turret") &&
            Read(e, {"gameplay.turret", "enabled"}, true) && tick_ >= cooldowns_[e] &&
            Alive(Target(e, Read(e, {"gameplay.turret", "range"}, 0.)))) {
            auto fire = Fire(e);
            if (!fire)
                return fire;
        }
    for (auto it = shots_.begin(); it != shots_.end();) {
        auto e = it->first;
        const auto &shot = it->second;
        if (!Alive(e) || !Alive(shot.owner) || !Alive(shot.target) || dead_.contains(shot.owner) ||
            dead_.contains(shot.target) || tick_ >= shot.expires) {
            if (Alive(e))
                pending_destroy_.insert(e);
            it = shots_.erase(it);
            continue;
        }
        auto world = GetEntityWorldTransform(services_.scene, services_.entities, e),
             target = GetEntityWorldTransform(services_.scene, services_.entities, shot.target);
        if (world && target) {
            double dx = target.Value().position.x - world.Value().position.x,
                   dy = target.Value().position.y - world.Value().position.y,
                   length = std::hypot(dx, dy);
            double distance = Read(e, {"gameplay.projectile", "speed"}, 0.) * delta;
            if (length > 0) {
                auto scale = std::min(1., distance / length);
                world.Value().position.x += static_cast<float>(dx * scale);
                world.Value().position.y += static_cast<float>(dy * scale);
                auto moved =
                    SetEntityWorldTransform(services_.scene, services_.entities, e, world.Value());
                if (!moved)
                    return moved;
            }
        }
        ++it;
    }
    return R::Success();
}

bool Gameplay::Overlap(Entity a, Entity b) const {
    constexpr auto type = "gameplay.collider2d";
    const auto layerA = static_cast<std::uint64_t>(Read<std::int64_t>(a, {type, "layer"}, 1)),
               layerB = static_cast<std::uint64_t>(Read<std::int64_t>(b, {type, "layer"}, 1));
    if (!(layerA & static_cast<std::uint64_t>(Read<std::int64_t>(b, {type, "mask"}, 0))) ||
        !(layerB & static_cast<std::uint64_t>(Read<std::int64_t>(a, {type, "mask"}, 0))))
        return false;
    auto wa = GetEntityWorldTransform(services_.scene, services_.entities, a),
         wb = GetEntityWorldTransform(services_.scene, services_.entities, b);
    if (!wa || !wb)
        return false;
    const auto &x = wa.Value(), &y = wb.Value();
    double dx = std::abs(static_cast<double>(x.position.x) - y.position.x),
           dy = std::abs(static_cast<double>(x.position.y) - y.position.y);
    bool ca = Read<std::int64_t>(a, {type, "shape"}, 0) == 1,
         cb = Read<std::int64_t>(b, {type, "shape"}, 0) == 1;
    double ax = Read(a, {type, "half_x"}, .4) * std::abs(x.scale.x),
           ay = Read(a, {type, "half_y"}, .4) * std::abs(x.scale.y);
    double bx = Read(b, {type, "half_x"}, .4) * std::abs(y.scale.x),
           by = Read(b, {type, "half_y"}, .4) * std::abs(y.scale.y);
    double ra = Read(a, {type, "radius"}, .4) * std::max(std::abs(x.scale.x), std::abs(x.scale.y)),
           rb = Read(b, {type, "radius"}, .4) * std::max(std::abs(y.scale.x), std::abs(y.scale.y));
    if (ca && cb)
        return dx * dx + dy * dy <= (ra + rb) * (ra + rb);
    if (!ca && !cb)
        return dx <= ax + bx && dy <= ay + by;
    double qx = std::max(0., dx - (ca ? bx : ax)), qy = std::max(0., dy - (ca ? by : ay)),
           r = ca ? ra : rb;
    return qx * qx + qy * qy <= r * r;
}

R Gameplay::End(bool allow_death_destroy) {
    // Bounded sweep broadphase; stable handle ordering resolves equal positions.
    struct Bound {
        Entity entity;
        double low = 0, high = 0;
    };

    std::vector<Bound> bodies;
    for (auto e : Entities())
        if (Has(e, "gameplay.collider2d")) {
            auto w = GetEntityWorldTransform(services_.scene, services_.entities, e);
            if (!w)
                continue;
            double extent =
                Read<std::int64_t>(e, {"gameplay.collider2d", "shape"}, 0) == 1
                    ? Read(e, {"gameplay.collider2d", "radius"}, .4) *
                          std::max(std::abs(w.Value().scale.x), std::abs(w.Value().scale.y))
                    : Read(e, {"gameplay.collider2d", "half_x"}, .4) * std::abs(w.Value().scale.x);
            bodies.push_back({e, w.Value().position.x - extent, w.Value().position.x + extent});
        }
    std::sort(bodies.begin(), bodies.end(), [](const auto &a, const auto &b) {
        return a.low < b.low || (a.low == b.low && EntityLess{}(a.entity, b.entity));
    });
    std::set<std::pair<Entity, Entity>, ContactLess> next;
    for (std::size_t i = 0; i < bodies.size(); ++i)
        for (std::size_t j = i + 1; j < bodies.size() && bodies[j].low <= bodies[i].high; ++j) {
            auto a = bodies[i].entity, b = bodies[j].entity;
            if (!Alive(a) || !Alive(b) || !Overlap(a, b))
                continue;
            if ((shots_.contains(a) && shots_.at(a).owner == b) ||
                (shots_.contains(b) && shots_.at(b).owner == a))
                continue;
            auto pair = EntityLess{}(a, b) ? std::pair{a, b} : std::pair{b, a};
            if (next.size() >= 8192)
                return R::Failure(Error("gameplay.contact_budget"));
            next.insert(pair);
            const bool trigger = Read(a, {"gameplay.collider2d", "trigger"}, false) ||
                                 Read(b, {"gameplay.collider2d", "trigger"}, false);
            const auto name = contacts_.contains(pair)
                                  ? (trigger ? "TriggerStay" : "CollisionStay")
                                  : (trigger ? "TriggerEnter" : "CollisionEnter");
            Emit(name, a, b);
            Emit(name, b, a);
            for (auto [shot, target] : {std::pair{a, b}, std::pair{b, a}}) {
                auto at = shots_.find(shot);
                if (at == shots_.end() || pending_destroy_.contains(shot))
                    continue;
                if (!Has(target, "editor.example.health"))
                    continue;
                if (dead_.contains(target)) {
                    pending_destroy_.insert(shot);
                    continue;
                }
                auto damage =
                    Damage(target, Read<std::int64_t>(shot, {"gameplay.projectile", "damage"}, 0),
                           false);
                if (!damage)
                    return R::Failure(damage.GetError());
                Emit("ProjectileHit", shot, target, damage.Value());
                pending_destroy_.insert(shot);
                if (dead_.contains(target)) {
                    death_destroy_.try_emplace(target, tick_ + 1);
                    auto owner = at->second.owner;
                    if (Alive(owner) && Has(owner, "gameplay.state")) {
                        auto currency =
                            Read<std::int64_t>(owner, {"gameplay.state", "currency"}, 0);
                        auto changed = Write(owner, {"gameplay.state", "currency"},
                                             std::min<std::int64_t>(10000, currency + 1));
                        if (!changed)
                            return changed;
                    }
                }
            }
        }
    for (const auto &[entity, due] : death_destroy_)
        if (allow_death_destroy && tick_ >= due)
            pending_destroy_.insert(entity);
    for (const auto &[a, b] : contacts_)
        if (!next.contains({a, b})) {
            auto name = Read(a, {"gameplay.collider2d", "trigger"}, false) ||
                                Read(b, {"gameplay.collider2d", "trigger"}, false)
                            ? "TriggerExit"
                            : "CollisionExit";
            if (Alive(a))
                Emit(name, a, Alive(b) ? b : Entity{});
            if (Alive(b))
                Emit(name, b, Alive(a) ? a : Entity{});
        }
    contacts_ = std::move(next);
    for (const auto &[a, b] : contacts_)
        if (pending_destroy_.contains(a) || pending_destroy_.contains(b)) {
            auto name = Read(a, {"gameplay.collider2d", "trigger"}, false) ||
                                Read(b, {"gameplay.collider2d", "trigger"}, false)
                            ? "TriggerExit"
                            : "CollisionExit";
            if (Alive(a))
                Emit(name, a, {});
            if (Alive(b))
                Emit(name, b, {});
        }
    for (auto e : pending_destroy_)
        if (services_.entities.Resolve(e)) {
            Emit("Destroyed", e);
            if (services_.destroy) {
                auto destroyed = services_.destroy(e);
                if (!destroyed)
                    return destroyed;
            }
        }
    pending_destroy_.clear();
    std::erase_if(shots_, [&](const auto &v) { return !Alive(v.first); });
    std::erase_if(waves_, [&](const auto &v) { return !Alive(v.first); });
    std::erase_if(timers_, [&](const auto &v) { return !Alive(v.first); });
    std::erase_if(cooldowns_, [&](const auto &v) { return !Alive(v.first); });
    std::erase_if(dead_, [&](auto e) { return !Alive(e); });
    std::erase_if(death_destroy_, [&](const auto &v) { return !Alive(v.first); });
    std::erase_if(wave_done_, [&](auto e) { return !Alive(e); });
    std::erase_if(contacts_, [&](const auto &v) { return !Alive(v.first) || !Alive(v.second); });
    if (events_ > 512)
        return R::Failure(Error("gameplay.event_backpressure"));
    return R::Success();
}

foundation::Result<Entity> Gameplay::Spawn(Entity source, runtime::AssetId prefab) {
    using Spawned = foundation::Result<Entity>;
    if (++mutations_ > 256)
        return Spawned::Failure(Error("gameplay.tick_budget"));
    auto world = GetEntityWorldTransform(services_.scene, services_.entities, source);
    if (!world)
        return Spawned::Failure(world.GetError());
    if (!services_.spawn)
        return Spawned::Failure(Error("gameplay.spawn_unavailable"));
    return services_.spawn(prefab, world.Value());
}

R Gameplay::Fire(Entity self) {
    if (!Alive(self) || dead_.contains(self))
        return R::Failure(Error("gameplay.dead_entity"));
    if (!Has(self, "gameplay.turret"))
        return R::Failure(Error("gameplay.turret_missing"));
    auto target = Target(self, Read(self, {"gameplay.turret", "range"}, 0.));
    if (!Alive(target))
        return R::Failure(Error("gameplay.target_missing"));
    if (tick_ < cooldowns_[self])
        return R::Failure(Error("gameplay.cooldown"));
    auto prefab = Read<std::int64_t>(self, {"gameplay.turret", "projectile"}, 0);
    auto shot =
        Spawn(self, runtime::AssetId{foundation::StringId{std::bit_cast<std::uint64_t>(prefab)}});
    if (!shot)
        return R::Failure(shot.GetError());
    if (!Has(shot.Value(), "gameplay.projectile") || !Has(shot.Value(), "gameplay.collider2d")) {
        pending_destroy_.insert(shot.Value());
        return R::Failure(Error("gameplay.projectile_component_missing"));
    }
    auto lifetime = Read<std::int64_t>(shot.Value(), {"gameplay.projectile", "lifetime"}, 0);
    if (lifetime < 1) {
        pending_destroy_.insert(shot.Value());
        return R::Failure(Error("gameplay.projectile_lifetime"));
    }
    auto damage = Write(shot.Value(), {"gameplay.projectile", "damage"},
                        Read<std::int64_t>(self, {"gameplay.turret", "damage"}, 10));
    if (!damage) {
        pending_destroy_.insert(shot.Value());
        return damage;
    }
    shots_[shot.Value()] = {self, target, tick_ + static_cast<std::uint64_t>(lifetime)};
    cooldowns_[self] =
        tick_ +
        static_cast<std::uint64_t>(
            std::max<std::int64_t>(1,
                                   Read<std::int64_t>(self, {"gameplay.turret", "cooldown"}, 1)));
    Emit("ProjectileFired", self, shot.Value());
    return R::Success();
}

void Gameplay::Cancel() {
    shots_.clear();
    waves_.clear();
    wave_done_.clear();
    timers_.clear();
    cooldowns_.clear();
    contacts_.clear();
    pending_destroy_.clear();
    dead_.clear();
    death_destroy_.clear();
    input_ = {};
}
} // namespace epidemic::game::composition::arena

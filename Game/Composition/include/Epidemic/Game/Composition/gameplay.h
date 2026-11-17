#pragma once
#include "Epidemic/Game/Composition/scene_composition.h"
#include "Epidemic/Runtime/Graph/graph_vm.h"
#include <deque>
#include <functional>
#include <set>
#include <span>
#include <map>
#include <string>
#include <string_view>
#include <vector>
#include <cstdint>
#include <utility>

namespace epidemic::game::composition::arena {
using Entity = gameplay::entities::EntityHandle;
using EntityId = gameplay::entities::EntityId;

struct EntityLess {
    bool operator()(Entity a, Entity b) const {
        return a.slot < b.slot || (a.slot == b.slot && a.generation < b.generation);
    }
};

struct ContactLess {
    bool operator()(const std::pair<Entity, Entity> &a, const std::pair<Entity, Entity> &b) const {
        EntityLess less;
        return less(a.first, b.first) || (!less(b.first, a.first) && less(a.second, b.second));
    }
};
namespace comp = gameplay::composition;

struct Collider {
    double radius = .4, half_x = .4, half_y = .4;
    std::int64_t shape = 0, layer = 1, mask = 65535;
    bool trigger = false;
};

struct Projectile {
    std::int64_t lifetime = 120, damage = 10;
    double speed = 4;
};

struct Turret {
    double range = 6;
    std::int64_t cooldown = 30, projectile = 0, damage = 10;
    bool enabled = true;
};

struct Wave {
    std::int64_t prefab = 0, count = 5, interval = 30, waves = 1;
    bool enabled = false;
};

struct State {
    std::int64_t currency = 0, wave = 0;
};

struct Motion {
    double x = 0, y = 0;
    bool player = false, enabled = true;
};

struct InputBindings {
    std::int64_t left = 65, right = 68, up = 87, down = 83, attack = 32;
};

// Register into the host's existing registry before Freeze. Runtime values live
// exclusively in its ComponentStore, never in a second Entity/Scene model.
[[nodiscard]] foundation::Result<void> Register(comp::ComponentRegistry &registry);

struct Operation {
    std::string_view id, name, category, argument, result;
    bool mutates = false;
    runtime::graph::Operation kind = runtime::graph::Operation::NativeCall;
};

[[nodiscard]] std::span<const Operation> Operations();

struct Event {
    std::uint64_t tick = 0;
    Entity subject{}, other{};
    std::string name;
    std::int64_t value = 0;
};

struct Input {
    double x = 0, y = 0;
    bool attack = false;
};

class Gameplay final {
  public:
    struct Services {
        gameplay::entities::EntityService &entities;
        comp::ComponentStore &components;
        SceneCompositionInstance &scene;
        std::function<foundation::Result<Entity>(runtime::AssetId, const runtime::Transform &)>
            spawn;
        std::function<foundation::Result<void>(Entity)> destroy;
        std::function<void(const Event &)> event;
    };

    explicit Gameplay(Services services) : services_(std::move(services)) {
    }

    [[nodiscard]] foundation::Result<std::int64_t> Execute(std::string_view operation, Entity self,
                                                           std::int64_t argument);
    [[nodiscard]] foundation::Result<void> Begin(std::uint64_t tick, const Input &input);
    [[nodiscard]] foundation::Result<void> End(bool allow_death_destroy = true);
    void Cancel();

    [[nodiscard]] std::uint64_t Tick() const {
        return tick_;
    }

    [[nodiscard]] std::size_t Jobs() const {
        return shots_.size() + waves_.size() + timers_.size() + death_destroy_.size();
    }

  private:
    Services services_;
    std::uint64_t tick_ = 0;
    Input input_{};
    std::size_t mutations_ = 0;
    std::size_t events_ = 0;

    struct Shot {
        Entity owner{}, target{};
        std::uint64_t expires = 0;
    };

    struct WaveJob {
        std::uint64_t next = 0;
        std::int64_t spawned = 0, wave = 1;
    };

    std::map<Entity, Shot, EntityLess> shots_;
    std::map<Entity, WaveJob, EntityLess> waves_;
    std::set<Entity, EntityLess> wave_done_;
    std::map<Entity, std::uint64_t, EntityLess> timers_, cooldowns_, death_destroy_;
    std::set<Entity, EntityLess> dead_, pending_destroy_;
    std::set<std::pair<Entity, Entity>, ContactLess> contacts_;
    bool Alive(Entity entity) const;
    bool Has(Entity entity, std::string_view type) const;

    struct PropertyName {
        std::string_view component{}, field{};
    };

    template <class T> T Read(Entity entity, const PropertyName &property, T fallback) const {
        auto handle = services_.entities.Resolve(entity);
        if (!handle)
            return fallback;
        auto component =
            services_.components.Find(handle->id, gameplay::TypeId::FromString(property.component));
        if (!component)
            return fallback;
        auto value = services_.components.Read<T>(*component,
                                                  foundation::StringId::FromString(property.field));
        return value ? value.Value() : fallback;
    }

    template <class T>
    foundation::Result<void> Write(Entity entity, const PropertyName &property, T value) {
        auto handle = services_.entities.Resolve(entity);
        if (!handle)
            return foundation::Result<void>::Failure(Error("gameplay.stale_entity"));
        auto component =
            services_.components.Find(handle->id, gameplay::TypeId::FromString(property.component));
        if (!component)
            return foundation::Result<void>::Failure(Error("gameplay.component_missing"));
        return services_.components.WriteProperty(*component,
                                                  {foundation::StringId::FromString(property.field),
                                                   std::move(value)});
    }

    static foundation::Error Error(std::string_view code) {
        return foundation::Error::Create(code, "native gameplay");
    }

    void Emit(std::string name, Entity subject, Entity other = {}, std::int64_t value = 0);
    foundation::Result<std::int64_t> Damage(Entity target, std::int64_t amount, bool heal);
    std::vector<Entity> Entities() const;
    Entity Target(Entity self, double range) const;
    foundation::Result<Entity> Spawn(Entity source, runtime::AssetId prefab);
    foundation::Result<void> Fire(Entity self);
    bool Overlap(Entity a, Entity b) const;
};
} // namespace epidemic::game::composition::arena

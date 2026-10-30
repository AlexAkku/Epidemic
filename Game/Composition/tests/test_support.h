#pragma once
#include "Epidemic/Game/Composition/graph_host.h"
#include <array>
#include <cstdio>
#include <stdexcept>

namespace composition_tests {
using namespace epidemic;
using namespace epidemic::runtime;
namespace fw = epidemic::gameplay;
namespace gc = epidemic::game::composition;
namespace bridge = fw::runtime_bridge;

inline graph::ElementId Id(std::string_view value) {
    return graph::ElementId::FromString(value);
}

inline void Check(bool value, const char *message) {
    if (!value)
        throw std::runtime_error(message);
}

template <typename T> void ErrorIs(const foundation::Result<T> &result, std::string_view code) {
    Check(!result && result.GetError().HasCode(code), "unexpected composition error");
}

template <typename T> T Take(foundation::Result<T> &&result) {
    if (!result)
        throw std::runtime_error(result.GetError().code);
    return std::move(result).Value();
}

class Backend final : public bridge::IRuntimeBridgeBackend {
  public:
    foundation::Result<bridge::RuntimeMaterializationResult>
    Materialize(bridge::RuntimePersistentObjectHandle) override {
        return foundation::Result<bridge::RuntimeMaterializationResult>::Success(
            {bridge::RuntimeObjectHandle{next_++}, true});
    }

    foundation::Result<void> Dematerialize(bridge::RuntimeObjectHandle) override {
        return foundation::Result<void>::Success();
    }

    foundation::Result<void> Destroy(bridge::RuntimeObjectHandle) override {
        return foundation::Result<void>::Success();
    }

    foundation::Result<void> ApplyImpulse(bridge::RuntimePhysicsBodyHandle,
                                          bridge::RuntimeVector3) override {
        return foundation::Result<void>::Success();
    }

    foundation::Result<void> ProjectEnvironment(bridge::RuntimeRegionHandle,
                                                const bridge::RuntimeEnvironmentValues &,
                                                fw::Revision) override {
        return foundation::Result<void>::Success();
    }

    std::vector<bridge::RuntimeContactObservation> ConsumeContacts() override {
        return {};

    }

  private:
    std::uint64_t next_ = 1;
};

class Clock final : public simulation::ISimulationClock {
  public:
    explicit Clock(std::shared_ptr<IGameClock> clock) : clock_(std::move(clock)) {
    }

    simulation::SimulationTime Now() const override {
        return clock_->Now();
    }

  private:
    std::shared_ptr<IGameClock> clock_;
};

inline simulation::SimulationServices Simulation(const std::shared_ptr<IGameClock> &clock) {
    simulation::SimulationDependencies dependencies;
    dependencies.clock = std::make_shared<Clock>(clock);
    auto services = Take(simulation::CreateSimulationServices({}, dependencies));
    services.scheduler->SetBudget({16, 512});
    return services;
}

struct Fixture {
    fw::entities::EntityService entities;
    fw::entities::EntityArchetypeId archetype{};

    Backend backend;
    bridge::RuntimeBridgeService bridge{backend};

    TimeServices time = Take(CreateTimeServices());
    simulation::SimulationServices simulation = Simulation(time.clock);
    GraphScheduler scheduler{time.clock, simulation, SimulationZoneId{1}};

    gc::GraphExecutionHost host{scheduler, bridge};

    Fixture() {
        fw::entities::EntityArchetypeDefinition definition;
        definition.canonical_name = "game.enemy";
        definition.parts.push_back(
            {fw::entities::EntityPartId::FromString("game.body"), "game.body", {}, {}});
        archetype = Take(entities.RegisterArchetype(std::move(definition)));
        entities.Freeze();
    }

    fw::entities::EntityId Entity() {
        fw::entities::CreateEntityRequest request;
        request.archetype = archetype;
        return Take(entities.Create(request)).id;
    }

    void Tick() {
        Check(host.Prepare().HasValue(), "prepare graph host");
        Check(simulation.runtime->Tick().Value().failures.empty(), "tick simulation");
    }

    void Advance() {
        Check(time.runtime->Skip(GameDuration{1}).HasValue(), "advance game clock");
    }

};

inline graph::GraphDefinition Definition() {
    using namespace graph;
    constexpr auto in = PortDirection::Input, out = PortDirection::Output;
    const auto data = [](std::string_view name, PortDirection direction) {
        return PortDefinition{Id(name), std::string(name), PortKind::Data, direction, kI64Type};

    };

    const auto control = [](std::string_view name, PortDirection direction) {
        return PortDefinition{Id(name), std::string(name), PortKind::Control, direction, {}};

    };

    GraphDefinition definition{AssetId::FromString("game.behavior"), "Enemy Behavior", {}, {}};

    definition.nodes = {{Id("event"),
                         Id("event.type"),
                         "Event",
                         {{Id("event.in"), "Event", PortKind::Event, in, kI64Type},
                          data("payload", out),
                          control("event.next", out)}},
                        {Id("before"),
                         Id("call.type"),
                         "Before",
                         {data("before.arg", in), data("before.result", out),
                          control("before.in", in), control("before.next", out)}},
                        {Id("wait"),
                         Id("wait.type"),
                         "Wait",
                         {data("wait.value", in), data("wait.ticks", in), data("wait.result", out),
                          control("wait.in", in), control("wait.next", out)}},
                        {Id("after"),
                         Id("call.type"),
                         "After",
                         {data("after.arg", in), data("after.result", out), control("after.in", in),
                          control("after.next", out)}}};

    const auto edge = [&](std::string_view from, std::string_view to) {
        definition.connections.push_back(
            {Id(std::string(from) + "/" + std::string(to)), Id(from), Id(to)});
    };

    edge("payload", "before.arg");
    edge("payload", "wait.ticks");
    edge("before.result", "wait.value");
    edge("wait.result", "after.arg");
    edge("event.next", "before.in");
    edge("before.next", "wait.in");
    edge("wait.next", "after.in");
    return definition;
}

inline std::array<gc::NodeBinding, 3> Bindings(gc::DomainOperation invoke) {
    return {gc::NodeBinding{Id("event.type"), graph::Operation::EventEntry, {}},
            gc::NodeBinding{Id("call.type"), graph::Operation::NativeCall, std::move(invoke)},
            gc::NodeBinding{Id("wait.type"), graph::Operation::Wait, {}}};

}

inline graph::ExecutionStatus Status(const gc::GraphActivation &activation) {
    const auto &report = activation.Report();
    if (!report)
        throw std::runtime_error("missing execution report");
    return report->status;
}

inline graph::ExecutionStatus
Status(const std::shared_ptr<fw::composition::IGraphActivation> &activation) {
    const auto native = std::dynamic_pointer_cast<gc::GraphActivation>(activation);
    if (!native)
        throw std::runtime_error("unexpected activation implementation");
    return Status(*native);
}

} // namespace composition_tests

#pragma once
#include "Epidemic/GameFramework/Integration/graph_scope.h"
#include "Epidemic/Foundation/string_id.h"
#include <span>
#include <unordered_map>

namespace epidemic::gameplay::composition {
using GraphAssetId = foundation::StringId;

struct GraphInput {
    foundation::StringId port{};

    std::int64_t value = 0;
};

struct GraphEventRoute {
    foundation::StringId event{};

    foundation::StringId port{};

};

struct GameEvent {
    foundation::StringId type{};

    std::int64_t payload = 0;
};

struct GraphBehaviorDefinition {
    foundation::StringId id{};

    GraphAssetId graph{};

    std::vector<GraphEventRoute> events;
    std::vector<GraphInput> inputs;
};

class IGraphProgram {
  public:
    virtual ~IGraphProgram() = default;
};

class IGraphBehaviorHost {
  public:
    virtual ~IGraphBehaviorHost() = default;

    [[nodiscard]] virtual foundation::Result<void>
    CheckDefinition(const GraphBehaviorDefinition &) const {
        return foundation::Result<void>::Success();
    }

    [[nodiscard]] virtual foundation::Result<std::shared_ptr<const IGraphProgram>>
    Bind(const std::shared_ptr<GraphScope> &scope, const GraphBehaviorDefinition &definition) = 0;
    [[nodiscard]] virtual foundation::Result<std::shared_ptr<IGraphActivation>>
    Dispatch(const std::shared_ptr<const IGraphProgram> &program, const GameEvent &event) = 0;
};

struct BehaviorHandle {
    std::uint64_t generation = 0;
    [[nodiscard]] bool operator==(const BehaviorHandle &) const noexcept = default;
};

// An explicit event adapter, not another entity owner, VM, or global event bus.
class GraphBehaviorService final {
  public:
    explicit GraphBehaviorService(IGraphBehaviorHost &host) : host_(host) {
    }

    ~GraphBehaviorService();
    GraphBehaviorService(const GraphBehaviorService &) = delete;
    GraphBehaviorService &operator=(const GraphBehaviorService &) = delete;
    [[nodiscard]] foundation::Result<BehaviorHandle>
    Attach(const std::shared_ptr<GraphScope> &owner, const GraphBehaviorDefinition &definition);
    [[nodiscard]] foundation::Result<void>
    Validate(const GraphBehaviorDefinition &definition) const;
    [[nodiscard]] foundation::Result<void> Detach(BehaviorHandle behavior);
    [[nodiscard]] foundation::Result<std::shared_ptr<IGraphActivation>>
    Send(BehaviorHandle behavior, const GameEvent &event);
    [[nodiscard]] foundation::Result<std::size_t> SendTo(const GraphScope &target,
                                                         const GameEvent &event);
    void Synchronize();

  private:
    struct Behavior {
        GraphBehaviorDefinition definition;
        std::shared_ptr<GraphScope> owner;
        std::shared_ptr<GraphScope> scope;
        std::shared_ptr<const IGraphProgram> program;
    };

    IGraphBehaviorHost &host_;
    std::uint64_t next_ = 1;
    std::unordered_map<std::uint64_t, Behavior> behaviors_;
};

} // namespace epidemic::gameplay::composition

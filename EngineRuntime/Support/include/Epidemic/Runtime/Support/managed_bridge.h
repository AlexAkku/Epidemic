#pragma once

#include "Epidemic/Runtime/Graph/graph_compiler.h"
#include "Epidemic/Runtime/Support/managed_bridge_abi.h"
#include <filesystem>
#include <optional>

namespace epidemic::runtime::managed {
struct HostOptions {
    // Explicit absolute deployment paths; discovery/installation is not a core dependency.
    std::filesystem::path hostfxr;
    std::filesystem::path runtime_config;
    std::filesystem::path assembly;
    std::wstring entry_type; // Assembly-qualified type, exporting static GetApi.
};
struct BatchReport {
    std::int32_t status = EP_BRIDGE_OK;
    std::uint32_t completed = 0;
    std::optional<foundation::Error> diagnostic;
};
class ManagedSession;
class ManagedHost final : public std::enable_shared_from_this<ManagedHost> {
  public:
    ManagedHost(const ManagedHost &) = delete;
    ManagedHost &operator=(const ManagedHost &) = delete;
    [[nodiscard]] static foundation::Result<std::shared_ptr<ManagedHost>> Load(const HostOptions &options);
    [[nodiscard]] foundation::Result<std::shared_ptr<ManagedSession>> Open(std::shared_ptr<graph::INativeInvocation> native);
  private:
    explicit ManagedHost(const EpBridgeManagedApi &api) : api_(api) {}
    EpBridgeManagedApi api_{};
};

// Invoke is synchronous and confined to the opening thread. Close is allowed on
// any thread when idle; active calls return Busy. An invocation retains its own
// session even if a callback releases the last external reference.
class ManagedSession final : public std::enable_shared_from_this<ManagedSession> {
  public:
    ~ManagedSession();
    ManagedSession(const ManagedSession &) = delete;
    ManagedSession &operator=(const ManagedSession &) = delete;
    [[nodiscard]] BatchReport InvokeBatch(std::span<const std::int64_t> input, std::span<std::int64_t> output);
    [[nodiscard]] foundation::Result<void> Close();
  private:
    struct State;
    explicit ManagedSession(std::shared_ptr<State> state) : state_(std::move(state)) {}
    std::shared_ptr<State> state_;
    friend class ManagedHost;
};

// Optional Graph adapter: every call site/instance owns a managed session and a
// fresh native callback state. Graph and C++-only Support never link back here.
[[nodiscard]] foundation::Result<std::shared_ptr<const graph::INativeCallableFactory>>
CreateManagedGraphCallable(std::shared_ptr<ManagedHost> host,
                           std::shared_ptr<const graph::INativeCallableFactory> native);
} // namespace epidemic::runtime::managed

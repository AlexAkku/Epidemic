#include "Epidemic/Runtime/Support/managed_bridge.h"
#include "Epidemic/Runtime/Support/schema_graph_callable.h"
#include "Epidemic/Runtime/Graph/graph_vm.h"
#include <array>
#include <cstdio>
#include <functional>
#include <stdexcept>
#include <thread>

using namespace epidemic;
using namespace epidemic::runtime;
using namespace epidemic::runtime::managed;
namespace {
void Check(bool value, const char *message) {
    if (!value) { std::fputs(message, stderr); std::fputc('\n', stderr); throw std::runtime_error(message); }
}
template <typename T> void ErrorIs(const foundation::Result<T> &value, std::string_view code) {
    Check(!value && value.GetError().HasCode(code), "unexpected managed host error");
}
HostOptions Entry(HostOptions options, std::wstring_view name) {
    options.entry_type = L"Epidemic.ManagedBridge.Tests." + std::wstring(name) + L", Epidemic.ManagedBridge.Tests";
    return options;
}
class Native final : public graph::INativeInvocation {
  public:
    int calls = 0;
    std::function<void()> action;
    foundation::Result<std::int64_t> Invoke(std::int64_t value) override {
        ++calls;
        if (action) action();
        if (value == 13) return foundation::Result<std::int64_t>::Failure(foundation::Error::Create("test.native.denied", "denied"));
        if (value == 14) throw std::runtime_error("native exception");
        return foundation::Result<std::int64_t>::Success(value + 1);
    }
};
std::pair<BatchReport, std::int64_t> One(const std::shared_ptr<ManagedSession> &session, std::int64_t input) {
    std::int64_t output = -99;
    const auto report = session->InvokeBatch({&input, 1}, {&output, 1});
    return {report, output};
}
void CallsAndFailures(const HostOptions &options) {
    auto host = ManagedHost::Load(options).Value();
    auto native = std::make_shared<Native>();
    auto first = host->Open(native).Value();
    auto second_native = std::make_shared<Native>();
    auto second = host->Open(second_native).Value();
    std::weak_ptr<ManagedHost> retained_host = host;
    host.reset();
    const std::array<std::int64_t, 2> input{1, 2};
    std::array<std::int64_t, 2> output{};
    const auto report = first->InvokeBatch(input, output);
    Check(report.status == EP_BRIDGE_OK && report.completed == 2 && output == std::array<std::int64_t, 2>{5, 8} &&
          !retained_host.expired(), "native -> managed -> native; session retains host");
    Check(One(second, 1).second == 5 && second_native->calls == 1, "independent managed/native state");
    Check(One(first, -2).second == 7, "live session/callback survive full GC");
    const std::array<std::int64_t, 3> failing{3, 13, 4};
    std::array<std::int64_t, 3> partial{-99, -99, -99};
    const auto failed = first->InvokeBatch(failing, partial);
    Check(failed.status == EP_BRIDGE_NATIVE_FAILURE && failed.completed == 1 && failed.diagnostic &&
          failed.diagnostic->HasCode("test.native.denied") && partial == std::array<std::int64_t, 3>{12, -99, -99},
          "native error and confirmed prefix preserved");
    Check(One(first, -1).first.status == EP_BRIDGE_MANAGED_EXCEPTION && One(first, -7).first.status == EP_BRIDGE_OVERFLOW &&
          One(first, 14).first.status == EP_BRIDGE_NATIVE_EXCEPTION, "exceptions contained on both boundaries");
    Check(One(first, -3).second == EP_BRIDGE_INVALID_STATE && One(first, -4).second == EP_BRIDGE_WRONG_THREAD,
          "escaped callback lease and worker callback rejected");
    BatchReport wrong_thread;
    std::thread worker([&] { wrong_thread = One(first, 1).first; }); worker.join();
    Check(wrong_thread.status == EP_BRIDGE_WRONG_THREAD, "session Invoke is thread confined");
    const auto closed = second->Close();
    Check(closed.HasValue() && second->Close().HasValue() && One(second, 1).first.status == EP_BRIDGE_INVALID_HANDLE,
          "Close idempotent; closed session cannot run");
    auto probe_host = retained_host.lock();
    Check(static_cast<bool>(probe_host), "host retained by first session");
    auto probe = probe_host->Open(std::make_shared<Native>()).Value();
    Check(first->Close().HasValue() && One(probe, -3).second == EP_BRIDGE_INVALID_HANDLE && One(probe, -6).second == 1,
          "retained managed context cannot use freed callback; callable root released");
    std::array<std::int64_t, 3> overlap{1, 2, 3};
    Check(probe->InvokeBatch({overlap.data(), 2}, {overlap.data() + 1, 2}).status == EP_BRIDGE_INVALID_ARGUMENT,
          "overlapping buffers rejected");
    Check(probe->InvokeBatch({}, {}).status == EP_BRIDGE_OK, "empty batch");
}
void LifetimeAndReentry(const HostOptions &options) {
    auto host = ManagedHost::Load(options).Value();
    auto native = std::make_shared<Native>();
    auto session = host->Open(native).Value();
    native->action = [&] {
        Check(One(session, 1).first.status == EP_BRIDGE_BUSY, "reentrant call rejected");
        ErrorIs(session->Close(), "managed.bridge.busy");
    };
    Check(One(session, 1).first.status == EP_BRIDGE_OK, "reentrancy leaves outer invocation intact");
    native->action = {};
    std::weak_ptr<ManagedSession> weak_session = session;
    std::weak_ptr<graph::INativeInvocation> weak_native = native;
    native->action = [&] { session.reset(); native.reset(); };
    const auto released = One(session, 2);
    Check(released.first.status == EP_BRIDGE_OK && weak_session.expired() && weak_native.expired(),
          "last external reference may be released inside callback; cleanup deferred until return");
    auto other = host->Open(std::make_shared<Native>()).Value();
    bool closed = false;
    std::thread close_thread([&] { closed = other->Close().HasValue(); }); close_thread.join();
    Check(closed && One(other, 1).first.status == EP_BRIDGE_INVALID_HANDLE, "idle session closes on other thread");
    auto throws = ManagedHost::Load(Entry(options, L"ThrowDisposeEntry")).Value()->Open(std::make_shared<Native>()).Value();
    ErrorIs(throws->Close(), "managed.bridge.managed_exception");
    Check(throws->Close().HasValue() && One(throws, 1).first.status == EP_BRIDGE_INVALID_HANDLE, "Dispose failure still invalidates session");
}
struct Owner { std::int64_t bias = 10; std::int64_t calls = 0; };
foundation::Result<std::int64_t> SchemaCallback(Owner &owner, const std::int64_t &value) {
    if (value == 13) return foundation::Result<std::int64_t>::Failure(foundation::Error::Create("schema.native.denied", "denied"));
    return foundation::Result<std::int64_t>::Success(value + owner.bias + ++owner.calls);
}
void GraphRoundTrip(const HostOptions &options) {
    auto registry = std::make_shared<schema::NativeSchemaRegistry>();
    const auto owner = registry->RegisterType<Owner>("managed.native.owner").Value();
    (void)registry->RegisterType<std::int64_t>(graph::kI64TypeName).Value();
    const auto operation = registry->RegisterOperation(owner, "callback", &SchemaCallback).Value();
    const auto native = CreateSchemaGraphCallable(registry, {owner, operation}, Owner{}).Value();
    const auto callable = CreateManagedGraphCallable(ManagedHost::Load(options).Value(), native).Value();
    using namespace graph;
    const auto id = [](std::string_view value) { return ElementId::FromString(value); };
    GraphDefinition definition{AssetId::FromString("managed.graph"), "Managed", {}, {}};
    definition.nodes = {
        {id("input"), id("input.type"), "Input", {{id("input.out"), "Value", PortKind::Data, PortDirection::Output, kI64Type}}},
        {id("managed"), id("managed.type"), "Managed", {{id("managed.in"), "Input", PortKind::Data, PortDirection::Input, kI64Type},
                                                        {id("managed.out"), "Result", PortKind::Data, PortDirection::Output, kI64Type}}},
        {id("output"), id("output.type"), "Output", {{id("output.in"), "Value", PortKind::Data, PortDirection::Input, kI64Type}}}};
    definition.connections = {{id("edge1"), id("input.out"), id("managed.in")}, {id("edge2"), id("managed.out"), id("output.in")}};
    const std::array bindings{NodeBinding{id("input.type"), Operation::Input, {}}, NodeBinding{id("managed.type"), Operation::NativeCall, callable},
                              NodeBinding{id("output.type"), Operation::Output, {}}};
    const auto program = CompileGraph(definition, bindings).Value();
    auto first = GraphInstance::Create(program).Value(); auto second = GraphInstance::Create(program).Value();
    Check(first->SetInput(id("input.out"), 3).HasValue() && second->SetInput(id("input.out"), 4).HasValue(), "graph inputs");
    Check(first->Run().status == ExecutionStatus::Completed && second->Run().status == ExecutionStatus::Completed &&
          first->ReadValue(id("output.in")).Value() == 29 && second->ReadValue(id("output.in")).Value() == 31,
          "one compiled graph, independent managed/native sessions, real Schema callbacks");
    auto denied = GraphInstance::Create(program).Value(); Check(denied->SetInput(id("input.out"), 13).HasValue(), "negative graph input");
    const auto failure = denied->Run();
    Check(failure.status == ExecutionStatus::Failed && failure.diagnostic && failure.diagnostic->node == id("managed") &&
          failure.diagnostic->error.HasCode("schema.native.denied"), "Graph diagnostic retains original native error through managed layer");
}
void BatchAndProtocol(const HostOptions &options) {
    ErrorIs(ManagedHost::Load(Entry(options, L"BadAbiEntry")), "managed.bridge.abi_mismatch");
    ErrorIs(ManagedHost::Load(Entry(options, L"Missing")), "managed.bridge.entry_load");
    auto missing = options; missing.assembly.clear(); ErrorIs(ManagedHost::Load(missing), "managed.bridge.invalid_argument");
    auto host = ManagedHost::Load(Entry(options, L"BatchEntry")).Value();
    auto native = std::make_shared<Native>();
    auto session = host->Open(native).Value();
    const std::array<std::int64_t, 4> input{1, 2, 3, 4}; std::array<std::int64_t, 4> output{};
    Check(session->InvokeBatch(input, output).status == EP_BRIDGE_OK && output == std::array<std::int64_t, 4>{2, 3, 4, 5} && native->calls == 4,
          "managed batch forwards native batch");
    auto malformed = ManagedHost::Load(Entry(options, L"BadResponseEntry")).Value()->Open(std::make_shared<Native>()).Value();
    const auto rejected = One(malformed, 1).first;
    Check(rejected.status == EP_BRIDGE_INVALID_RESPONSE && rejected.completed == 0, "untrusted completion count rejected");
}
} // namespace
int wmain(int argc, wchar_t **argv) {
    if (argc != 4) return 64;
    try {
        const auto options = Entry(HostOptions{argv[1], argv[2], argv[3], {}}, L"Entry");
        CallsAndFailures(options); LifetimeAndReentry(options); GraphRoundTrip(options); BatchAndProtocol(options); return 0;
    } catch (const std::exception &error) { std::fputs(error.what(), stderr); std::fputc('\n', stderr); return 1; }
}

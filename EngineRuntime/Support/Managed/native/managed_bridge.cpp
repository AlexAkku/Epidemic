#include "Epidemic/Runtime/Support/managed_bridge.h"

#include <Windows.h>
#include <hostfxr.h>
#include <coreclr_delegates.h>
#include <atomic>
#include <mutex>
#include <thread>
#include <type_traits>

namespace epidemic::runtime::managed {
static_assert(sizeof(EpBridgeNativeApi) == 24 && sizeof(EpBridgeManagedApi) == 32);
static_assert(std::is_standard_layout_v<EpBridgeNativeApi> &&
              std::is_standard_layout_v<EpBridgeManagedApi>);

namespace {
foundation::Error Error(std::int32_t status) {
    static constexpr std::string_view codes[] =
        {"ok",           "invalid_argument",  "abi_mismatch",    "invalid_handle",   "busy",
         "wrong_thread", "managed_exception", "native_failure",  "native_exception", "overflow",
         "capacity",     "invalid_state",     "invalid_response"};

    const auto code = status >= 0 && status <= EP_BRIDGE_INVALID_RESPONSE
                          ? codes[status]
                          : codes[EP_BRIDGE_INVALID_RESPONSE];
    return foundation::Error::Create("managed.bridge." + std::string(code),
                                     "managed bridge call failed");
}

BatchReport Rejected(std::int32_t status) {
    return {status, 0, Error(status)};

}

bool BuffersValid(const std::int64_t *input, std::int64_t *output, std::size_t count) {
    if (count == 0)
        return true;
    if (!input || !output || count > EP_BRIDGE_MAX_BATCH)
        return false;
    const auto a = reinterpret_cast<std::uintptr_t>(input);
    const auto b = reinterpret_cast<std::uintptr_t>(output);
    return a == b || (a < b ? b - a : a - b) >= count * sizeof(std::int64_t);
}

struct CallbackState {
    std::shared_ptr<graph::INativeInvocation> invocation;
    std::thread::id thread = std::this_thread::get_id();
    std::atomic_bool active{false};

    std::optional<foundation::Error> error;
};

std::int32_t EP_BRIDGE_CALL NativeBatch(void *context, const std::int64_t *input,
                                        std::int64_t *output, std::uint32_t count,
                                        std::uint32_t *completed) noexcept {
    if (!completed)
        return EP_BRIDGE_INVALID_ARGUMENT;
    *completed = 0;
    if (!context || count > EP_BRIDGE_MAX_BATCH || !BuffersValid(input, output, count))
        return EP_BRIDGE_INVALID_ARGUMENT;
    auto &state = *static_cast<CallbackState *>(context);
    if (std::this_thread::get_id() != state.thread)
        return EP_BRIDGE_WRONG_THREAD;
    if (!state.active.load())
        return EP_BRIDGE_INVALID_STATE;
    try {
        for (std::uint32_t index = 0; index < count; ++index) {
            const auto value = state.invocation->Invoke(input[index]);
            if (!value) {
                state.error = value.GetError();
                return EP_BRIDGE_NATIVE_FAILURE;
            }

            output[index] = value.Value();
            ++*completed;
        }

        return EP_BRIDGE_OK;
    } catch (...) {
        try {
            state.error = Error(EP_BRIDGE_NATIVE_EXCEPTION);
        } catch (...) {
            return EP_BRIDGE_NATIVE_EXCEPTION;
        } // Diagnostic allocation cannot escape the C callback.

        return EP_BRIDGE_NATIVE_EXCEPTION;
    }

}

template <typename T> T Export(HMODULE module, const char *name) {
    return reinterpret_cast<T>(GetProcAddress(module, name));
}

struct HostContext {
    hostfxr_handle handle = nullptr;
    hostfxr_close_fn close = nullptr;

    ~HostContext() {
        if (handle)
            close(handle);
    }

};

bool File(const std::filesystem::path &path) {
    std::error_code error;
    return path.is_absolute() && std::filesystem::is_regular_file(path, error) && !error;
}

} // namespace

struct ManagedSession::State {
    std::shared_ptr<ManagedHost> host;
    EpBridgeManagedApi api{};

    std::unique_ptr<CallbackState> callback;
    std::mutex gate;
    std::uint64_t handle = 0;
    bool busy = false;
    bool closed = false;

    ~State() {
        if (handle && !closed)
            (void)api.close(handle);
    }

};

foundation::Result<std::shared_ptr<ManagedHost>> ManagedHost::Load(const HostOptions &options) {
    using Result = foundation::Result<std::shared_ptr<ManagedHost>>;
    if (!File(options.hostfxr) || !File(options.runtime_config) || !File(options.assembly) ||
        options.entry_type.empty())
        return Result::Failure(Error(EP_BRIDGE_INVALID_ARGUMENT));
    const auto module =
        LoadLibraryExW(options.hostfxr.c_str(), nullptr,
                       LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR | LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module)
        return Result::Failure(
            foundation::Error::Create("managed.bridge.host_load", "cannot load hostfxr"));
    HMODULE pinned = nullptr;
    const bool retained =
        GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_PIN, options.hostfxr.c_str(), &pinned) != 0;
    // CLR and managed entry points remain process resident. Balance this loader
    // reference, but never pretend that a host context close unloads the runtime.
    FreeLibrary(module);
    if (!retained)
        return Result::Failure(
            foundation::Error::Create("managed.bridge.host_pin", "cannot retain hostfxr"));
    const auto initialize =
        Export<hostfxr_initialize_for_runtime_config_fn>(pinned,
                                                         "hostfxr_initialize_for_runtime_config");
    const auto get_delegate =
        Export<hostfxr_get_runtime_delegate_fn>(pinned, "hostfxr_get_runtime_delegate");
    const auto close = Export<hostfxr_close_fn>(pinned, "hostfxr_close");
    if (!initialize || !get_delegate || !close)
        return Result::Failure(Error(EP_BRIDGE_ABI_MISMATCH));
    HostContext context{nullptr, close};

    const auto initialized = initialize(options.runtime_config.c_str(), nullptr, &context.handle);
    if (initialized < 0 || !context.handle)
        return Result::Failure(foundation::Error::Create("managed.bridge.runtime_config",
                                                         "hostfxr initialization failed"));
    void *raw_loader = nullptr;
    if (get_delegate(context.handle, hdt_load_assembly_and_get_function_pointer, &raw_loader) !=
            0 ||
        !raw_loader)
        return Result::Failure(Error(EP_BRIDGE_ABI_MISMATCH));
    const auto loader = reinterpret_cast<load_assembly_and_get_function_pointer_fn>(raw_loader);
    void *entry = nullptr;
    if (loader(options.assembly.c_str(), options.entry_type.c_str(), L"GetApi",
               UNMANAGEDCALLERSONLY_METHOD, nullptr, &entry) != 0 ||
        !entry)
        return Result::Failure(foundation::Error::Create("managed.bridge.entry_load",
                                                         "managed GetApi entry was not loaded"));
    EpBridgeManagedApi api{};

    const auto status =
        reinterpret_cast<EpBridgeGetApi>(entry)(EP_BRIDGE_ABI_VERSION, sizeof(api), &api);
    if (status != EP_BRIDGE_OK)
        return Result::Failure(Error(status));
    if (api.version != EP_BRIDGE_ABI_VERSION || api.size < sizeof(api) || !api.open ||
        !api.invoke_batch || !api.close)
        return Result::Failure(Error(EP_BRIDGE_ABI_MISMATCH));
    return Result::Success(std::shared_ptr<ManagedHost>(new ManagedHost(api)));
}

foundation::Result<std::shared_ptr<ManagedSession>>
ManagedHost::Open(std::shared_ptr<graph::INativeInvocation> native) {
    using Result = foundation::Result<std::shared_ptr<ManagedSession>>;
    if (!native)
        return Result::Failure(Error(EP_BRIDGE_INVALID_ARGUMENT));
    auto state = std::make_shared<ManagedSession::State>();
    state->host = shared_from_this();
    state->api = api_;
    state->callback = std::make_unique<CallbackState>();
    state->callback->invocation = std::move(native);
    const EpBridgeNativeApi callbacks{sizeof(EpBridgeNativeApi), EP_BRIDGE_ABI_VERSION,
                                      state->callback.get(), &NativeBatch};

    const auto status = api_.open(&callbacks, &state->handle);
    if (status != EP_BRIDGE_OK || state->handle == 0)
        return Result::Failure(Error(status == EP_BRIDGE_OK ? EP_BRIDGE_INVALID_RESPONSE : status));
    return Result::Success(std::shared_ptr<ManagedSession>(new ManagedSession(std::move(state))));
}

ManagedSession::~ManagedSession() = default;

BatchReport ManagedSession::InvokeBatch(std::span<const std::int64_t> input,
                                        std::span<std::int64_t> output) {
    const auto self = shared_from_this(); // Keeps this object alive across a native callback.
    const auto state = state_;
    std::unique_lock lock(state->gate);
    if (state->closed)
        return Rejected(EP_BRIDGE_INVALID_HANDLE);
    if (std::this_thread::get_id() != state->callback->thread)
        return Rejected(EP_BRIDGE_WRONG_THREAD);
    if (state->busy)
        return Rejected(EP_BRIDGE_BUSY);
    if (input.size() != output.size() || input.size() > EP_BRIDGE_MAX_BATCH ||
        !BuffersValid(input.data(), output.data(), input.size()))
        return Rejected(EP_BRIDGE_INVALID_ARGUMENT);
    state->busy = true;
    state->callback->error.reset();
    state->callback->active.store(true);
    lock.unlock();

    struct Guard {
        std::shared_ptr<State> state;

        ~Guard() {
            std::lock_guard guard(state->gate);
            state->callback->active.store(false);
            state->busy = false;
        }

    } guard{state};

    BatchReport report;
    report.status =
        state->api.invoke_batch(state->handle, input.data(), output.data(),
                                static_cast<std::uint32_t>(input.size()), &report.completed);
    if (report.status < EP_BRIDGE_OK || report.status > EP_BRIDGE_INVALID_RESPONSE ||
        report.completed > input.size() ||
        (report.status == EP_BRIDGE_OK && report.completed != input.size())) {
        report.status = EP_BRIDGE_INVALID_RESPONSE;
        report.completed = 0;
    }

    if (report.status != EP_BRIDGE_OK) {
        if ((report.status == EP_BRIDGE_NATIVE_FAILURE ||
             report.status == EP_BRIDGE_NATIVE_EXCEPTION) &&
            state->callback->error)
            report.diagnostic = state->callback->error;
        else
            report.diagnostic = Error(report.status);
    }

    return report;
}

foundation::Result<void> ManagedSession::Close() {
    const auto state = state_;
    std::lock_guard lock(state->gate);
    if (state->closed)
        return foundation::Result<void>::Success();
    if (state->busy)
        return foundation::Result<void>::Failure(Error(EP_BRIDGE_BUSY));
    const auto status = state->api.close(state->handle);
    state->closed = true;
    state->handle = 0;
    state->callback.reset();
    return status == EP_BRIDGE_OK ? foundation::Result<void>::Success()
                                  : foundation::Result<void>::Failure(Error(status));
}

namespace {
class Invocation final : public graph::INativeInvocation {
  public:
    explicit Invocation(std::shared_ptr<ManagedSession> session) : session_(std::move(session)) {
    }

    foundation::Result<std::int64_t> Invoke(std::int64_t argument) override {
        std::int64_t output = 0;
        const auto report = session_->InvokeBatch({&argument, 1}, {&output, 1});
        if (report.status != EP_BRIDGE_OK) {
            if (report.diagnostic)
                return foundation::Result<std::int64_t>::Failure(*report.diagnostic);
            return foundation::Result<std::int64_t>::Failure(Error(EP_BRIDGE_INVALID_RESPONSE));
        }

        return foundation::Result<std::int64_t>::Success(output);
    }

  private:
    std::shared_ptr<ManagedSession> session_;
};

class Factory final : public graph::INativeCallableFactory {
  public:
    Factory(std::shared_ptr<ManagedHost> host,
            std::shared_ptr<const graph::INativeCallableFactory> native)
        : host_(std::move(host)), native_(std::move(native)) {
    }

    foundation::Result<std::unique_ptr<graph::INativeInvocation>>
    CreateInvocation() const override {
        using Result = foundation::Result<std::unique_ptr<graph::INativeInvocation>>;
        auto native = native_->CreateInvocation();
        if (!native)
            return Result::Failure(native.GetError());
        auto session =
            host_->Open(std::shared_ptr<graph::INativeInvocation>(std::move(native).Value()));
        if (!session)
            return Result::Failure(session.GetError());
        return Result::Success(std::make_unique<Invocation>(std::move(session).Value()));
    }

  private:
    std::shared_ptr<ManagedHost> host_;
    std::shared_ptr<const graph::INativeCallableFactory> native_;
};

} // namespace

foundation::Result<std::shared_ptr<const graph::INativeCallableFactory>>
CreateManagedGraphCallable(std::shared_ptr<ManagedHost> host,
                           std::shared_ptr<const graph::INativeCallableFactory> native) {
    using Result = foundation::Result<std::shared_ptr<const graph::INativeCallableFactory>>;
    if (!host || !native)
        return Result::Failure(Error(EP_BRIDGE_INVALID_ARGUMENT));
    return Result::Success(std::make_shared<Factory>(std::move(host), std::move(native)));
}

} // namespace epidemic::runtime::managed

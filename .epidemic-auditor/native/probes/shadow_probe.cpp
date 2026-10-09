#include "shadow_probe.h"
// Optional MS6.5/6.6 engine-side observation. NOT external evidence.
// Production builds compile all calls to zero unless explicitly opted in.
#include <cstdint>

#if defined(EP_AUDITOR_SHADOW_PROBES) && EP_AUDITOR_SHADOW_PROBES
#include <atomic>
#include <array>
#include <cstring>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <functional>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <stdexcept>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#else
#include <unistd.h>
#endif

#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
extern "C" bool auditor_exact_current_call(std::uint64_t*, std::uint64_t*) noexcept;
#endif
namespace epidemic::auditor::shadow {
namespace detail {
thread_local std::string active_action;
thread_local Context active_context;
thread_local Identity armed_action;
thread_local std::string armed_name;
thread_local Identity automatic_action;
thread_local bool armed_automatic = false;
std::atomic<bool> configured_arm_claimed{false};
std::atomic<std::uint64_t> identity_sequence{1};
std::uint64_t pid() noexcept {
#ifdef _WIN32
    return GetCurrentProcessId();
#else
    return static_cast<std::uint64_t>(getpid());
#endif
}
std::uint64_t tid() noexcept {
#ifdef _WIN32
    return GetCurrentThreadId();
#else
    return std::hash<std::thread::id>{}(std::this_thread::get_id());
#endif
}
std::uint64_t clock_ns() noexcept {
#ifdef _WIN32
    LARGE_INTEGER value{}, frequency{};
    if (!QueryPerformanceCounter(&value) || !QueryPerformanceFrequency(&frequency) || !frequency.QuadPart) return 0;
    const auto whole = static_cast<std::uint64_t>(value.QuadPart) / static_cast<std::uint64_t>(frequency.QuadPart);
    const auto fraction = static_cast<std::uint64_t>(value.QuadPart) % static_cast<std::uint64_t>(frequency.QuadPart);
    return whole * 1000000000ull + fraction * 1000000000ull / static_cast<std::uint64_t>(frequency.QuadPart);
#else
    return static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
        std::chrono::steady_clock::now().time_since_epoch()).count());
#endif
}
std::uint64_t process_epoch() noexcept {
#ifdef _WIN32
    // Stable across EXE and DLL implementations of this header; unlike a
    // function-local QPC epoch, this survives crossing module boundaries.
    static const auto epoch = []() -> std::uint64_t {
        FILETIME created{}, exited{}, kernel{}, user{};
        if (GetProcessTimes(GetCurrentProcess(), &created, &exited, &kernel, &user))
            return (static_cast<std::uint64_t>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
        return 0; // Unavailable identity is loss, never a fabricated module-local epoch.
    }();
    return epoch;
#else
    static const auto fallback_epoch = clock_ns();
    return fallback_epoch;
#endif
}
bool safe(const std::string &s) noexcept {
    if (s.empty() || s.size() > 128) return false;
    for (const unsigned char c : s)
        if (!(c >= 'a' && c <= 'z') && !(c >= 'A' && c <= 'Z') &&
            !(c >= '0' && c <= '9') && c != '_' && c != '.' && c != '-' && c != ':') return false;
    return true;
}
std::string environment(const char *name) {
#ifdef _WIN32
    char *value = nullptr; size_t size = 0;
    if (_dupenv_s(&value, &size, name) || !value) return {};
    std::string result(value);
    free(value);
    return result;
#else
    const auto *value = std::getenv(name);
    return value ? value : "";
#endif
}
class Sink {
    std::mutex lock_;
#ifdef _WIN32
    HANDLE data_ = INVALID_HANDLE_VALUE;
#else
    std::ofstream data_;
#endif
    std::filesystem::path health_;
    std::uint64_t emitted_ = 0;
    std::uint64_t dropped_ = 0;
    std::uint64_t nonce_ = 0;
    std::uint64_t bytes_ = 0;
    static constexpr std::uint64_t MAX_BYTES = 8ull * 1024 * 1024;
    static constexpr std::uint64_t MAX_EVENTS = 100000;
public:
    explicit Sink(const char *env) noexcept {
        try {
            const auto directory = environment(env);
            if (directory.empty()) return;
            const auto root = std::filesystem::path(directory);
            // The parent runner creates/owns this directory. Do not create arbitrary paths.
            if (!std::filesystem::is_directory(root) || std::filesystem::is_symlink(root)) return;
#ifdef _WIN32
            const auto attributes = GetFileAttributesW(root.c_str());
            if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT)) return;
#endif
            nonce_ = clock_ns();
#ifdef _WIN32
            // EXE and DLL sinks can be initialized in the same clock tick. Never
            // truncate an existing producer. The exclusive filename is EventId scope.
            for (unsigned attempt = 0; attempt < 128; ++attempt, ++nonce_) {
                const auto basename = std::to_string(pid()) + "-" + std::to_string(nonce_);
                const auto path = root / (basename + ".jsonl");
                data_ = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW,
                                    FILE_ATTRIBUTE_NORMAL, nullptr);
                if (data_ != INVALID_HANDLE_VALUE) { health_ = root / (basename + ".health.json"); break; }
                if (GetLastError() != ERROR_FILE_EXISTS && GetLastError() != ERROR_ALREADY_EXISTS) break;
            }
#else
            const auto basename = std::to_string(pid()) + "-" + std::to_string(nonce_);
            const auto path = root / (basename + ".jsonl");
            health_ = root / (basename + ".health.json");
            data_.open(path, std::ios::binary | std::ios::trunc);
#endif
        } catch (...) { }
    }
    Sink(const Sink&) = delete;
    Sink& operator=(const Sink&) = delete;
    ~Sink() noexcept {
        std::lock_guard<std::mutex> guard(lock_);
#ifdef _WIN32
        if (data_ == INVALID_HANDLE_VALUE) return;
        const bool flushed = FlushFileBuffers(data_) != FALSE;
        const bool closed = CloseHandle(data_) != FALSE;
        const bool complete = flushed && closed;
        data_ = INVALID_HANDLE_VALUE;
#else
        if (!data_.is_open()) return;
        data_.flush();
        const bool complete = static_cast<bool>(data_);
        data_.close();
#endif
        try {
#ifdef _WIN32
            std::ostringstream h;
#else
            std::ofstream h(health_, std::ios::binary | std::ios::trunc);
#endif
            h << "{\"schemaVersion\":1,\"emitted\":" << emitted_
              << ",\"dropped\":" << dropped_ << ",\"complete\":"
              << (complete && !dropped_ ? "true" : "false") << "}\n";
#ifdef _WIN32
            const auto payload = h.str();
            const auto health = CreateFileW(health_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                                           FILE_ATTRIBUTE_NORMAL, nullptr);
            if (health != INVALID_HANDLE_VALUE) {
                DWORD written = 0;
                WriteFile(health, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr);
                FlushFileBuffers(health);
                CloseHandle(health);
            }
#endif
        } catch (...) { }
    }
    bool opened() const noexcept {
#ifdef _WIN32
        return data_ != INVALID_HANDLE_VALUE;
#else
        return data_.is_open();
#endif
    }
    void reject() noexcept {
        std::lock_guard<std::mutex> guard(lock_);
        if (opened()) ++dropped_;
    }
    void write(const char *kind, const std::string &fields, Identity* emitted_id = nullptr) noexcept {
        std::lock_guard<std::mutex> guard(lock_);
        if (!opened()) return;
        try {
            if (!safe(kind)) { ++dropped_; return; }
            if (emitted_ >= MAX_EVENTS || bytes_ >= MAX_BYTES) { ++dropped_; return; }
            const auto timestamp = clock_ns(), epoch = process_epoch();
            if (!timestamp || !epoch) { ++dropped_; return; }
            const auto id = std::to_string(pid()) + ":" + std::to_string(nonce_) + ":" + std::to_string(emitted_ + 1);
            std::ostringstream line;
            line << "{\"schemaVersion\":1,\"type\":\"" << kind
                 << "\",\"event_id\":\"" << id
                 << "\",\"timestamp_ns\":" << timestamp
                 << ",\"pid\":" << pid() << ",\"process_epoch\":" << epoch << ",\"tid\":" << tid()
                 << ",\"clock_domain\":\"monotonic_trace_ns\",\"provenance\":\"engine_probe\""
                 << fields << "}\n";
            const auto payload = line.str();
            if (payload.size() > 65536 || payload.size() > MAX_BYTES - bytes_) { ++dropped_; return; }
#ifdef _WIN32
            DWORD written = 0;
            if (!WriteFile(data_, payload.data(), static_cast<DWORD>(payload.size()), &written, nullptr)
                || written != payload.size()) { ++dropped_; return; }
#else
            data_.write(payload.data(), static_cast<std::streamsize>(payload.size()));
            if (!data_) { ++dropped_; return; }
#endif
            ++emitted_;
            bytes_ += payload.size();
            if (emitted_id && id.size() < emitted_id->size()) {
                std::memcpy(emitted_id->data(), id.c_str(), id.size() + 1);
            }
        } catch (...) { ++dropped_; }
    }
};
Sink &causal() { static Sink sink("EP_AUDITOR_CAUSAL_DIRECTORY"); return sink; }
Sink &diagnostic() { static Sink sink("EP_AUDITOR_DIAGNOSTICS_DIRECTORY"); return sink; }
void causal_loss() noexcept { try { causal().reject(); } catch (...) {} }
void diagnostic_loss() noexcept { try { diagnostic().reject(); } catch (...) {} }
}  // namespace detail

Identity identity() noexcept {
    Identity result{};
    try {
        const auto sequence = detail::identity_sequence.fetch_add(1, std::memory_order_relaxed);
        if (!sequence) { detail::causal_loss(); return result; }
        const auto text = std::to_string(detail::pid()) + ':' + std::to_string(detail::process_epoch())
            + ':' + std::to_string(detail::clock_ns()) + ':' + std::to_string(sequence);
        if (text.size() >= result.size()) { detail::causal_loss(); return result; }
        std::memcpy(result.data(), text.c_str(), text.size() + 1);
    } catch (...) { detail::causal_loss(); }
    return result;
}
Context context() noexcept {
    if (!detail::active_context.trace[0]) {
        detail::active_context.trace = identity();
        detail::active_context.span = identity();
    }
    return detail::active_context;
}
Context child_context() noexcept {
    auto result = context();
    result.parent_span = result.span;
    result.span = identity();
    return result;
}
ContextScope::ContextScope(const Context& next) noexcept : previous_(context()) {
    try {
        previous_action_ = detail::active_action;
        detail::active_action = next.action.data();
    } catch (...) { detail::causal_loss(); }
    detail::active_context = next;
}
ContextScope::~ContextScope() noexcept {
    detail::active_context = previous_;
    detail::active_action.swap(previous_action_);
}
std::string context_fields(bool include_action = true, bool include_scenario = true) {
    const auto current = context();
    std::string fields;
    const auto add = [&fields](const char* key, const Identity& value) {
        if (value[0]) fields += std::string(",\"") + key + "\":\"" + value.data() + '"';
    };
    add("trace_id", current.trace); add("span_id", current.span);
    add("parent_span_id", current.parent_span); add("parent_event_id", current.cause);
    if (include_action) add("action_id", current.action);
    if (include_scenario) add("scenario_id", current.scenario);
    return fields;
}
Identity emit_causal_fields(const char *kind, const char *correlation_id, const std::string& extra,
                           bool include_action = true, bool include_scenario = true) noexcept {
    Identity emitted{};
#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
    std::uint64_t ordinal = 0, producer_base = 0;
    const bool has_call = auditor_exact_current_call(&ordinal, &producer_base);
    // An exact-enabled producer must not silently downgrade a lost bridge to
    // a complete causal capture with absent CallIds.
    if (!has_call) detail::causal_loss();
#endif
    try {
        const std::string value = correlation_id ? correlation_id : "";
        if (!detail::safe(value)) { detail::causal_loss(); return emitted; }
        auto fields = context_fields(include_action, include_scenario) + ",\"correlation_id\":\"" + value + "\"" + extra;
#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
        if (has_call) fields += ",\"exact_call_ordinal\":" + std::to_string(ordinal)
            + ",\"exact_module_base\":" + std::to_string(producer_base);
#endif
        const std::string operation = kind ? kind : "";
        if (operation.compare(0, 4, "job.") == 0) fields += ",\"job_id\":\"" + value + "\"";
        if (operation.compare(0, 8, "command.") == 0) fields += ",\"command_id\":\"" + value + "\"";
        detail::causal().write(kind, fields, &emitted);
    } catch (...) { detail::causal_loss(); }
    return emitted;
}
Identity emit_causal(const char *kind, const char *correlation_id) noexcept {
    return emit_causal_fields(kind, correlation_id, {});
}
void entity_change(std::uint64_t high, std::uint64_t low, unsigned kind, unsigned before, unsigned after) noexcept {
    try {
        const auto id = "EntityId:" + std::to_string(high) + ':' + std::to_string(low);
        const auto extra = ",\"entity_id\":\"" + id + "\",\"entity_change_kind\":" + std::to_string(kind)
            + ",\"state_before\":" + std::to_string(before) + ",\"state_after\":" + std::to_string(after);
        // A committed journal observation, not an invented pre-mutation request.
        (void)emit_causal_fields(kind == 0 ? "entity.created" : "entity.lifecycle", id.c_str(), extra);
        if (before != after) (void)emit_causal_fields("state.changed", id.c_str(), extra);
    } catch (...) { detail::causal_loss(); }
}
void causal(const char *kind, const char *correlation_id) noexcept {
    (void)emit_causal(kind, correlation_id);
}
Identity emit_causal(const char *kind, std::uint64_t correlation_id) noexcept {
    try { return emit_causal(kind, std::to_string(correlation_id).c_str()); }
    catch (...) { detail::causal_loss(); return {}; }
}
void flow(const char* kind, const char* correlation_id) noexcept {
    detail::active_context.cause = emit_causal(kind, correlation_id);
}
void flow(const char* kind, std::uint64_t correlation_id) noexcept {
    detail::active_context.cause = emit_causal(kind, correlation_id);
}
void causal(const char *kind, std::uint64_t correlation_id) noexcept {
    try { causal(kind, std::to_string(correlation_id).c_str()); } catch (...) { detail::causal_loss(); }
}
std::string quoted_name(const char* name, const std::string& fallback) {
    const std::string value = name ? name : fallback;
    if (value.empty() || value.size() > 256) throw std::length_error("action name exceeds bound");
    std::string escaped;
    for (const auto ch : value) {
        const auto byte = static_cast<unsigned char>(ch);
        if (byte < 32) throw std::invalid_argument("action name contains control character");
        if (ch == '"' || ch == '\\') escaped += '\\';
        escaped += ch;
    }
    return escaped;
}
void action(const char *kind, const char *action_id, const char* name) noexcept {
    try {
        const std::string value = action_id ? action_id : "";
        if (!detail::safe(value)) { detail::causal_loss(); return; }
        const std::string operation = kind ? kind : "";
        if ((operation == "action.start" && !detail::active_action.empty())
            || (operation == "action.end" && detail::active_action != value)) { detail::causal_loss(); return; }
        (void)emit_causal_fields(kind, value.c_str(), ",\"action_id\":\"" + value
            + "\",\"name\":\"" + quoted_name(name, value) + "\"", false);
        if (operation == "action.start") {
            detail::active_action = value;
            std::memcpy(detail::active_context.action.data(), value.c_str(), value.size() + 1);
        } else if (operation == "action.end") {
            detail::active_action.clear(); detail::active_context.action = {};
            if (value == detail::automatic_action.data()) detail::automatic_action = {};
        }
    } catch (...) { detail::causal_loss(); }
}
void arm_next_input(const char* action_id, const char* name) noexcept {
    try {
        const std::string value = action_id ? action_id : "";
        if (!detail::safe(value) || detail::armed_action[0] || !detail::active_action.empty()) {
            detail::causal_loss(); return;
        }
        detail::armed_name = name ? name : value;
        (void)quoted_name(detail::armed_name.c_str(), value);
        std::memcpy(detail::armed_action.data(), value.c_str(), value.size() + 1);
    } catch (...) { detail::causal_loss(); }
}
void input(const char* input_id) noexcept {
    try {
        if (!detail::armed_action[0] && detail::active_action.empty() && !detail::configured_arm_claimed.load()) {
            const auto configured = detail::environment("EP_AUDITOR_ARM_NEXT_INPUT_ID");
            const auto name = detail::environment("EP_AUDITOR_ARM_NEXT_INPUT_NAME");
            bool expected = false;
            if (!configured.empty() && detail::configured_arm_claimed.compare_exchange_strong(expected, true)) {
                arm_next_input(configured.c_str(), name.empty() ? nullptr : name.c_str());
                detail::armed_automatic = true;
            }
        }
        Identity accepted{};
        auto independent = context();
        independent.cause = {}; // Separate user inputs are not caused by earlier inputs.
        {
            const ContextScope scope(independent);
            accepted = emit_causal("input.accepted", input_id);
        }
        if (detail::armed_action[0]) {
            detail::active_context.cause = accepted;
            action("action.start", detail::armed_action.data(), detail::armed_name.c_str());
            if (detail::armed_automatic) detail::automatic_action = detail::armed_action;
            detail::armed_automatic = false;
            detail::armed_action = {}; detail::armed_name.clear();
        }
    } catch (...) { detail::causal_loss(); }
}
void frame(std::uint64_t duration_ns) noexcept {
    try {
        const auto configured = detail::environment("EP_AUDITOR_LONG_FRAME_NS");
        if (!configured.empty() && configured.find_first_not_of("0123456789") != std::string::npos) {
            detail::causal_loss(); return;
        }
        const auto threshold = configured.empty() ? 200000000ull : std::stoull(configured);
        if (!threshold || threshold > 60000000000ull) { detail::causal_loss(); return; }
        if (duration_ns >= threshold) {
            (void)emit_causal_fields("capture.trigger", "long_frame", ",\"trigger_id\":\"long_frame\",\"duration_ns\":"
                + std::to_string(duration_ns));
        }
        if (detail::automatic_action[0]) action("action.end", detail::automatic_action.data());
    } catch (...) { detail::causal_loss(); }
}
void scenario(const char *kind, const char *scenario_id) noexcept {
    try {
        const std::string value = scenario_id ? scenario_id : "";
        if (!detail::safe(value)) { detail::causal_loss(); return; }
        const std::string operation = kind ? kind : "";
        auto& active = detail::active_context.scenario;
        if ((operation == "scenario.start" && active[0]) || (operation == "scenario.end" && value != active.data())) {
            detail::causal_loss(); return;
        }
        (void)emit_causal_fields(kind, value.c_str(), ",\"scenario_id\":\"" + value + "\"", true, false);
        if (operation == "scenario.start") std::memcpy(active.data(), value.c_str(), value.size() + 1);
        else if (operation == "scenario.end") active = {};
    } catch (...) { detail::causal_loss(); }
}
void write_diagnostic(const char* kind, std::string fields) noexcept {
#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
    std::uint64_t ordinal = 0, producer_base = 0;
    if (auditor_exact_current_call(&ordinal, &producer_base)) {
        try {
            fields += ",\"exact_call_ordinal\":" + std::to_string(ordinal)
                + ",\"exact_module_base\":" + std::to_string(producer_base);
        } catch (...) { detail::diagnostic_loss(); return; }
    } else detail::diagnostic_loss();
#endif
    detail::diagnostic().write(kind, fields);
}
void io(const char *kind, const char *operation_id, std::uint64_t bytes, std::uint64_t duration_ns) noexcept {
    try {
        const std::string value = operation_id ? operation_id : "";
        if (!detail::safe(value)) { detail::diagnostic_loss(); return; }
        std::string fields = context_fields(false) + ",\"operation_id\":\"" + value + "\",\"bytes\":" +
                             std::to_string(bytes) + ",\"duration_ns\":" + std::to_string(duration_ns);
        if (!detail::active_action.empty()) fields += ",\"action_id\":\"" + detail::active_action + "\"";
#ifdef _WIN32
        void *frames[32]{}; const auto count = CaptureStackBackTrace(1,32,frames,nullptr);
        fields += ",\"stack\":[";
        for (unsigned index = 0; index < count; ++index) {
            if (index) fields += ',';
            fields += std::to_string(reinterpret_cast<std::uintptr_t>(frames[index]));
        }
        fields += ']';
#endif
        write_diagnostic(kind, std::move(fields));
    } catch (...) { detail::diagnostic_loss(); }
}
void allocation(const char *kind, std::uint64_t address, std::uint64_t size,
                       const char *function) noexcept {
    try {
        std::string fields = context_fields(false) + ",\"address\":" + std::to_string(address) + ",\"size_bytes\":" + std::to_string(size);
        if (!detail::active_action.empty()) fields += ",\"action_id\":\"" + detail::active_action + "\"";
        if (function) {
            if (!detail::safe(function)) { detail::diagnostic_loss(); return; }
            fields += ",\"function_id\":\"" + std::string(function) + "\"";
        }
#ifdef _WIN32
        void *frames[32]{};
        const auto count = CaptureStackBackTrace(1, 32, frames, nullptr);
        fields += ",\"stack\":[";
        for (unsigned index = 0; index < count; ++index) {
            if (index) fields += ',';
            fields += std::to_string(reinterpret_cast<std::uintptr_t>(frames[index]));
        }
        fields += ']';
#endif
        write_diagnostic(kind, std::move(fields));
    } catch (...) { detail::diagnostic_loss(); }
}
void exception(const char *kind, std::uint64_t code, std::uint64_t address) noexcept {
    try {
        write_diagnostic(kind, context_fields() + ",\"code\":" + std::to_string(code) +
                                   ",\"address\":" + std::to_string(address));
    } catch (...) { detail::diagnostic_loss(); }
}
} // namespace epidemic::auditor::shadow
#endif

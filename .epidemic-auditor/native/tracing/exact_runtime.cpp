#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cwchar>

// Published once after initialization. ASM reads the aligned flag atomically.
extern "C" {
volatile LONG auditor_exact_disabled = 0;
}

namespace {
#pragma pack(push, 1)
struct Header {
    char magic[8] = {'E', 'P', 'T', 'R', 'A', 'C', 'E', '2'};
    std::uint32_t version = 2, header_bytes = 0;
    std::uint64_t frequency = 0;
    std::uint32_t pid = 0;
    std::uint64_t birth = 0, image_base = 0, capacity = 0, written = 0, dropped = 0;
    std::uint32_t complete = 0, path_chars = 0, event_bytes = 72;
};
struct Event {
    std::uint64_t sequence, qpc, cpu_100ns, cycles, address, image_base, thread_birth;
    std::uint32_t pid, tid;
    std::uint8_t kind, reserved[7];
};
#pragma pack(pop)
static_assert(sizeof(Header) == 80 && sizeof(Event) == 72);
SRWLOCK lock = SRWLOCK_INIT;
bool initialized = false;
Header header;
Event *events = nullptr;
HANDLE output = INVALID_HANDLE_VALUE;
// Optional live compiler-event ring. The normal exact capture stays unchanged.
struct RollingHeader {
    char magic[8] = {'E', 'P', 'R', 'O', 'L', 'L', '0', '1'};
    std::uint64_t generation = 0, total = 0, capacity = 65536;
    std::uint64_t frequency = 0, pid = 0, birth = 0, image_base = 0;
};
static_assert(sizeof(RollingHeader) == 64);
RollingHeader *rolling = nullptr;
HANDLE rolling_file = INVALID_HANDLE_VALUE, rolling_mapping = nullptr;
wchar_t image_path[32768]{};
#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
struct ActiveCall {
    std::uint64_t ordinal, producer_base;
};
thread_local ActiveCall active_calls[1024]{};
thread_local std::size_t active_depth = 0;
thread_local bool active_call_unresolved = false;
#endif

std::uint64_t ticks(FILETIME value) noexcept {
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32u) | value.dwLowDateTime;
}
bool write_bytes(const void *bytes, DWORD count) noexcept {
    const auto *cursor = static_cast<const unsigned char *>(bytes);
    while (count) {
        DWORD written = 0;
        if (!WriteFile(output, cursor, count, &written, nullptr) || !written)
            return false;
        cursor += written;
        count -= written;
    }
    return true;
}
enum class CapturePathKind {
    exact,
    rolling
};
enum class Radix : std::uint8_t {
    decimal = 10,
    hexadecimal = 16
};
struct CapturePath {
    wchar_t value[32768]{};
    bool valid = false;
};
CapturePath make_capture_path(const wchar_t *directory, CapturePathKind kind) noexcept {
    // No CRT inline/template calls under the recorder lock: the linker may select
    // an engine COMDAT compiled with /Gh and recursively enter the recorder.
    CapturePath result{};
    std::size_t length = 0;
    const auto character = [&](wchar_t value) {
        if (length == 32767)
            return false;
        result.value[length++] = value;
        return true;
    };
    const auto text = [&](const wchar_t *value) {
        while (*value)
            if (!character(*value++))
                return false;
        return true;
    };
    const auto number = [&](std::uint64_t value, Radix radix) {
        wchar_t reversed[32]{};
        std::size_t count = 0;
        const auto base = static_cast<std::uint64_t>(radix);
        do {
            const auto digit = value % base;
            reversed[count++] = static_cast<wchar_t>(digit < 10 ? L'0' + digit : L'a' + digit - 10);
            value /= base;
        } while (value);
        while (count)
            if (!character(reversed[--count]))
                return false;
        return true;
    };
    result.valid = text(L"\\\\?\\") && text(directory) &&
                   text(kind == CapturePathKind::exact ? L"\\trace-" : L"\\events-") &&
                   number(header.pid, Radix::decimal) && character(L'-') && number(header.birth, Radix::decimal) &&
                   character(L'-') && number(header.image_base, Radix::hexadecimal) && text(L".bin");
    return result;
}
void flush() noexcept {
    AcquireSRWLockExclusive(&lock);
    if (output != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER position{};
        position.QuadPart = header.header_bytes;
        const bool complete = SetFilePointerEx(output, position, nullptr, FILE_BEGIN) &&
                              write_bytes(events, static_cast<DWORD>(header.written * sizeof(Event))) &&
                              FlushFileBuffers(output);
        header.complete = complete && header.dropped == 0 ? 1U : 0U;
        position.QuadPart = 0;
        if (!(SetFilePointerEx(output, position, nullptr, FILE_BEGIN) && write_bytes(&header, sizeof(header)) &&
              FlushFileBuffers(output))) {
            // A failed final commit must not leave a readable COMPLETE header.
            if (SetFilePointerEx(output, position, nullptr, FILE_BEGIN))
                SetEndOfFile(output);
        }
        CloseHandle(output);
        output = INVALID_HANDLE_VALUE;
    }
    if (rolling) {
        UnmapViewOfFile(rolling);
        rolling = nullptr;
    }
    if (rolling_mapping) {
        CloseHandle(rolling_mapping);
        rolling_mapping = nullptr;
    }
    if (rolling_file != INVALID_HANDLE_VALUE) {
        CloseHandle(rolling_file);
        rolling_file = INVALID_HANDLE_VALUE;
    }
    ReleaseSRWLockExclusive(&lock);
}
void initialize() noexcept {
    initialized = true;
    wchar_t directory[32768]{};
    const DWORD length = GetEnvironmentVariableW(L"EP_AUDITOR_TRACE_DIRECTORY", directory, 32768);
    if (!length || length > 32500 || directory[1] != L':')
        return;
    header.capacity = 262144;
    wchar_t capacity[32]{};
    if (GetEnvironmentVariableW(L"EP_AUDITOR_TRACE_CAPACITY", capacity, 32)) {
        wchar_t *end = nullptr;
        header.capacity = _wcstoui64(capacity, &end, 10);
        if (!end || *end || !header.capacity || header.capacity > 1048576)
            return;
    }
    LARGE_INTEGER frequency{};
    FILETIME born{}, ended{}, kernel{}, user{};
    HMODULE image = nullptr;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0 ||
        !GetProcessTimes(GetCurrentProcess(), &born, &ended, &kernel, &user) ||
        !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                            reinterpret_cast<LPCWSTR>(&initialize), &image))
        return;
    header.frequency = static_cast<std::uint64_t>(frequency.QuadPart);
    header.pid = GetCurrentProcessId();
    header.birth = ticks(born);
    header.image_base = reinterpret_cast<std::uint64_t>(image);
    header.path_chars = GetModuleFileNameW(image, image_path, 32768);
    if (!header.path_chars || header.path_chars >= 32768)
        return;
    header.header_bytes = sizeof(Header) + header.path_chars * sizeof(wchar_t);
    wchar_t rolling_directory[32768]{};
    const DWORD rolling_length = GetEnvironmentVariableW(L"EP_AUDITOR_ROLLING_DIRECTORY", rolling_directory, 32768);
    if (rolling_length && rolling_length <= 32500 && rolling_directory[1] == L':') {
        const auto rolling_path = make_capture_path(rolling_directory, CapturePathKind::rolling);
        if (rolling_path.valid) {
            rolling_file = CreateFileW(rolling_path.value, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                                       CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
            if (rolling_file != INVALID_HANDLE_VALUE) {
                constexpr DWORD bytes = sizeof(RollingHeader) + 65536 * sizeof(Event);
                rolling_mapping = CreateFileMappingW(rolling_file, nullptr, PAGE_READWRITE, 0, bytes, nullptr);
                if (rolling_mapping)
                    rolling = static_cast<RollingHeader *>(MapViewOfFile(rolling_mapping, FILE_MAP_WRITE, 0, 0, bytes));
                if (rolling) {
                    RollingHeader initial;
                    initial.frequency = header.frequency;
                    initial.pid = header.pid;
                    initial.birth = header.birth;
                    initial.image_base = header.image_base;
                    *rolling = initial;
                }
            }
        }
    }
    events = static_cast<Event *>(VirtualAlloc(nullptr, static_cast<SIZE_T>(header.capacity * sizeof(Event)),
                                               MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!events)
        return;
    const auto path = make_capture_path(directory, CapturePathKind::exact);
    if (!path.valid)
        return;
    output =
        CreateFileW(path.value, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE)
        return;
    if (!write_bytes(&header, sizeof(header)) || !write_bytes(image_path, header.path_chars * sizeof(wchar_t)) ||
        std::atexit(flush) != 0) {
        CloseHandle(output);
        output = INVALID_HANDLE_VALUE;
    }
}
enum class RecordKind : std::uint8_t {
    enter = 1,
    exit = 2
};
void record(std::uint64_t address, RecordKind kind) noexcept {
    AcquireSRWLockExclusive(&lock);
    if (!initialized) {
        initialize();
        if (output == INVALID_HANDLE_VALUE)
            InterlockedExchange(&auditor_exact_disabled, 1);
    }
    if (output != INVALID_HANDLE_VALUE) {
        {
            LARGE_INTEGER timestamp{};
            FILETIME born{}, ended{}, kernel{}, user{};
            ULONG64 cycles = 0;
            HMODULE image = nullptr;
            if (!QueryPerformanceCounter(&timestamp) ||
                !GetThreadTimes(GetCurrentThread(), &born, &ended, &kernel, &user) ||
                !QueryThreadCycleTime(GetCurrentThread(), &cycles) ||
                !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS |
                                        GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                                    reinterpret_cast<LPCWSTR>(address), &image))
                ++header.dropped;
            else {
                Event value{};
                value.sequence = header.written + header.dropped;
                value.qpc = static_cast<std::uint64_t>(timestamp.QuadPart);
                value.cpu_100ns = ticks(kernel) + ticks(user);
                value.cycles = cycles;
                value.address = address;
                value.image_base = reinterpret_cast<std::uint64_t>(image);
                value.thread_birth = ticks(born);
                value.pid = header.pid;
                value.tid = GetCurrentThreadId();
                value.kind = static_cast<std::uint8_t>(kind);
                if (rolling) {
                    InterlockedIncrement64(reinterpret_cast<volatile LONG64 *>(&rolling->generation));
                    Event live = value;
                    live.sequence = rolling->total;
                    auto *records = reinterpret_cast<Event *>(rolling + 1);
                    records[live.sequence % rolling->capacity] = live;
                    ++rolling->total;
                    InterlockedIncrement64(reinterpret_cast<volatile LONG64 *>(&rolling->generation));
                }
                if (header.written == header.capacity) {
                    ++header.dropped;
                    ReleaseSRWLockExclusive(&lock);
                    return;
                }
                events[header.written++] = value;
#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
                if (kind == RecordKind::enter) {
                    if (active_depth == 1024)
                        active_call_unresolved = true;
                    else
                        active_calls[active_depth++] = ActiveCall{value.sequence, header.image_base};
                } else if (active_depth)
                    --active_depth;
                else
                    active_call_unresolved = true;
#endif
            }
        }
    }
    ReleaseSRWLockExclusive(&lock);
}
}
extern "C" void auditor_exact_enter(std::uint64_t address) noexcept {
    record(address, RecordKind::enter);
}
extern "C" void auditor_exact_exit(std::uint64_t address) noexcept {
    record(address, RecordKind::exit);
}
#if defined(EP_AUDITOR_SHADOW_EXACT_CONTEXT)
extern "C" bool auditor_exact_current_call(std::uint64_t *ordinal, std::uint64_t *producer_base) noexcept {
    AcquireSRWLockShared(&lock);
    const bool valid = output != INVALID_HANDLE_VALUE && !header.dropped && active_depth && !active_call_unresolved;
    if (valid) {
        *ordinal = active_calls[active_depth - 1].ordinal;
        *producer_base = active_calls[active_depth - 1].producer_base;
    }
    ReleaseSRWLockShared(&lock);
    return valid;
}
#endif

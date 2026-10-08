#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <cstdint>
#include <cstdlib>
#include <cwchar>

namespace {
#pragma pack(push, 1)
struct Header {
    char magic[8] = {'E','P','T','R','A','C','E','2'};
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
Event* events = nullptr;
HANDLE output = INVALID_HANDLE_VALUE;
wchar_t image_path[32768]{};

std::uint64_t ticks(FILETIME value) noexcept {
    return (static_cast<std::uint64_t>(value.dwHighDateTime) << 32) | value.dwLowDateTime;
}
bool write_bytes(const void* bytes, DWORD count) noexcept {
    const auto* cursor = static_cast<const unsigned char*>(bytes);
    while (count) {
        DWORD written = 0;
        if (!WriteFile(output, cursor, count, &written, nullptr) || !written) return false;
        cursor += written;
        count -= written;
    }
    return true;
}
void flush() noexcept {
    AcquireSRWLockExclusive(&lock);
    if (output != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER position{};
        position.QuadPart = header.header_bytes;
        const bool complete = SetFilePointerEx(output, position, nullptr, FILE_BEGIN)
            && write_bytes(events, static_cast<DWORD>(header.written * sizeof(Event)))
            && FlushFileBuffers(output);
        header.complete = complete && header.dropped == 0 ? 1U : 0U;
        position.QuadPart = 0;
        if (!(SetFilePointerEx(output, position, nullptr, FILE_BEGIN)
              && write_bytes(&header, sizeof(header)) && FlushFileBuffers(output))) {
            // A failed final commit must not leave a readable COMPLETE header.
            if (SetFilePointerEx(output, position, nullptr, FILE_BEGIN)) SetEndOfFile(output);
        }
        CloseHandle(output);
        output = INVALID_HANDLE_VALUE;
    }
    ReleaseSRWLockExclusive(&lock);
}
void initialize() noexcept {
    initialized = true;
    wchar_t directory[32768]{};
    const DWORD length = GetEnvironmentVariableW(L"EP_AUDITOR_TRACE_DIRECTORY", directory, 32768);
    if (!length || length > 32500 || directory[1] != L':') return;
    header.capacity = 262144;
    wchar_t capacity[32]{};
    if (GetEnvironmentVariableW(L"EP_AUDITOR_TRACE_CAPACITY", capacity, 32)) {
        wchar_t* end = nullptr;
        header.capacity = _wcstoui64(capacity, &end, 10);
        if (!end || *end || !header.capacity || header.capacity > 1048576) return;
    }
    LARGE_INTEGER frequency{};
    FILETIME born{}, ended{}, kernel{}, user{};
    HMODULE image = nullptr;
    if (!QueryPerformanceFrequency(&frequency) || frequency.QuadPart <= 0
        || !GetProcessTimes(GetCurrentProcess(), &born, &ended, &kernel, &user)
        || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
            reinterpret_cast<LPCWSTR>(&initialize), &image)) return;
    header.frequency = static_cast<std::uint64_t>(frequency.QuadPart);
    header.pid = GetCurrentProcessId();
    header.birth = ticks(born);
    header.image_base = reinterpret_cast<std::uint64_t>(image);
    header.path_chars = GetModuleFileNameW(image, image_path, 32768);
    if (!header.path_chars || header.path_chars >= 32768) return;
    header.header_bytes = sizeof(Header) + header.path_chars * sizeof(wchar_t);
    events = static_cast<Event*>(VirtualAlloc(nullptr, static_cast<SIZE_T>(header.capacity * sizeof(Event)),
        MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE));
    if (!events) return;
    wchar_t path[32768]{};
    if (swprintf_s(path, L"\\\\?\\%ls\\trace-%u-%llu-%llx.bin", directory, header.pid, header.birth, header.image_base) < 0) return;
    output = CreateFileW(path, GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (output == INVALID_HANDLE_VALUE) return;
    if (!write_bytes(&header, sizeof(header)) || !write_bytes(image_path, header.path_chars * sizeof(wchar_t))
        || std::atexit(flush) != 0) {
        CloseHandle(output);
        output = INVALID_HANDLE_VALUE;
    }
}
void record(std::uint64_t address, std::uint8_t kind) noexcept {
    AcquireSRWLockExclusive(&lock);
    if (!initialized) initialize();
    if (output != INVALID_HANDLE_VALUE) {
        if (header.written == header.capacity) ++header.dropped;
        else {
            LARGE_INTEGER timestamp{};
            FILETIME born{}, ended{}, kernel{}, user{};
            ULONG64 cycles = 0;
            HMODULE image = nullptr;
            if (!QueryPerformanceCounter(&timestamp) || !GetThreadTimes(GetCurrentThread(), &born, &ended, &kernel, &user)
                || !QueryThreadCycleTime(GetCurrentThread(), &cycles)
                || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<LPCWSTR>(address), &image)) ++header.dropped;
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
                value.kind = kind;
                events[header.written++] = value;
            }
        }
    }
    ReleaseSRWLockExclusive(&lock);
}
}
extern "C" void auditor_exact_enter(std::uint64_t address) noexcept { record(address, 1); }
extern "C" void auditor_exact_exit(std::uint64_t address) noexcept { record(address, 2); }

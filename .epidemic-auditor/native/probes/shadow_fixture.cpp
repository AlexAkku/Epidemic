#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <winsock2.h>
#endif
#include "shadow_probe.h"
#include <Epidemic/Memory/allocator.h>
#ifdef _WIN32
#include <d3d11.h>
#endif
#include <thread>
#include <cstring>
#include <memory>
#include <cstdlib>
#include <filesystem>
#include <chrono>

#ifdef _WIN32
bool leak_module_case(bool leak) {
    wchar_t image[32768]{};
    if (!GetModuleFileNameW(nullptr,image,32768)) return false;
    const auto path = std::filesystem::path(image).parent_path() / L"EpidemicAuditorLeakFixture.dll";
    const auto module = LoadLibraryExW(path.c_str(),nullptr,LOAD_LIBRARY_SEARCH_DLL_LOAD_DIR|LOAD_LIBRARY_SEARCH_DEFAULT_DIRS);
    if (!module) return false;
    using Fixture = int (*)(int);
    const auto function = reinterpret_cast<Fixture>(GetProcAddress(module,"AuditorLeakFixture"));
    const bool result = function && function(leak ? 1 : 0) == 0;
    const bool unloaded = FreeLibrary(module) != FALSE;
    return result && unloaded;
}

bool asynchronous_io_case(bool cancel) {
    using namespace epidemic::auditor::shadow;
    const auto name = L"\\\\.\\pipe\\EpidemicAuditorFixture-" + std::to_wstring(GetCurrentProcessId());
    struct Handle { HANDLE value; ~Handle() { if (value != INVALID_HANDLE_VALUE && value) CloseHandle(value); } };
    Handle server{CreateNamedPipeW(name.c_str(), PIPE_ACCESS_INBOUND|FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE|PIPE_READMODE_BYTE|PIPE_WAIT|PIPE_REJECT_REMOTE_CLIENTS, 1, 128, 128, 0, nullptr)};
    if (server.value == INVALID_HANDLE_VALUE) return false;
    Handle client{CreateFileW(name.c_str(), GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr)};
    if (client.value == INVALID_HANDLE_VALUE) return false;
    OVERLAPPED connection{};
    Handle connected{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    if (!connected.value) return false;
    connection.hEvent = connected.value;
    if (!ConnectNamedPipe(server.value, &connection) && GetLastError() != ERROR_PIPE_CONNECTED) return false;
    Handle ready{CreateEventW(nullptr, TRUE, FALSE, nullptr)};
    if (!ready.value) return false;
    OVERLAPPED read{}; read.hEvent = ready.value;
    char received = 0; DWORD bytes = 0;
    if (ReadFile(server.value, &received, 1, &bytes, &read) || GetLastError() != ERROR_IO_PENDING) return false;
    const auto operation_id = identity();
    causal("io.submit", operation_id.data());
    struct PendingRead {
        HANDLE pipe; OVERLAPPED* operation;
        ~PendingRead() {
            CancelIoEx(pipe, operation);
            DWORD transferred = 0;
            GetOverlappedResult(pipe, operation, &transferred, TRUE);
        }
    } pending{server.value, &read};
    if (cancel) {
        if (!CancelIoEx(server.value, &read)) return false;
    } else {
        const char sent = 42;
        if (!WriteFile(client.value, &sent, 1, &bytes, nullptr) || bytes != 1) return false;
    }
    if (WaitForSingleObject(ready.value, 3000) != WAIT_OBJECT_0) { CancelIoEx(server.value, &read); return false; }
    const bool completed = GetOverlappedResult(server.value, &read, &bytes, FALSE) != FALSE;
    const auto error = completed ? ERROR_SUCCESS : GetLastError();
    if (completed || error == ERROR_OPERATION_ABORTED)
        causal(error == ERROR_OPERATION_ABORTED ? "io.cancel" : "io.complete", operation_id.data());
    return cancel ? (!completed && error == ERROR_OPERATION_ABORTED) : (completed && bytes == 1 && received == 42);
}

bool disk_io_case() {
    wchar_t directory[MAX_PATH]{}, path[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH,directory) || !GetTempFileNameW(directory,L"epd",0,path)) return false;
    const auto file = CreateFileW(path,GENERIC_READ|GENERIC_WRITE,0,nullptr,OPEN_EXISTING,
        FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE|FILE_FLAG_NO_BUFFERING|FILE_FLAG_WRITE_THROUGH,nullptr);
    if (file == INVALID_HANDLE_VALUE) { DeleteFileW(path); return false; }
    struct File { HANDLE value; ~File() { CloseHandle(value); } } owned{file};
    constexpr DWORD size = 8 * 1024 * 1024;
    auto *buffer = VirtualAlloc(nullptr,size,MEM_RESERVE|MEM_COMMIT,PAGE_READWRITE);
    if (!buffer) return false;
    struct Buffer { void *value; ~Buffer() { VirtualFree(value,0,MEM_RELEASE); } } memory{buffer};
    std::memset(buffer,42,size);
    DWORD written = 0, received = 0;
    const bool write = WriteFile(file,buffer,size,&written,nullptr) && written == size && FlushFileBuffers(file);
    LARGE_INTEGER beginning{};
    const bool seek = SetFilePointerEx(file,beginning,nullptr,FILE_BEGIN) != FALSE;
    const bool read = seek && ReadFile(file,buffer,size,&received,nullptr) && received == size;
    return write && read && static_cast<unsigned char*>(buffer)[size-1] == 42;
}

bool graphics_case() {
    const auto window = CreateWindowExW(0, L"STATIC", L"Auditor hidden API fixture", WS_POPUP,
                                       0, 0, 16, 16, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return false;
    MSG message{}; PeekMessageW(&message, window, 0, 0, PM_NOREMOVE);
    ID3D11Device *device = nullptr; ID3D11DeviceContext *context = nullptr;
    const auto created = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, nullptr, 0,
                                          D3D11_SDK_VERSION, &device, nullptr, &context);
    if (context) context->Release();
    if (device) device->Release();
    const bool destroyed = DestroyWindow(window) != FALSE;
    return SUCCEEDED(created) && destroyed;
}
bool menu_case() {
    using namespace epidemic::auditor::shadow;
    const auto window = CreateWindowExW(0, L"STATIC", L"Auditor menu scenario", WS_OVERLAPPEDWINDOW,
        0, 0, 16, 16, nullptr, nullptr, GetModuleHandleW(nullptr), nullptr);
    if (!window) return false;
    const auto menu = CreateMenu();
    bool success = menu && AppendMenuW(menu, MF_STRING, 1, L"Continue");
    const auto transition = identity();
    flow("state.request", transition.data());
    if (success) success = SetMenu(window, menu) && GetMenu(window) == menu;
    if (success) {
        SendMessageW(window, WM_INITMENU, reinterpret_cast<WPARAM>(menu), 0);
        success = GetMenuItemCount(menu) == 1;
        if (success) flow("state.change", transition.data());
    }
    SetMenu(window, nullptr);
    if (menu) DestroyMenu(menu);
    DestroyWindow(window);
    return success;
}

bool loopback_network() {
    WSADATA data{};
    if (WSAStartup(MAKEWORD(2,2), &data)) return false;
    struct Winsock { ~Winsock() { WSACleanup(); } } cleanup;
    const auto server = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server == INVALID_SOCKET) return false;
    struct Socket { SOCKET value; ~Socket() { if (value != INVALID_SOCKET) closesocket(value); } } owned_server{server};
    sockaddr_in address{}; address.sin_family = AF_INET; address.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    int size = sizeof(address);
    if (bind(server, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) || listen(server, 1)
        || getsockname(server, reinterpret_cast<sockaddr*>(&address), &size)) return false;
    const auto client = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    Socket owned_client{client};
    if (client == INVALID_SOCKET) return false;
    DWORD timeout = 3000;
    setsockopt(client, SOL_SOCKET, SO_SNDTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    if (connect(client, reinterpret_cast<const sockaddr*>(&address), sizeof(address))) return false;
    const auto accepted = accept(server, nullptr, nullptr);
    Socket owned_accepted{accepted};
    if (accepted == INVALID_SOCKET) return false;
    setsockopt(accepted, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout));
    const char payload[] = "physical-loopback";
    if (send(client, payload, static_cast<int>(sizeof(payload)), 0) != static_cast<int>(sizeof(payload))) return false;
    char received[sizeof(payload)]{};
    int used = 0;
    while (used < static_cast<int>(sizeof(payload))) {
        const auto count = recv(accepted, received + used, static_cast<int>(sizeof(payload)) - used, 0);
        if (count <= 0) return false;
        used += count;
    }
    return std::memcmp(payload, received, sizeof(payload)) == 0;
}

__declspec(noinline) void handled_exception() {
    __try { RaiseException(0xE0424242UL, 0, 0, nullptr); }
    __except (GetExceptionCode() == 0xE0424242UL ? EXCEPTION_EXECUTE_HANDLER : EXCEPTION_CONTINUE_SEARCH) {
        OutputDebugStringA("EP_RECOVERY:handled");
    }
}
#endif

#if defined(EP_AUDITOR_ASAN_FIXTURE)
__declspec(noinline) int memory_case(const char *mode) {
    auto *memory = static_cast<char*>(std::malloc(16));
    if (!memory) return 7;
    memory[0] = 42;
    volatile char *observed = memory;
    if (std::strcmp(mode, "uaf") == 0) {
        std::free(memory);
        return observed[0];
    }
    if (std::strcmp(mode, "double-free") == 0) {
        std::free(memory);
        std::free(const_cast<char*>(observed));
        return 0;
    }
    if (std::strcmp(mode, "oob") == 0) observed[16] = 1;
    if (std::strcmp(mode, "heap-corruption") == 0) observed[-1] = 1;
    if (std::strcmp(mode, "leak") == 0) {
        epidemic::auditor::shadow::allocation("allocation", reinterpret_cast<std::uintptr_t>(memory), 16);
        return 0; // Windows ASan has no LeakSanitizer; this remains a scoped probe candidate.
    }
    std::free(memory);
    return std::strcmp(mode, "memory-good") == 0 ? 0 : 8;
}
#endif

int main(int argc, char** argv) {
    using namespace epidemic::auditor::shadow;
#if defined(EP_AUDITOR_ASAN_FIXTURE)
    if (argc == 2) return memory_case(argv[1]);
#endif
#ifdef _WIN32
    if (argc == 987) return MoveFileExW(L"not-an-existing-file", L"not-an-existing-target", 0) ? 0 : 11;
    if (argc == 2 && std::strcmp(argv[1], "network") == 0) return loopback_network() ? 0 : 12;
    if (argc == 2 && std::strcmp(argv[1], "graphics") == 0) return graphics_case() ? 0 : 13;
    if (argc == 2 && std::strcmp(argv[1], "disk-io") == 0) return disk_io_case() ? 0 : 14;
    if (argc == 2 && std::strcmp(argv[1], "async-io") == 0) return asynchronous_io_case(false) ? 0 : 16;
    if (argc == 2 && std::strcmp(argv[1], "cancel-io") == 0) return asynchronous_io_case(true) ? 0 : 16;
    if (argc == 2 && std::strcmp(argv[1], "leak-good") == 0) return leak_module_case(false) ? 0 : 15;
    if (argc == 2 && std::strcmp(argv[1], "leak-bad") == 0) return leak_module_case(true) ? 0 : 15;
    if (argc == 2 && std::strcmp(argv[1], "background-io") != 0) {
        if (std::strcmp(argv[1], "menu") == 0) {
            action("action.start", "menu-action", "Enter native fixture menu");
            scenario("scenario.start", "menu-scenario");
            const bool success = menu_case();
            scenario("scenario.end", "menu-scenario");
            action("action.end", "menu-action");
            return success ? 0 : 3;
        }
        if (std::strcmp(argv[1], "rolling-crash") == 0 || std::strcmp(argv[1], "rolling-assert") == 0
            || std::strcmp(argv[1], "rolling-contract") == 0) {
            SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
            causal("job.submit", "before-crash");
            causal("job.execute", "before-crash");
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            causal("job.submit", "immediately-before-crash");
            causal("job.execute", "immediately-before-crash");
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            const DWORD code = std::strcmp(argv[1], "rolling-assert") == 0 ? 0xC0000420UL
                : std::strcmp(argv[1], "rolling-contract") == 0 ? 0xE0434242UL : 0xE0424242UL;
            RaiseException(code, 0, 0, nullptr);
            return 3;
        }
        if (std::strcmp(argv[1], "rolling-long-frame") == 0) {
            const auto start = std::chrono::steady_clock::now();
            for (unsigned tick = 0; tick < 5; ++tick) {
                const auto token = identity();
                causal("job.submit", token.data());
                causal("job.execute", token.data());
                std::this_thread::sleep_for(std::chrono::milliseconds(100));
            }
            frame(static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
                std::chrono::steady_clock::now() - start).count()));
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            causal("job.submit", "after-long-frame");
            causal("job.execute", "after-long-frame");
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
            return 0;
        }
        if (std::strcmp(argv[1], "verifier-good") == 0 || std::strcmp(argv[1], "verifier-bad") == 0) {
            auto *block = HeapAlloc(GetProcessHeap(), 0, 16);
            if (!block) return 9;
            volatile char *memory = static_cast<char*>(block);
            memory[0] = 42;
            if (std::strcmp(argv[1], "verifier-bad") == 0) memory[16] = 1;
            return HeapFree(GetProcessHeap(), 0, block) ? 0 : 10;
        }
        if (std::strcmp(argv[1], "handled") == 0 || std::strcmp(argv[1], "storm") == 0) {
            const unsigned count = std::strcmp(argv[1], "storm") == 0 ? 100 : 1;
            for (unsigned index = 0; index < count; ++index) handled_exception();
            return 0;
        }
        if (std::strcmp(argv[1], "crash") == 0) RaiseException(0xE0424242UL, 0, 0, nullptr);
        else if (std::strcmp(argv[1], "assert") == 0) RaiseException(0xC0000420UL, 0, 0, nullptr);
        else if (std::strcmp(argv[1], "contract") == 0) RaiseException(0xE0434242UL, 0, 0, nullptr);
        return 3;
    }
#else
    (void)argc; (void)argv;
#endif
    auto worker_context = child_context();
    worker_context.cause = emit_causal("job.submit", "fixture-job1");
    std::thread worker([worker_context] {
        const ContextScope scope(worker_context);
        flow("job.execute", "fixture-job1");
    });
    worker.join();
    action("action.start", "fixture-a1");
    scenario("scenario.start", "fixture-s1");
    epidemic::memory::DefaultAllocator allocator;
    auto *memory = allocator.Allocate(512, alignof(std::max_align_t));
    allocator.Deallocate(memory, 512, alignof(std::max_align_t));
#ifdef _WIN32
    wchar_t directory[MAX_PATH]{}, path[MAX_PATH]{};
    if (!GetTempPathW(MAX_PATH, directory) || !GetTempFileNameW(directory, L"epa", 0, path)) return 4;
    const auto file = CreateFileW(path, GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                                  FILE_ATTRIBUTE_TEMPORARY | FILE_FLAG_DELETE_ON_CLOSE, nullptr);
    if (file == INVALID_HANDLE_VALUE) { DeleteFileW(path); return 5; }
    char content[512]{}; DWORD bytes = 0;
    const bool wrote = WriteFile(file, content, sizeof(content), &bytes, nullptr) && bytes == sizeof(content);
    const bool seeked = SetFilePointer(file, 0, nullptr, FILE_BEGIN) != INVALID_SET_FILE_POINTER;
    bool read = false;
    auto read_file = [&] {
        const auto begin = std::chrono::steady_clock::now();
        read = ReadFile(file, content, sizeof(content), &bytes, nullptr) != FALSE;
        const auto elapsed = static_cast<std::uint64_t>(std::chrono::duration_cast<std::chrono::nanoseconds>(
            std::chrono::steady_clock::now() - begin).count());
        io("io.read", "fixture-io1", bytes, elapsed);
    };
    if (argc == 2 && std::strcmp(argv[1], "background-io") == 0) {
        std::thread reader(read_file);
        reader.join();
    } else read_file();
    const auto mapping = CreateFileMappingW(file, nullptr, PAGE_READWRITE, 0, 0, nullptr);
    const auto view = mapping ? MapViewOfFile(mapping, FILE_MAP_READ, 0, 0, sizeof(content)) : nullptr;
    const bool mapped = view != nullptr;
    if (view) UnmapViewOfFile(view);
    if (mapping) CloseHandle(mapping);
    WIN32_FIND_DATAW found{};
    const auto enumeration = FindFirstFileW((std::wstring(directory) + L"*").c_str(), &found);
    const bool enumerated = enumeration != INVALID_HANDLE_VALUE;
    if (enumerated) FindClose(enumeration);
    CloseHandle(file);
    if (!wrote || !seeked || !read || !mapped || !enumerated || bytes != sizeof(content)) return 6;
#endif
    scenario("scenario.end", "fixture-s1");
    action("action.end", "fixture-a1");
    return 0;
}

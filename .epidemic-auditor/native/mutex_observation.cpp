#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <objbase.h>
#include <array>
#include <cstring>
#include <iostream>
#include <map>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {
struct Handle {
    HANDLE value = nullptr;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
    Handle(const Handle&) = delete;
    Handle& operator=(const Handle&) = delete;
    explicit Handle(HANDLE input = nullptr) : value(input) {}
};
unsigned long long stamp() {
    LARGE_INTEGER value{};
    if (!QueryPerformanceCounter(&value)) throw std::runtime_error("mutex QPC unavailable");
    return static_cast<unsigned long long>(value.QuadPart);
}
std::string text(const std::wstring& value) {
    const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
    std::string bytes(static_cast<std::size_t>(size), '\0');
    if (size && !WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), bytes.data(), size, nullptr, nullptr))
        throw std::runtime_error("mutex text conversion failed");
    std::ostringstream result; result << '"';
    for (const unsigned char character : bytes) {
        if (character == '"' || character == '\\') result << '\\' << character;
        else if (character < 32) result << "\\u00" << "0123456789abcdef"[character >> 4] << "0123456789abcdef"[character & 15];
        else result << character;
    }
    return result.str() + '"';
}
template<class T> T read(HANDLE process, DWORD64 address) {
    T value{}; SIZE_T count = 0;
    if (!ReadProcessMemory(process, reinterpret_cast<const void*>(address), &value, sizeof(value), &count) || count != sizeof(value))
        throw std::runtime_error("mutex remote read failed");
    return value;
}
void patch(HANDLE process, DWORD64 address, unsigned char byte) {
    DWORD old = 0; SIZE_T count = 0;
    if (!VirtualProtectEx(process, reinterpret_cast<void*>(address), 1, PAGE_EXECUTE_READWRITE, &old))
        throw std::runtime_error("mutex breakpoint protection failed");
    const bool written = WriteProcessMemory(process, reinterpret_cast<void*>(address), &byte, 1, &count) && count == 1;
    DWORD ignored = 0;
    const bool restored = VirtualProtectEx(process, reinterpret_cast<void*>(address), 1, old, &ignored) != FALSE;
    if (!written || !restored || !FlushInstructionCache(process, reinterpret_cast<void*>(address), 1))
        throw std::runtime_error("mutex breakpoint write/restore failed");
}
struct Breakpoint { unsigned char original = 0; unsigned kind = 0; };
std::string frame(HANDLE process, DWORD64 address) {
    alignas(SYMBOL_INFOW) unsigned char bytes[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)]{};
    auto* symbol = reinterpret_cast<SYMBOL_INFOW*>(bytes);
    symbol->SizeOfStruct = sizeof(SYMBOL_INFOW); symbol->MaxNameLen = MAX_SYM_NAME;
    DWORD64 displacement = 0;
    const bool resolved = SymFromAddrW(process, address, &displacement, symbol) != FALSE;
    IMAGEHLP_LINEW64 line{}; line.SizeOfStruct = sizeof(line); DWORD offset = 0;
    const bool located = SymGetLineFromAddrW64(process, address, &offset, &line) != FALSE;
    std::ostringstream result;
    result << "{\"raw_address\":" << address << ",\"function\":" << (resolved ? text(std::wstring(symbol->Name, symbol->NameLen)) : "null")
           << ",\"source_file\":" << (located ? text(line.FileName) : "null") << ",\"source_line\":";
    if (located) result << line.LineNumber; else result << "null";
    return result.str() + '}';
}
}

int observe_mutex_fixture(const wchar_t* binary, const wchar_t* variant) {
#if !defined(_M_X64)
    (void)binary; (void)variant;
    throw std::runtime_error("mutex native debugger requires x64");
#else
    const std::wstring mode(variant), image(binary);
    if (mode != L"good" && mode != L"bad" && mode != L"existing") throw std::runtime_error("invalid mutex fixture variant");
    if (image.find(L'"') != std::wstring::npos) throw std::runtime_error("invalid mutex fixture image");
    GUID id{}; wchar_t guid[40]{};
    if (FAILED(CoCreateGuid(&id)) || !StringFromGUID2(id, guid, 40)) throw std::runtime_error("mutex identity unavailable");
    const std::wstring name = L"Local\\AuditorObservedMutex_" + std::wstring(guid);
    Handle release(CreateEventW(nullptr, TRUE, FALSE, (name + L"_release").c_str()));
    Handle existing(mode == L"existing" ? CreateMutexW(nullptr, FALSE, name.c_str()) : nullptr);
    if (!release.value || (mode == L"existing" && !existing.value)) throw std::runtime_error("mutex fixture controls unavailable");
    std::wstring command = L"\"" + image + L"\" mutex " + (mode == L"bad" ? L"bad" : L"good") + L" \"" + name + L"\"";
    STARTUPINFOW startup{}; startup.cb = sizeof(startup); PROCESS_INFORMATION child{};
    if (!CreateProcessW(image.c_str(), command.data(), nullptr, nullptr, FALSE, DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child))
        throw std::runtime_error("mutex debug fixture launch failed");
    Handle process(child.hProcess), primary(child.hThread);
    if (!DebugSetProcessKillOnExit(TRUE)) throw std::runtime_error("mutex debugger kill-on-exit failed");
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(process.value, &created, &exited, &kernel, &user)) throw std::runtime_error("mutex creation identity unavailable");
    const auto creation = (static_cast<unsigned long long>(created.dwHighDateTime) << 32) | created.dwLowDateTime;
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);
    if (!SymInitializeW(process.value, nullptr, FALSE)) throw std::runtime_error("mutex symbol initialization failed");
    struct Symbols { HANDLE process; ~Symbols() { SymCleanup(process); } } symbols{process.value};
    const auto ntdll = GetModuleHandleW(L"ntdll.dll");
    wchar_t local_ntdll_path[32768]{};
    if (!ntdll || !GetModuleFileNameW(ntdll, local_ntdll_path, 32768)) throw std::runtime_error("mutex trusted native module unavailable");
    const auto create_offset = reinterpret_cast<DWORD64>(GetProcAddress(ntdll, "NtCreateMutant")) - reinterpret_cast<DWORD64>(ntdll);
    const auto release_offset = reinterpret_cast<DWORD64>(GetProcAddress(ntdll, "NtReleaseMutant")) - reinterpret_cast<DWORD64>(ntdll);
    if (!ntdll || !create_offset || !release_offset) throw std::runtime_error("mutex native exports unavailable");
    std::map<DWORD64, Breakpoint> breakpoints;
    std::map<DWORD, DWORD64> stepping;
    std::map<DWORD, HANDLE> threads;
    struct ThreadHandles { std::map<DWORD, HANDLE>& rows; ~ThreadHandles() { for (auto& row : rows) CloseHandle(row.second); } } cleanup{threads};
    auto arm = [&](DWORD64 address, unsigned kind) {
        if (breakpoints.count(address)) throw std::runtime_error("mutex overlapping breakpoint roles");
        const auto original = read<unsigned char>(process.value, address);
        if (original == 0xcc) throw std::runtime_error("mutex breakpoint original already trapped");
        breakpoints.emplace(address, Breakpoint{original, kind}); patch(process.value, address, 0xcc);
    };
    bool exited_ok = false, initial_break = false, acquired = false, released = false, not_new = false;
    DWORD exit_code = MAXDWORD; DWORD64 handle_pointer = 0, mutex_handle = 0;
    DWORD create_status = MAXDWORD, release_status = MAXDWORD;
    unsigned long long acquire_begin = 0, acquire_end = 0, release_begin = 0, release_end = 0;
    std::vector<std::string> stack;
    const auto deadline = GetTickCount64() + 12000;
    while (!exited_ok && GetTickCount64() < deadline) {
        if (mode == L"bad" && acquired && !released) {
            LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency);
            if (stamp() - acquire_end >= static_cast<unsigned long long>(frequency.QuadPart) * 3 && !SetEvent(release.value))
                throw std::runtime_error("mutex external release failed");
        }
        DEBUG_EVENT event{};
        if (!WaitForDebugEventEx(&event, 100)) {
            if (GetLastError() == ERROR_SEM_TIMEOUT) continue;
            throw std::runtime_error("mutex debug event wait failed");
        }
        DWORD disposition = DBG_CONTINUE;
        const auto tid = event.dwThreadId;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT || event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            const HANDLE source = event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT ? event.u.CreateProcessInfo.hThread : event.u.CreateThread.hThread;
            HANDLE duplicate = nullptr;
            if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS))
                throw std::runtime_error("mutex debug thread identity unavailable");
            threads.emplace(tid, duplicate);
            if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
                SymLoadModuleExW(process.value, event.u.CreateProcessInfo.hFile, image.c_str(), nullptr,
                    reinterpret_cast<DWORD64>(event.u.CreateProcessInfo.lpBaseOfImage), 0, nullptr, 0);
                if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            }
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            const auto file = event.u.LoadDll.hFile;
            wchar_t path[32768]{};
            if (file && GetFinalPathNameByHandleW(file, path, 32768, FILE_NAME_NORMALIZED)) {
                SymLoadModuleExW(process.value, file, path, nullptr, reinterpret_cast<DWORD64>(event.u.LoadDll.lpBaseOfDll), 0, nullptr, 0);
                std::wstring filename(path); const auto slash = filename.find_last_of(L"\\/");
                filename = filename.substr(slash == std::wstring::npos ? 0 : slash + 1);
                if (_wcsicmp(filename.c_str(), L"ntdll.dll") == 0) {
                    std::wstring canonical(path);
                    if (canonical.rfind(L"\\\\?\\", 0) == 0) canonical.erase(0, 4);
                    if (_wcsicmp(canonical.c_str(), local_ntdll_path) != 0)
                        throw std::runtime_error("mutex target native module path is not trusted");
                    const auto base = reinterpret_cast<DWORD64>(event.u.LoadDll.lpBaseOfDll);
                    for (const auto offset : {create_offset, release_offset})
                        if (read<std::array<unsigned char, 16>>(process.value, base + offset)
                            != read<std::array<unsigned char, 16>>(GetCurrentProcess(), reinterpret_cast<DWORD64>(ntdll) + offset))
                            throw std::runtime_error("mutex target native export bytes disagree with trusted module");
                    arm(base + create_offset, 1); arm(base + release_offset, 2);
                }
            }
            if (file) CloseHandle(file);
        } else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
            if (threads.count(tid)) { CloseHandle(threads.at(tid)); threads.erase(tid); }
            stepping.erase(tid);
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            exit_code = event.u.ExitProcess.dwExitCode; exited_ok = true;
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            const auto code = event.u.Exception.ExceptionRecord.ExceptionCode;
            const auto address = reinterpret_cast<DWORD64>(event.u.Exception.ExceptionRecord.ExceptionAddress);
            if (code == EXCEPTION_SINGLE_STEP && stepping.count(tid)) {
                const auto entry = stepping.at(tid); stepping.erase(tid);
                patch(process.value, entry, 0xcc);
                CONTEXT context{}; context.ContextFlags = CONTEXT_CONTROL;
                if (!GetThreadContext(threads.at(tid), &context)) throw std::runtime_error("mutex step context unavailable");
                context.EFlags &= ~0x100UL;
                if (!SetThreadContext(threads.at(tid), &context)) throw std::runtime_error("mutex step context restore failed");
            } else if (code == EXCEPTION_BREAKPOINT && breakpoints.count(address)) {
                const auto point = breakpoints.at(address);
                CONTEXT context{}; context.ContextFlags = CONTEXT_FULL;
                if (!GetThreadContext(threads.at(tid), &context)) throw std::runtime_error("mutex breakpoint context unavailable");
                patch(process.value, address, point.original); context.Rip = address;
                if (tid == child.dwThreadId && point.kind == 1 && !acquired && !not_new && (context.R9 & 0xffU)) {
                    acquire_begin = stamp(); handle_pointer = context.Rcx;
                    const auto returned = read<DWORD64>(process.value, context.Rsp); arm(returned, 3);
                    CONTEXT walking = context; STACKFRAME64 frame_state{};
                    frame_state.AddrPC = {context.Rip, 0, AddrModeFlat}; frame_state.AddrStack = {context.Rsp, 0, AddrModeFlat};
                    frame_state.AddrFrame = {context.Rbp, 0, AddrModeFlat};
                    stack.push_back(frame(process.value, context.Rip));
                    for (unsigned index = 0; index < 32 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process.value, threads.at(tid),
                         &frame_state, &walking, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr); ++index)
                        if (frame_state.AddrPC.Offset) stack.push_back(frame(process.value, frame_state.AddrPC.Offset));
                } else if (tid == child.dwThreadId && point.kind == 3) {
                    acquire_end = stamp(); create_status = static_cast<DWORD>(context.Rax);
                    not_new = create_status == 0x40000000UL;
                    if (create_status != 0 && !not_new) throw std::runtime_error("mutex native creation failed");
                    if (!not_new) { mutex_handle = read<DWORD64>(process.value, handle_pointer); acquired = mutex_handle != 0; }
                } else if (tid == child.dwThreadId && point.kind == 2 && acquired && !released && context.Rcx == mutex_handle) {
                    release_begin = stamp(); arm(read<DWORD64>(process.value, context.Rsp), 4);
                } else if (tid == child.dwThreadId && point.kind == 4) {
                    release_status = static_cast<DWORD>(context.Rax);
                    if (release_status != 0) throw std::runtime_error("mutex observed release failed");
                    release_end = stamp(); released = true;
                }
                if (point.kind <= 2) { context.EFlags |= 0x100UL; stepping.emplace(tid, address); }
                else breakpoints.erase(address);
                if (!SetThreadContext(threads.at(tid), &context)) throw std::runtime_error("mutex instruction restoration failed");
            } else if (code == EXCEPTION_BREAKPOINT && !initial_break) initial_break = true;
            else disposition = DBG_EXCEPTION_NOT_HANDLED;
        }
        if (!ContinueDebugEvent(event.dwProcessId, tid, disposition)) throw std::runtime_error("mutex debug continuation failed");
    }
    if (!exited_ok) { TerminateProcess(process.value, 124); throw std::runtime_error("mutex owned fixture exceeded deadline"); }
    LARGE_INTEGER frequency{}; QueryPerformanceFrequency(&frequency);
    const bool complete = acquired && released && exit_code == 0;
    std::cout << "{\"schemaVersion\":1,\"observationKind\":\"native_mutex_api\",\"provenance\":\"external\",\"confidence\":\"observed\","
              << "\"pid\":" << child.dwProcessId << ",\"creation_filetime_100ns\":" << creation << ",\"owner_os_tid\":" << child.dwThreadId
              << ",\"resource_name\":" << text(name) << ",\"variant\":" << text(mode) << ",\"mutex_handle\":" << mutex_handle
              << ",\"complete\":" << (complete ? "true" : "false") << ",\"not_new\":" << (not_new ? "true" : "false")
              << ",\"child_exit_code\":" << exit_code << ",\"qpc_frequency\":" << frequency.QuadPart
              << ",\"create_status\":" << create_status << ",\"release_status\":" << release_status
              << ",\"acquire_begin\":" << acquire_begin << ",\"acquire_end\":" << acquire_end
              << ",\"release_begin\":" << release_begin << ",\"release_end\":" << release_end
              << ",\"acquisition_stack\":[";
    if (complete) for (std::size_t index = 0; index < stack.size(); ++index) std::cout << (index ? "," : "") << stack[index];
    std::cout << "],\"scope\":\"Intrusive debugger of controlled initial-owner fixture; observed NtCreateMutant/NtReleaseMutant boundary bounds include observer pauses; no arbitrary engine/syscall coverage claim\"}" << std::endl;
    return complete || (mode == L"existing" && not_new) ? 0 : 2;
#endif
}

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
    constexpr std::size_t kMaxBufferedEventBytes =
    std::size_t{8} * 1024U * 1024U;
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
        else if (character < 32) {
        const auto code = static_cast<unsigned int>(character);
        result << "\\u00"
            << "0123456789abcdef"[code >> 4U]
            << "0123456789abcdef"[code & 15U];
    }
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

int observe_exceptions(
    const wchar_t* binary,
    const std::vector<std::wstring>& arguments,
    DWORD timeout_ms
) {
#if !defined(_M_X64)
    (void)binary; (void)arguments; (void)timeout_ms;
    throw std::runtime_error("exception debugger requires x64");
#else
    const std::wstring image(binary);
    if (image.find(L'"') != std::wstring::npos) throw std::runtime_error("invalid exception fixture image");
    std::wstring command = L"\"" + image + L"\"";
    for (const auto& argument : arguments) {
        // Windows argv quoting: escape quotes and double trailing backslashes.
        command += L" \"";
        std::size_t slashes = 0;
        for (const auto character : argument) {
            if (character == L'\\') { ++slashes; continue; }
            command.append(character == L'"' ? slashes * 2 + 1 : slashes, L'\\');
            slashes = 0;
            command += character;
        }
        command.append(slashes * 2, L'\\');
        command += L'"';
    }
    if (command.size() > 32766) throw std::runtime_error("exception command exceeds Windows limit");
    STARTUPINFOW startup{}; startup.cb = sizeof(startup); PROCESS_INFORMATION child{};
    if (!CreateProcessW(image.c_str(), command.data(), nullptr, nullptr, FALSE,
                        DEBUG_ONLY_THIS_PROCESS | CREATE_NO_WINDOW, nullptr, nullptr, &startup, &child))
        throw std::runtime_error("exception debug fixture launch failed");
    Handle process(child.hProcess), primary(child.hThread);
    // Every exceptional return also destroys our child. No arbitrary PID attach.
    struct OwnedChild { HANDLE process; bool exited = false;
        ~OwnedChild() { if (!exited) { TerminateProcess(process, 124); WaitForSingleObject(process, 5000); } }
    } owned{process.value};
    if (!DebugSetProcessKillOnExit(TRUE)) throw std::runtime_error("exception kill-on-exit failed");
    FILETIME created{}, exited{}, kernel{}, user{};
    if (!GetProcessTimes(process.value, &created, &exited, &kernel, &user))
        throw std::runtime_error("exception process identity unavailable");
    const auto epoch = (static_cast<unsigned long long>(created.dwHighDateTime) << 32U) | created.dwLowDateTime;
    LARGE_INTEGER frequency{};
    if (!QueryPerformanceFrequency(&frequency)) throw std::runtime_error("exception QPC unavailable");
    SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS | SYMOPT_NO_PROMPTS);
    // Empty symbol search path: only image/PDB files supplied by loaded modules.
    if (!SymInitializeW(process.value, L"", FALSE)) throw std::runtime_error("exception symbols unavailable");
    struct Symbols { HANDLE process; ~Symbols() { SymCleanup(process); } } symbols{process.value};
    std::map<DWORD, HANDLE> threads;
    struct Threads { std::map<DWORD, HANDLE>& rows;
        ~Threads() { for (auto& row : rows) CloseHandle(row.second); }
    } cleanup{threads};
    std::vector<std::string> observations, continuations, verifier_modules, api_calls;
    std::map<DWORD64, unsigned long long> module_epochs;
    const auto captured_frame = [&](DWORD64 address) {
        const auto base = SymGetModuleBase64(process.value, address);
        auto result = frame(process.value, address);
        result.pop_back();
        return result + ",\"module_base\":" + std::to_string(base)
            + ",\"module_load_qpc\":" + std::to_string(module_epochs.count(base) ? module_epochs.at(base) : 0) + '}';
    };
    struct PendingException { std::size_t index; unsigned long long cpu, qpc; bool cpu_available; };
    std::map<DWORD, PendingException> pending_exceptions;
    const auto thread_cpu = [&](DWORD tid, unsigned long long& value) {
        FILETIME creation{}, exit{}, system{}, application{};
        if (!threads.count(tid) || !GetThreadTimes(threads.at(tid), &creation, &exit, &system, &application)) return false;
        value = ((static_cast<unsigned long long>(system.dwHighDateTime) << 32U) | system.dwLowDateTime)
              + ((static_cast<unsigned long long>(application.dwHighDateTime) << 32U) | application.dwLowDateTime);
        return true;
    };
    std::size_t observation_bytes = 0;
    struct ApiPoint { unsigned char original; std::string name, family; DWORD64 module_base; };
    std::map<DWORD64, ApiPoint> api_points;
    std::map<DWORD, DWORD64> api_steps;
    std::vector<HANDLE> suspended_threads;
    std::size_t api_bytes = 0;
    bool api_loss = false;
    // Export entry observation is intrusive and records attempted calls. Return
    // values, physical disk latency and packet traffic are not inferred from it.
    auto arm_api = [&](HANDLE file, const wchar_t* path, DWORD64 remote_base) {
        std::wstring filename(path); const auto slash = filename.find_last_of(L"\\/");
        filename = filename.substr(slash == std::wstring::npos ? 0 : slash + 1);
        struct Entry { const char* name; const char* family; };
        static const Entry kernel[] = {{"CreateFileW","file"},{"CloseHandle","file"},{"ReadFile","file"},
            {"WriteFile","file"},{"CancelIoEx","file"},{"GetOverlappedResult","file"},
            {"SetFilePointer","file"},{"SetFilePointerEx","file"},
            {"CreateFileMappingW","file"},{"MapViewOfFile","file"},{"FindFirstFileW","file"},
            {"FindNextFileW","file"},{"FindClose","file"},{"MoveFileExW","file"},{"VirtualAlloc","memory"},{"VirtualFree","memory"},
            {"CreateEventW","synchronization"},{"SetEvent","synchronization"},{"CreateThread","thread"}};
        static const Entry socket[] = {{"connect","socket"},{"send","socket"},{"recv","socket"},{"closesocket","socket"}};
        static const Entry window[] = {{"CreateWindowExW","window"},{"DestroyWindow","window"},{"PeekMessageW","window"}};
        static const Entry graphics[] = {{"D3D11CreateDevice","graphics"},{"D3D11CreateDeviceAndSwapChain","graphics"}};
        const Entry* entries = nullptr; std::size_t count = 0;
        if (_wcsicmp(filename.c_str(), L"KernelBase.dll") == 0) { entries = kernel; count = std::size(kernel); }
        else if (_wcsicmp(filename.c_str(), L"WS2_32.dll") == 0) { entries = socket; count = std::size(socket); }
        else if (_wcsicmp(filename.c_str(), L"USER32.dll") == 0) { entries = window; count = std::size(window); }
        else if (_wcsicmp(filename.c_str(), L"D3D11.dll") == 0) { entries = graphics; count = std::size(graphics); }
        if (!entries) return;
        wchar_t system[MAX_PATH]{}, observed[32768]{};
        if (!GetSystemDirectoryW(system, MAX_PATH) || !GetFinalPathNameByHandleW(file, observed, 32768, FILE_NAME_NORMALIZED)) {
            api_loss = true; return;
        }
        std::wstring canonical(observed);
        if (canonical.rfind(L"\\\\?\\", 0) == 0) canonical.erase(0, 4);
        if (_wcsicmp(canonical.c_str(), (std::wstring(system) + L"\\" + filename).c_str()) != 0) {
            api_loss = true; return;
        }
        const auto local = LoadLibraryExW(filename.c_str(), nullptr, LOAD_LIBRARY_SEARCH_SYSTEM32);
        if (!local) { api_loss = true; return; }
        struct Module { HMODULE value; ~Module() { FreeLibrary(value); } } module{local};
        for (std::size_t index = 0; index < count; ++index) {
            const auto address = reinterpret_cast<DWORD64>(GetProcAddress(local, entries[index].name));
            HMODULE owner = nullptr;
            if (!address || !GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                    reinterpret_cast<const wchar_t*>(address), &owner) || owner != local) continue; // forwarded/unsupported
            const auto remote = remote_base + address - reinterpret_cast<DWORD64>(local);
            if (api_points.count(remote)) continue;
            if (read<std::array<unsigned char,16>>(process.value, remote)
                != read<std::array<unsigned char,16>>(GetCurrentProcess(), address)) { api_loss = true; continue; }
            const auto original = read<unsigned char>(process.value, remote);
            if (original == 0xcc) { api_loss = true; continue; }
            api_points.emplace(remote, ApiPoint{original, entries[index].name, entries[index].family, remote_base});
            patch(process.value, remote, 0xcc);
        }
    };
    bool initial_break = false, loss = false;
    DWORD exit_code = MAXDWORD;
    const auto begin = stamp(), deadline = GetTickCount64() + timeout_ms;
    while (!owned.exited && GetTickCount64() < deadline) {
        DEBUG_EVENT event{};
        if (!WaitForDebugEventEx(&event, 100)) {
            if (GetLastError() == ERROR_SEM_TIMEOUT) continue;
            throw std::runtime_error("exception debug wait failed");
        }
        DWORD disposition = DBG_CONTINUE;
        const auto tid = event.dwThreadId;
        if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT || event.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT) {
            const auto source = event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT
                ? event.u.CreateProcessInfo.hThread : event.u.CreateThread.hThread;
            HANDLE duplicate = nullptr;
            if (!DuplicateHandle(GetCurrentProcess(), source, GetCurrentProcess(), &duplicate, 0, FALSE, DUPLICATE_SAME_ACCESS))
                throw std::runtime_error("exception thread unavailable");
            threads.emplace(tid, duplicate);
            if (event.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT) {
                module_epochs[reinterpret_cast<DWORD64>(event.u.CreateProcessInfo.lpBaseOfImage)] = stamp();
                SymLoadModuleExW(process.value, event.u.CreateProcessInfo.hFile, image.c_str(), nullptr,
                                reinterpret_cast<DWORD64>(event.u.CreateProcessInfo.lpBaseOfImage), 0, nullptr, 0);
                if (event.u.CreateProcessInfo.hFile) CloseHandle(event.u.CreateProcessInfo.hFile);
            }
        } else if (event.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) {
            module_epochs[reinterpret_cast<DWORD64>(event.u.LoadDll.lpBaseOfDll)] = stamp();
            const auto file = event.u.LoadDll.hFile;
            wchar_t path[32768]{};
            if (file && GetFinalPathNameByHandleW(file, path, 32768, FILE_NAME_NORMALIZED)) {
                SymLoadModuleExW(process.value, file, path, nullptr,
                                reinterpret_cast<DWORD64>(event.u.LoadDll.lpBaseOfDll), 0, nullptr, 0);
                arm_api(file, path, reinterpret_cast<DWORD64>(event.u.LoadDll.lpBaseOfDll));
                std::wstring filename(path); const auto slash = filename.find_last_of(L"\\/");
                filename = filename.substr(slash == std::wstring::npos ? 0 : slash + 1);
                if (_wcsicmp(filename.c_str(), L"verifier.dll") == 0 || _wcsicmp(filename.c_str(), L"vrfcore.dll") == 0) {
                    if (verifier_modules.size() < 8) verifier_modules.push_back(text(path));
                    else loss = true;
                }
            }
            if (file) CloseHandle(file);
        } else if (event.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT) {
            const auto base = reinterpret_cast<DWORD64>(event.u.UnloadDll.lpBaseOfDll);
            module_epochs.erase(base);
            SymUnloadModule64(process.value,base);
            for (auto point = api_points.begin(); point != api_points.end();) {
                if (point->second.module_base == base) point = api_points.erase(point);
                else ++point;
            }
        } else if (event.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT) {
            if (threads.count(tid)) { CloseHandle(threads.at(tid)); threads.erase(tid); }
        } else if (event.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) {
            owned.exited = true; exit_code = event.u.ExitProcess.dwExitCode;
        } else if (event.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) {
            const auto& data = event.u.DebugString;
            if (!data.fUnicode && data.nDebugStringLength > 0 && data.nDebugStringLength <= 128) {
                std::array<char, 129> message{}; SIZE_T copied = 0;
                if (ReadProcessMemory(process.value, data.lpDebugStringData, message.data(), data.nDebugStringLength, &copied)
                    && copied == data.nDebugStringLength && std::string(message.data()).rfind("EP_RECOVERY:", 0) == 0) {
                    const auto tick = stamp();
                    if (pending_exceptions.count(tid)) {
                        const auto previous = pending_exceptions.at(tid);
                        std::string suffix = ",\"continuation_qpc\":" + std::to_string(tick);
                        unsigned long long cpu = 0;
                        if (previous.cpu_available && thread_cpu(tid, cpu) && cpu >= previous.cpu)
                            suffix += ",\"thread_cpu_interval_100ns\":" + std::to_string(cpu - previous.cpu);
                        suffix += ",\"continuation_scope\":\"debugger-observed target marker; handler-exclusive cost unknown\"}";
                        if (suffix.size() > kMaxBufferedEventBytes - observation_bytes) loss = true;
                        else {
                            auto& row = observations.at(previous.index);
                            row.pop_back(); row += suffix;
                            observation_bytes += suffix.size() - 1;
                        }
                        pending_exceptions.erase(tid);
                    }
                    if (continuations.size() < 4096) continuations.push_back("{\"tid\":" + std::to_string(tid)
                        + ",\"qpc\":" + std::to_string(tick) + "}");
                    else loss = true;
                }
            }
        } else if (event.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) {
            const auto& info = event.u.Exception;
            const auto code = info.ExceptionRecord.ExceptionCode;
            const auto address = reinterpret_cast<DWORD64>(info.ExceptionRecord.ExceptionAddress);
            if (code == EXCEPTION_SINGLE_STEP && api_steps.count(tid)) {
                const auto entry = api_steps.at(tid); api_steps.erase(tid);
                patch(process.value, entry, 0xcc);
                CONTEXT context{}; context.ContextFlags = CONTEXT_CONTROL;
                if (!GetThreadContext(threads.at(tid), &context)) throw std::runtime_error("API step context failed");
                context.EFlags &= ~0x100UL;
                if (!SetThreadContext(threads.at(tid), &context)) throw std::runtime_error("API step restore failed");
                for (const auto thread : suspended_threads)
                    if (ResumeThread(thread) == MAXDWORD) throw std::runtime_error("API peer resume failed");
                suspended_threads.clear();
            } else if (code == EXCEPTION_BREAKPOINT && api_points.count(address)) {
                CONTEXT context{}; context.ContextFlags = CONTEXT_FULL;
                if (!threads.count(tid) || !GetThreadContext(threads.at(tid), &context))
                    throw std::runtime_error("API thread context unavailable");
                context.Rip = address;
                const auto& api = api_points.at(address);
                if (api_calls.size() >= 4096) api_loss = true;
                else {
                    std::ostringstream row;
                    row << "{\"name\":" << text(std::wstring(api.name.begin(),api.name.end()))
                        << ",\"family\":" << text(std::wstring(api.family.begin(),api.family.end()))
                        << ",\"tid\":" << tid << ",\"qpc\":" << stamp()
                        << ",\"address\":" << address;
                    if (api.name == "ReadFile" || api.name == "WriteFile") {
                        const auto overlapped = read<DWORD64>(process.value, context.Rsp + 40);
                        row << ",\"requested_bytes\":" << static_cast<DWORD>(context.R8)
                            << ",\"synchronous_attempt\":" << (!overlapped ? "true" : "false")
                            << ",\"main_thread\":" << (tid == child.dwThreadId ? "true" : "false");
                    }
                    row << ",\"stack\":[" << captured_frame(address);
                    auto walking = context; STACKFRAME64 stack{};
                    stack.AddrPC = {context.Rip,0,AddrModeFlat}; stack.AddrFrame = {context.Rbp,0,AddrModeFlat};
                    stack.AddrStack = {context.Rsp,0,AddrModeFlat};
                    for (unsigned depth = 1; depth < 32 && StackWalk64(IMAGE_FILE_MACHINE_AMD64,process.value,threads.at(tid),
                            &stack,&walking,nullptr,SymFunctionTableAccess64,SymGetModuleBase64,nullptr); ++depth) {
                        if (!stack.AddrPC.Offset) break;
                        row << ',' << captured_frame(stack.AddrPC.Offset);
                    }
                    auto payload = row.str() + "]}";
                    if (payload.size() > 65536 || payload.size() > kMaxBufferedEventBytes - api_bytes) api_loss = true;
                    else { api_bytes += payload.size(); api_calls.push_back(std::move(payload)); }
                }
                // All debuggee threads are stopped at this event. Keep peers
                // suspended through the one restored instruction, so none can
                // cross an unarmed export and disappear from the observation.
                for (const auto& thread : threads) if (thread.first != tid) {
                    if (SuspendThread(thread.second) == MAXDWORD) throw std::runtime_error("API peer suspend failed");
                    suspended_threads.push_back(thread.second);
                }
                patch(process.value,address,api.original);
                context.EFlags |= 0x100UL; api_steps.emplace(tid,address);
                if (!SetThreadContext(threads.at(tid),&context)) throw std::runtime_error("API entry restore failed");
            } else if (!initial_break && code == EXCEPTION_BREAKPOINT && info.dwFirstChance) {
                initial_break = true;
            } else {
                // Let the target's SEH handle it. DBG_CONTINUE here would hide crashes.
                disposition = DBG_EXCEPTION_NOT_HANDLED;
                if (observations.size() >= 4096) loss = true;
                else {
                    const auto tick = stamp();
                    unsigned long long thread_ticks = 0;
                    const bool cpu_available = thread_cpu(tid, thread_ticks);
                    std::ostringstream row;
                    row << "{\"code\":" << code << ",\"address\":"
                        << reinterpret_cast<DWORD64>(info.ExceptionRecord.ExceptionAddress)
                        << ",\"tid\":" << tid << ",\"qpc\":" << tick
                        << ",\"first_chance\":" << (info.dwFirstChance ? "true" : "false") << ",\"stack\":[";
                    CONTEXT context{}; context.ContextFlags = CONTEXT_FULL;
                    if (!threads.count(tid) || !GetThreadContext(threads.at(tid), &context)) loss = true;
                    else {
                        STACKFRAME64 stack{};
                        stack.AddrPC = {context.Rip, 0, AddrModeFlat};
                        stack.AddrFrame = {context.Rbp, 0, AddrModeFlat};
                        stack.AddrStack = {context.Rsp, 0, AddrModeFlat};
                        row << captured_frame(context.Rip);
                        for (unsigned depth = 1; depth < 32 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process.value,
                             threads.at(tid), &stack, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr); ++depth) {
                            if (!stack.AddrPC.Offset) break;
                            row << ',' << captured_frame(stack.AddrPC.Offset);
                        }
                    }
                    auto payload = row.str() + "]}";
                    if (payload.size() > 65536 || payload.size() > kMaxBufferedEventBytes - observation_bytes) loss = true;
                    else {
                        observation_bytes += payload.size(); observations.push_back(std::move(payload));
                        if (info.dwFirstChance)
                            pending_exceptions[tid] = {observations.size() - 1, thread_ticks, tick, cpu_available};
                        else pending_exceptions.erase(tid);
                    }
                }
            }
        }
        if (!ContinueDebugEvent(event.dwProcessId, event.dwThreadId, disposition))
            throw std::runtime_error("exception continue failed");
    }
    const bool deadline_exceeded = !owned.exited && GetTickCount64() >= deadline;
    std::vector<std::string> timeout_threads;
    if (deadline_exceeded) {
        for (const auto& thread : threads) {
            if (timeout_threads.size() == 16) break;
            if (SuspendThread(thread.second) == MAXDWORD) {
                timeout_threads.push_back("{\"tid\":" + std::to_string(thread.first) + ",\"context_available\":false}");
                continue;
            }
            CONTEXT context{};
            context.ContextFlags = CONTEXT_FULL;
            std::ostringstream row;
            row << "{\"tid\":" << thread.first;
            const bool available = GetThreadContext(thread.second, &context) != FALSE;
            row << ",\"context_available\":" << (available ? "true" : "false") << ",\"stack\":[";
            if (available) {
                row << captured_frame(context.Rip);
                STACKFRAME64 stack{};
                stack.AddrPC = {context.Rip, 0, AddrModeFlat};
                stack.AddrFrame = {context.Rbp, 0, AddrModeFlat};
                stack.AddrStack = {context.Rsp, 0, AddrModeFlat};
                for (unsigned depth = 1; depth < 16 && StackWalk64(IMAGE_FILE_MACHINE_AMD64, process.value,
                     thread.second, &stack, &context, nullptr, SymFunctionTableAccess64, SymGetModuleBase64, nullptr); ++depth) {
                    if (!stack.AddrPC.Offset) break;
                    row << ',' << captured_frame(stack.AddrPC.Offset);
                }
            }
            if (ResumeThread(thread.second) == MAXDWORD) throw std::runtime_error("timeout thread resume failed");
            timeout_threads.push_back(row.str() + "]}");
        }
    }
    const auto end = stamp();
    const bool api_complete = !api_loss && owned.exited && api_steps.empty() && !api_points.empty();
    const bool complete = owned.exited && !loss && api_complete;
    const bool times = owned.exited && GetProcessTimes(process.value, &created, &exited, &kernel, &user);
    const auto cpu = ((static_cast<unsigned long long>(kernel.dwHighDateTime) << 32U) | kernel.dwLowDateTime)
                   + ((static_cast<unsigned long long>(user.dwHighDateTime) << 32U) | user.dwLowDateTime);
    std::cout << "{\"schemaVersion\":1,\"provenance\":\"external\",\"confidence\":\"observed\",\"complete\":"
            << (complete ? "true" : "false")
            << ",\"pid\":" << child.dwProcessId
            << ",\"process_epoch\":" << epoch
            << ",\"main_thread_id\":" << child.dwThreadId
            << ",\"exit_code\":" << exit_code
            << ",\"observation_timeout_ms\":" << timeout_ms
            << ",\"deadline_exceeded\":"
            << (deadline_exceeded ? "true" : "false")
            << ",\"qpc_frequency\":" << frequency.QuadPart
            << ",\"begin_qpc\":" << begin
            << ",\"end_qpc\":" << end
            << ",\"process_cpu_100ns\":";
    if (times) std::cout << cpu; else std::cout << "null";
    std::cout << ",\"exceptions\":[";
    for (std::size_t index = 0; index < observations.size(); ++index) std::cout << (index ? "," : "") << observations[index];
    std::cout << "],\"recovery_markers\":[";
    for (std::size_t index = 0; index < continuations.size(); ++index) std::cout << (index ? "," : "") << continuations[index];
    std::cout << "],\"verifier_modules\":[";
    for (std::size_t index = 0; index < verifier_modules.size(); ++index) std::cout << (index ? "," : "") << verifier_modules[index];
    std::cout << "],\"api_entry_scope_complete\":" << (api_complete ? "true" : "false")
              << ",\"api_supported_exports\":[";
    bool first_export = true;
    for (const auto& item : api_points) {
        std::cout << (first_export ? "" : ",") << text(std::wstring(item.second.name.begin(),item.second.name.end()));
        first_export = false;
    }
    std::cout << "],\"api_calls\":[";
    for (std::size_t index = 0; index < api_calls.size(); ++index) std::cout << (index ? "," : "") << api_calls[index];
    std::cout << "],\"timeout_threads\":[";
    for (std::size_t index = 0; index < timeout_threads.size(); ++index) std::cout << (index ? "," : "") << timeout_threads[index];
    std::cout << "],\"timeout_thread_capacity\":16,\"scope\":\"Owned target debugger; recovery markers describe target continuation; process CPU includes all target work, not per-exception cost\"}" << std::endl;
    return complete ? 0 : 2;
#endif
}

int observe_exception_fixture(const wchar_t* binary, std::wstring mode) {
    if (mode != L"handled" && mode != L"crash" && mode != L"storm" && mode != L"assert" && mode != L"contract"
        && mode != L"verifier-good" && mode != L"verifier-bad")
        throw std::runtime_error("invalid exception fixture variant");
    return observe_exceptions(binary, {mode}, 15000UL);
}

int observe_mutex_fixture(const wchar_t* binary, const std::wstring& mode) {
#if !defined(_M_X64)
    (void)binary; (void)mode;
    throw std::runtime_error("mutex native debugger requires x64");
#else
    const std::wstring image(binary);
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
    const auto creation = (static_cast<unsigned long long>(created.dwHighDateTime) << 32U) | created.dwLowDateTime;
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
              << "\"pid\":" << child.dwProcessId 
              << ",\"creation_filetime_100ns\":" << creation 
              << ",\"owner_os_tid\":" << child.dwThreadId
              << ",\"resource_name\":" << text(name) << ",\"variant\":" 
              << text(mode) << ",\"mutex_handle\":" << mutex_handle
              << ",\"complete\":" << (complete ? "true" : "false") 
              << ",\"not_new\":" << (not_new ? "true" : "false")
              << ",\"child_exit_code\":" << exit_code
              << ",\"qpc_frequency\":" << frequency.QuadPart
              << ",\"create_status\":" << create_status 
              << ",\"release_status\":" << release_status
              << ",\"acquire_begin\":" << acquire_begin 
              << ",\"acquire_end\":" << acquire_end
              << ",\"release_begin\":" << release_begin 
              << ",\"release_end\":" << release_end
              << ",\"acquisition_stack\":[";
    if (complete) for (std::size_t index = 0; index < stack.size(); ++index) std::cout << (index ? "," : "") << stack[index];
    std::cout << "],\"scope\":\"Intrusive debugger of controlled initial-owner fixture; observed NtCreateMutant/NtReleaseMutant boundary bounds include observer pauses; no arbitrary engine/syscall coverage claim\"}" << std::endl;
    return complete || (mode == L"existing" && not_new) ? 0 : 2;
#endif
}

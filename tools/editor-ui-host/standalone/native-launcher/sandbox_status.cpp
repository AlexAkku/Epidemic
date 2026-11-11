#include <Windows.h>
#include <TlHelp32.h>
#include <winternl.h>
#include <array>
#include <cstdint>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

namespace {
struct Handle {
    HANDLE value;
    ~Handle() { if (value && value != INVALID_HANDLE_VALUE) CloseHandle(value); }
};
std::wstring CommandLine(HANDLE process) {
    using Query = LONG(NTAPI *)(HANDLE, ULONG, PVOID, ULONG, PULONG);
    const auto query = reinterpret_cast<Query>(GetProcAddress(GetModuleHandleW(L"ntdll.dll"), "NtQueryInformationProcess"));
    if (!query) return {};
    // Windows 10+ ProcessCommandLineInformation; query failure is reported by
    // omission, never inferred from a PID, window title or integrity level.
    constexpr ULONG command_line_information = 60;
    ULONG bytes = 0;
    query(process, command_line_information, nullptr, 0, &bytes);
    if (bytes < sizeof(UNICODE_STRING) || bytes > 1024 * 1024) return {};
    std::vector<std::byte> buffer(bytes);
    if (query(process, command_line_information, buffer.data(), bytes, &bytes) < 0) return {};
    const auto text = reinterpret_cast<const UNICODE_STRING *>(buffer.data());
    const auto begin = reinterpret_cast<std::uintptr_t>(buffer.data());
    const auto data = reinterpret_cast<std::uintptr_t>(text->Buffer);
    if (data < begin || data > begin + buffer.size() || text->Length > begin + buffer.size() - data) return {};
    return {text->Buffer, text->Length / sizeof(wchar_t)};
}
std::wstring Image(HANDLE process) {
    std::array<wchar_t, 32768> buffer{};
    DWORD size = static_cast<DWORD>(buffer.size());
    if (!QueryFullProcessImageNameW(process, 0, buffer.data(), &size)) return {};
    return {buffer.data(), size};
}
}
std::string InspectSandbox() {
    Handle snapshot{CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0)};
    if (snapshot.value == INVALID_HANDLE_VALUE) return "{\"error\":\"snapshot\"}";
    std::map<DWORD, DWORD> parents;
    PROCESSENTRY32W entry{}; entry.dwSize = sizeof(entry);
    if (Process32FirstW(snapshot.value, &entry))
        do { parents.emplace(entry.th32ProcessID, entry.th32ParentProcessID); } while (Process32NextW(snapshot.value, &entry));
    std::set<DWORD> owned{GetCurrentProcessId()};
    for (std::size_t pass = 0; pass < 16; ++pass) {
        const auto count = owned.size();
        for (const auto &[pid, parent] : parents) if (owned.contains(parent)) owned.insert(pid);
        if (owned.size() == count) break;
    }
    const auto own_image = Image(GetCurrentProcess());
    std::ostringstream result; result << "{\"processes\":[";
    bool first = true;
    for (const auto pid : owned) {
        if (pid == GetCurrentProcessId()) continue;
        Handle process{OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid)};
        if (!process.value) continue;
        if (_wcsicmp(Image(process.value).c_str(), own_image.c_str()) != 0) continue;
        const auto command = CommandLine(process.value);
        const bool renderer = command.find(L"--type=renderer") != std::wstring::npos;
        HANDLE raw_token = nullptr;
        if (!OpenProcessToken(process.value, TOKEN_QUERY, &raw_token)) continue;
        Handle token{raw_token};
        alignas(TOKEN_MANDATORY_LABEL) std::array<std::byte, 256> label{};
        DWORD bytes = 0;
        if (!GetTokenInformation(token.value, TokenIntegrityLevel, label.data(), static_cast<DWORD>(label.size()), &bytes)) continue;
        const auto sid = reinterpret_cast<TOKEN_MANDATORY_LABEL *>(label.data())->Label.Sid;
        if (!IsValidSid(sid) || *GetSidSubAuthorityCount(sid) == 0) continue;
        const DWORD integrity = *GetSidSubAuthority(sid, static_cast<DWORD>(*GetSidSubAuthorityCount(sid) - 1));
        DWORD app_container = 0;
        GetTokenInformation(token.value, TokenIsAppContainer, &app_container, sizeof(app_container), &bytes);
        BOOL job = FALSE; IsProcessInJob(process.value, nullptr, &job);
        PROCESS_MITIGATION_SYSTEM_CALL_DISABLE_POLICY win32k{};
        const bool policy = GetProcessMitigationPolicy(process.value, ProcessSystemCallDisablePolicy, &win32k, sizeof(win32k)) != FALSE;
        if (!first) result << ',';
        first = false;
        result << "{\"pid\":" << pid << ",\"renderer\":" << (renderer ? "true" : "false")
               << ",\"restricted\":" << (IsTokenRestricted(token.value) ? "true" : "false")
               << ",\"integrity\":" << integrity << ",\"appContainer\":" << app_container
               << ",\"job\":" << (job ? "true" : "false")
               << ",\"win32kDisabled\":" << (policy && win32k.DisallowWin32kSystemCalls ? "true" : "false") << '}';
    }
    result << "]}"; return result.str();
}

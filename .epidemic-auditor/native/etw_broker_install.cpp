#include "etw_broker_security.h"
#include "etw_broker_protocol.h"
#include <iostream>

namespace etw_broker {
struct ServiceHandle {
    SC_HANDLE value = nullptr;
    explicit ServiceHandle(SC_HANDLE input) : value(input) { if (!value) throw std::runtime_error("service manager operation failed"); }
    ~ServiceHandle() { CloseServiceHandle(value); }
    ServiceHandle(const ServiceHandle&) = delete;
    ServiceHandle& operator=(const ServiceHandle&) = delete;
};
static std::string checked_digest(const wchar_t* input) {
    const std::wstring value(input);
    if (value.size() != 64 || value.find_first_not_of(L"0123456789abcdef") != std::wstring::npos) throw std::runtime_error("qualified SHA256 required");
    std::string result;
    for (const wchar_t character : value) result += static_cast<char>(character);
    return result;
}
static void archive_file(const std::filesystem::path& source, const std::filesystem::path& target, const std::wstring& sid) {
    no_reparse(source);
    no_reparse(target.parent_path());
    std::string expected;
    {
        Handle input(CreateFileW(api_path(source).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        expected = hash_file(input.value);
        if (GetFileAttributesW(api_path(target).c_str()) == INVALID_FILE_ATTRIBUTES) {
            Descriptor descriptor(L"O:SYG:SYD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;0x001200a9;;;" + sid + L")");
            SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor.value, FALSE};
            Handle output(CreateFileW(api_path(target).c_str(), GENERIC_WRITE, FILE_SHARE_READ, &attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
            LARGE_INTEGER zero{};
            if (!SetFilePointerEx(input.value, zero, nullptr, FILE_BEGIN)) throw std::runtime_error("archive seek failed");
            std::array<unsigned char, 65536> buffer{};
            DWORD count = 0;
            do {
                if (!ReadFile(input.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) throw std::runtime_error("archive read failed");
                DWORD written = 0;
                if (count && (!WriteFile(output.value, buffer.data(), count, &written, nullptr) || written != count)) throw std::runtime_error("archive write failed");
            } while (count);
            if (!FlushFileBuffers(output.value)) throw std::runtime_error("archive flush failed");
        }
    }
    no_reparse(target);
    { Handle copied(CreateFileW(api_path(target).c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
      if (hash_file(copied.value) != expected) throw std::runtime_error("archive SHA differs; C source retained"); }
    if (!DeleteFileW(api_path(source).c_str())) throw std::runtime_error("verified C duplicate deletion failed");
}
static void archive_directory(const std::filesystem::path& source, const std::filesystem::path& target, const std::wstring& sid) {
    no_reparse(source);
    if (GetFileAttributesW(api_path(target).c_str()) == INVALID_FILE_ATTRIBUTES) secure_directory(target, sid);
    auto pinned = pin_path(target);
    verify_store(pinned.back()->value, sid);
    for (const auto& entry : std::filesystem::directory_iterator(source)) {
        no_reparse(entry.path());
        const auto destination = target / entry.path().filename();
        if (entry.is_directory()) archive_directory(entry.path(), destination, sid);
        else if (entry.is_regular_file()) archive_file(entry.path(), destination, sid);
        else throw std::runtime_error("unexpected legacy store object");
    }
    if (!RemoveDirectoryW(api_path(source).c_str())) throw std::runtime_error("legacy empty directory removal failed");
}
static void repair_image(SC_HANDLE manager, const std::filesystem::path& executable, const std::wstring& sid,
        HANDLE source, const std::string& expected, const std::string& previous) {
    no_reparse(executable);
    {
        Handle old(CreateFileW(executable.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (hash_file(old.value) != previous) throw std::runtime_error("installed image differs from exact previous admission");
    }
    ServiceHandle service(OpenServiceW(manager, service_name, SERVICE_QUERY_CONFIG | SERVICE_QUERY_STATUS | SERVICE_STOP | SERVICE_START));
    DWORD required = 0;
    QueryServiceConfigW(service.value, nullptr, 0, &required);
    if (!required || required > 16384) throw std::runtime_error("fixed service configuration unavailable");
    std::vector<unsigned char> config(required);
    auto* value = reinterpret_cast<QUERY_SERVICE_CONFIGW*>(config.data());
    if (!QueryServiceConfigW(service.value, value, required, &required) || value->dwServiceType != SERVICE_WIN32_OWN_PROCESS
        || std::wstring(value->lpBinaryPathName) != L"\"" + executable.native() + L"\" --service " + sid
        || std::wstring(value->lpServiceStartName) != L"LocalSystem") throw std::runtime_error("repair must match the exact fixed service and configured SID");
    SERVICE_STATUS stopped{};
    if (!ControlService(service.value, SERVICE_CONTROL_STOP, &stopped) && GetLastError() != ERROR_SERVICE_NOT_ACTIVE)
        throw std::runtime_error("owned service stop failed");
    const auto deadline = GetTickCount64() + 60000;
    while (true) {
        if (!QueryServiceStatus(service.value, &stopped)) throw std::runtime_error("owned service state unavailable");
        if (stopped.dwCurrentState == SERVICE_STOPPED) break;
        if (GetTickCount64() >= deadline) throw std::runtime_error("owned service stop deadline exceeded");
        Sleep(100);
    }
    const auto replacement = executable.parent_path() / L"replacement.exe";
    const auto backup = executable.parent_path() / (L"previous_" + std::wstring(previous.begin(), previous.end()) + L".exe");
    struct Restore {
        SC_HANDLE service; std::filesystem::path image, replacement, backup; bool created = false, moved = false, switched = false, committed = false;
        ~Restore() {
            if (!committed && moved) {
                if (switched) DeleteFileW(image.c_str());
                MoveFileW(backup.c_str(), image.c_str());
            }
            if (!committed) { if (created) DeleteFileW(replacement.c_str()); StartServiceW(service, 0, nullptr); }
        }
    } restore{service.value, executable, replacement, backup};
    Descriptor descriptor(L"O:SYG:SYD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;0x001200a9;;;" + sid + L")");
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor.value, FALSE};
    {
        Handle output(CreateFileW(replacement.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        restore.created = true;
        LARGE_INTEGER zero{};
        if (!SetFilePointerEx(source, zero, nullptr, FILE_BEGIN)) throw std::runtime_error("repair seek failed");
        std::array<unsigned char, 65536> buffer{};
        DWORD count = 0;
        do {
            if (!ReadFile(source, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) throw std::runtime_error("repair read failed");
            DWORD written = 0;
            if (count && (!WriteFile(output.value, buffer.data(), count, &written, nullptr) || written != count)) throw std::runtime_error("repair write failed");
        } while (count);
        if (!FlushFileBuffers(output.value)) throw std::runtime_error("repair flush failed");
    }
    {
        Handle copied(CreateFileW(replacement.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
        if (hash_file(copied.value) != expected) throw std::runtime_error("repair replacement hash mismatch");
    }
    if (!MoveFileW(executable.c_str(), backup.c_str())) throw std::runtime_error("fixed previous image preservation failed");
    restore.moved = true;
    if (!MoveFileW(replacement.c_str(), executable.c_str())) throw std::runtime_error("fixed image replacement failed");
    restore.switched = true;
    const auto store = std::filesystem::path(store_path);
    const auto archives = store / L"archives";
    const auto deployments = store / L"deployments";
    for (const auto& path : {archives, deployments}) if (!std::filesystem::exists(path)) secure_directory(path, sid);
    // Only our protected legacy store and fixed backup filename grammar can
    // be migrated; copied bytes are verified before their C duplicate goes.
    const auto legacy = known_folder(FOLDERID_ProgramData) / L"EpidemicAuditorEtwBroker";
    if (std::filesystem::exists(legacy)) {
        auto legacy_pin = pin_path(legacy);
        verify_store(legacy_pin.back()->value, sid);
        const auto captures = legacy / L"captures";
        if (std::filesystem::exists(captures)) archive_directory(captures, archives / L"legacy_c_captures", sid);
    }
    std::vector<std::filesystem::path> backups;
    for (const auto& entry : std::filesystem::directory_iterator(executable.parent_path())) {
        const auto name = entry.path().filename().native();
        if (name == L"previous.exe" || (name.size() == 77 && name.substr(0, 9) == L"previous_"
                && name.substr(73) == L".exe" && name.substr(9, 64).find_first_not_of(L"0123456789abcdef") == std::wstring::npos)) backups.push_back(entry.path());
    }
    // Keep the rollback image until all other migration work succeeds.
    for (const auto& path : backups) if (path != backup) archive_file(path, deployments / path.filename(), sid);
    if (!StartServiceW(service.value, 0, nullptr)) throw std::runtime_error("repaired service start failed");
    restore.committed = true;
    archive_file(backup, deployments / backup.filename(), sid);
}
int install(const wchar_t* target_sid, const wchar_t* expected_sha, const wchar_t* previous_sha) {
    HANDLE raw = nullptr;
    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_ADJUST_PRIVILEGES, &raw)) throw std::runtime_error("installer token unavailable");
    Handle token(raw);
    if (!elevated(token.value)) throw std::runtime_error("one-time service installation requires administrator elevation");
    const auto client_sid = canonical_sid(target_sid);
    const auto expected_digest = checked_digest(expected_sha);
    TOKEN_PRIVILEGES privilege{}; privilege.PrivilegeCount = 1;
    if (!LookupPrivilegeValueW(nullptr, SE_RESTORE_NAME, &privilege.Privileges[0].Luid)) throw std::runtime_error("installer owner privilege unavailable");
    privilege.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    if (!AdjustTokenPrivileges(token.value, FALSE, &privilege, 0, nullptr, nullptr) || GetLastError() == ERROR_NOT_ALL_ASSIGNED) throw std::runtime_error("installer cannot assign protected SYSTEM ownership");
    PSID sid = nullptr;
    if (!ConvertStringSidToSidW(target_sid, &sid)) throw std::runtime_error("client SID invalid");
    wchar_t name[256]{}, domain[256]{};
    DWORD name_size = 256, domain_size = 256;
    SID_NAME_USE kind{};
    const BOOL found = LookupAccountSidW(nullptr, sid, name, &name_size, domain, &domain_size, &kind);
    LocalFree(sid);
    if (!found || kind != SidTypeUser || std::wstring(target_sid).size() > 184) throw std::runtime_error("client must be a resolvable user SID");
    if (client_sid == L"S-1-5-18" || client_sid == L"S-1-5-19" || client_sid == L"S-1-5-20") throw std::runtime_error("service accounts cannot be configured clients");
    const auto binary_root = known_folder(FOLDERID_ProgramFiles) / L"EpidemicAuditorEtwBroker";
    const auto data_root = std::filesystem::path(store_path);
    const auto executable = binary_root / L"EpidemicAuditorEtwBroker.exe";
    ServiceHandle manager(OpenSCManagerW(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE));
    wchar_t source[32768]{};
    if (!GetModuleFileNameW(nullptr, source, 32768)) throw std::runtime_error("installer image unavailable");
    no_reparse(source);
    Handle input(CreateFileW(source, GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (hash_file(input.value) != expected_digest) throw std::runtime_error("installer image SHA256 differs from qualification");
    // The project parent may be user-owned: pin it before creating the
    // SYSTEM-owned store, then verify and pin that store before any writes.
    auto parent_pin = pin_path(data_root.parent_path());
    if (!std::filesystem::exists(data_root)) secure_directory(data_root, client_sid);
    auto store_pin = pin_path(data_root);
    verify_store(store_pin.back()->value, client_sid);
    if (!std::filesystem::exists(data_root / L"captures")) secure_directory(data_root / L"captures", client_sid);
    if (previous_sha) {
        repair_image(manager.value, executable, client_sid, input.value, expected_digest, checked_digest(previous_sha));
        std::cout << "{\"status\":\"FIXED_IMAGE_REPAIR_START_REQUESTED\"}\n";
        return 0;
    }
    struct Rollback {
        std::vector<std::filesystem::path> directories;
        std::filesystem::path binary;
        SC_HANDLE service = nullptr;
        bool committed = false;
        ~Rollback() {
            if (service) {
                if (!committed) { SERVICE_STATUS status{}; ControlService(service, SERVICE_CONTROL_STOP, &status); DeleteService(service); }
                CloseServiceHandle(service);
            }
            if (committed) return;
            if (!binary.empty()) DeleteFileW(binary.c_str());
            for (auto item = directories.rbegin(); item != directories.rend(); ++item) RemoveDirectoryW(item->c_str());
        }
    } rollback;
    // Fresh installation only. Refuse existing paths, links and service state;
    // do not grant an updater arbitrary privileged filesystem operations.
    secure_directory(binary_root, client_sid);
    rollback.directories.push_back(binary_root);
    // Lock the qualified source against writes/deletion, create destination
    // with its final protected descriptor, and never copy source ACLs.
    Descriptor binary_descriptor(L"O:SYG:SYD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;0x001200a9;;;" + client_sid + L")");
    SECURITY_ATTRIBUTES binary_attributes{sizeof(SECURITY_ATTRIBUTES), binary_descriptor.value, FALSE};
    {
        Handle output(CreateFileW(executable.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &binary_attributes, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        rollback.binary = executable;
        LARGE_INTEGER zero{};
        if (!SetFilePointerEx(input.value, zero, nullptr, FILE_BEGIN)) throw std::runtime_error("installer seek failed");
        std::array<unsigned char, 65536> buffer{};
        DWORD count = 0;
        do {
            if (!ReadFile(input.value, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr)) throw std::runtime_error("installer read failed");
            DWORD written = 0;
            if (count && (!WriteFile(output.value, buffer.data(), count, &written, nullptr) || written != count)) throw std::runtime_error("installer write failed");
        } while (count);
        if (!FlushFileBuffers(output.value)) throw std::runtime_error("installer flush failed");
    }
    Handle installed(CreateFileW(executable.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    if (hash_file(installed.value) != expected_digest) throw std::runtime_error("installed image SHA256 mismatch");
    no_reparse(executable);
    const auto image = L"\"" + executable.native() + L"\" --service " + client_sid;
    rollback.service = CreateServiceW(manager.value, service_name, L"Epidemic Auditor ETW Broker",
        SERVICE_START | SERVICE_CHANGE_CONFIG | SERVICE_QUERY_STATUS | SERVICE_STOP | DELETE, SERVICE_WIN32_OWN_PROCESS,
        SERVICE_AUTO_START, SERVICE_ERROR_NORMAL, image.c_str(), nullptr, nullptr, nullptr, L"LocalSystem", nullptr);
    if (!rollback.service) throw std::runtime_error("service registration failed");
    SERVICE_DESCRIPTIONW description{const_cast<wchar_t*>(L"Fixed GeneralProfile capture only; local non-elevated SID client; broker-owned storage.")};
    if (!ChangeServiceConfig2W(rollback.service, SERVICE_CONFIG_DESCRIPTION, &description)) throw std::runtime_error("service description failed");
    wchar_t privileges[] = L"SeSystemProfilePrivilege\0SeImpersonatePrivilege\0SeChangeNotifyPrivilege\0";
    SERVICE_REQUIRED_PRIVILEGES_INFOW required{privileges};
    if (!ChangeServiceConfig2W(rollback.service, SERVICE_CONFIG_REQUIRED_PRIVILEGES_INFO, &required)) throw std::runtime_error("service privilege restriction failed");
    if (!StartServiceW(rollback.service, 0, nullptr)) throw std::runtime_error("service start failed");
    rollback.committed = true;
    std::cout << "{\"status\":\"INSTALLED_START_REQUESTED\",\"service\":\"EpidemicAuditorEtwBroker\"}\n";
    return 0;
}
}

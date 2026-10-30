#pragma once
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <sddl.h>
#include <shlobj.h>
#include <aclapi.h>
#include <string>
#include <filesystem>
#include <stdexcept>
#include <vector>
#include <bcrypt.h>
#include <array>
#include <memory>
#include <cstring>

namespace etw_broker {
inline std::wstring api_path(const std::filesystem::path &path) {
    if (!path.is_absolute())
        throw std::runtime_error("absolute broker path required");
    const auto value = path.native();
    return value.rfind(L"\\\\?\\", 0) == 0 ? value : L"\\\\?\\" + value;
}

struct Handle {
    HANDLE value = INVALID_HANDLE_VALUE;

    explicit Handle(HANDLE input) : value(input) {
        if (value == INVALID_HANDLE_VALUE || value == nullptr)
            throw std::runtime_error("Windows handle unavailable");
    }

    ~Handle() {
        CloseHandle(value);
    }

    Handle(const Handle &) = delete;
    Handle &operator=(const Handle &) = delete;
};

struct Descriptor {
    PSECURITY_DESCRIPTOR value = nullptr;

    explicit Descriptor(const std::wstring &sddl) {
        if (!ConvertStringSecurityDescriptorToSecurityDescriptorW(sddl.c_str(), SDDL_REVISION_1,
                                                                  &value, nullptr))
            throw std::runtime_error("security descriptor invalid");
    }

    ~Descriptor() {
        LocalFree(value);
    }

    Descriptor(const Descriptor &) = delete;
    Descriptor &operator=(const Descriptor &) = delete;
};

inline std::filesystem::path known_folder(REFKNOWNFOLDERID id) {
    PWSTR result = nullptr;
    if (FAILED(SHGetKnownFolderPath(id, KF_FLAG_DEFAULT, nullptr, &result)))
        throw std::runtime_error("known folder unavailable");
    const std::filesystem::path path(result);
    CoTaskMemFree(result);
    if (!path.is_absolute() || path.native().size() > 160)
        throw std::runtime_error("known folder outside bounded path contract");
    return path;
}

inline void no_reparse(const std::filesystem::path &path) {
    auto current = path.root_path();
    for (const auto &part : path.relative_path()) {
        current /= part;
        const DWORD attributes = GetFileAttributesW(api_path(current).c_str());
        if (attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error("missing or reparse broker path");
    }

}

inline void secure_directory(const std::filesystem::path &path, const std::wstring &sid) {
    no_reparse(path.parent_path());
    Descriptor descriptor(L"O:SYG:SYD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;0x001200a9;;;" +
                          sid + L")");
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), descriptor.value, FALSE};

    if (!CreateDirectoryW(api_path(path).c_str(), &attributes))
        throw std::runtime_error("broker directory must be newly created");
    no_reparse(path);
}

// Pin every ancestor against rename/deletion before accessing a store under
// the user-owned project. OPEN_REPARSE_POINT inspects the actual directory.
inline std::vector<std::unique_ptr<Handle>> pin_path(const std::filesystem::path &path) {
    std::vector<std::unique_ptr<Handle>> pinned;
    auto current = path.root_path();
    std::vector<std::filesystem::path> parts{current};

    for (const auto &part : path.relative_path()) {
        current /= part;
        parts.push_back(current);
    }

    for (const auto &part : parts) {
        auto handle = std::make_unique<Handle>(
            CreateFileW(api_path(part).c_str(), FILE_READ_ATTRIBUTES | READ_CONTROL,
                        FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_EXISTING,
                        FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, nullptr));
        BY_HANDLE_FILE_INFORMATION info{};

        if (!GetFileInformationByHandle(handle->value, &info) ||
            !(info.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) ||
            (info.dwFileAttributes & FILE_ATTRIBUTE_REPARSE_POINT))
            throw std::runtime_error("store ancestor missing or reparse");
        pinned.push_back(std::move(handle));
    }

    return pinned;
}

inline void verify_store(HANDLE handle, const std::wstring &sid) {
    PSID owner = nullptr;
    PACL actual = nullptr;
    PSECURITY_DESCRIPTOR raw = nullptr;
    if (GetSecurityInfo(handle, SE_FILE_OBJECT,
                        OWNER_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION, &owner, nullptr,
                        &actual, nullptr, &raw) != ERROR_SUCCESS)
        throw std::runtime_error("store security unavailable");

    struct FreeSecurity {
        PSECURITY_DESCRIPTOR value;

        ~FreeSecurity() {
            LocalFree(value);
        }

    } release{raw};

    SECURITY_DESCRIPTOR_CONTROL control{};

    DWORD revision = 0;
    if (!owner || !IsWellKnownSid(owner, WinLocalSystemSid) || !actual ||
        !GetSecurityDescriptorControl(raw, &control, &revision) || !(control & SE_DACL_PROTECTED))
        throw std::runtime_error("store must have protected SYSTEM ownership");
    Descriptor expected(L"O:SYG:SYD:P(A;OICI;FA;;;SY)(A;OICI;FA;;;BA)(A;OICI;0x001200a9;;;" + sid +
                        L")");
    BOOL present = FALSE, defaulted = FALSE;
    PACL wanted = nullptr;
    if (!GetSecurityDescriptorDacl(expected.value, &present, &wanted, &defaulted) || !present ||
        !wanted || actual->AceCount != wanted->AceCount)
        throw std::runtime_error("store ACL inventory differs");
    for (DWORD index = 0; index < actual->AceCount; ++index) {
        void *left = nullptr;
        void *right = nullptr;
        if (!GetAce(actual, index, &left) || !GetAce(wanted, index, &right))
            throw std::runtime_error("store ACL entry unavailable");
        const auto length = static_cast<ACE_HEADER *>(left)->AceSize;
        if (length != static_cast<ACE_HEADER *>(right)->AceSize ||
            std::memcmp(left, right, length) != 0)
            throw std::runtime_error("store grants unexpected access");
    }

}

inline std::wstring user_sid(HANDLE token) {
    DWORD needed = 0;
    GetTokenInformation(token, TokenUser, nullptr, 0, &needed);
    if (needed == 0 || needed > 16384)
        throw std::runtime_error("token identity size invalid");
    std::vector<unsigned char> buffer(needed);
    if (!GetTokenInformation(token, TokenUser, buffer.data(), needed, &needed))
        throw std::runtime_error("token identity unavailable");
    LPWSTR text = nullptr;
    if (!ConvertSidToStringSidW(reinterpret_cast<TOKEN_USER *>(buffer.data())->User.Sid, &text))
        throw std::runtime_error("token SID invalid");
    const std::wstring result(text);
    LocalFree(text);
    return result;
}

inline bool elevated(HANDLE token) {
    TOKEN_ELEVATION value{};

    DWORD size = 0;
    if (!GetTokenInformation(token, TokenElevation, &value, sizeof(value), &size))
        throw std::runtime_error("token elevation unavailable");
    return value.TokenIsElevated != 0;
}

inline std::wstring canonical_sid(const wchar_t *input) {
    PSID parsed = nullptr;
    if (!ConvertStringSidToSidW(input, &parsed))
        throw std::runtime_error("SID invalid");
    LPWSTR output = nullptr;
    const BOOL converted = ConvertSidToStringSidW(parsed, &output);
    LocalFree(parsed);
    if (!converted)
        throw std::runtime_error("SID conversion failed");
    const std::wstring value(output);
    LocalFree(output);
    if (value != input)
        throw std::runtime_error("canonical SID required");
    return value;
}

inline std::string hash_file(HANDLE file) {
    LARGE_INTEGER zero{};

    if (!SetFilePointerEx(file, zero, nullptr, FILE_BEGIN))
        throw std::runtime_error("hash seek failed");
    BCRYPT_ALG_HANDLE algorithm = nullptr;
    if (BCryptOpenAlgorithmProvider(&algorithm, BCRYPT_SHA256_ALGORITHM, nullptr, 0) < 0)
        throw std::runtime_error("SHA256 provider failed");

    struct Algorithm {
        BCRYPT_ALG_HANDLE value = nullptr;

        ~Algorithm() {
            BCryptCloseAlgorithmProvider(value, 0);
        }

    } algorithm_scope{algorithm};

    BCRYPT_HASH_HANDLE hash = nullptr;
    if (BCryptCreateHash(algorithm, &hash, nullptr, 0, nullptr, 0, 0) < 0)
        throw std::runtime_error("SHA256 init failed");

    struct Hash {
        BCRYPT_HASH_HANDLE value = nullptr;

        ~Hash() {
            BCryptDestroyHash(value);
        }

    } hash_scope{hash};

    std::array<unsigned char, 65536> buffer{};

    DWORD count = 0;
    do {
        if (!ReadFile(file, buffer.data(), static_cast<DWORD>(buffer.size()), &count, nullptr))
            throw std::runtime_error("hash read failed");
        if (count && BCryptHashData(hash, buffer.data(), count, 0) < 0)
            throw std::runtime_error("SHA256 update failed");
    } while (count);
    std::array<unsigned char, 32> result{};

    if (BCryptFinishHash(hash, result.data(), static_cast<ULONG>(result.size()), 0) < 0)
        throw std::runtime_error("SHA256 finish failed");
    std::string text;
    constexpr char digits[] = "0123456789abcdef";
    for (const auto value : result) {
        const auto byte = static_cast<unsigned int>(value);
        text += digits[byte >> 4U];
        text += digits[byte & 15U];
    }

    return text;
}

inline bool authorize(HANDLE pipe, const std::wstring &sid) {
    if (!ImpersonateNamedPipeClient(pipe))
        return false;
    bool allowed = false;
    try {
        HANDLE raw = nullptr;
        if (OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, TRUE, &raw)) {
            Handle token(raw);
            allowed = user_sid(token.value) == sid && !elevated(token.value);
        }

    } catch (...) {
        allowed = false;
    }

    // Never execute a privileged action while impersonating the client.
    if (!RevertToSelf())
        std::terminate();
    return allowed;
}

} // namespace etw_broker

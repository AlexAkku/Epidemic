#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <dbghelp.h>
#include <psapi.h>
#include <objbase.h>
#include <wct.h>
#include <evntrace.h>
#include <evntcons.h>
#include <algorithm>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>

int observe_mutex_fixture(const wchar_t *binary, const std::wstring &mode);
int observe_exception_fixture(const wchar_t *binary, std::wstring mode);
int observe_exceptions(const wchar_t *binary, const std::vector<std::wstring> &arguments,
                       DWORD timeout_ms);

static std::string utf8(const std::wstring &text) {
    if (text.empty())
        return {};

    const int count =
        WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                            static_cast<int>(text.size()), nullptr, 0, nullptr, nullptr);
    if (!count)
        throw std::runtime_error("UTF-16 conversion failed");
    std::string result(static_cast<std::size_t>(count), '\0');
    if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, text.data(),
                             static_cast<int>(text.size()), result.data(), count, nullptr, nullptr))
        throw std::runtime_error("UTF-16 conversion failed");
    return result;
}

static std::string quoted(const std::wstring &text) {
    const auto bytes = utf8(text);
    std::ostringstream output;
    output << '"';
    for (const unsigned char byte : bytes) {
        if (byte == '"' || byte == '\\')
            output << '\\' << static_cast<char>(byte);
        else if (byte < 32) {
            const char hex[] = "0123456789abcdef";
            const auto code = static_cast<unsigned int>(byte);
            output << "\\u00" << hex[code >> 4U] << hex[code & 15U];
        } else
            output << static_cast<char>(byte);
    }

    output << '"';
    return output.str();
}

struct ProcessHandle {
    HANDLE value;

    explicit ProcessHandle(DWORD pid)
        : value(OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid)) {
        if (!value)
            throw std::runtime_error("OpenProcess failed: " + std::to_string(GetLastError()));
    }

    ~ProcessHandle() {
        CloseHandle(value);
    }

    ProcessHandle(const ProcessHandle &) = delete;
    ProcessHandle &operator=(const ProcessHandle &) = delete;
};

extern "C" __declspec(noinline) unsigned long AuditorObservationFixture(unsigned long value) {
    volatile unsigned long marker = value ^ 0x51A3UL;
    return marker;
}

static unsigned long long number(const wchar_t *value) {
    std::size_t consumed = 0;
    const std::wstring text(value);
    const auto result = std::stoull(text, &consumed, 0);
    if (consumed != text.size())
        throw std::runtime_error("invalid numeric argument");
    return result;
}

static void wait_chain(std::ostream &output, DWORD pid, DWORD tid) {
    HANDLE thread = OpenThread(THREAD_QUERY_LIMITED_INFORMATION, FALSE, tid);
    if (!thread)
        throw std::runtime_error("OpenThread failed: " + std::to_string(GetLastError()));

    struct ThreadScope {
        HANDLE value;

        ~ThreadScope() {
            CloseHandle(value);
        }

    } thread_scope{thread};

    const DWORD owner = GetProcessIdOfThread(thread);
    if (owner != pid)
        throw std::runtime_error("thread does not belong to target process");
    HWCT session = OpenThreadWaitChainSession(0, nullptr);
    if (!session)
        throw std::runtime_error("OpenThreadWaitChainSession failed");

    struct SessionScope {
        HWCT value;

        ~SessionScope() {
            CloseThreadWaitChainSession(value);
        }

    } scope{session};

    // Win32 output buffer. Zero-initialized until GetThreadWaitChain
    // fills the nodes; enum members are read only on successful capture.
    WAITCHAIN_NODE_INFO
        nodes[WCT_MAX_NODE_COUNT]{}; // NOLINT(bugprone-invalid-enum-default-initialization)

    DWORD count = WCT_MAX_NODE_COUNT;
    BOOL cycle = FALSE;
    LARGE_INTEGER begin{}, query_end{}, frequency{};

    QueryPerformanceFrequency(&frequency);
    QueryPerformanceCounter(&begin);
    const BOOL observed =
        GetThreadWaitChain(session, 0, WCT_OUT_OF_PROC_FLAG, tid, &count, nodes, &cycle);
    const DWORD error = observed ? ERROR_SUCCESS : GetLastError();
    QueryPerformanceCounter(&query_end);
    output << "{\"os_tid\":" << tid << ",\"capacity\":" << WCT_MAX_NODE_COUNT
           << ",\"available\":" << (observed ? "true" : "false") << ",\"error_code\":" << error
           << ",\"node_count\":" << count << ",\"cycle\":";
    if (observed)
        output << (cycle ? "true" : "false");
    else
        output << "null";
    output << ",\"nodes\":[";
    if (observed) {
        if (count > WCT_MAX_NODE_COUNT)
            throw std::runtime_error("WCT exceeded node capacity");
        for (DWORD index = 0; index < count; ++index) {
            if (index)
                output << ',';
            const auto &node = nodes[index];
            output << "{\"object_type\":" << node.ObjectType
                   << ",\"object_status\":" << node.ObjectStatus;
            if (node.ObjectType == WctThreadType) {
                output << ",\"kind\":\"thread\",\"pid\":" << node.ThreadObject.ProcessId
                       << ",\"os_tid\":" << node.ThreadObject.ThreadId;
            } else {
                const auto end = std::find(std::begin(node.LockObject.ObjectName),
                                           std::end(node.LockObject.ObjectName), L'\0');
                output << ",\"kind\":\"wait_resource\",\"name\":"
                       << quoted(std::wstring(std::begin(node.LockObject.ObjectName), end));
            }

            output << '}';
        }

    }

    output
        << "],\"clock_domain\":\"windows_qpc_ticks\",\"query_begin\":" << begin.QuadPart
        << ",\"query_end\":" << query_end.QuadPart << ",\"qpc_frequency\":" << frequency.QuadPart
        << ",\"scope\":\"WCT snapshot; COM callbacks not registered; no hold duration inferred\"}";
}

int wmain(int argc, wchar_t **argv) {
    try {
        if (argc == 5 && std::wstring(argv[1]) == L"--symbol-file") {
            const std::filesystem::path image(argv[2]), addresses(argv[4]);
            const auto base = number(argv[3]);
            WIN32_FILE_ATTRIBUTE_DATA attributes{};

            if (!GetFileAttributesExW(addresses.c_str(), GetFileExInfoStandard, &attributes)) {
                throw std::runtime_error("symbol batch file size unavailable");
            }

            if ((attributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) != 0) {
                throw std::runtime_error("symbol batch path is a directory");
            }

            const auto file_size =
                (static_cast<unsigned long long>(attributes.nFileSizeHigh) << 32U) |
                static_cast<unsigned long long>(attributes.nFileSizeLow);

            if (file_size > 512ULL * 1024ULL) {
                throw std::runtime_error("symbol batch exceeded bound");
            }

            const auto process = GetCurrentProcess();
            SymSetOptions(SYMOPT_LOAD_LINES | SYMOPT_EXACT_SYMBOLS | SYMOPT_FAIL_CRITICAL_ERRORS |
                          SYMOPT_NO_PROMPTS | SYMOPT_UNDNAME);
            if (!SymInitializeW(process, image.parent_path().c_str(), FALSE))
                throw std::runtime_error("offline symbols unavailable");

            struct Cleanup {
                HANDLE process;

                ~Cleanup() {
                    SymCleanup(process);
                }

            } cleanup{process};

            if (!SymLoadModuleExW(process, nullptr, image.c_str(), nullptr, base, 0, nullptr, 0))
                throw std::runtime_error("offline module load failed");
            IMAGEHLP_MODULEW64 module{};

            module.SizeOfStruct = sizeof(module);
            const bool matched = SymGetModuleInfoW64(process, base, &module) &&
                                 module.SymType == SymPdb && !module.PdbUnmatched &&
                                 !module.DbgUnmatched;
            wchar_t guid[40]{};

            if (matched &&
                !StringFromGUID2(module.PdbSig70, guid, static_cast<int>(std::size(guid))))
                throw std::runtime_error("PDB identity unavailable");
            std::cout << "{\"pdb_matched\":" << (matched ? "true" : "false")
                      << ",\"pdb_guid\":" << quoted(guid) << ",\"pdb_age\":" << module.PdbAge
                      << ",\"symbols\":[";
            std::ifstream input(addresses);
            DWORD64 address = 0;
            unsigned count = 0;
            while (input >> address) {
                if (++count > 8192)
                    throw std::runtime_error("symbol batch exceeded count bound");
                if (count > 1)
                    std::cout << ',';
                alignas(SYMBOL_INFOW) unsigned char
                    bytes[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)]{};

                auto *symbol = reinterpret_cast<SYMBOL_INFOW *>(bytes);
                symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
                symbol->MaxNameLen = MAX_SYM_NAME;
                DWORD64 displacement = 0;
                const bool resolved =
                    matched && SymFromAddrW(process, address, &displacement, symbol);
                IMAGEHLP_LINEW64 line{};

                line.SizeOfStruct = sizeof(line);
                DWORD offset = 0;
                const bool source =
                    resolved && SymGetLineFromAddrW64(process, address, &offset, &line);
                std::cout << "{\"address\":" << address << ",\"function_address\":";
                if (resolved)
                    std::cout << symbol->Address;
                else
                    std::cout << "null";
                std::cout << ",\"function\":";
                if (resolved)
                    std::cout << quoted(std::wstring(symbol->Name, symbol->NameLen));
                else
                    std::cout << "null";
                std::cout << ",\"source_file\":";
                if (source)
                    std::cout << quoted(line.FileName);
                else
                    std::cout << "null";
                std::cout << ",\"source_line\":";
                if (source)
                    std::cout << line.LineNumber;
                else
                    std::cout << "null";
                std::cout << '}';
            }

            if (!input.eof())
                throw std::runtime_error("malformed symbol address batch");
            std::cout << "]}\n";
            return 0;
        }

        if (argc == 3 && std::wstring(argv[1]) == L"--trace-header") {
            EVENT_TRACE_LOGFILEW logfile{};

            logfile.LogFileName = argv[2];
            logfile.ProcessTraceMode = PROCESS_TRACE_MODE_EVENT_RECORD;
            const auto trace = OpenTraceW(&logfile);
            if (trace == INVALID_PROCESSTRACE_HANDLE)
                throw std::runtime_error("OpenTrace failed: " + std::to_string(GetLastError()));
            const auto header = logfile.LogfileHeader;
            if (CloseTrace(trace) != ERROR_SUCCESS)
                throw std::runtime_error("CloseTrace failed");
            std::cout << "{\"buffer_size\":" << header.BufferSize
                      << ",\"buffers_written\":" << header.BuffersWritten
                      << ",\"buffers_lost\":" << header.BuffersLost
                      << ",\"events_lost\":" << header.EventsLost
                      << ",\"log_file_mode\":" << header.LogFileMode
                      << ",\"start_time\":" << header.StartTime.QuadPart
                      << ",\"end_time\":" << header.EndTime.QuadPart << "}\n";
            return 0;
        }

        if (argc == 4 && std::wstring(argv[1]) == L"--mutex-fixture")
            return observe_mutex_fixture(argv[2], std::wstring(argv[3]));
        if (argc == 4 && std::wstring(argv[1]) == L"--exception-fixture")
            return observe_exception_fixture(argv[2], std::wstring(argv[3]));
        if (argc >= 3 && (std::wstring(argv[1]) == L"--exceptions" ||
                          std::wstring(argv[1]) == L"--exceptions-verifier")) {

            const bool verifier = std::wstring(argv[1]) == L"--exceptions-verifier";
            int binary_index = 2;
            DWORD timeout_ms = verifier ? 90000UL : 15000UL;
            if (!verifier && std::wstring(argv[2]) == L"--timeout-ms") {
                if (argc < 5)
                    throw std::runtime_error("--timeout-ms requires a value and binary");
                const std::wstring value(argv[3]);
                if (value.empty() || value.find_first_not_of(L"0123456789") != std::wstring::npos)
                    throw std::runtime_error("invalid native observer timeout");
                std::size_t consumed = 0;
                const auto requested = std::stoull(value, &consumed);
                if (consumed != value.size() || requested == 0 || requested > 60000)
                    throw std::runtime_error(
                        "native observer timeout must be 1..60000 milliseconds");
                timeout_ms = static_cast<DWORD>(requested);
                binary_index = 4;
            }

            if (verifier) {
                const std::wstring path(argv[2]);
                const auto pos = path.find_last_of(L"\\/");
                const auto name = path.substr(pos == std::wstring::npos ? 0 : pos + 1);

                if (argc != 4 || _wcsicmp(name.c_str(), L"EpidemicAuditorShadowFixture.exe") != 0 ||
                    (std::wstring(argv[3]) != L"verifier-good" &&
                     std::wstring(argv[3]) != L"verifier-bad" &&
                     std::wstring(argv[3]) != L"leak-good" &&
                     std::wstring(argv[3]) != L"leak-bad")) {

                    throw std::runtime_error("extended verifier observation is restricted "
                                             "to the owned fixture");
                }

            }

            std::vector<std::wstring> arguments;

            for (int index = binary_index + 1; index < argc; ++index) {
                arguments.emplace_back(argv[index]);
            }

            return observe_exceptions(argv[binary_index], arguments, timeout_ms);
        }

        DWORD pid = GetCurrentProcessId();
        DWORD64 address = reinterpret_cast<DWORD64>(&AuditorObservationFixture);
        bool self = false;
        DWORD thread_id = 0;
        bool wait_only = false;
        for (int index = 1; index < argc; ++index) {
            const std::wstring option(argv[index]);
            if (option == L"--self")
                self = true;
            else if (option == L"--wait-only")
                wait_only = true;
            else if ((option == L"--pid" || option == L"--address" || option == L"--thread") &&
                     index + 1 < argc) {
                const auto value = number(argv[++index]);
                if (option == L"--pid") {
                    if (!value || value > MAXDWORD)
                        throw std::runtime_error("invalid target PID");
                    pid = static_cast<DWORD>(value);
                    address = 0;
                } else if (option == L"--thread") {
                    if (!value || value > MAXDWORD)
                        throw std::runtime_error("invalid target thread ID");
                    thread_id = static_cast<DWORD>(value);
                } else
                    address = value;
            } else
                throw std::runtime_error("usage: --self or --pid PID [--address address]");
        }

        if (!self && pid == GetCurrentProcessId())
            throw std::runtime_error("explicit observation target required");
        if (self && AuditorObservationFixture(17) != (17UL ^ 0x51A3UL))
            throw std::runtime_error("fixture function did not execute");
        ProcessHandle process(pid);
        FILETIME created{}, exited{}, kernel{}, user{};

        if (!GetProcessTimes(process.value, &created, &exited, &kernel, &user))
            throw std::runtime_error("GetProcessTimes failed");
        if (wait_only) {
            if (!thread_id)
                throw std::runtime_error("wait-only observation requires target thread");
            std::cout << "{\"schemaVersion\":1,\"observationKind\":\"wait_chain_only\",\"pid\":"
                      << pid << ",\"creation_filetime_100ns\":"
                      << ((static_cast<unsigned long long>(created.dwHighDateTime) << 32U) |
                          created.dwLowDateTime)
                      << ",\"wait_chain\":";
            wait_chain(std::cout, pid, thread_id);
            std::cout << ",\"provenance\":\"external\",\"confidence\":\"observed\"}\n";
            return 0;
        }

        HMODULE modules[4096]{};

        DWORD needed = 0;
        if (!EnumProcessModulesEx(process.value, modules, sizeof(modules), &needed,
                                  LIST_MODULES_ALL))
            throw std::runtime_error("EnumProcessModulesEx failed: " +
                                     std::to_string(GetLastError()));
        wchar_t own_image[32768]{};

        if (!GetModuleFileNameW(nullptr, own_image, static_cast<DWORD>(std::size(own_image))))
            throw std::runtime_error("own image unavailable");
        const std::wstring image(own_image);
        const auto search = image.substr(0, image.find_last_of(L"\\/"));
        SymSetOptions(SYMOPT_UNDNAME | SYMOPT_LOAD_LINES | SYMOPT_FAIL_CRITICAL_ERRORS |
                      SYMOPT_NO_PROMPTS);
        const bool symbols = SymInitializeW(process.value, search.c_str(), FALSE) != FALSE;

        struct SymbolScope {
            HANDLE handle = nullptr;
            bool active = false;

            ~SymbolScope() {
                if (active)
                    SymCleanup(handle);
            }

        } scope{process.value, symbols};

        std::ostringstream output;
        output << "{\"schemaVersion\":1,\"pid\":" << pid << ",\"creation_filetime_100ns\":"
               << ((static_cast<unsigned long long>(created.dwHighDateTime) << 32U) |
                   created.dwLowDateTime)
               << ",\"module_capacity\":4096,\"module_count\":" << needed / sizeof(HMODULE)
               << ",\"module_list_complete\":" << (needed <= sizeof(modules) ? "true" : "false")
               << ",\"modules\":[";
        const auto count = std::min<std::size_t>(needed / sizeof(HMODULE), std::size(modules));
        for (std::size_t index = 0; index < count; ++index) {
            if (index)
                output << ',';
            wchar_t path[32768]{};

            MODULEINFO information{};

            const bool observed = GetModuleFileNameExW(process.value, modules[index], path,
                                                       static_cast<DWORD>(std::size(path))) != 0 &&
                                  GetModuleInformation(process.value, modules[index], &information,
                                                       sizeof(information)) != FALSE;
            output << "{\"available\":" << (observed ? "true" : "false");
            if (observed) {
                output << ",\"path\":" << quoted(path)
                       << ",\"base\":" << reinterpret_cast<DWORD64>(information.lpBaseOfDll)
                       << ",\"size\":" << information.SizeOfImage;
                DWORD unused = 0;
                const DWORD version_size = GetFileVersionInfoSizeW(path, &unused);
                std::vector<unsigned char> version(version_size <= 1024 * 1024 ? version_size : 0);
                VS_FIXEDFILEINFO *file_info = nullptr;
                UINT file_info_size = 0;
                const bool has_version =
                    version_size && version_size <= 1024 * 1024 &&
                    GetFileVersionInfoW(path, 0, version_size, version.data()) &&
                    VerQueryValueW(version.data(), L"\\", reinterpret_cast<void **>(&file_info),
                                   &file_info_size) &&
                    file_info_size >= sizeof(VS_FIXEDFILEINFO) && file_info &&
                    file_info->dwSignature == 0xFEEF04BD;
                output << ",\"file_version\":";
                if (has_version)
                    output << '"' << HIWORD(file_info->dwFileVersionMS) << '.'
                           << LOWORD(file_info->dwFileVersionMS) << '.'
                           << HIWORD(file_info->dwFileVersionLS) << '.'
                           << LOWORD(file_info->dwFileVersionLS) << '"';
                else
                    output << "null";
                if (symbols)
                    SymLoadModuleExW(process.value, nullptr, path, nullptr,
                                     reinterpret_cast<DWORD64>(information.lpBaseOfDll),
                                     information.SizeOfImage, nullptr, 0);
                IMAGEHLP_MODULEW64 module_symbols{};

                module_symbols.SizeOfStruct = sizeof(module_symbols);
                const bool pdb =
                    symbols &&
                    SymGetModuleInfoW64(process.value,
                                        reinterpret_cast<DWORD64>(information.lpBaseOfDll),
                                        &module_symbols) &&
                    module_symbols.SymType == SymPdb && !module_symbols.PdbUnmatched &&
                    !module_symbols.DbgUnmatched;
                output << ",\"pdb_identity\":";
                if (pdb) {
                    wchar_t guid[40]{};

                    if (!StringFromGUID2(module_symbols.PdbSig70, guid,
                                         static_cast<int>(std::size(guid))))
                        throw std::runtime_error("PDB GUID conversion failed");
                    output << "{\"guid\":" << quoted(guid) << ",\"age\":" << module_symbols.PdbAge
                           << ",\"path\":" << quoted(module_symbols.LoadedPdbName) << '}';
                } else
                    output << "null";
            }

            output << '}';
        }

        output << "],\"symbol\":{\"raw_address\":" << address;
        alignas(SYMBOL_INFOW) unsigned char
            symbol_bytes[sizeof(SYMBOL_INFOW) + MAX_SYM_NAME * sizeof(wchar_t)]{};

        auto *symbol = reinterpret_cast<SYMBOL_INFOW *>(symbol_bytes);
        symbol->SizeOfStruct = sizeof(SYMBOL_INFOW);
        symbol->MaxNameLen = MAX_SYM_NAME;
        DWORD64 displacement = 0;
        const bool resolved =
            symbols && address && SymFromAddrW(process.value, address, &displacement, symbol);
        IMAGEHLP_LINEW64 line{};

        line.SizeOfStruct = sizeof(line);
        DWORD line_displacement = 0;
        const bool source =
            resolved && SymGetLineFromAddrW64(process.value, address, &line_displacement, &line);
        output << ",\"status\":\""
               << (source     ? "resolved"
                   : resolved ? "partial"
                              : "unavailable")
               << "\",\"function\":";
        if (resolved)
            output << quoted(std::wstring(symbol->Name, symbol->NameLen));
        else
            output << "null";
        output << ",\"source_file\":";
        if (source)
            output << quoted(line.FileName);
        else
            output << "null";
        output << ",\"source_line\":";
        if (source)
            output << line.LineNumber;
        else
            output << "null";
        output << ",\"function_address\":";
        if (resolved)
            output << symbol->Address;
        else
            output << "null";
        output << ",\"displacement\":";
        if (resolved)
            output << displacement;
        else
            output << "null";
        output << "},\"wait_chain\":";
        if (thread_id)
            wait_chain(output, pid, thread_id);
        else
            output << "null";
        output << ",\"provenance\":\"external\",\"confidence\":\"observed\"}";
        std::cout << output.str() << '\n';
        return 0;
    } catch (const std::exception &error) {
        std::cerr << error.what() << '\n';
        return 2;
    }

}

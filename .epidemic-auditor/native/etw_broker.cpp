#include "etw_broker_profile.h"
#include "etw_broker_protocol.h"
#include "etw_broker_security.h"
#include <algorithm>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <sstream>

namespace etw_broker {
static HANDLE stop_event =
    nullptr; // One service-lifetime event, released by process exit.
static SERVICE_STATUS_HANDLE status_handle = nullptr;
static std::wstring client_sid;

static std::string utf8(const std::wstring &value) {
  if (value.empty())
    return {};
  const int count = WideCharToMultiByte(
      CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
      static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  if (!count)
    throw std::runtime_error("UTF-8 conversion failed");
  std::string result(static_cast<std::size_t>(count), '\0');
  if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(),
                           static_cast<int>(value.size()), result.data(), count,
                           nullptr, nullptr))
    throw std::runtime_error("UTF-8 conversion failed");
  return result;
}
static std::string json_quote(const std::string &value) {
  std::string result = "\"";
  for (const unsigned char character : value) {
    if (character == '\\' || character == '"')
      result += '\\';
    if (character < 32)
      throw std::runtime_error("control character in response");
    result += static_cast<char>(character);
  }
  return result + '"';
}
static void write_new(const std::filesystem::path &path,
                      const std::string &value) {
  no_reparse(path.parent_path());
  Handle file(CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
                          CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
  DWORD written = 0;
  if (!WriteFile(file.value, value.data(), static_cast<DWORD>(value.size()),
                 &written, nullptr) ||
      written != value.size() || !FlushFileBuffers(file.value))
    throw std::runtime_error("receipt write failed");
}
static std::uint64_t file_bytes(const std::filesystem::path &path) {
  Handle file(
      CreateFileW(api_path(path).c_str(), FILE_READ_ATTRIBUTES,
                  FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                  nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
  LARGE_INTEGER size{};
  if (!GetFileSizeEx(file.value, &size) || size.QuadPart < 0)
    throw std::runtime_error("broker file size unavailable");
  return static_cast<std::uint64_t>(size.QuadPart);
}
static std::string read_small(const std::filesystem::path &path) {
  if (file_bytes(path) > 65536)
    throw std::runtime_error("WPR diagnostics exceeded bound");
  std::ifstream input(path, std::ios::binary);
  if (!input)
    throw std::runtime_error("WPR diagnostic unavailable");
  return {std::istreambuf_iterator<char>(input),
          std::istreambuf_iterator<char>()};
}
static std::string guid() {
  GUID value{};
  if (FAILED(CoCreateGuid(&value)))
    throw std::runtime_error("capture GUID unavailable");
  wchar_t text[40]{};
  if (StringFromGUID2(value, text, 40) != 39)
    throw std::runtime_error("capture GUID malformed");
  std::wstring result(text + 1, 36);
  std::transform(result.begin(), result.end(), result.begin(),
                 [](wchar_t character) {
                   return character >= L'A' && character <= L'F'
                              ? static_cast<wchar_t>(character + (L'a' - L'A'))
                              : character;
                 });
  return utf8(result);
}
static unsigned long long qpc() {
  LARGE_INTEGER value{};
  if (!QueryPerformanceCounter(&value))
    throw std::runtime_error("QPC unavailable");
  return static_cast<unsigned long long>(value.QuadPart);
}
struct WprResult {
  DWORD exit_code = 0;
  bool timed_out = false;
  std::string stdout_text;
  std::wstring command;
};

static void verify_retention(const std::filesystem::path &root) {
  unsigned long long bytes = 0;
  unsigned captures = 0;
  for (const auto &entry :
       std::filesystem::recursive_directory_iterator(root)) {
    no_reparse(entry.path());
    if (entry.is_regular_file())
      bytes += file_bytes(entry.path());
    // WPR creates many nested temporary folders inside a single capture.
    if (entry.is_directory() && entry.path().parent_path() == root)
      ++captures;
    if (bytes > (32ULL << 30U) || captures > 128)
      throw std::runtime_error(
          "broker retention capacity exhausted; evidence retained");
  }
}

class Broker {
  std::filesystem::path root = std::filesystem::path(store_path) / L"captures";
  std::vector<std::unique_ptr<Handle>> pinned_paths;
  std::filesystem::path wpr;
  std::filesystem::path directory;
  std::string capture_id;
  std::string state = "IDLE";
  unsigned sequence = 0;
  ULONGLONG started_ms = 0;
  bool may_have_session = false;
  Peer owner;
  std::unique_ptr<Handle> owner_process;

  WprResult run(const std::wstring &arguments, DWORD timeout_ms,
                bool honor_stop = true) {
    no_reparse(wpr);
    no_reparse(directory);
    const auto stem = std::to_wstring(sequence++);
    const auto out = directory / (stem + L".stdout.log"),
               err = directory / (stem + L".stderr.log");
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};
    Handle stdout_file(CreateFileW(out.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                   &attributes, CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle stderr_file(CreateFileW(err.c_str(), GENERIC_WRITE, FILE_SHARE_READ,
                                   &attributes, CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL, nullptr));
    Handle stdin_file(CreateFileW(
        L"NUL", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, &attributes,
        OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr));
    SIZE_T bytes = 0;
    InitializeProcThreadAttributeList(nullptr, 1, 0, &bytes);
    std::vector<unsigned char> storage(bytes);
    auto *list = reinterpret_cast<LPPROC_THREAD_ATTRIBUTE_LIST>(storage.data());
    if (!InitializeProcThreadAttributeList(list, 1, 0, &bytes))
      throw std::runtime_error("handle-list initialization failed");
    struct Attributes {
      LPPROC_THREAD_ATTRIBUTE_LIST value;
      ~Attributes() { DeleteProcThreadAttributeList(value); }
    } cleanup{list};
    HANDLE inherited[] = {stdout_file.value, stderr_file.value,
                          stdin_file.value};
    if (!UpdateProcThreadAttribute(list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST,
                                   static_cast<void *>(inherited),
                                   sizeof(inherited), nullptr, nullptr))
      throw std::runtime_error("handle allowlist failed");
    STARTUPINFOEXW start{};
    start.StartupInfo.cb = sizeof(start);
    start.StartupInfo.dwFlags = STARTF_USESTDHANDLES;
    start.StartupInfo.hStdOutput = stdout_file.value;
    start.StartupInfo.hStdError = stderr_file.value;
    start.StartupInfo.hStdInput = stdin_file.value;
    start.lpAttributeList = list;
    const std::wstring immutable_command = L"\"" + wpr.native() + L"\" " +
                                           arguments + L" -instancename " +
                                           instance_name;
    std::wstring command(immutable_command);
    Handle job(CreateJobObjectW(nullptr, nullptr));
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limits{};
    limits.BasicLimitInformation.LimitFlags =
        JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    if (!SetInformationJobObject(job.value, JobObjectExtendedLimitInformation,
                                 &limits, sizeof(limits)))
      throw std::runtime_error("WPR job configuration failed");
    PROCESS_INFORMATION created{};
    if (!CreateProcessW(
            wpr.c_str(), command.data(), nullptr, nullptr, TRUE,
            CREATE_NO_WINDOW | CREATE_SUSPENDED | EXTENDED_STARTUPINFO_PRESENT,
            nullptr, directory.c_str(), &start.StartupInfo, &created))
      throw std::runtime_error("fixed WPR launch failed");
    Handle process(created.hProcess), thread(created.hThread);
    if (!AssignProcessToJobObject(job.value, process.value)) {
      // This suspended process was created exclusively by the broker.
      TerminateProcess(process.value, ERROR_PROCESS_ABORTED);
      WaitForSingleObject(process.value, 5000);
      throw std::runtime_error("WPR process ownership failed");
    }
    if (ResumeThread(thread.value) == static_cast<DWORD>(-1))
      throw std::runtime_error("WPR resume failed");
    HANDLE waits[] = {process.value, stop_event};
    const DWORD waited =
        WaitForMultipleObjects(honor_stop ? 2 : 1, waits, FALSE, timeout_ms);
    WprResult result;
    result.command = immutable_command;
    result.timed_out = waited != WAIT_OBJECT_0;
    if (result.timed_out) {
      TerminateJobObject(job.value, ERROR_TIMEOUT);
      WaitForSingleObject(process.value, 5000);
    }
    if (!GetExitCodeProcess(process.value, &result.exit_code) ||
        result.exit_code == STILL_ACTIVE)
      throw std::runtime_error("WPR exit status unavailable");
    // Child is finished; closing inherited output handles flushes these files.
    result.stdout_text = read_small(out);
    if (file_bytes(err) > 65536)
      throw std::runtime_error("WPR error output exceeded bound");
    write_new(directory / (stem + L".command.json"),
              "{\"application\":" + json_quote(utf8(wpr.native())) +
                  ",\"command\":" + json_quote(utf8(result.command)) +
                  ",\"exit_code\":" + std::to_string(result.exit_code) +
                  ",\"timed_out\":" + (result.timed_out ? "true" : "false") +
                  "}");
    return result;
  }
  std::string response(bool ok, const std::string &error = {}) const {
    return "{\"protocolVersion\":1,\"ok\":" +
           std::string(ok ? "true" : "false") +
           ",\"state\":" + json_quote(state) +
           ",\"capture_id\":" + json_quote(capture_id) +
           ",\"profile\":\"GeneralProfile\",\"broker_pid\":" +
           std::to_string(GetCurrentProcessId()) +
           ",\"owner_pid\":" + std::to_string(owner.pid) +
           ",\"owner_creation_filetime_100ns\":" +
           std::to_string(owner.creation) + ",\"directory\":" +
           json_quote(directory.empty() ? "" : utf8(directory.native())) +
           ",\"error\":" + json_quote(error) + "}";
  }
  void quota() const {
    ULARGE_INTEGER available{}, total{}, free{};
    // At most 2 GiB temporary input + 2 GiB merged ETL + 2 GiB reserve.
    if (!GetDiskFreeSpaceExW(root.c_str(), &available, &total, &free)) {
      const auto error = GetLastError();
      throw std::runtime_error(
          "capture disk headroom query failed, Win32 error " +
          std::to_string(error));
    }
    if (available.QuadPart < (6ULL << 30U))
      throw std::runtime_error(
          "capture disk headroom below bounded 6 GiB budget");
    verify_retention(root);
  }

public:
  Broker() {
    pinned_paths = pin_path(root);
    verify_store(pinned_paths.back()->value, client_sid);
    no_reparse(root);
    wchar_t system[32768]{};
    if (!GetSystemDirectoryW(system, 32768))
      throw std::runtime_error("Windows system directory unavailable");
    wpr = std::filesystem::path(system) / L"wpr.exe";
    no_reparse(wpr);
    // Recover only this broker's named instance, never the operator/default
    // session.
    capture_id = guid();
    directory = root / std::wstring(capture_id.begin(), capture_id.end());
    secure_directory(directory, client_sid);
    may_have_session = true;
    const auto recovered = run(L"-cancel", 30000, false);
    write_new(directory / L"recovery.json",
              "{\"scope\":\"broker named instance only\",\"exit_code\":" +
                  std::to_string(recovered.exit_code) + "}");
    const auto observed = run(L"-status", 15000, false);
    if (observed.timed_out || observed.exit_code != 0 ||
        observed.stdout_text.find("WPR is not recording") == std::string::npos)
      throw std::runtime_error("broker startup recovery is unproven");
    may_have_session = false;
    state = "IDLE";
  }
  ~Broker() {
    if (may_have_session) {
      try {
        run(L"-cancel", 30000, false);
      } catch (...) {
        OutputDebugStringW(L"Epidemic ETW broker cancellation failed; named "
                           L"instance recovery required.\n");
      }
    }
  }
  std::string execute(const Request &request, const Peer &peer) {
    if (request.operation == Operation::invalid)
      return response(false, "invalid_fixed_request");
    if (request.operation == Operation::start) {
      if (may_have_session)
        return response(false, "busy");
      quota();
      owner = peer;
      owner_process = std::make_unique<Handle>(OpenProcess(
          SYNCHRONIZE | PROCESS_QUERY_LIMITED_INFORMATION, FALSE, peer.pid));
      FILETIME created{}, ended{}, kernel{}, user{};
      if (!GetProcessTimes(owner_process->value, &created, &ended, &kernel,
                           &user) ||
          ((static_cast<std::uint64_t>(created.dwHighDateTime) << 32U) |
           created.dwLowDateTime) != peer.creation ||
          WaitForSingleObject(owner_process->value, 0) != WAIT_TIMEOUT)
        throw std::runtime_error("capture owner lifetime changed");
      capture_id = guid();
      directory = root / std::wstring(capture_id.begin(), capture_id.end());
      secure_directory(directory, client_sid);
      const auto temporary = directory / L"temp";
      secure_directory(temporary, client_sid);
      const auto profile = directory / L"GeneralProfile.wprp";
      write_new(profile, general_profile_xml);
      sequence = 0;
      started_ms = GetTickCount64();
      may_have_session = true; // Failures after START retain cleanup ownership.
      const auto begin = qpc();
      const auto result =
          run(L"-start \"" + profile.native() +
                  L"!GeneralProfile\" -filemode -recordtempto \"" +
                  temporary.native() + L"\"",
              30000);
      if (result.timed_out || result.exit_code != 0) {
        state = "FAILED";
        recover_failed_operation();
        return response(false, "wpr_start_failed");
      }
      const auto observed = run(L"-status", 15000);
      if (observed.timed_out || observed.exit_code != 0 ||
          observed.stdout_text.find("WPR recording is in progress") ==
              std::string::npos) {
        state = "FAILED";
        recover_failed_operation();
        return response(false, "recording_not_observed");
      }
      LARGE_INTEGER frequency{};
      if (!QueryPerformanceFrequency(&frequency))
        throw std::runtime_error("QPC frequency unavailable");
      write_new(directory / L"start_receipt.json",
                "{\"capture_id\":" + json_quote(capture_id) +
                    ",\"profile\":\"GeneralProfile\",\"instance\":"
                    "\"EpidemicAuditorEtwBroker\",\"start_qpc_begin\":" +
                    std::to_string(begin) + ",\"profile_xml_sha256\":" +
                    json_quote(general_profile_sha256) +
                    ",\"profile_origin_sha256\":" +
                    json_quote(general_profile_origin_sha256) +
                    ",\"start_qpc_end\":" + std::to_string(qpc()) +
                    ",\"qpc_frequency\":" + std::to_string(frequency.QuadPart) +
                    ",\"provenance\":\"external_privileged_broker\","
                    "\"confidence\":\"observed\"}");
      state = "RECORDING";
      return response(true);
    }
    if (request.operation == Operation::status) {
      if (state == "RECORDING") {
        const auto observed = run(L"-status", 15000);
        if (observed.timed_out || observed.exit_code != 0 ||
            observed.stdout_text.find("WPR recording is in progress") ==
                std::string::npos) {
          state = "FAILED";
          recover_failed_operation();
          return response(false, "recording_disappeared");
        }
      }
      return response(true);
    }
    if (!owner.owns(peer))
      return response(false, "capture_owner_mismatch");
    if (request.operation == Operation::cancel &&
        request.capture_id == capture_id && state == "COMPLETE") {
      // Explicit lease release by the original capture owner after its
      // verified raw import. This cannot select any privileged path.
      const auto trace = directory / L"trace.etl";
      no_reparse(trace);
      std::string digest;
      {
        Handle file(CreateFileW(trace.c_str(), GENERIC_READ, FILE_SHARE_READ,
                                nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL,
                                nullptr));
        digest = hash_file(file.value);
      }
      if (!DeleteFileW(trace.c_str()))
        throw std::runtime_error("owned completed trace release failed");
      write_new(directory / L"release_receipt.json",
                "{\"capture_id\":" + json_quote(capture_id) +
                    ",\"trace_sha256\":" + json_quote(digest) +
                    ",\"state\":\"RELEASED\"}");
      state = "RELEASED";
      return response(true);
    }
    if (state != "RECORDING" || request.capture_id != capture_id)
      return response(false, "capture_not_active");
    if (request.operation == Operation::cancel) {
      const auto result = run(L"-cancel", 30000, false);
      state =
          result.exit_code == 0 && !result.timed_out ? "CANCELLED" : "FAILED";
      if (state == "CANCELLED")
        may_have_session = false;
      write_new(directory / L"cancel_receipt.json",
                "{\"state\":" + json_quote(state) +
                    ",\"capture_id\":" + json_quote(capture_id) + "}");
      return response(state == "CANCELLED",
                      state == "FAILED" ? "wpr_cancel_failed" : "");
    }
    const auto trace = directory / L"trace.etl";
    const auto begin = qpc();
    const auto result = run(L"-stop \"" + trace.native() + L"\"", 240000);
    bool good = !result.timed_out && result.exit_code == 0 &&
                std::filesystem::is_regular_file(trace);
    if (good) {
      no_reparse(trace);
      const auto bytes = file_bytes(trace);
      good = bytes > 0 && bytes <= (2ULL << 30U);
    }
    state = good ? "COMPLETE" : "FAILED";
    if (good)
      may_have_session = false;
    else
      recover_failed_operation();
    write_new(directory / L"stop_receipt.json",
              "{\"state\":" + json_quote(state) +
                  ",\"capture_id\":" + json_quote(capture_id) +
                  ",\"stop_qpc_begin\":" + std::to_string(begin) +
                  ",\"stop_qpc_end\":" + std::to_string(qpc()) +
                  ",\"trace_loss\":null,\"qualification\":\"decoder and "
                  "fixture verification required\"}");
    return response(good, good ? "" : "wpr_stop_or_trace_bound_failed");
  }
  void expire_or_shutdown(bool shutdown = false) {
    bool capacity_exceeded = false;
    if (may_have_session) {
      ULARGE_INTEGER available{}, total{}, free{};
      capacity_exceeded =
          !GetDiskFreeSpaceExW(root.c_str(), &available, &total, &free) ||
          available.QuadPart < (2ULL << 30U);
      unsigned long long bytes = 0;
      for (const auto &entry :
           std::filesystem::recursive_directory_iterator(directory)) {
        no_reparse(entry.path());
        if (entry.is_regular_file())
          bytes += file_bytes(entry.path());
        if (bytes > (2ULL << 30U)) {
          capacity_exceeded = true;
          break;
        }
      }
    }
    if (may_have_session &&
        (shutdown || GetTickCount64() - started_ms >= 180000 ||
         capacity_exceeded ||
         (owner_process &&
          WaitForSingleObject(owner_process->value, 0) == WAIT_OBJECT_0))) {
      recover_failed_operation();
      state = capacity_exceeded ? "CAPACITY_EXCEEDED_INCOMPLETE"
                                : "EXPIRED_OR_SHUTDOWN";
    }
  }
  void recover_failed_operation() {
    if (!may_have_session)
      return;
    const auto cancelled = run(L"-cancel", 30000, false);
    if (cancelled.exit_code == 0 && !cancelled.timed_out)
      may_have_session = false;
    else {
      const auto observed = run(L"-status", 15000, false);
      if (observed.exit_code == 0 && !observed.timed_out &&
          observed.stdout_text.find("WPR is not recording") !=
              std::string::npos)
        may_have_session = false;
    }
  }
};

static bool io(HANDLE pipe, void *buffer, DWORD capacity, DWORD &count,
               bool write, DWORD timeout) {
  Handle event(CreateEventW(nullptr, TRUE, FALSE, nullptr));
  OVERLAPPED pending{};
  pending.hEvent = event.value;
  const BOOL complete =
      write ? WriteFile(pipe, buffer, capacity, &count, &pending)
            : ReadFile(pipe, buffer, capacity, &count, &pending);
  if (complete)
    return true;
  if (GetLastError() != ERROR_IO_PENDING)
    return false;
  HANDLE waits[] = {event.value, stop_event};
  if (WaitForMultipleObjects(2, waits, FALSE, timeout) != WAIT_OBJECT_0) {
    CancelIoEx(pipe, &pending);
    GetOverlappedResult(pipe, &pending, &count,
                        TRUE); // Drain before OVERLAPPED/event lifetime ends.
    return false;
  }
  return GetOverlappedResult(pipe, &pending, &count, FALSE) != FALSE;
}
// Service status and Win32 error are distinct fields of the fixed SCM callback
// API. NOLINTNEXTLINE(bugprone-easily-swappable-parameters)
static void report(DWORD state, DWORD error = NO_ERROR) {
  SERVICE_STATUS status{};
  status.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  status.dwCurrentState = state;
  status.dwControlsAccepted =
      state == SERVICE_RUNNING ? SERVICE_ACCEPT_STOP | SERVICE_ACCEPT_SHUTDOWN
                               : 0;
  status.dwWin32ExitCode = error;
  status.dwWaitHint =
      state == SERVICE_START_PENDING || state == SERVICE_STOP_PENDING ? 60000
                                                                      : 0;
  SetServiceStatus(status_handle, &status);
}
static DWORD WINAPI control(DWORD code, DWORD, void *, void *) {
  if (code == SERVICE_CONTROL_STOP || code == SERVICE_CONTROL_SHUTDOWN)
    SetEvent(stop_event);
  return NO_ERROR;
}
static void WINAPI service_main(DWORD, wchar_t **) {
  stop_event = CreateEventW(nullptr, TRUE, FALSE, nullptr);
  if (!stop_event)
    return;
  status_handle = RegisterServiceCtrlHandlerExW(service_name, control, nullptr);
  if (!status_handle)
    return;
  report(SERVICE_START_PENDING);
  try {
    Broker broker;
    // Specific client access excludes FILE_CREATE_PIPE_INSTANCE. Remote
    // connections are rejected even when they possess this account SID.
    Descriptor descriptor(L"O:SYG:SYD:P(A;;GA;;;SY)(A;;0x00100183;;;" +
                          client_sid + L")");
    SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),
                                   descriptor.value, FALSE};
    Handle pipe(CreateNamedPipeW(pipe_name,
                                 PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED |
                                     FILE_FLAG_FIRST_PIPE_INSTANCE,
                                 PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE |
                                     PIPE_WAIT | PIPE_REJECT_REMOTE_CLIENTS,
                                 1, 4096, 256, 1000, &attributes));
    report(SERVICE_RUNNING);
    while (WaitForSingleObject(stop_event, 0) != WAIT_OBJECT_0) {
      broker.expire_or_shutdown();
      Handle connected(CreateEventW(nullptr, TRUE, FALSE, nullptr));
      OVERLAPPED pending{};
      pending.hEvent = connected.value;
      bool ready = ConnectNamedPipe(pipe.value, &pending) != FALSE;
      if (!ready) {
        const DWORD error = GetLastError();
        if (error == ERROR_PIPE_CONNECTED)
          ready = true;
        else if (error == ERROR_IO_PENDING) {
          HANDLE waits[] = {connected.value, stop_event};
          if (WaitForMultipleObjects(2, waits, FALSE, 1000) == WAIT_OBJECT_0) {
            DWORD ignored = 0;
            ready = GetOverlappedResult(pipe.value, &pending, &ignored,
                                        FALSE) != FALSE;
          } else {
            CancelIoEx(pipe.value, &pending);
            DWORD ignored = 0;
            GetOverlappedResult(pipe.value, &pending, &ignored, TRUE);
          }
        } else
          throw std::runtime_error("pipe connection failed");
      }
      if (ready) {
        char request[96]{};
        DWORD count = 0;
        if (io(pipe.value, request, sizeof(request), count, false, 3000) &&
            authorize(pipe.value, client_sid)) {
          std::string result;
          try {
            ULONG pid = 0;
            if (!GetNamedPipeClientProcessId(pipe.value, &pid) || !pid)
              throw std::runtime_error("kernel client PID unavailable");
            Handle client(
                OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid));
            FILETIME created{}, ended{}, kernel{}, user{};
            if (!GetProcessTimes(client.value, &created, &ended, &kernel,
                                 &user))
              throw std::runtime_error("client lifetime unavailable");
            const Peer peer{
                pid,
                (static_cast<std::uint64_t>(created.dwHighDateTime) << 32U) |
                    created.dwLowDateTime};
            result = broker.execute(parse(std::string(request, count)), peer);
          } catch (const std::exception &failure) {
            const auto win32_error = GetLastError();
            try {
              broker.recover_failed_operation();
            } catch (...) {
              OutputDebugStringW(L"Epidemic ETW broker recovery failed; "
                                 L"original operation remains failed.\n");
            }
            std::string reason(failure.what());
            if (reason.size() > 160)
              reason.resize(160);
            for (auto &character : reason)
              if (static_cast<unsigned char>(character) < 32 ||
                  static_cast<unsigned char>(character) > 126)
                character = '?';
            result = "{\"protocolVersion\":1,\"ok\":false,\"broker_pid\":" +
                     std::to_string(GetCurrentProcessId()) +
                     ",\"win32_error\":" + std::to_string(win32_error) +
                     ",\"error\":" +
                     json_quote("broker_operation_failed: " + reason) + "}";
          }
          DWORD written = 0;
          if (io(pipe.value, result.data(), static_cast<DWORD>(result.size()),
                 written, true, 3000)) {
            // Bounded acknowledgement prevents DisconnectNamedPipe
            // discarding a response before the client consumes it.
            char acknowledgement[3]{};
            DWORD consumed = 0;
            io(pipe.value, acknowledgement, sizeof(acknowledgement), consumed,
               false, 1000);
          }
        }
      }
      DisconnectNamedPipe(pipe.value);
    }
    report(SERVICE_STOP_PENDING);
    broker.expire_or_shutdown(true);
    report(SERVICE_STOPPED);
  } catch (const std::exception &) {
    report(SERVICE_STOPPED, ERROR_SERVICE_SPECIFIC_ERROR);
  }
}
static int self_test() {
  const std::string id = "00112233-4455-6677-8899-aabbccddeeff";
  const std::vector<std::string> invalid = {"",
                                            "START CPU",
                                            "START GeneralProfile -filemode",
                                            "START GeneralProfile\nSTATUS",
                                            "EXEC calc.exe",
                                            "STOP ../../trace.etl",
                                            "STOP " + id + " x",
                                            "CANCEL " + id + "\n",
                                            "STATUS ",
                                            std::string(300, 'A')};
  for (const auto &value : invalid)
    if (parse(value).operation != Operation::invalid)
      return 1;
  const Peer owner{42, 100};
  if (!owner.owns(Peer{42, 100}) || owner.owns(Peer{43, 100}) ||
      owner.owns(Peer{42, 101}) || Peer{}.owns(Peer{}))
    return 1;
  if (parse("START GeneralProfile").operation != Operation::start ||
      parse("STATUS").operation != Operation::status ||
      parse("STOP " + id).operation != Operation::stop ||
      parse("CANCEL " + id).operation != Operation::cancel)
    return 1;
  HANDLE raw = nullptr;
  if (!OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY | TOKEN_DUPLICATE,
                        &raw))
    return 1;
  Handle primary(raw);
  if (elevated(primary.value))
    return 1; // Self-qualification is run non-elevated.
  HANDLE copied = nullptr;
  if (!DuplicateToken(primary.value, SecurityIdentification, &copied))
    return 1;
  Handle token(copied);
  const auto sid = user_sid(primary.value);
  Descriptor pipe(L"O:SYG:SYD:P(A;;GA;;;SY)(A;;0x00100183;;;" + sid + L")");
  // CreateFile adds FILE_READ_ATTRIBUTES for pipe clients even if callers
  // request only data access. A real Win32 open exercises that hidden right.
  Descriptor local_pipe(L"D:P(A;;GA;;;SY)(A;;0x00100183;;;" + sid + L")");
  SECURITY_ATTRIBUTES pipe_attributes{sizeof(SECURITY_ATTRIBUTES),
                                      local_pipe.value, FALSE};
  const auto test_name =
      std::wstring(L"\\\\.\\pipe\\EpidemicAuditorEtwBroker.selftest.") +
      std::to_wstring(GetCurrentProcessId());
  Handle test_pipe(CreateNamedPipeW(
      test_name.c_str(),
      PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED | FILE_FLAG_FIRST_PIPE_INSTANCE,
      PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_REJECT_REMOTE_CLIENTS, 1,
      4096, 256, 1000, &pipe_attributes));
  const auto raw_client =
      CreateFileW(test_name.c_str(), 0x00100183, 0, nullptr, OPEN_EXISTING,
                  FILE_FLAG_OVERLAPPED, nullptr);
  if (raw_client == INVALID_HANDLE_VALUE)
    throw std::runtime_error("actual fixed-right pipe client open failed");
  Handle test_client(raw_client);
  const auto base =
      std::filesystem::temp_directory_path() /
      (L"broker-path-selftest-" + std::to_wstring(GetCurrentProcessId()) +
       L"-" + std::to_wstring(GetTickCount64()));
  const auto folder = base / std::wstring(100, L'x');
  const auto long_file = folder / std::wstring(100, L'y');
  struct FixtureCleanup {
    std::wstring base, folder, file;
    bool base_created = false, folder_created = false, file_created = false;
    std::vector<std::wstring> retention_paths{};
    bool finish() noexcept {
      bool complete = true;
      if (file_created) {
        if (DeleteFileW(file.c_str()))
          file_created = false;
        else
          complete = false;
      }
      for (auto iterator = retention_paths.rbegin();
           iterator != retention_paths.rend(); ++iterator) {
        if (iterator->empty())
          continue;
        if (RemoveDirectoryW(iterator->c_str()))
          iterator->clear();
        else
          complete = false;
      }
      if (folder_created) {
        if (RemoveDirectoryW(folder.c_str()))
          folder_created = false;
        else
          complete = false;
      }
      if (base_created) {
        if (RemoveDirectoryW(base.c_str()))
          base_created = false;
        else
          complete = false;
      }
      return complete;
    }
    ~FixtureCleanup() noexcept {
      if (!finish())
        std::fputs("broker self-test fixture cleanup failed\n", stderr);
    }
  } fixture_cleanup{api_path(base), api_path(folder), api_path(long_file)};
  fixture_cleanup.retention_paths.reserve(257);
  if (!CreateDirectoryW(api_path(base).c_str(), nullptr))
    throw std::runtime_error("long path fixture root setup failed");
  fixture_cleanup.base_created = true;
  if (!CreateDirectoryW(api_path(folder).c_str(), nullptr))
    throw std::runtime_error("long path fixture folder setup failed");
  fixture_cleanup.folder_created = true;
  {
    Handle fixture(CreateFileW(api_path(long_file).c_str(), GENERIC_WRITE, 0,
                               nullptr, CREATE_NEW, FILE_ATTRIBUTE_NORMAL,
                               nullptr));
    fixture_cleanup.file_created = true;
  }
  const auto create_owned = [&](const std::filesystem::path &path) {
    if (fixture_cleanup.retention_paths.size() ==
        fixture_cleanup.retention_paths.capacity())
      throw std::runtime_error(
          "retention fixture capacity exhausted before creation");
    auto native = api_path(path);
    if (!CreateDirectoryW(native.c_str(), nullptr))
      throw std::runtime_error("retention fixture setup failed");
    fixture_cleanup.retention_paths.push_back(std::move(native));
  };
  for (unsigned index = 0; index < 129; ++index)
    create_owned(folder / (L"temporary-" + std::to_wstring(index)));
  verify_retention(base); // One capture with >128 internal folders is admitted.
  for (unsigned index = 0; index < 128; ++index)
    create_owned(base / (L"capture-" + std::to_wstring(index)));
  bool rejected_retention = false;
  try {
    verify_retention(base);
  } catch (const std::runtime_error &error) {
    rejected_retention =
        std::string(error.what()) ==
        "broker retention capacity exhausted; evidence retained";
  }
  if (!rejected_retention)
    throw std::runtime_error("capture retention limit was not enforced");
  Descriptor binary(L"O:SYG:SYD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;0x001200a9;;;" +
                    sid + L")");
  Descriptor foreign(L"O:SYG:SYD:P(A;;GA;;;SY)(A;;0x00100183;;;S-1-0-0)");
  const auto access = [&](PSECURITY_DESCRIPTOR descriptor, DWORD desired) {
    GENERIC_MAPPING mapping{FILE_GENERIC_READ, FILE_GENERIC_WRITE,
                            FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};
    std::array<unsigned char, 4096> privileges{};
    DWORD length = static_cast<DWORD>(privileges.size()), granted = 0;
    BOOL allowed = FALSE;
    if (!AccessCheck(descriptor, token.value, desired, &mapping,
                     reinterpret_cast<PRIVILEGE_SET *>(privileges.data()),
                     &length, &granted, &allowed))
      throw std::runtime_error("ACL self-test API failed");
    return allowed != FALSE;
  };
  const bool results[] = {access(pipe.value, 0x00100183),
                          !access(pipe.value, FILE_CREATE_PIPE_INSTANCE),
                          !access(foreign.value, 0x00100183),
                          access(binary.value, FILE_GENERIC_READ),
                          !access(binary.value, FILE_GENERIC_WRITE),
                          !access(binary.value, WRITE_DAC),
                          !access(binary.value, WRITE_OWNER),
                          !access(binary.value, DELETE)};
  for (unsigned int index = 0; index < sizeof(results) / sizeof(results[0]);
       ++index) {
    if (!results[index]) {
      std::cerr << "ACL assertion failed: " << index << '\n';
      return 1;
    }
  }
  {
    Handle locked_fixture(CreateFileW(fixture_cleanup.file.c_str(),
                                      GENERIC_READ, 0, nullptr, OPEN_EXISTING,
                                      FILE_ATTRIBUTE_NORMAL, nullptr));
    if (fixture_cleanup.finish())
      throw std::runtime_error("broker self-test accepted blocked cleanup");
  }
  if (!fixture_cleanup.finish())
    throw std::runtime_error("broker self-test cleanup unproven; no PASS");
  std::cout
      << "{\"status\":\"PASS\",\"positive\":4,\"negative\":" << invalid.size()
      << ",\"acl_checks\":8,\"cleanup_controls\":2,\"client_elevated\":false,"
         "\"scope\":\"protocol and actual "
         "token ACL checks; service/real ETL qualification still required\"}\n";
  return 0;
}
} // namespace etw_broker
int wmain(int argc, wchar_t **argv) {
  try {
    if (argc == 2 && std::wstring(argv[1]) == L"--self-test")
      return etw_broker::self_test();
    if ((argc == 4 || argc == 5) && std::wstring(argv[1]) == L"--install")
      return etw_broker::install(argv[2], argv[3],
                                 argc == 5 ? argv[4] : nullptr);
    if (argc != 3 || std::wstring(argv[1]) != L"--service")
      throw std::runtime_error(
          "fixed --service SID or one-time --install SID only");
    etw_broker::client_sid = etw_broker::canonical_sid(argv[2]);
    SERVICE_TABLE_ENTRYW table[] = {
        {const_cast<wchar_t *>(etw_broker::service_name),
         etw_broker::service_main},
        {nullptr, nullptr}};
    if (!StartServiceCtrlDispatcherW(table))
      throw std::runtime_error(
          "broker must be launched by the service manager");
    return 0;
  } catch (const std::exception &error) {
    const auto code = GetLastError();
    std::cerr << error.what() << " (Win32 " << code << ")\n";
    if ((argc == 4 || argc == 5) && std::wstring(argv[1]) == L"--install") {
      try {
        const auto sid = etw_broker::canonical_sid(argv[2]);
        auto pinned = etw_broker::pin_path(etw_broker::store_path);
        etw_broker::verify_store(pinned.back()->value, sid);
        const auto path = std::filesystem::path(etw_broker::store_path) /
                          (L"installer_failure_" +
                           std::to_wstring(GetCurrentProcessId()) + L".json");
        etw_broker::Descriptor descriptor(
            L"O:SYG:SYD:P(A;;FA;;;SY)(A;;FA;;;BA)(A;;0x001200a9;;;" + sid +
            L")");
        SECURITY_ATTRIBUTES attributes{sizeof(SECURITY_ATTRIBUTES),
                                       descriptor.value, FALSE};
        etw_broker::Handle file(CreateFileW(
            path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, &attributes,
            CREATE_NEW, FILE_ATTRIBUTE_NORMAL, nullptr));
        const auto text = "{\"error\":" + etw_broker::json_quote(error.what()) +
                          ",\"win32_error\":" + std::to_string(code) + "}";
        DWORD count = 0;
        WriteFile(file.value, text.data(), static_cast<DWORD>(text.size()),
                  &count, nullptr);
        FlushFileBuffers(file.value);
      } catch (...) {
        // Debug output avoids writing through an untrusted installation store.
        OutputDebugStringW(L"Epidemic ETW broker installation failure "
                           L"diagnostics unavailable.\n");
      }
    }
    return 2;
  }
}

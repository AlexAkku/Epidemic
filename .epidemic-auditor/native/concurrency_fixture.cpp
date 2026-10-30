#define WIN32_LEAN_AND_MEAN
#include <atomic>
#include <chrono>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <vector>
#include <windows.h>

using Clock = std::chrono::steady_clock;
static void busy(unsigned milliseconds) {
  const auto end = Clock::now() + std::chrono::milliseconds(milliseconds);
  volatile unsigned long long value = 17;
  while (Clock::now() < end)
    value = value * 1664525ULL + 1013904223ULL;
}
struct Handle {
  HANDLE value;
  explicit Handle(HANDLE handle) : value(handle) {
    if (!value)
      throw std::runtime_error("fixture handle creation failed");
  }
  ~Handle() { CloseHandle(value); }
  Handle(const Handle &) = delete;
  Handle &operator=(const Handle &) = delete;
};
static void emit(const std::string &mode, const std::string &variant,
                 DWORD worker, DWORD owner = 0) {
  std::cout << "{\"eventType\":\"fixture.control\",\"provenance\":\"fixture_"
               "control\",\"confidence\":\"declared\","
            << "\"pid\":" << GetCurrentProcessId() << ",\"mode\":\"" << mode
            << "\",\"variant\":\"" << variant << "\",\"worker_tid\":" << worker
            << ",\"owner_tid\":" << owner << "}" << std::endl;
}
int wmain(int argc, wchar_t **argv) {
  try {
    if (argc != 4)
      throw std::runtime_error("usage: MODE good|bad NAMED_OBJECT");
    const std::wstring mode(argv[1]), variant(argv[2]);
    if (variant != L"good" && variant != L"bad")
      throw std::runtime_error("invalid fixture variant");
    const bool bad = variant == L"bad";
    const auto label = bad ? "bad" : "good";
    if (mode == L"starvation") {
      DWORD_PTR allowed = 0, system = 0;
      if (!GetProcessAffinityMask(GetCurrentProcess(), &allowed, &system))
        throw std::runtime_error("affinity unavailable");
      const DWORD_PTR first = allowed & (~allowed + 1);
      const DWORD_PTR rest = allowed & ~first;
      const DWORD_PTR second = rest & (~rest + 1);
      if (!first || !second)
        throw std::runtime_error(
            "two available CPUs required for controlled fixture");
      Handle gate(CreateEventW(nullptr, TRUE, FALSE, nullptr));
      std::jthread hog([&] {
        if (WaitForSingleObject(gate.value, 5000) == WAIT_OBJECT_0)
          busy(2500);
      });
      std::jthread worker([&] {
        if (WaitForSingleObject(gate.value, 5000) == WAIT_OBJECT_0)
          busy(2500);
      });
      const HANDLE hog_handle = hog.native_handle(),
                   worker_handle = worker.native_handle();
      const bool configured =
          SetThreadAffinityMask(hog_handle, first) &&
          SetThreadAffinityMask(worker_handle, bad ? first : second) &&
          SetThreadPriority(hog_handle, bad ? THREAD_PRIORITY_HIGHEST
                                            : THREAD_PRIORITY_NORMAL) &&
          SetThreadPriority(worker_handle, bad ? THREAD_PRIORITY_IDLE
                                               : THREAD_PRIORITY_NORMAL) &&
          SetThreadPriorityBoost(worker_handle, TRUE);
      emit("starvation", label, GetThreadId(worker_handle),
           GetThreadId(hog_handle));
      SetEvent(gate.value);
      hog.join();
      worker.join();
      if (!configured)
        throw std::runtime_error("controlled scheduler configuration failed");
    } else if (mode == L"mutex" || mode == L"contention") {
      Handle mutex(CreateMutexW(nullptr, TRUE, argv[3]));
      const unsigned count = mode == L"contention" && bad ? 3 : 1;
      std::atomic<bool> worker_failed = false;
      std::vector<std::jthread> workers;
      for (unsigned index = 0; index < count; ++index) {
        workers.emplace_back([&] {
          if (WaitForSingleObject(mutex.value, 30000) == WAIT_OBJECT_0) {
            busy(20);
            if (!ReleaseMutex(mutex.value))
              worker_failed = true;
          } else
            worker_failed = true;
        });
        emit(mode == L"mutex" ? "mutex" : "contention", label,
             GetThreadId(workers.back().native_handle()), GetCurrentThreadId());
      }
      if (bad) {
        const std::wstring release_name = std::wstring(argv[3]) + L"_release";
        Handle release(OpenEventW(SYNCHRONIZE, FALSE, release_name.c_str()));
        const auto result = WaitForSingleObject(release.value, 20000);
        if (!ReleaseMutex(mutex.value))
          throw std::runtime_error("fixture owner release failed");
        for (auto &worker : workers)
          worker.join();
        if (result != WAIT_OBJECT_0 || worker_failed)
          throw std::runtime_error("bounded external release or waiter failed");
        return 0;
      }
      std::this_thread::sleep_for(std::chrono::milliseconds(20));
      if (!ReleaseMutex(mutex.value))
        throw std::runtime_error("fixture owner release failed");
      for (auto &worker : workers)
        worker.join();
      if (worker_failed)
        throw std::runtime_error("fixture waiter acquisition/release failed");
    } else if (mode == L"cancel" || mode == L"completion") {
      Handle event(OpenEventW(SYNCHRONIZE, FALSE, argv[3]));
      emit(mode == L"cancel" ? "cancel" : "completion", label,
           GetCurrentThreadId());
      if (WaitForSingleObject(event.value, 10000) != WAIT_OBJECT_0)
        throw std::runtime_error("external control request absent");
      busy(bad ? 1500 : 10);
    } else if (mode == L"orphan") {
      wchar_t image[32768]{};
      if (!GetModuleFileNameW(nullptr, image, 32768))
        throw std::runtime_error("fixture image unavailable");
      std::wstring command =
          L"\"" + std::wstring(image) + L"\" orphan_child good unused";
      STARTUPINFOW start{};
      start.cb = sizeof(start);
      PROCESS_INFORMATION process{};
      if (!CreateProcessW(nullptr, command.data(), nullptr, nullptr, FALSE,
                          CREATE_NO_WINDOW, nullptr, nullptr, &start, &process))
        throw std::runtime_error("orphan fixture launch failed");
      Handle child(process.hProcess), child_thread(process.hThread);
      emit("orphan", label, process.dwThreadId, GetCurrentThreadId());
      std::cout << "{\"child_pid\":" << process.dwProcessId
                << ",\"fixture_control\":true}" << std::endl;
      if (!bad && WaitForSingleObject(child.value, 10000) != WAIT_OBJECT_0)
        throw std::runtime_error("child fixture timeout");
    } else if (mode == L"orphan_child") {
      busy(1200);
    } else
      throw std::runtime_error("unknown fixture mode");
    return 0;
  } catch (const std::exception &error) {
    std::cerr << error.what() << '\n';
    return 2;
  }
}

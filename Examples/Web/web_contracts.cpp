#include <Epidemic/Core/task_scheduler.h>
#include <Epidemic/Platform/iplatform_runtime.h>
#include <array>
#include <cstdio>
#include <stdexcept>

void Require(bool condition, const char *message) {
  if (!condition)
    throw std::runtime_error(message);
}
int main() {
  try {
    using namespace epidemic;
    core::tasks::SimpleTaskScheduler scheduler(1);
    Require(scheduler.WorkerCount() == 0,
            "single-thread Web must not create workers");
    core::tasks::TaskGroup group;
    int completed = 0;
    auto handle = scheduler.Schedule([&] { ++completed; }, group);
    scheduler.Wait(handle);
    scheduler.Wait(group);
    scheduler.WaitIdle();
    Require(completed == 1 && !scheduler.Cancel(handle),
            "inline task completion/cancellation");
    auto failing =
        scheduler.Schedule([] { throw std::runtime_error("expected"); });
    bool caught = false;
    try {
      scheduler.Wait(failing);
    } catch (const std::runtime_error &) {
      caught = true;
    }
    Require(caught, "task exceptions must survive the WASM boundary");
    try {
      scheduler.WaitIdle();
    } catch (const std::runtime_error &) {
    }
    auto path = foundation::Path::FromString("/package.bin");
    std::array<std::byte, 3> bytes{std::byte{1}, std::byte{2}, std::byte{3}};
    Require(platform::CommitFilePackage(path, bytes, 3).HasValue(),
            "virtual package commit");
    Require(!platform::ReadFilePackage(path, 2).HasValue(),
            "bounded read must reject oversized package");
    Require(!platform::CommitFilePackage(path, bytes, 2).HasValue(),
            "bounded write must reject oversized package");
    auto restored = platform::ReadFilePackage(path, 3);
    Require(restored.HasValue() &&
                restored.Value() ==
                    std::vector<std::byte>(bytes.begin(), bytes.end()),
            "failed commit must preserve old package");
    scheduler.Shutdown();
    bool stopped = false;
    try {
      static_cast<void>(scheduler.Schedule([] {}));
    } catch (const std::runtime_error &) {
      stopped = true;
    }
    Require(stopped, "stopped scheduler must reject new work");
    std::puts("PASS: Web scheduler completion/errors/shutdown and bounded "
              "virtual package IO");
    return 0;
  } catch (const std::exception &e) {
    std::fprintf(stderr, "FAIL: %s\n", e.what());
    return 1;
  }
}

#include "Epidemic/Foundation/error.h"
#include "Epidemic/GameFramework/Integration/core_adapters.h"
#include "Epidemic/Platform/iplatform_runtime.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

using namespace epidemic::gameplay;
using namespace epidemic::gameplay::savegame;
namespace epidemic::gameplay::integration::detail {
void SetSaveStoreFaultPointForTesting(std::string_view);
void ClearSaveStoreFaultPointForTesting() noexcept;
} // namespace epidemic::gameplay::integration::detail
namespace {
void CheckStore(bool value, const char *message) {
  if (!value) {
    std::cerr << message << '\n';
    std::exit(1);
  }
}
class MemoryStore final : public ISavePackageStore {
public:
  std::vector<std::byte> package;
  std::string failure;
  bool truncate = false, corrupt = false, throw_callback = false;
  epidemic::foundation::Result<std::vector<std::byte>> Load() override {
    if (throw_callback)
      throw std::runtime_error("controlled open exception");
    if (failure == "open" || package.empty())
      return epidemic::foundation::Result<std::vector<std::byte>>::Failure(
          epidemic::foundation::Error::Create("test.open",
                                              "controlled open failure"));
    auto result = package;
    if (truncate)
      result.resize(result.size() / 2);
    if (corrupt)
      result[0] ^= std::byte{1};
    return epidemic::foundation::Result<std::vector<std::byte>>::Success(
        std::move(result));
  }
  epidemic::foundation::Result<void>
  Commit(std::span<const std::byte> bytes) override {
    if (throw_callback)
      throw std::runtime_error("controlled commit exception");
    if (failure == "open")
      return epidemic::foundation::Result<void>::Failure(
          epidemic::foundation::Error::Create("test.open",
                                              "controlled temp open failure"));
    std::vector<std::byte> temporary(bytes.begin(), bytes.end());
    if (failure == "write" || failure == "replace")
      return epidemic::foundation::Result<void>::Failure(
          epidemic::foundation::Error::Create(
              "test.commit", "controlled write/replace failure"));
    package.swap(temporary);
    return epidemic::foundation::Result<void>::Success();
  }
};
struct Lease final : ISaveBarrierLease {};
struct Barrier final : ISaveBarrier {
  epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>>
  AcquireCaptureLease() override {
    return epidemic::foundation::Result<
        std::unique_ptr<ISaveBarrierLease>>::Success(std::make_unique<Lease>());
  }
  epidemic::foundation::Result<std::unique_ptr<ISaveBarrierLease>>
  AcquireRestoreLease() override {
    return epidemic::foundation::Result<
        std::unique_ptr<ISaveBarrierLease>>::Success(std::make_unique<Lease>());
  }
};
SaveGameImage KnownTimeImage(std::int64_t ticks) {
  time::GameplayTimeService owner;
  const auto clock = owner.RegisterClock("storage.clock");
  CheckStore(bool(clock), "register known time clock");
  owner.Freeze();
  CheckStore(bool(owner.AdvanceTo(clock.Value(), {ticks})),
             "seed known time state");
  SaveRestoreWorkspace workspace;
  auto participant = CreateGameplayTimeServiceSaveParticipant(owner, workspace);
  Barrier barrier;
  SaveGameOrchestrator orchestrator;
  CheckStore(bool(orchestrator.SetBarrier(barrier)) &&
                 bool(orchestrator.RegisterParticipant(*participant)),
             "minimal production participant setup");
  auto image = orchestrator.Capture({});
  CheckStore(bool(image), "capture real time image");
  return std::move(image.Value());
}
void RestoreKnownTime(const SaveGameImage &image, std::int64_t ticks) {
  time::GameplayTimeService fresh;
  const auto clock = fresh.RegisterClock("storage.clock");
  fresh.Freeze();
  SaveRestoreWorkspace workspace;
  auto participant = CreateGameplayTimeServiceSaveParticipant(fresh, workspace);
  Barrier barrier;
  SaveGameOrchestrator orchestrator;
  CheckStore(bool(clock) && bool(orchestrator.SetBarrier(barrier)) &&
                 bool(orchestrator.RegisterParticipant(*participant)),
             "fresh time composition setup");
  CheckStore(bool(orchestrator.Restore(image, {})),
             "fresh composition restores decoded real Time state");
  CheckStore(fresh.GetClock(clock.Value())->now.ticks == ticks,
             "fresh process restored known gameplay time");
}
void RawWrite(const std::filesystem::path &path,
              const std::vector<std::byte> &bytes) {
  std::ofstream file(path, std::ios::binary | std::ios::trunc);
  file.write(reinterpret_cast<const char *>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  CheckStore(bool(file), "write deliberately corrupted test save");
}
} // namespace
void TestDurableStorage(const std::filesystem::path &directory) {
  const auto old = KnownTimeImage(42);
  const auto next = KnownTimeImage(73);
  MemoryStore memory;
  CheckStore(!LoadSaveGameImage(memory), "missing in-memory save rejected");
  CheckStore(bool(CommitSaveGameImage(memory, old)),
             "memory first generation commit");
  const auto baseline = memory.package;
  for (auto failure : {"open", "write", "replace"}) {
    memory.failure = failure;
    CheckStore(!CommitSaveGameImage(memory, next) && memory.package == baseline,
               "controlled memory failure preserves old generation");
  }
  memory.failure.clear();
  memory.truncate = true;
  CheckStore(!LoadSaveGameImage(memory), "stored truncation rejected");
  memory.truncate = false;
  memory.corrupt = true;
  CheckStore(!LoadSaveGameImage(memory), "stored corruption rejected");
  memory.corrupt = false;
  memory.throw_callback = true;
  CheckStore(!CommitSaveGameImage(memory, next) && !LoadSaveGameImage(memory) &&
                 memory.package == baseline,
             "storage exceptions converted without publication");
  memory.throw_callback = false;
  CheckStore(bool(CommitSaveGameImage(memory, next)),
             "memory sequential generation commit");
  RestoreKnownTime(LoadSaveGameImage(memory).Value(), 73);
  auto invalid = next;
  invalid.sections.front().payload_hash ^= 1;
  const auto validBytes = memory.package;
  CheckStore(!CommitSaveGameImage(memory, invalid) &&
                 memory.package == validBytes,
             "invalid image never reaches store commit");

  std::filesystem::create_directories(directory);
  const auto path = directory / "game.save";
  integration::FileSavePackageStore file(path);
  CheckStore(!LoadSaveGameImage(file), "missing filesystem save rejected");
  CheckStore(bool(CommitSaveGameImage(file, old)),
             "filesystem encode and commit");
  {
    integration::FileSavePackageStore reopened(path);
    RestoreKnownTime(LoadSaveGameImage(reopened).Value(), 42);
  }
  const auto fileBaseline = file.Load().Value();
  const epidemic::foundation::Path nativePath(path);
  CheckStore(
      epidemic::platform::ReadFilePackage(nativePath, 1024 * 1024).Value() ==
          fileBaseline,
      "Base bounded byte read returns exact package");
  CheckStore(!epidemic::platform::ReadFilePackage(nativePath, 0),
             "Base read rejects zero resource cap");
  CheckStore(
      !epidemic::platform::CommitFilePackage(nativePath, fileBaseline, 1) &&
          file.Load().Value() == fileBaseline,
      "Base oversized commit preserves old file");
  CheckStore(bool(epidemic::platform::CommitFilePackage(
                 nativePath, fileBaseline, 1024 * 1024)) &&
                 file.Load().Value() == fileBaseline,
             "Base identical byte package commit preserves exact content");
  bool nulRejected = false;
  try {
    integration::FileSavePackageStore invalidPath(
        std::filesystem::path(std::wstring(L"save\0suffix", 11)));
  } catch (const std::invalid_argument &) {
    nulRejected = true;
  }
  CheckStore(nulRejected,
             "embedded NUL path rejected before filesystem access");
  for (auto failure : {"open", "write", "flush", "replace"}) {
    integration::detail::SetSaveStoreFaultPointForTesting(failure);
    const auto result = CommitSaveGameImage(file, next);
    integration::detail::ClearSaveStoreFaultPointForTesting();
    CheckStore(!result && result.GetError().HasContext() &&
                   file.Load().Value() == fileBaseline,
               "filesystem failure preserves old save and informative error");
    std::size_t temporaries = 0;
    for (const auto &entry : std::filesystem::directory_iterator(directory))
      if (entry.path() != path)
        ++temporaries;
    CheckStore(temporaries == 0,
               "handled failure cleans only its own temporary file");
  }
  CheckStore(bool(CommitSaveGameImage(file, next)),
             "filesystem next generation replaces old");
  RestoreKnownTime(LoadSaveGameImage(file).Value(), 73);
  auto corrupt = file.Load().Value();
  corrupt[0] ^= std::byte{1};
  RawWrite(path, corrupt);
  CheckStore(!LoadSaveGameImage(file),
             "corrupt filesystem save rejected before restore");
  corrupt.resize(corrupt.size() / 2);
  RawWrite(path, corrupt);
  CheckStore(!LoadSaveGameImage(file), "truncated filesystem save rejected");
  CheckStore(bool(CommitSaveGameImage(file, next)),
             "valid save replaces corrupt generation");
  SaveWireLimits limits;
  limits.max_wire_bytes = 80;
  integration::FileSavePackageStore bounded(path, limits);
  CheckStore(!bounded.Load() && !bounded.Commit(file.Load().Value()),
             "oversized file rejected before allocation or write");
  CheckStore(!LoadSaveGameImage(bounded),
             "oversized package cannot restore live state");
  std::unique_ptr<ISavePackageStore> polymorphic =
      std::make_unique<MemoryStore>();
  CheckStore(bool(CommitSaveGameImage(*polymorphic, old)),
             "polymorphic store accepts package");
  polymorphic.reset();
  CheckStore(file.Load().Value() == EncodeSaveGameImage(next).Value(),
             "virtual store destruction never removes committed save");
}
int main(int argc, char **argv) {
  if (argc == 4) {
    const std::string_view mode = argv[1];
    const std::filesystem::path path = argv[2];
    const auto ticks = std::stoll(argv[3]);
    integration::FileSavePackageStore store(path);
    if (mode == "write" || mode == "interrupt") {
      if (mode == "interrupt")
        integration::detail::SetSaveStoreFaultPointForTesting("interrupt");
      CheckStore(bool(CommitSaveGameImage(store, KnownTimeImage(ticks))),
                 "child writes production durable save");
    } else if (mode == "read") {
      auto image = LoadSaveGameImage(store);
      CheckStore(bool(image), "fresh process reopens save");
      RestoreKnownTime(image.Value(), ticks);
    } else
      return 2;
    return 0;
  }
  if (argc != 2)
    return 2;
  TestDurableStorage(argv[1]);
  return 0;
}

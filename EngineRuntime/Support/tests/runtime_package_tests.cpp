#include "Epidemic/Foundation/path.h"
#include "Epidemic/Runtime/Support/runtime_support.h"
#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
using namespace epidemic::runtime;
namespace epidemic::platform::detail {
void SetAtomicFileFaultPointForTesting(std::string_view);
void ClearAtomicFileFaultPointForTesting() noexcept;
} // namespace epidemic::platform::detail
namespace {
void Require(bool ok, const char *message) {
  if (!ok)
    throw std::runtime_error(message);
}
PersistenceSnapshot KnownSnapshot() {
  PersistenceSnapshot value;
  value.current_revision = 7;
  PersistentObjectRecord object;
  object.persistent_id = PersistentObjectId{11};
  object.asset_id = AssetId::FromString("runtime.package.asset");
  object.kind = PersistentObjectKind::Object;
  object.tier = PersistenceTier::Protected;
  object.location.chunk_id = ChunkId{8};
  object.revision = 7;
  object.payload.schema_id =
      epidemic::foundation::StringId::FromString("application.v1");
  object.payload.schema_version = 1;
  object.payload.bytes = {std::byte{42}, std::byte{73}};
  value.objects.push_back(object);
  TombstoneRecord tombstone;
  tombstone.persistent_id = PersistentObjectId{12};
  tombstone.revision = 6;
  tombstone.reason = epidemic::foundation::StringId::FromString("deleted");
  value.tombstones.push_back(tombstone);
  LazyRuleRecord rule;
  rule.rule_id = LazyRuleId{13};
  rule.target_id = object.persistent_id;
  rule.rule_seed = 99;
  rule.revision = 7;
  value.lazy_rules.push_back(rule);
  ZoneOverrideSnapshot zone;
  zone.location = object.location;
  zone.record_ids = {object.persistent_id};
  zone.tombstones = {tombstone};
  zone.lazy_rules = {rule};
  zone.revision = 7;
  value.zone_overrides.push_back(zone);
  return value;
}
TimeOptions ClockOptions() {
  TimeOptions o;
  o.game_ticks_per_real_second = 3;
  o.initial_time_scale = {1, 3};
  return o;
}
std::vector<std::byte> Bytes(const std::filesystem::path &path) {
  std::ifstream file(path, std::ios::binary);
  Require(bool(file), "test package read");
  std::vector<char> raw{std::istreambuf_iterator<char>{file},
                        std::istreambuf_iterator<char>{}};
  std::vector<std::byte> bytes;
  for (char c : raw)
    bytes.push_back(std::byte(static_cast<unsigned char>(c)));
  return bytes;
}
void Put(const std::filesystem::path &path,
         const std::vector<std::byte> &bytes) {
  std::ofstream file(
      path, std::ios::binary); // ofstream's out mode already truncates.
  Require(bool(file), "test package write");
  file.write(reinterpret_cast<const char *>(bytes.data()),
             static_cast<std::streamsize>(bytes.size()));
  Require(bool(file), "test complete write");
}
void RepairHash(std::vector<std::byte> &bytes) {
  std::uint64_t hash = 14695981039346656037ull;
  for (std::size_t i = 0; i < bytes.size() - 8; ++i) {
    hash ^= std::to_integer<unsigned char>(bytes[i]);
    hash *= 1099511628211ull;
  }
  for (std::size_t i = 0; i < 8; ++i)
    bytes[bytes.size() - 8 + i] = std::byte((hash >> (8U * i)) & 255U);
}
void WriteKnown(const std::filesystem::path &directory) {
  auto clock = CreateTimeServices(ClockOptions());
  Require(bool(clock), "Runtime clock bootstrap");
  Require(
      bool(clock.Value().runtime->Advance(std::chrono::microseconds{333333})),
      "Runtime fractional advance");
  Require(bool(CommitRuntimeTimeCheckpoint(
              epidemic::foundation::Path(directory / "time.bin"),
              clock.Value().runtime->CaptureCheckpoint())),
          "Runtime Time complete durable commit");
  auto backend = CreateFilePersistenceBackend(
      epidemic::foundation::Path(directory / "persistence.bin"));
  Require(bool(backend), "Runtime file backend factory");
  Require(bool(ValidatePersistenceSnapshot(KnownSnapshot())),
          "Runtime valid snapshot accepted");
  Require(bool(backend.Value()->CommitSnapshot(
              KnownSnapshot(), PersistenceDurability::SaveAndFlushRequired)),
          "Runtime snapshot durable commit");
}
void ReadKnown(const std::filesystem::path &directory) {
  auto checkpoint = LoadRuntimeTimeCheckpoint(
      epidemic::foundation::Path(directory / "time.bin"));
  Require(bool(checkpoint) && checkpoint.Value().tick_remainder_numerator != 0,
          "Runtime Time persisted fractional remainder");
  auto clock = CreateTimeServices(ClockOptions());
  Require(bool(clock), "fresh Runtime clock bootstrap");
  Require(bool(clock.Value().runtime->RestoreCheckpoint(checkpoint.Value())),
          "Runtime Time fresh checkpoint restore");
  const auto restored = clock.Value().runtime->CaptureCheckpoint();
  Require(restored.tick_remainder_numerator ==
                  checkpoint.Value().tick_remainder_numerator &&
              restored.revision == checkpoint.Value().revision &&
              restored.now == checkpoint.Value().now &&
              restored.time_scale == checkpoint.Value().time_scale,
          "Runtime fresh clock publishes complete checkpoint");
  auto control = CreateTimeServices(ClockOptions());
  Require(bool(control) && bool(control.Value().runtime->Advance(
                               std::chrono::microseconds{333333})),
          "Runtime continuation control bootstrap");
  Require(bool(control.Value().runtime->Advance(
              std::chrono::microseconds{666668})) &&
              bool(clock.Value().runtime->Advance(
                  std::chrono::microseconds{666668})) &&
              control.Value().clock->GetSnapshot() ==
                  clock.Value().clock->GetSnapshot(),
          "Runtime fractional continuation equals unsaved control clock");
  auto backend = CreateFilePersistenceBackend(
      epidemic::foundation::Path(directory / "persistence.bin"));
  Require(bool(backend), "fresh Runtime file backend");
  auto snapshot = backend.Value()->Load();
  Require(bool(snapshot) && snapshot.Value().current_revision == 7 &&
              snapshot.Value().objects.size() == 1 &&
              snapshot.Value().tombstones.size() == 1 &&
              snapshot.Value().lazy_rules.size() == 1 &&
              snapshot.Value().zone_overrides.size() == 1 &&
              snapshot.Value().objects[0].payload.bytes ==
                  KnownSnapshot().objects[0].payload.bytes,
          "Runtime complete snapshot reopen");
  PersistenceOptions options;
  options.backend = backend.Value();
  options.durability = PersistenceDurability::SaveAndFlushRequired;
  auto services = CreatePersistenceServices(options);
  Require(bool(services) && services.Value()
                                .query->FindObject(PersistentObjectId{11})
                                .has_value(),
          "Runtime fresh store restore from durable backend");
}
} // namespace
int RuntimePackageProcessMode(int argc, char **argv) {
  Require(argc == 3, "Runtime package process mode arguments");
  const std::filesystem::path directory = argv[2];
  if (std::string_view(argv[1]) == "runtime-write")
    WriteKnown(directory);
  else if (std::string_view(argv[1]) == "runtime-read")
    ReadKnown(directory);
  else if (std::string_view(argv[1]) == "runtime-record-fixtures") {
    Require(!std::filesystem::exists(directory / "time.bin") &&
                !std::filesystem::exists(directory / "persistence.bin"),
            "immutable Runtime fixtures must not be overwritten");
    WriteKnown(directory);
  } else
    throw std::runtime_error("unknown Runtime package mode");
  return 0;
}
void TestRuntimeDurablePackages() {
  auto directory =
      std::filesystem::temp_directory_path() /
      ("epidemic-runtime-package-" +
       std::to_string(
           std::chrono::steady_clock::now().time_since_epoch().count()));
  std::filesystem::create_directory(directory);
  try {
    WriteKnown(directory);
    ReadKnown(directory);
    Require(!CreateFilePersistenceBackend(epidemic::foundation::Path{}),
            "Runtime empty backend path rejected");
    const auto fixture_root = std::filesystem::path(EPIDEMIC_RUNTIME_FIXTURES);
    Require(Bytes(directory / "time.bin") == Bytes(fixture_root / "time.bin") &&
                Bytes(directory / "persistence.bin") ==
                    Bytes(fixture_root / "persistence.bin"),
            "Runtime immutable v1 goldens byte-stable");
    ReadKnown(fixture_root);
    auto backend = CreateFilePersistenceBackend(
        epidemic::foundation::Path(directory / "persistence.bin"));
    Require(bool(backend), "backend bootstrap for negative tests");
    auto bad = KnownSnapshot();
    bad.objects.push_back(bad.objects[0]);
    Require(!ValidatePersistenceSnapshot(bad),
            "Runtime duplicate snapshot rejected");
    Require(!backend.Value()->CommitSnapshot(
                bad, PersistenceDurability::SaveAndFlushRequired),
            "Runtime invalid commit preserves old generation");
    ReadKnown(directory);
    Require(!backend.Value()->CommitSnapshot(
                KnownSnapshot(), static_cast<PersistenceDurability>(99)),
            "Runtime invalid durability rejected");
    auto huge = KnownSnapshot();
    huge.objects[0].payload.bytes.resize(1024 * 1024 + 1);
    Require(!backend.Value()->CommitSnapshot(
                huge, PersistenceDurability::SaveAndFlushRequired),
            "Runtime payload resource limit rejected");
    ReadKnown(directory);
    auto clock_bytes = Bytes(directory / "time.bin");
    auto persistence_bytes = Bytes(directory / "persistence.bin");
    for (std::size_t length = 0; length < clock_bytes.size(); ++length) {
      Put(directory / "time.bin",
          {clock_bytes.begin(),
           clock_bytes.begin() + static_cast<std::ptrdiff_t>(length)});
      Require(!LoadRuntimeTimeCheckpoint(
                  epidemic::foundation::Path(directory / "time.bin")),
              "every Runtime clock truncation rejected");
    }
    Put(directory / "time.bin", clock_bytes);
    for (std::size_t length = 0; length < persistence_bytes.size(); ++length) {
      Put(directory / "persistence.bin",
          {persistence_bytes.begin(),
           persistence_bytes.begin() + static_cast<std::ptrdiff_t>(length)});
      Require(!backend.Value()->Load(),
              "every Runtime snapshot truncation rejected");
    }
    Put(directory / "persistence.bin", persistence_bytes);
    for (auto offset : {20u, 40u}) {
      auto bad_bytes = persistence_bytes;
      bad_bytes[offset] = std::byte{255};
      bad_bytes[offset + 1] = std::byte{255};
      RepairHash(bad_bytes);
      Put(directory / "persistence.bin", bad_bytes);
      Require(!backend.Value()->Load(),
              "Runtime repaired-checksum invalid count or enum rejected");
    }
    Put(directory / "persistence.bin", persistence_bytes);
    for (auto name : {"time.bin", "persistence.bin"}) {
      auto original = Bytes(directory / name);
      const auto reject = [&] {
        return std::string_view(name) == "time.bin"
                   ? !LoadRuntimeTimeCheckpoint(
                         epidemic::foundation::Path(directory / name))
                   : !backend.Value()->Load();
      };
      auto bad_bytes = original;
      bad_bytes[0] ^= std::byte{1};
      RepairHash(bad_bytes);
      Put(directory / name, bad_bytes);
      Require(reject(), "Runtime bad magic rejected");
      bad_bytes = original;
      bad_bytes[8] = std::byte{2};
      RepairHash(bad_bytes);
      Put(directory / name, bad_bytes);
      Require(reject(), "Runtime future version rejected");
      bad_bytes = original;
      bad_bytes[12] ^= std::byte{1};
      Put(directory / name, bad_bytes);
      Require(reject(), "Runtime checksum corruption rejected");
      bad_bytes = original;
      bad_bytes.insert(bad_bytes.end() - 8, std::byte{1});
      RepairHash(bad_bytes);
      Put(directory / name, bad_bytes);
      Require(reject(), "Runtime trailing bytes rejected");
      Put(directory / name, original);
    }
    auto permuted = KnownSnapshot();
    auto second = permuted.objects[0];
    second.persistent_id = PersistentObjectId{20};
    permuted.objects.push_back(second);
    Require(bool(backend.Value()->CommitSnapshot(
                permuted, PersistenceDurability::SaveRequired)),
            "Runtime multiple object commit");
    auto sorted = Bytes(directory / "persistence.bin");
    std::reverse(permuted.objects.begin(), permuted.objects.end());
    Require(bool(backend.Value()->CommitSnapshot(
                permuted, PersistenceDurability::MemoryOnly)),
            "Runtime weaker durability uses stronger atomic file policy");
    Require(sorted == Bytes(directory / "persistence.bin"),
            "Runtime canonical record ordering byte-stable");
    WriteKnown(directory);
    for (auto fault : {"open", "write", "flush", "replace"}) {
      epidemic::platform::detail::SetAtomicFileFaultPointForTesting(fault);
      auto failed = backend.Value()->CommitSnapshot(
          KnownSnapshot(), PersistenceDurability::SaveAndFlushRequired);
      auto failed_time = CommitRuntimeTimeCheckpoint(
          epidemic::foundation::Path(directory / "time.bin"),
          LoadRuntimeTimeCheckpoint(
              epidemic::foundation::Path(directory / "time.bin"))
              .Value());
      epidemic::platform::detail::ClearAtomicFileFaultPointForTesting();
      Require(!failed && !failed_time,
              "Runtime native commit failure reported");
      Require(
          Bytes(directory / "persistence.bin") == persistence_bytes &&
              Bytes(directory / "time.bin") == clock_bytes,
          "Runtime storage failure preserves both old independent packages");
      ReadKnown(directory);
    }
    auto empty_backend = CreateFilePersistenceBackend(
        epidemic::foundation::Path(directory / "empty.bin"));
    Require(bool(empty_backend) && !empty_backend.Value()->Load(),
            "Runtime missing backend file requires explicit initialization");
    Require(bool(empty_backend.Value()->CommitSnapshot(
                {}, PersistenceDurability::SaveAndFlushRequired)) &&
                empty_backend.Value()->Load().Value().objects.empty(),
            "Runtime empty snapshot roundtrip");
    PersistenceOptions options;
    options.backend = backend.Value();
    options.durability = PersistenceDurability::SaveAndFlushRequired;
    auto services = CreatePersistenceServices(options);
    Require(bool(services), "transactional durable Runtime store bootstrap");
    auto object = KnownSnapshot().objects[0];
    object.persistent_id = PersistentObjectId{25};
    auto tx = services.Value().store->OpenTransaction();
    Require(bool(tx->UpsertObject(object)) && bool(tx->Commit()),
            "Runtime durable create cycle");
    auto fresh = CreatePersistenceServices(options);
    Require(
        bool(fresh) &&
            fresh.Value().query->FindObject(object.persistent_id).has_value(),
        "Runtime durable create fresh reopen");
    TombstoneRecord deleted;
    deleted.persistent_id = object.persistent_id;
    deleted.reason =
        epidemic::foundation::StringId::FromString("cycle.deleted");
    tx = fresh.Value().store->OpenTransaction();
    Require(bool(tx->DeleteObject(deleted)) && bool(tx->Commit()),
            "Runtime durable delete cycle");
    auto after = CreatePersistenceServices(options);
    Require(bool(after) &&
                !after.Value().query->FindObject(object.persistent_id) &&
                after.Value()
                    .query->FindTombstone(object.persistent_id)
                    .has_value(),
            "Runtime durable deletion and tombstone fresh reopen");
    WriteKnown(directory);
    auto time = LoadRuntimeTimeCheckpoint(
        epidemic::foundation::Path(directory / "time.bin"));
    Require(bool(time), "time package bootstrap");
    time.Value().tick_remainder_numerator = -1;
    Require(
        !CommitRuntimeTimeCheckpoint(
            epidemic::foundation::Path(directory / "time.bin"), time.Value()),
        "Runtime invalid clock commit preserves old generation");
    ReadKnown(directory);
    std::ofstream corrupt(directory / "persistence.bin", std::ios::binary);
    corrupt << "truncated";
    corrupt.close();
    Require(!backend.Value()->Load(), "Runtime truncated package rejected");
    Require(!LoadRuntimeTimeCheckpoint(
                epidemic::foundation::Path(directory / "missing.bin")),
            "Runtime missing time package rejected");
  } catch (...) {
    std::filesystem::remove_all(directory);
    throw;
  }
  std::filesystem::remove_all(directory);
}

#include "blob_store.h"
#include <Epidemic/Runtime/Support/runtime_support.h>
#include <limits>
#include <stdexcept>
namespace epidemic::runtime::presentation {
namespace {
template <class T> T Require(foundation::Result<T> result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
    return std::move(result).Value();
}
void Require(const foundation::Result<void> &result) {
    if (!result.HasValue())
        throw std::runtime_error(result.GetError().message);
}
constexpr auto Schema = foundation::StringId::FromString("presentation.save_package");
}
std::vector<std::byte> LoadBlob(const std::filesystem::path &path) {
    auto backend = Require(CreateFilePersistenceBackend(foundation::Path{path}));
    const auto snapshot = Require(backend->Load());
    if (snapshot.objects.size() != 1 || snapshot.objects[0].payload.schema_id != Schema ||
        snapshot.objects[0].payload.schema_version != 1)
        throw std::runtime_error("Unknown save package storage schema");
    return snapshot.objects[0].payload.bytes;
}
void SaveBlob(const std::filesystem::path &path, std::span<const std::byte> bytes) {
    if (path.empty())
        throw std::invalid_argument("Save path is empty");
    if (!path.parent_path().empty())
        std::filesystem::create_directories(path.parent_path());
    auto backend = Require(CreateFilePersistenceBackend(foundation::Path{path}));
    PersistenceSnapshot snapshot;
    if (std::filesystem::exists(path))
        snapshot = Require(backend->Load());
    if (snapshot.current_revision == std::numeric_limits<std::uint64_t>::max())
        throw std::runtime_error("Save revision exhausted");
    ++snapshot.current_revision;
    snapshot.objects = {{.persistent_id = PersistentObjectId{1},
                         .asset_id = AssetId::FromString("presentation.save_package"),
                         .kind = PersistentObjectKind::AbstractFact,
                         .tier = PersistenceTier::Protected,
                         .state = PersistenceState::Saved,
                         .payload = {.schema_id = Schema, .schema_version = 1, .bytes = {bytes.begin(), bytes.end()}},
                         .revision = snapshot.current_revision}};
    Require(backend->CommitSnapshot(snapshot, PersistenceDurability::SaveAndFlushRequired));
}
}
